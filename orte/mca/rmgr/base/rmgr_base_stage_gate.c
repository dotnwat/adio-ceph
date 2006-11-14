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
/** @file:
 */

/*
 * includes
 */
#include "orte_config.h"

#include <string.h>

#include "orte/orte_constants.h"
#include "orte/orte_types.h"

#include "opal/util/output.h"
#include "opal/util/trace.h"

#include "orte/dss/dss.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/smr/smr.h"

#include "orte/mca/rmgr/base/rmgr_private.h"


int orte_rmgr_base_proc_stage_gate_init(orte_jobid_t job)
{
    int rc;

    /* init the stage gates */
    if (ORTE_SUCCESS != (rc = orte_smr.init_job_stage_gates(job, orte_rmgr_base_proc_stage_gate_mgr, NULL))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
 
    return ORTE_SUCCESS;
}


int orte_rmgr_base_proc_stage_gate_mgr(orte_gpr_notify_message_t *msg)
{
    orte_buffer_t buffer;
    orte_process_name_t *recipients=NULL;
    orte_std_cntr_t n=0;
    int rc;
    orte_jobid_t job;
    opal_list_t attrs;
    opal_list_item_t *item;

    OPAL_TRACE(1);

    /* check to see if this came from a trigger that we ignore because
     * that stage gate does NOT set an xcast barrier - processes simply
     * record their state and continue processing. The only triggers that
     * involve a xcast barrier are the STGx and FINALIZED ones - ignore the rest.
      */
    if (!orte_schema.check_std_trigger_name(msg->target, ORTE_STG1_TRIGGER) &&
        !orte_schema.check_std_trigger_name(msg->target, ORTE_STG2_TRIGGER) &&
        !orte_schema.check_std_trigger_name(msg->target, ORTE_STG3_TRIGGER) &&
        !orte_schema.check_std_trigger_name(msg->target, ORTE_NUM_FINALIZED_TRIGGER)) {
        return ORTE_SUCCESS;
     }

     /* All stage gate triggers are named, so we can extract the jobid
      * directly from the trigger name
      */
     if (ORTE_SUCCESS != (rc = orte_schema.extract_jobid_from_std_trigger_name(&job, msg->target))) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }

    OPAL_TRACE_ARG1(1, job);
    
    /* need the list of peers for this job so we can send them the xcast.
        * obtain this list from the name service's get_job_peers function
        */
    OBJ_CONSTRUCT(&attrs, opal_list_t);
    if (ORTE_SUCCESS != (rc = orte_rmgr.add_attribute(&attrs, ORTE_NS_USE_JOBID, ORTE_JOBID,
                                                      &job, ORTE_RMGR_ATTR_OVERRIDE))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&attrs);
        return rc;
    }
    
    if (ORTE_SUCCESS != (rc = orte_ns.get_peers(&recipients, &n, &attrs))) {
        ORTE_ERROR_LOG(rc);
        goto CLEANUP;
    }
        
    /* set the job state to the appropriate level */
    if (orte_schema.check_std_trigger_name(msg->target, ORTE_ALL_LAUNCHED_TRIGGER)) {
        if (ORTE_SUCCESS != (rc = orte_smr.set_job_state(job, ORTE_JOB_STATE_LAUNCHED))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    } else if (orte_schema.check_std_trigger_name(msg->target, ORTE_STG1_TRIGGER)) {
        if (ORTE_SUCCESS != (rc = orte_smr.set_job_state(job, ORTE_JOB_STATE_AT_STG1))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    } else if (orte_schema.check_std_trigger_name(msg->target, ORTE_STG2_TRIGGER)) {
        if (ORTE_SUCCESS != (rc = orte_smr.set_job_state(job, ORTE_JOB_STATE_AT_STG2))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    } else if (orte_schema.check_std_trigger_name(msg->target, ORTE_STG3_TRIGGER)) {
        if (ORTE_SUCCESS != (rc = orte_smr.set_job_state(job, ORTE_JOB_STATE_AT_STG3))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    } else if (orte_schema.check_std_trigger_name(msg->target, ORTE_NUM_FINALIZED_TRIGGER)) {
        if (ORTE_SUCCESS != (rc = orte_smr.set_job_state(job, ORTE_JOB_STATE_FINALIZED))) {
            ORTE_ERROR_LOG(rc);
            goto CLEANUP;
        }
    }

    /* set the message type to SUBSCRIPTION. When we give this to the processes, we want
     * them to break the message down and deliver it to the various subsystems.
     */
    msg->msg_type = ORTE_GPR_SUBSCRIPTION_MSG;
    msg->id = ORTE_GPR_TRIGGER_ID_MAX;

    /* need to pack the msg for sending */
    OBJ_CONSTRUCT(&buffer, orte_buffer_t);
    if (ORTE_SUCCESS != (rc = orte_dss.pack(&buffer, &msg, 1, ORTE_GPR_NOTIFY_MSG))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&buffer);
        goto CLEANUP;
    }

    /* send the message */
    if (ORTE_SUCCESS != (rc = orte_rml.xcast(orte_process_info.my_name, recipients,
                                        n, &buffer, NULL, NULL))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_DESTRUCT(&buffer);

CLEANUP:
    if (NULL != recipients) free(recipients);

    while (NULL != (item = opal_list_remove_first(&attrs))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&attrs);
    
    return rc;
}
