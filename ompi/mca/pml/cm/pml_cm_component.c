/*
 * Copyright (c) 2006-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "pml_cm.h"
#include "opal/event/event.h"
#include "opal/mca/base/mca_base_param.h"
#include "ompi/datatype/convertor.h"
#include "ompi/mca/mtl/mtl.h"
#include "ompi/mca/mtl/base/base.h"
#include "ompi/mca/pml/base/pml_base_bsend.h"

#include "pml_cm_sendreq.h"
#include "pml_cm_recvreq.h"
#include "pml_cm_component.h"

static int mca_pml_cm_component_open(void);
static int mca_pml_cm_component_close(void);
static mca_pml_base_module_t* mca_pml_cm_component_init( int* priority,
                            bool enable_progress_threads, bool enable_mpi_threads);
static int mca_pml_cm_component_fini(void);

mca_pml_base_component_1_0_0_t mca_pml_cm_component = {

    /* First, the mca_base_component_t struct containing meta
     * information about the component itself */

    {
        /* Indicate that we are a pml v1.0.0 component (which also implies
	 *          a specific MCA version) */

         MCA_PML_BASE_VERSION_1_0_0,

         "cm", /* MCA component name */
         OMPI_MAJOR_VERSION,  /* MCA component major version */
         OMPI_MINOR_VERSION,  /* MCA component minor version */
         OMPI_RELEASE_VERSION,  /* MCA component release version */
         mca_pml_cm_component_open,  /* component open */
         mca_pml_cm_component_close  /* component close */
     },

     /* Next the MCA v1.0.0 component meta data */

     {
         /* This component is not checkpoint ready */
         MCA_BASE_METADATA_PARAM_NONE
     },

     mca_pml_cm_component_init,  /* component init */
     mca_pml_cm_component_fini   /* component finalize */
};

/* Array of send completion callback - one per send type 
 * These are called internally by the library when the send
 * is completed from its perspective.
 */
void (*send_completion_callbacks[MCA_PML_BASE_SEND_SIZE])
    (struct mca_mtl_request_t *mtl_request) =
  { mca_pml_cm_send_request_completion,
    mca_pml_cm_send_request_completion,
    mca_pml_cm_send_request_completion,
    mca_pml_cm_send_request_completion,
    mca_pml_cm_send_request_completion } ;

static int
mca_pml_cm_component_open(void)
{
    int ret;

    ret = ompi_mtl_base_open();
    if (OMPI_SUCCESS != ret) return ret;

    mca_base_param_reg_int(&mca_pml_cm_component.pmlm_version,
                           "free_list_num",
                           "Initial size of request free lists",
                           false,
                           false,
                           4,
                           &ompi_pml_cm.free_list_num);

    mca_base_param_reg_int(&mca_pml_cm_component.pmlm_version,
                           "free_list_max",
                           "Maximum size of request free lists",
                           false,
                           false,
                           -1,
                           &ompi_pml_cm.free_list_max);

    mca_base_param_reg_int(&mca_pml_cm_component.pmlm_version,
                           "free_list_inc",
                           "Number of elements to add when growing request free lists",
                           false,
                           false,
                           64,
                           &ompi_pml_cm.free_list_inc);

    mca_base_param_reg_int(&mca_pml_cm_component.pmlm_version,
                           "priority",
                           "CM PML selection priority",
                           false,
                           false,
                           30,
                           &ompi_pml_cm.default_priority);

    return OMPI_SUCCESS;
}


static int
mca_pml_cm_component_close(void)
{
    return ompi_mtl_base_close();
}


static mca_pml_base_module_t*
mca_pml_cm_component_init(int* priority,
                          bool enable_progress_threads,
                          bool enable_mpi_threads)
{
    int ret;

    if((*priority) > ompi_pml_cm.default_priority) { 
        *priority = ompi_pml_cm.default_priority;
        return NULL;
    }
    *priority = ompi_pml_cm.default_priority;
    opal_output_verbose( 10, 0, 
                         "in cm pml priority is %d\n", *priority);
    /* find a useable MTL */
    ret = ompi_mtl_base_select(enable_progress_threads, enable_mpi_threads);
    if (OMPI_SUCCESS != ret) { 
        *priority = -1;
        return NULL;
    } else if(strcmp(ompi_mtl_base_selected_component->mtl_version.mca_component_name, "psm") != 0) {
        /* if mtl is not PSM then back down priority, and require the user to */
        /*  specify pml cm directly if that is what they want priority */
        /*  of 1 is sufficient in that case as it is the only pml that */ 
        /*  will be considered */
        *priority = 1;
    }

    
    /* update our tag / context id max values based on MTL
       information */
    ompi_pml_cm.super.pml_max_contextid = ompi_mtl->mtl_max_contextid;
    ompi_pml_cm.super.pml_max_tag = ompi_mtl->mtl_max_tag;
    
    return &ompi_pml_cm.super;
}


static int
mca_pml_cm_component_fini(void)
{
    if (NULL != ompi_mtl && NULL != ompi_mtl->mtl_finalize) {
        return ompi_mtl->mtl_finalize(ompi_mtl);
    }

    return OMPI_SUCCESS;
}

