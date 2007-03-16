/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2007 High Performance Computing Center Stuttgart, 
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
#include <stdio.h>
#include <unistd.h>

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/util/output.h"
#include "ompi/constants.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/pml/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "ompi/mca/pml/base/static-components.h"

int mca_pml_base_progress(void) 
{
    return OMPI_SUCCESS;
}

#define xstringify(pml) #pml
#define stringify(pml) xstringify(pml)

/*
 * Global variables
 */
int mca_pml_base_output = 0;
mca_pml_base_module_t mca_pml = {
    NULL,                    /* pml_add_procs */
    NULL,                    /* pml_del_procs */
    NULL,                    /* pml_enable */
    mca_pml_base_progress,   /* pml_progress */
    NULL,                    /* pml_add_comm */
    NULL,                    /* pml_del_comm */
    NULL,                    /* pml_irecv_init */
    NULL,                    /* pml_irecv */
    NULL,                    /* pml_recv */
    NULL,                    /* pml_isend_init */
    NULL,                    /* pml_isend */
    NULL,                    /* pml_send */
    NULL,                    /* pml_iprobe */
    NULL,                    /* pml_probe */
    NULL,                    /* pml_start */
    NULL,                    /* pml_dump */
    NULL,                    /* pml_ft_event */
    0,                       /* pml_max_contextid */
    0                        /* pml_max_tag */
};

opal_list_t mca_pml_base_components_available;
mca_pml_base_component_t mca_pml_base_selected_component;
ompi_pointer_array_t mca_pml_base_pml;

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int mca_pml_base_open(void)
{
    char* default_pml = NULL;
    char* wrapper_pml = NULL;
    int value;

    /*
     * Register some MCA parameters
     */
     /* Debugging/Verbose output */
     mca_base_param_reg_int_name("pml_base",
                                 "verbose",
                                 "Verbosity level of the PML framework",
                                 false, false,
                                 0, &value);
 
     mca_pml_base_output = opal_output_open(NULL);
     opal_output_set_verbosity(mca_pml_base_output, value);

    /* Open up all available components */

    if (OMPI_SUCCESS != 
        mca_base_components_open("pml", mca_pml_base_output, mca_pml_base_static_components, 
                                 &mca_pml_base_components_available,
                                 !MCA_pml_DIRECT_CALL)) {
        return OMPI_ERROR;
    }

    /* Set a sentinel in case we don't select any components (e.g.,
       ompi_info) */

    mca_pml_base_selected_component.pmlm_finalize = NULL;

    /**
     * Right now our selection of BTLs is completely broken. If we have
     * multiple PMLs that use BTLs than we will open all BTLs several times, leading to
     * undefined behaviors. The simplest solution, at least until we
     * figure out the correct way to do it, is to force a default PML that 
     * uses BTLs and any other PMLs that do not in the mca_pml_base_pml array.
     */

    OBJ_CONSTRUCT(&mca_pml_base_pml, ompi_pointer_array_t);
    
#if MCA_pml_DIRECT_CALL
    ompi_pointer_array_add(&mca_pml_base_pml, 
                           stringify(MCA_pml_DIRECT_CALL_COMPONENT));
#else
    

    mca_base_param_reg_string_name("pml", NULL, 
                                   "Specify a specific PML to use", 
                                   false, false, "", &default_pml);
    
    if(0 == strlen(default_pml)){ 
        ompi_pointer_array_add(&mca_pml_base_pml, strdup("ob1")); 
        ompi_pointer_array_add(&mca_pml_base_pml, strdup("cm"));
    } else { 
        ompi_pointer_array_add(&mca_pml_base_pml, strdup(default_pml));
    }

    /* 
     * Which PML Wrapper component to use, if any
     *  - NULL or "" = No wrapper
     *  - ow. select that specific wrapper component
     */
    mca_base_param_reg_string_name("pml", "wrapper",
                                   "Use a Wrapper component around the selected PML component",
                                   false, false,
                                   NULL, &wrapper_pml);
    if( NULL != wrapper_pml ) {
        ompi_pointer_array_add(&mca_pml_base_pml, strdup(wrapper_pml));
    }
#endif

    return OMPI_SUCCESS;

}
