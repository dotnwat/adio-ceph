/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2009 Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2008-2009 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H*/
#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif  /* HAVE_STDLIB_H */
#include <errno.h>
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"

#include "orte/util/proc_info.h"

#include "ompi/mca/allocator/base/base.h"
#include "mpool_sm.h"
#include "ompi/mca/common/sm/common_sm_mmap.h"
#include "ompi/proc/proc.h"

#if OPAL_ENABLE_FT    == 1
#include "opal/runtime/opal_cr.h"
#endif

/*
 * Local functions
 */
static int mca_mpool_sm_open(void);
static int mca_mpool_sm_close( void );
static mca_mpool_base_module_t* mca_mpool_sm_init(
    struct mca_mpool_base_resources_t* resources);

mca_mpool_sm_component_t mca_mpool_sm_component = {
    {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

      {
        MCA_MPOOL_BASE_VERSION_2_0_0,

        "sm", /* MCA component name */
        OMPI_MAJOR_VERSION,  /* MCA component major version */
        OMPI_MINOR_VERSION,  /* MCA component minor version */
        OMPI_RELEASE_VERSION,  /* MCA component release version */
        mca_mpool_sm_open,  /* component open  */
        mca_mpool_sm_close
      },
      {
          /* The component is checkpoint ready */
          MCA_BASE_METADATA_PARAM_CHECKPOINT
      },

      mca_mpool_sm_init
    }
};

static char *min_size_param = NULL;
static long default_min;

/**
  * component open/close/init function
  */
static int mca_mpool_sm_open(void)
{
    int value = 0;
    char *size_str = NULL;

    default_min = 67108864;

    /* register SM component parameters */
    mca_base_param_reg_string(&mca_mpool_sm_component.super.mpool_version,
                              "allocator",
                              "Name of allocator component to use with sm mpool",
                              false, false,
                              "bucket",
                              &mca_mpool_sm_component.sm_allocator_name);

    /* register values as string instead of int. A string-converted
     * signed long int allows the min_size or the sm_size
     * to be set up to 2GB-1 for 32 bit and much greater for 64 bit. */
    asprintf(&size_str, "%ld", default_min);
    mca_base_param_reg_string(&mca_mpool_sm_component.super.mpool_version,
                               "min_size",
                               "Minimum size of the sm mpool shared memory file",
                               false, false, size_str, &min_size_param);
    free(size_str);
    mca_base_param_reg_int(&mca_mpool_sm_component.super.mpool_version,
                               "verbose",
                               "Enable verbose output for mpool sm component",
                               false, false, 0, &value);
    if (value != 0) {
            mca_mpool_sm_component.verbose = opal_output_open(NULL);
    } else {
            mca_mpool_sm_component.verbose = -1;
    }
    mca_mpool_sm_component.sm_size = 0;

    return OMPI_SUCCESS;
}

static int mca_mpool_sm_close( void )
{
    if( NULL != mca_common_sm_mmap ) {
        if( OMPI_SUCCESS == mca_common_sm_mmap_fini( mca_common_sm_mmap ) ) {
#if OPAL_ENABLE_FT    == 1
            /* Only unlink the file if we are *not* restarting
             * If we are restarting the file will be unlinked at a later time.
             */
            if(OPAL_CR_STATUS_RESTART_PRE  != opal_cr_checkpointing_state &&
               OPAL_CR_STATUS_RESTART_POST != opal_cr_checkpointing_state ) {
                unlink( mca_common_sm_mmap->map_path );
            }
#else
            unlink( mca_common_sm_mmap->map_path );
#endif
        }
        OBJ_RELEASE( mca_common_sm_mmap );
    }

    if (NULL != mca_mpool_sm_component.sm_allocator_name) {
        free(mca_mpool_sm_component.sm_allocator_name);
    }
    if (NULL != min_size_param) {
        free(min_size_param);
    }
    return OMPI_SUCCESS;
}

