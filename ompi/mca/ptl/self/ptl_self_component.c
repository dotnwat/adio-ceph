/* -*- Mode: C; c-basic-offset:4 ; -*- */
/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>
#include <fcntl.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#include "ompi/include/constants.h"
#include "opal/event/event.h"
#include "opal/util/if.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/ptl/ptl.h"
#include "ompi/mca/pml/base/pml_base_sendreq.h"
#include "ompi/mca/ptl/base/ptl_base_recvfrag.h"
#include "opal/mca/base/mca_base_param.h"
#include "ptl_self.h"
#include "ompi/proc/proc.h"

mca_ptl_self_component_t mca_ptl_self_component = {
    {
        /* First, the mca_base_component_t struct containing meta information
           about the component itself */
        
        {
            /* Indicate that we are a pml v1.0.0 component (which also implies a
               specific MCA version) */
            
            MCA_PTL_BASE_VERSION_1_0_0,
            
            "self", /* MCA component name */
            OMPI_MAJOR_VERSION,  /* MCA component major version */
            OMPI_MINOR_VERSION,  /* MCA component minor version */
            OMPI_RELEASE_VERSION,  /* MCA component release version */
            mca_ptl_self_component_open,  /* component open */
            mca_ptl_self_component_close  /* component close */
        },
        
        /* Next the MCA v1.0.0 component meta data */
        
        {
            /* Whether the component is checkpointable or not */
            true
        },
        
        mca_ptl_self_component_init,
        NULL,
        NULL,
    }
};

extern mca_ptl_base_module_t mca_ptl_self_module;


static void mca_ptl_self_send_request_construct(mca_ptl_self_send_request_t* request)
{
    OBJ_CONSTRUCT(&request->req_frag, mca_ptl_base_recv_frag_t);
}

static void mca_ptl_self_send_request_destruct(mca_ptl_self_send_request_t* request)
{
    OBJ_DESTRUCT(&request->req_frag);
}

OBJ_CLASS_INSTANCE( mca_ptl_self_send_request_t, 
                    mca_pml_base_send_request_t,
                    mca_ptl_self_send_request_construct,
                    mca_ptl_self_send_request_destruct );

/*
 * utility routines for parameter registration
 */

static inline int mca_ptl_self_param_register_int(
   const char* param_name, 
   int default_value )
{
   int id = mca_base_param_register_int("ptl","self",param_name,NULL,default_value);
   int param_value = default_value;
   mca_base_param_lookup_int(id,&param_value);
   return param_value;
}
                                                                                                                            
/*
 *  Called by MCA framework to open the component, registers
 *  component parameters.
 */

int mca_ptl_self_component_open(void)
{
    /* register SELF component parameters */
    mca_ptl_self_component.self_buf_size =
        mca_ptl_self_param_register_int("buffer_size", 64*1024);
    mca_ptl_self_component.self_is_non_blocking =
        mca_ptl_self_param_register_int("nonblocking", 1);
    return OMPI_SUCCESS;
}

int mca_ptl_self_component_close(void)
{
    if( NULL == mca_ptl_self_component.self_local )
        return OMPI_SUCCESS;

    if (mca_ptl_self_component.self_send_requests.fl_num_allocated != 
        mca_ptl_self_component.self_send_requests.super.opal_list_length) {
        opal_output(0, "self send requests: %d allocated %d returned\n",
            mca_ptl_self_component.self_send_requests.fl_num_allocated, 
            mca_ptl_self_component.self_send_requests.super.opal_list_length);
    }

    OBJ_DESTRUCT( &(mca_ptl_self_component.self_send_requests) );

    return OMPI_SUCCESS;
}


mca_ptl_base_module_t** mca_ptl_self_component_init(int *num_ptl_modules,
                                                    bool enable_progress_threads,
                                                    bool enable_mpi_threads)
{
    mca_ptl_base_module_t** modules;

    *num_ptl_modules = 0;

    modules = (mca_ptl_base_module_t **)malloc(sizeof(mca_ptl_base_module_t*));
    if( NULL == modules )
        return NULL;

    modules[0] = &mca_ptl_self_module;

    mca_ptl_self_component.self_free_list_num = 4;
    mca_ptl_self_component.self_free_list_max = -1;
    mca_ptl_self_component.self_free_list_inc = 4;

    /* Initialize the local pointer to the processor */
    mca_ptl_self_component.self_local = ompi_proc_local();

    *num_ptl_modules = 1;

    OBJ_CONSTRUCT(&mca_ptl_self_component.self_send_requests, ompi_free_list_t);
    ompi_free_list_init(&mca_ptl_self_component.self_send_requests, 
                        sizeof(mca_ptl_self_send_request_t),
                        OBJ_CLASS(mca_ptl_self_send_request_t),
                        mca_ptl_self_component.self_free_list_num,
                        mca_ptl_self_component.self_free_list_max,
                        mca_ptl_self_component.self_free_list_inc,
                        NULL); /* use default allocator */

    return modules;
}
