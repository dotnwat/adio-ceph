/* -*- C -*-
 *
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
 *
 */
#ifndef ORTE_ERRMGR_PROXY_H
#define ORTE_ERRMGR_PROXY_H


#include "orte_config.h"
#include "orte/orte_types.h"

#include "orte/mca/ns/ns_types.h"
#include "orte/mca/gpr/gpr_types.h"

#include "orte/mca/errmgr/errmgr.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Module open / close
 */
int orte_errmgr_proxy_open(void);
int orte_errmgr_proxy_close(void);


/*
 * Startup / Shutdown
 */
orte_errmgr_base_module_t*
orte_errmgr_proxy_component_init(bool *allow_multi_user_threads, bool *have_hidden_threads, int *priority);

int orte_errmgr_proxy_finalize(void);

/*
 * globals used within the component
 */
typedef struct {
    int debug;
    orte_process_name_t *replica;
} orte_errmgr_proxy_globals_t;


extern orte_errmgr_proxy_globals_t orte_errmgr_proxy_globals;

/*
 * Component API functions
 */
int orte_errmgr_proxy_proc_aborted(orte_gpr_notify_message_t *msg);

int orte_errmgr_proxy_incomplete_start(orte_gpr_notify_message_t *msg);

void orte_errmgr_proxy_error_detected(int error_code, char *fmt, ...) __opal_attribute_format__(__printf__, 2, 3);

void orte_errmgr_proxy_abort(void) __opal_attribute_noreturn__;

int orte_errmgr_proxy_register_job(orte_jobid_t job);

int orte_errmgr_proxy_abort_procs_request(orte_process_name_t *procs, orte_std_cntr_t nprocs);

ORTE_MODULE_DECLSPEC extern mca_errmgr_base_component_t mca_errmgr_proxy_component;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