static mca_mpool_base_module_t* mca_mpool_sm_init(
    struct mca_mpool_base_resources_t* resources)
{
    char *file_name;
    int len;
    mca_mpool_sm_module_t* mpool_module;
    mca_allocator_base_component_t* allocator_component;
    long min_size;
    ompi_proc_t **procs;
    size_t num_all_procs, i, num_local_procs = 0;

    /* README: this needs to change if procs in different jobs (even
       spawned ones) are to talk using shared memory */
    procs = ompi_proc_world(&num_all_procs);
    for (i = 0 ; i < num_all_procs ; ++i) {
        if (OPAL_PROC_ON_LOCAL_NODE(procs[i]->proc_flags)) {
            num_local_procs++;
        }
    }
    free(procs);

    /* parse the min size and validate it */
    /* if other parameters are added, absolutely necessary to reset errno each time */
    errno = 0;
    min_size  = strtol(min_size_param, (char **)NULL, 10);
    if (errno == ERANGE) {
        opal_output(0, "mca_mpool_sm_init: min_size overflows! set to default (%ld)", default_min);
        min_size = default_min;
    } else if (errno == EINVAL) {
        opal_output(0, "mca_mpool_sm_init: invalid min_size entered. set it to (%ld)", default_min);
        min_size = default_min;
    }

    /* set sm_size */
    mca_mpool_sm_component.sm_size = resources->size;

    /* clip at the min size */
    if ( mca_mpool_sm_component.sm_size < min_size )
         mca_mpool_sm_component.sm_size = min_size;

    /* add something for the control structure */
    mca_mpool_sm_component.sm_size += sizeof(mca_common_sm_mmap_t);

    allocator_component = mca_allocator_component_lookup(
        mca_mpool_sm_component.sm_allocator_name);

    /* if specified allocator cannot be loaded - look for an alternative */
    if(NULL == allocator_component) {
        if(opal_list_get_size(&mca_allocator_base_components) == 0) {
            mca_base_component_list_item_t* item = (mca_base_component_list_item_t*)
                opal_list_get_first(&mca_allocator_base_components);
            allocator_component = (mca_allocator_base_component_t*)item->cli_component;
            opal_output(0, "mca_mpool_sm_init: unable to locate allocator: %s - using %s\n",
                mca_mpool_sm_component.sm_allocator_name, allocator_component->allocator_version.mca_component_name);
        } else {
            opal_output(0, "mca_mpool_sm_init: unable to locate allocator: %s\n",
                mca_mpool_sm_component.sm_allocator_name);
            return NULL;
        }
    }

    mpool_module = (mca_mpool_sm_module_t*)malloc(sizeof(mca_mpool_sm_module_t));
    mca_mpool_sm_module_init(mpool_module);
    mpool_module->mem_node = resources->mem_node;

    /* create initial shared memory mapping */
    len = asprintf( &file_name, "%s"OPAL_PATH_SEP"shared_mem_pool.%s",
                    orte_process_info.job_session_dir,
                    orte_process_info.nodename );
    if ( 0 > len ) {
        free(mpool_module);
        return NULL;
    }
    
    opal_output(mca_mpool_sm_component.verbose,
        "mca_mpool_sm_init: shared memory size used: (%ld)",
        mca_mpool_sm_component.sm_size);

    if(NULL == 
            (mca_common_sm_mmap = 
             mca_common_sm_mmap_init(mca_mpool_sm_component.sm_size,
                 file_name,sizeof(mca_common_sm_mmap_t), 8 )
             )) 
    {
        opal_output(mca_mpool_sm_component.verbose, 
                    "mca_mpool_sm_init: unable to create shared memory mapping (%s)", file_name);
        free(file_name);
        free(mpool_module);
        return NULL;
    }
    free(file_name);

    /* setup allocator */
    mpool_module->sm_allocator = 
      allocator_component->allocator_init(true,
                                          mca_common_sm_mmap_seg_alloc, NULL, NULL);
    if(NULL == mpool_module->sm_allocator) {
        opal_output(0, "mca_mpool_sm_init: unable to initialize allocator");
        free(mpool_module);
        return NULL;
    }
   
    return &mpool_module->super;
}

