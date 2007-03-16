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
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#ifndef ORTE_FILEM_BASE_H
#define ORTE_FILEM_BASE_H

#include "orte_config.h"

#include "orte/mca/rml/rml.h"
#include "orte/dss/dss.h"

#include "orte/mca/filem/filem.h"

/*
 * Global functions for MCA overall FILEM
 */

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

    /**
     * FileM request object maintenance functions
     */
    ORTE_DECLSPEC void orte_filem_base_construct(orte_filem_base_request_t *obj);
    ORTE_DECLSPEC void orte_filem_base_destruct( orte_filem_base_request_t *obj);

    /**
     * Initialize the FILEM MCA framework
     *
     * @retval ORTE_SUCCESS Upon success
     * @retval ORTE_ERROR   Upon failures
     * 
     * This function is invoked during orte_init();
     */
    ORTE_DECLSPEC int orte_filem_base_open(void);
    
    /**
     * Select an available component.
     *
     * @retval ORTE_SUCCESS Upon Success
     * @retval ORTE_NOT_FOUND If no component can be selected
     * @retval ORTE_ERROR Upon other failure
     *
     */
    ORTE_DECLSPEC int orte_filem_base_select(void);
    
    /**
     * Finalize the FILEM MCA framework
     *
     * @retval ORTE_SUCCESS Upon success
     * @retval ORTE_ERROR   Upon failures
     * 
     * This function is invoked during orte_finalize();
     */
    ORTE_DECLSPEC int orte_filem_base_close(void);

    /**
     * Globals
     */
    ORTE_DECLSPEC extern int  orte_filem_base_output;
    ORTE_DECLSPEC extern opal_list_t orte_filem_base_components_available;
    ORTE_DECLSPEC extern orte_filem_base_component_t orte_filem_base_selected_component;
    ORTE_DECLSPEC extern orte_filem_base_module_t orte_filem;

    /**
     * 'None' component functions
     * These are to be used when no component is selected.
     * They just return success, and empty strings as necessary.
     */
    int orte_filem_base_none_open(void);
    int orte_filem_base_none_close(void);

    int orte_filem_base_module_init(void);
    int orte_filem_base_module_finalize(void);

    int orte_filem_base_none_put(orte_filem_base_request_t *request);
    int orte_filem_base_none_get(orte_filem_base_request_t *request);
    int orte_filem_base_none_rm( orte_filem_base_request_t *request);

    /**
     * Some utility functions
     */
    ORTE_DECLSPEC int orte_filem_base_listener_init(orte_rml_buffer_callback_fn_t rml_cbfunc);
    ORTE_DECLSPEC int orte_filem_base_listener_cancel(void);

    /**
     * Get Node Name for an ORTE process
     */
    ORTE_DECLSPEC int orte_filem_base_get_proc_node_name(orte_process_name_t *proc, char **machine_name);
    ORTE_DECLSPEC int orte_filem_base_query_remote_path(char **remote_ref, orte_process_name_t *peer, int *flag);
    ORTE_DECLSPEC void orte_filem_base_query_callback(int status,
                                                      orte_process_name_t* peer,
                                                      orte_buffer_t *buffer,
                                                      orte_rml_tag_t tag,
                                                      void* cbdata);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* ORTE_FILEM_BASE_H */
