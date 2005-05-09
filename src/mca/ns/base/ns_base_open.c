/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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


#include "orte_config.h"
#include "include/orte_constants.h"

#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/base/mca_base_param.h"
#include "mca/errmgr/errmgr.h"
#include "util/output.h"

#include "dps/dps.h"

#include "mca/ns/base/base.h"


/*
 * The following file was created by configure.  It contains extern
 * statements and the definition of an array of pointers to each
 * component's public mca_base_component_t struct.
 */

#include "mca/ns/base/static-components.h"

/*
 * globals
 */

orte_process_name_t orte_name_all = {ORTE_CELLID_MAX, ORTE_JOBID_MAX, ORTE_VPID_MAX};

/*
 * Global variables
 */
int mca_ns_base_output = -1;
OMPI_DECLSPEC mca_ns_base_module_t orte_ns = {
    orte_ns_base_module_init_not_available,
    orte_ns_base_create_cellid_not_available,
    orte_ns_base_assign_cellid_to_process,
    orte_ns_base_create_jobid_not_available,
    orte_ns_base_create_process_name,
    orte_ns_base_copy_process_name,
    orte_ns_base_convert_string_to_process_name,
    orte_ns_base_get_vpid_range_not_available,
    orte_ns_base_free_name,
    orte_ns_base_get_proc_name_string,
    orte_ns_base_get_vpid_string,
    orte_ns_base_convert_vpid_to_string,
    orte_ns_base_convert_string_to_vpid,
    orte_ns_base_get_jobid_string,
    orte_ns_base_convert_jobid_to_string,
    orte_ns_base_convert_string_to_jobid,
    orte_ns_base_get_cellid_string,
    orte_ns_base_convert_cellid_to_string,
    orte_ns_base_convert_string_to_cellid,
    orte_ns_base_get_vpid,
    orte_ns_base_get_jobid,
    orte_ns_base_get_cellid,
    orte_ns_base_compare,
    orte_ns_base_derive_vpid,
    orte_ns_base_assign_rml_tag_not_available,
    orte_ns_base_define_data_type_not_available,
    orte_ns_base_set_my_name,
    orte_ns_base_get_peers
};
bool mca_ns_base_selected = false;
ompi_list_t mca_ns_base_components_available;
mca_ns_base_component_t mca_ns_base_selected_component;


/* constructor - used to initialize namelist instance */
static void orte_name_services_namelist_construct(orte_name_services_namelist_t* list)
{
    list->name = NULL;
}

/* destructor - used to free any resources held by instance */
static void orte_name_services_namelist_destructor(orte_name_services_namelist_t* list)
{
    if (NULL != list->name) {
	free(list->name);
    }
}

/* define instance of ompi_class_t */
OBJ_CLASS_INSTANCE(
		   orte_name_services_namelist_t,              /* type name */
		   ompi_list_item_t,                        /* parent "class" name */
		   orte_name_services_namelist_construct,    /* constructor */
		   orte_name_services_namelist_destructor);  /* destructor */



/**
 * Function for finding and opening either all MCA components, or the one
 * that was specifically requested via a MCA parameter.
 */
int orte_ns_base_open(void)
{
    int param, value, rc;
    orte_data_type_t tmp;

    /* Debugging / verbose output */
    
    param = mca_base_param_register_int("ns", "base", "verbose",
                                        NULL, 0);
    mca_base_param_lookup_int(param, &value);
    if (value != 0) {
        mca_ns_base_output = ompi_output_open(NULL);
    } else {
        mca_ns_base_output = -1;
    }

    /* register the base system types with the DPS */
    tmp = ORTE_NAME;
    if (ORTE_SUCCESS != (rc = orte_dps.register_type(orte_ns_base_pack_name,
                                          orte_ns_base_unpack_name,
                                          "ORTE_NAME", &tmp))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    tmp = ORTE_VPID;
    if (ORTE_SUCCESS != (rc = orte_dps.register_type(orte_ns_base_pack_vpid, 
                                          orte_ns_base_unpack_vpid,
                                          "ORTE_VPID", &tmp))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    tmp = ORTE_JOBID;
    if (ORTE_SUCCESS != (rc = orte_dps.register_type(orte_ns_base_pack_jobid, 
                                          orte_ns_base_unpack_jobid,
                                          "ORTE_JOBID", &tmp))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    tmp = ORTE_CELLID;
    if (ORTE_SUCCESS != (rc = orte_dps.register_type(orte_ns_base_pack_cellid, 
                                          orte_ns_base_unpack_cellid,
                                          "ORTE_CELLID", &tmp))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    /* Open up all available components */

    if (ORTE_SUCCESS != 
        mca_base_components_open("ns", 0, mca_ns_base_static_components, 
                                 &mca_ns_base_components_available, true)) {
        return ORTE_ERROR;
    }

    /* All done */

    return ORTE_SUCCESS;
}
