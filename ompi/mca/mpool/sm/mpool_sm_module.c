/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include <string.h>
#include "ompi/mca/mpool/sm/mpool_sm.h"
#include "ompi/mca/common/sm/common_sm_mmap.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include "opal/mca/maffinity/maffinity.h"
#include "opal/mca/maffinity/maffinity_types.h"
#include "opal/mca/maffinity/base/base.h"
#include "orte/util/proc_info.h"

#if OPAL_ENABLE_FT    == 1
#include "ompi/mca/mpool/base/base.h"
#include "ompi/runtime/ompi_cr.h"
#endif

static void sm_module_finalize(mca_mpool_base_module_t* module);

/* 
 *  Initializes the mpool module.
 */ 
void mca_mpool_sm_module_init(mca_mpool_sm_module_t* mpool)
{
    mpool->super.mpool_component = &mca_mpool_sm_component.super; 
    mpool->super.mpool_base = mca_mpool_sm_base; 
    mpool->super.mpool_alloc = mca_mpool_sm_alloc; 
    mpool->super.mpool_realloc = mca_mpool_sm_realloc; 
    mpool->super.mpool_free = mca_mpool_sm_free; 
    mpool->super.mpool_find = NULL; 
    mpool->super.mpool_register = NULL; 
    mpool->super.mpool_deregister = NULL; 
    mpool->super.mpool_release_memory = NULL;
    mpool->super.mpool_finalize = sm_module_finalize; 
    mpool->super.mpool_ft_event = mca_mpool_sm_ft_event;
    mpool->super.flags = 0;

    mpool->sm_size = 0;
    mpool->sm_allocator = NULL;
    mpool->sm_mmap = NULL;
    mpool->sm_common_mmap = NULL;
    mpool->mem_node = -1;
}

/*
 * base address of shared memory mapping
 */
void* mca_mpool_sm_base(mca_mpool_base_module_t* mpool)
{
    mca_mpool_sm_module_t *sm_mpool = (mca_mpool_sm_module_t*) mpool;
    return (NULL != sm_mpool->sm_common_mmap) ?
        sm_mpool->sm_common_mmap->map_addr : NULL;
}

/**
  * allocate function
  */
void* mca_mpool_sm_alloc(
    mca_mpool_base_module_t* mpool,
    size_t size,
    size_t align,
    uint32_t flags,
    mca_mpool_base_registration_t** registration)
{
    mca_mpool_sm_module_t* mpool_sm = (mca_mpool_sm_module_t*)mpool;
    opal_maffinity_base_segment_t mseg;

    mseg.mbs_start_addr =
        mpool_sm->sm_allocator->alc_alloc(mpool_sm->sm_allocator, size, align, registration);

    if(mpool_sm->mem_node >= 0) {
        mseg.mbs_len = size;
        opal_maffinity_base_bind(&mseg, 1, mpool_sm->mem_node);
    }

    return mseg.mbs_start_addr;
}

/**
  * realloc function
  */
void* mca_mpool_sm_realloc(
    mca_mpool_base_module_t* mpool,
    void* addr,
    size_t size,
    mca_mpool_base_registration_t** registration)
{
    mca_mpool_sm_module_t* mpool_sm = (mca_mpool_sm_module_t*)mpool;
    opal_maffinity_base_segment_t mseg;

    mseg.mbs_start_addr =
        mpool_sm->sm_allocator->alc_realloc(mpool_sm->sm_allocator, addr, size,
                                            registration);
    if(mpool_sm->mem_node >= 0) {
        mseg.mbs_len = size;
        opal_maffinity_base_bind(&mseg, 1, mpool_sm->mem_node);
    }

    return mseg.mbs_start_addr;
}

/**
  * free function
  */
void mca_mpool_sm_free(mca_mpool_base_module_t* mpool, void * addr,
                       mca_mpool_base_registration_t* registration)
{
    mca_mpool_sm_module_t* mpool_sm = (mca_mpool_sm_module_t*)mpool;
    mpool_sm->sm_allocator->alc_free(mpool_sm->sm_allocator, addr);
}

static void sm_module_finalize(mca_mpool_base_module_t* module)
{
    mca_mpool_sm_module_t *sm_module = (mca_mpool_sm_module_t*) module;

    if (NULL != sm_module->sm_common_mmap) {
        if (OMPI_SUCCESS == 
            mca_common_sm_mmap_fini(sm_module->sm_common_mmap)) {
#if OPAL_ENABLE_FT == 1
            /* Only unlink the file if we are *not* restarting.  If we
               are restarting the file will be unlinked at a later
               time. */
            if (OPAL_CR_STATUS_RESTART_PRE  != opal_cr_checkpointing_state &&
                OPAL_CR_STATUS_RESTART_POST != opal_cr_checkpointing_state ) {
                unlink(sm_module->sm_common_mmap->map_path);
            }
#else
            unlink(sm_module->sm_common_mmap->map_path);
#endif
        }
        OBJ_RELEASE(sm_module->sm_common_mmap);
        sm_module->sm_common_mmap = NULL;
    }
}

#if OPAL_ENABLE_FT    == 0
int mca_mpool_sm_ft_event(int state) {
    return OMPI_SUCCESS;
}
#else
int mca_mpool_sm_ft_event(int state) {
    mca_mpool_base_module_t *self_module = NULL;
    char * file_name = NULL;

    if(OPAL_CRS_CHECKPOINT == state) {
        /* Record the shared memory filename */
        asprintf( &file_name, "%s"OPAL_PATH_SEP"shared_mem_pool.%s",
                  orte_process_info.job_session_dir,
                  orte_process_info.nodename );
        opal_crs_base_metadata_write_token(NULL, CRS_METADATA_TOUCH, file_name);
        free(file_name);
        file_name = NULL;
    }
    else if(OPAL_CRS_CONTINUE == state) {
        if(ompi_cr_continue_like_restart) {
            /* Remove self from the list of all modules */
            self_module = mca_mpool_base_module_lookup("sm");
            mca_mpool_base_module_destroy(self_module);

            /* Release the old sm file, if it exists */
            if( NULL != mca_common_sm_mmap ) {
                if( OMPI_SUCCESS == mca_common_sm_mmap_fini( mca_common_sm_mmap ) ) {
                    /* Add old shared memory file for eventual removal */
                    opal_crs_base_cleanup_append(mca_common_sm_mmap->map_path, false);
                }
                OBJ_RELEASE( mca_common_sm_mmap );
            }
        }
    }
    else if(OPAL_CRS_RESTART == state ||
            OPAL_CRS_RESTART_PRE == state) {
        /* Remove self from the list of all modules */
        self_module = mca_mpool_base_module_lookup("sm");
        mca_mpool_base_module_destroy(self_module);

        /* Release the old sm file, if it exists */
        if( NULL != mca_common_sm_mmap ) {
            if( OMPI_SUCCESS == mca_common_sm_mmap_fini( mca_common_sm_mmap ) ) {
                /* Add old shared memory file for eventual removal */
                opal_crs_base_cleanup_append(mca_common_sm_mmap->map_path, false);
            }
            OBJ_RELEASE( mca_common_sm_mmap );
        }
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }

    return OMPI_SUCCESS;
}
#endif /* OPAL_ENABLE_FT */
