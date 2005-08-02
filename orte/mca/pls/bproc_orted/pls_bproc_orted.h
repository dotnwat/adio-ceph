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

#ifndef ORTE_PLS_BPROC_ORTED_H_
#define ORTE_PLS_BPROC_ORTED_H_

#include "orte_config.h"

#include "opal/mca/mca.h"
#include "opal/threads/condition.h"
#include "orte/mca/pls/pls.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Module open / close
 */
int orte_pls_bproc_orted_component_open(void);
int orte_pls_bproc_orted_component_close(void);
orte_pls_base_module_t* orte_pls_bproc_orted_init(int *priority);
                                                                                  
/*
 * Startup / Shutdown
 */
int orte_pls_bproc_orted_finalize(void);

/*
 * Interface
 */
int orte_pls_bproc_orted_launch(orte_jobid_t);
int orte_pls_bproc_orted_terminate_job(orte_jobid_t);
int orte_pls_bproc_orted_terminate_proc(const orte_process_name_t* proc_name);
 
/**
 * PLS Component
 */
struct orte_pls_bproc_orted_component_t {
    orte_pls_base_component_t super;
    int debug;
    int priority;
    opal_mutex_t lock;
    opal_condition_t condition;

};
typedef struct orte_pls_bproc_orted_component_t orte_pls_bproc_orted_component_t;

ORTE_DECLSPEC extern orte_pls_bproc_orted_component_t mca_pls_bproc_orted_component;
ORTE_DECLSPEC extern orte_pls_base_module_t orte_pls_bproc_orted_module;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* ORTE_PLS_BPROC_ORTED_H_ */

