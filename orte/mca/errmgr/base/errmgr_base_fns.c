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
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>
#include "include/orte_constants.h"
#include "mca/schema/schema.h"

#include "runtime/runtime.h"
#include "runtime/orte_wait.h"
#include "opal/util/output.h"
#include "opal/util/trace.h"
#include "util/proc_info.h"
#include "mca/ns/ns.h"

#include "mca/rmgr/rmgr.h"

#include "mca/errmgr/base/base.h"


void orte_errmgr_base_log(int error_code, char *filename, int line)
{
    OPAL_TRACE(1);
    
    if (NULL == orte_process_info.my_name) {
        opal_output(0, "[NO-NAME] ORTE_ERROR_LOG: %s in file %s at line %d",
                                ORTE_ERROR_NAME(error_code), filename, line);
    } else {
        opal_output(0, "[%lu,%lu,%lu] ORTE_ERROR_LOG: %s in file %s at line %d",
                        ORTE_NAME_ARGS(orte_process_info.my_name),
                        ORTE_ERROR_NAME(error_code), filename, line);
    }
    /* orte_errmgr_base_error_detected(error_code); */
}

void orte_errmgr_base_proc_aborted(orte_process_name_t *proc)
{
    orte_jobid_t job;
    int rc;
    
    OPAL_TRACE(1);
    
    if (ORTE_SUCCESS != (rc = orte_ns.get_jobid(&job, proc))) {
        ORTE_ERROR_LOG(rc);
        return;
    }
    
    orte_rmgr.terminate_job(job);
}

void orte_errmgr_base_incomplete_start(orte_jobid_t job)
{
     OPAL_TRACE(1);
    
   orte_rmgr.terminate_job(job);
}

void orte_errmgr_base_error_detected(int error_code)
{
    OPAL_TRACE(1);
    
}

void orte_errmgr_base_abort()
{
    OPAL_TRACE(1);
    
    /* kill and reap all children */
    orte_wait_kill(9);

    /* abnormal exit */
    orte_abort(-1, NULL);
}

int orte_errmgr_base_register_job(orte_jobid_t job)
{
    /* register subscription for process_status values
     * changing to abnormal termination codes
     */
     
    OPAL_TRACE(1);
    
     return ORTE_SUCCESS;
}
