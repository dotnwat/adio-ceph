/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */


#include "ompi_config.h"
#include <stdio.h>

#include "include/constants.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"
#include "mca/rml/base/base.h"

/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/rml/base/static-components.h"


/*
 * Global variables
 */

orte_rml_base_t orte_rml_base;
orte_rml_module_t orte_rml = {
    mca_oob_base_module_init,
    NULL,
    (orte_rml_module_get_uri_fn_t)mca_oob_get_contact_info,
    (orte_rml_module_set_uri_fn_t)mca_oob_set_contact_info,
    (orte_rml_module_parse_uris_fn_t)mca_oob_parse_contact_info,
    (orte_rml_module_ping_fn_t)mca_oob_ping,
    (orte_rml_module_send_fn_t)mca_oob_send,
    (orte_rml_module_send_nb_fn_t)mca_oob_send_nb,
    (orte_rml_module_send_buffer_fn_t)mca_oob_send_packed,
    (orte_rml_module_send_buffer_nb_fn_t)mca_oob_send_packed_nb,
    (orte_rml_module_recv_fn_t)mca_oob_recv,
    (orte_rml_module_recv_nb_fn_t)mca_oob_recv_nb,
    (orte_rml_module_recv_buffer_fn_t)mca_oob_recv_packed,
    (orte_rml_module_recv_buffer_nb_fn_t)mca_oob_recv_packed_nb,
    (orte_rml_module_recv_cancel_fn_t)mca_oob_recv_cancel,
    (orte_rml_module_barrier_fn_t)mca_oob_barrier,
    (orte_rml_module_xcast_fn_t)mca_oob_xcast
};
orte_process_name_t orte_rml_name_any = { ORTE_CELLID_MAX, ORTE_JOBID_MAX, ORTE_VPID_MAX };
orte_process_name_t orte_rml_name_seed = { 0, 0, 0 };

/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_rml_base_open(void)
{
    int id;
    int int_value;
    int rc;

    /* Initialize globals */
    OBJ_CONSTRUCT(&orte_rml_base.rml_components, ompi_list_t);
    
    /* lookup common parameters */
    id = mca_base_param_register_int("rml","base","debug",NULL,1);
    mca_base_param_lookup_int(id,&int_value);
    orte_rml_base.rml_debug = int_value;

    /* Open up all available components */
    if ((rc = mca_base_components_open("rml", 0, mca_rml_base_static_components, 
            &orte_rml_base.rml_components)) != OMPI_SUCCESS) {
        return rc;
    }
    return OMPI_SUCCESS;
}

