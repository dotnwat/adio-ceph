/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
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
/** @file:
 */

#ifndef ORTE_MCA_ERRMGR_PRIVATE_H
#define ORTE_MCA_ERRMGR_PRIVATE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/constants.h"
#include "orte/types.h"

#include "opal/dss/dss_types.h"
#include "orte/mca/plm/plm_types.h"
#include "orte/runtime/orte_globals.h"

#include "orte/mca/errmgr/errmgr.h"

/*
 * Functions for use solely within the ERRMGR framework
 */
BEGIN_C_DECLS

/* define a struct to hold framework-global values */
typedef struct {
    int output;
    bool shutting_down;
    opal_pointer_array_t modules;
    bool initialized;
} orte_errmgr_base_t;

ORTE_DECLSPEC extern orte_errmgr_base_t orte_errmgr_base;

/* Define the ERRMGR command flag */
typedef uint8_t orte_errmgr_cmd_flag_t;
#define ORTE_ERRMGR_CMD	OPAL_UINT8
    
/* define some commands */
#define ORTE_ERRMGR_ABORT_PROCS_REQUEST_CMD     0x01
#define ORTE_ERRMGR_REGISTER_CALLBACK_CMD       0x02

/*
 * Base functions
 */
ORTE_DECLSPEC void orte_errmgr_base_log(int error_code, char *filename, int line);

ORTE_DECLSPEC int orte_errmgr_base_update_state(orte_jobid_t job,
                                                orte_job_state_t jobstate,
                                                orte_process_name_t *proc_name,
                                                orte_proc_state_t state,
                                                orte_exit_code_t exit_code);
ORTE_DECLSPEC int orte_errmgr_base_abort(int error_code, char *fmt, ...)
#   if OPAL_HAVE_ATTRIBUTE_FORMAT_FUNCPTR
    __opal_attribute_format__(__printf__, 2, 3)
#   endif
    ;
ORTE_DECLSPEC int orte_errmgr_base_predicted_fault(char ***proc_list,
                                                   char ***node_list,
                                                   char ***suggested_nodes);
ORTE_DECLSPEC int orte_errmgr_base_suggest_map_targets(orte_proc_t *proc,
                                                       orte_node_t *oldnode,
                                                       opal_list_t *node_list);
ORTE_DECLSPEC int orte_errmgr_base_ft_event(int state);

/*
 * Additional External API function declared in errmgr.h
 */

END_C_DECLS
#endif
