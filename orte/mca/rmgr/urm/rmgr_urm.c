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
#include <errno.h>
#include <unistd.h>
#include <string.h>

#include "opal/util/trace.h"

#include "orte/include/orte_constants.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rds/base/base.h"
#include "orte/mca/ras/base/base.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/pls/base/base.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/iof/iof.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/rml/rml.h"

#include "orte/mca/rmgr/urm/rmgr_urm.h"


static int orte_rmgr_urm_query(void);

static int orte_rmgr_urm_create(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid);

static int orte_rmgr_urm_allocate(
    orte_jobid_t jobid);

static int orte_rmgr_urm_deallocate(
    orte_jobid_t jobid);

static int orte_rmgr_urm_map(
    orte_jobid_t jobid);

static int orte_rmgr_urm_launch(
    orte_jobid_t jobid);

static int orte_rmgr_urm_terminate_job(
    orte_jobid_t jobid);

static int orte_rmgr_urm_terminate_proc(
    const orte_process_name_t* proc_name);

static int orte_rmgr_urm_spawn(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid,
    orte_rmgr_cb_fn_t cbfn);

static int orte_rmgr_urm_finalize(void);


orte_rmgr_base_module_t orte_rmgr_urm_module = {
    orte_rmgr_urm_query,
    orte_rmgr_urm_create,
    orte_rmgr_urm_allocate,
    orte_rmgr_urm_deallocate,
    orte_rmgr_urm_map,
    orte_rmgr_urm_launch,
    orte_rmgr_urm_terminate_job,
    orte_rmgr_urm_terminate_proc,
    orte_rmgr_urm_spawn,
    orte_rmgr_base_proc_stage_gate_init,
    orte_rmgr_base_proc_stage_gate_mgr,
    orte_rmgr_urm_finalize
};


/*
 * Resource discovery
 */

static int orte_rmgr_urm_query(void)
{
    int rc;

    OPAL_TRACE(1);
    
    if(ORTE_SUCCESS != (rc = orte_rds_base_query())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    return ORTE_SUCCESS;
}


/*
 *  Create the job segment and initialize the application context.
 */

static int orte_rmgr_urm_create(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid)
{
    int rc;

    OPAL_TRACE(1);
    
    /* allocate a jobid  */
    if (ORTE_SUCCESS != (rc = orte_ns.create_jobid(jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* create and initialize job segment */
    if (ORTE_SUCCESS != 
        (rc = orte_rmgr_base_put_app_context(*jobid, app_context, 
                                             num_context))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* setup the launch stage gate counters and subscriptions */
    if (ORTE_SUCCESS !=
        (rc = orte_rmgr_base_proc_stage_gate_init(*jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
   
    return ORTE_SUCCESS;
}


static int orte_rmgr_urm_allocate(orte_jobid_t jobid)
{
     OPAL_TRACE(1);
    
   return mca_rmgr_urm_component.urm_ras->allocate(jobid);
}

static int orte_rmgr_urm_deallocate(orte_jobid_t jobid)
{
    OPAL_TRACE(1);
    
    return mca_rmgr_urm_component.urm_ras->deallocate(jobid);
}

static int orte_rmgr_urm_map(orte_jobid_t jobid)
{
    OPAL_TRACE(1);
    
    return mca_rmgr_urm_component.urm_rmaps->map(jobid);
}

static int orte_rmgr_urm_launch(orte_jobid_t jobid)
{
    OPAL_TRACE(1);
    
    return mca_rmgr_urm_component.urm_pls->launch(jobid);
}

static int orte_rmgr_urm_terminate_job(orte_jobid_t jobid)
{
    OPAL_TRACE(1);
    
    return mca_rmgr_urm_component.urm_pls->terminate_job(jobid);
}

static int orte_rmgr_urm_terminate_proc(const orte_process_name_t* proc_name)
{
    OPAL_TRACE(1);
    
    return mca_rmgr_urm_component.urm_pls->terminate_proc(proc_name);
}


static void orte_rmgr_urm_wireup_stdin(orte_jobid_t jobid)
{
    int rc;
    orte_process_name_t* name;

    OPAL_TRACE(1);
    
    if (ORTE_SUCCESS != (rc = orte_ns.create_process_name(&name, 0, jobid, 0))) {
        ORTE_ERROR_LOG(rc);
        return;
    }
    if (ORTE_SUCCESS != (rc = orte_iof.iof_push(name, ORTE_NS_CMP_JOBID, ORTE_IOF_STDIN, 0))) {
        ORTE_ERROR_LOG(rc);
    }
}


static void orte_rmgr_urm_callback(orte_gpr_notify_data_t *data, void *cbdata)
{
    orte_rmgr_cb_fn_t cbfunc = (orte_rmgr_cb_fn_t)cbdata;
    orte_gpr_value_t **values, *value;
    orte_gpr_keyval_t** keyvals;
    orte_jobid_t jobid;
    size_t i, j, k;
    int rc;

    OPAL_TRACE(1);
    
    /* we made sure in the subscriptions that at least one
     * value is always returned
     * get the jobid from the segment name in the first value
     */
    values = (orte_gpr_value_t**)(data->values)->addr;
    if (ORTE_SUCCESS != (rc =
            orte_schema.extract_jobid_from_segment_name(&jobid,
                        values[0]->segment))) {
        ORTE_ERROR_LOG(rc);
        return;
    }

    for(i = 0, k=0; k < data->cnt &&
                    i < (data->values)->size; i++) {
        if (NULL != values[i]) {
            k++;
            value = values[i];
            /* determine the state change */
            keyvals = value->keyvals;
            for(j=0; j<value->cnt; j++) {
                orte_gpr_keyval_t* keyval = keyvals[j];
                if(strcmp(keyval->key, ORTE_PROC_NUM_AT_STG1) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_AT_STG1);
                    /* BWB - XXX - FIX ME: this needs to happen when all
                       are LAUNCHED, before STG1 */
                    orte_rmgr_urm_wireup_stdin(jobid);
                    continue;
                }
                if(strcmp(keyval->key, ORTE_PROC_NUM_AT_STG2) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_AT_STG2);
                    continue;
                }
                if(strcmp(keyval->key, ORTE_PROC_NUM_AT_STG3) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_AT_STG3);
                    continue;
                }
                if(strcmp(keyval->key, ORTE_PROC_NUM_FINALIZED) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_FINALIZED);
                    continue;
                }
                if(strcmp(keyval->key, ORTE_PROC_NUM_TERMINATED) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_TERMINATED);
                    continue;
                }
                if(strcmp(keyval->key, ORTE_PROC_NUM_ABORTED) == 0) {
                    (*cbfunc)(jobid,ORTE_PROC_STATE_ABORTED);
                    continue;
                }
            }
        }
    }
}


/*
 *  Shortcut for the multiple steps involved in spawning a new job.
 */


static int orte_rmgr_urm_spawn(
    orte_app_context_t** app_context,
    size_t num_context,
    orte_jobid_t* jobid,
    orte_rmgr_cb_fn_t cbfunc)
{
    int rc;
    orte_process_name_t* name;
 
    OPAL_TRACE(1);
    
    /* 
     * Perform resource discovery.
     */
    if (mca_rmgr_urm_component.urm_rds == false &&
        ORTE_SUCCESS != (rc = orte_rds_base_query())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    } else {
        mca_rmgr_urm_component.urm_rds = true;
    }

    /*
     * Initialize job segment and allocate resources
     */
    if (ORTE_SUCCESS != 
        (rc = orte_rmgr_urm_create(app_context,num_context,jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_rmgr_urm_allocate(*jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_rmgr_urm_map(*jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*
     * setup I/O forwarding 
     */

    if (ORTE_SUCCESS != (rc = orte_ns.create_process_name(&name, 0, *jobid, 0))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_iof.iof_pull(name, ORTE_NS_CMP_JOBID, ORTE_IOF_STDOUT, 1))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if (ORTE_SUCCESS != (rc = orte_iof.iof_pull(name, ORTE_NS_CMP_JOBID, ORTE_IOF_STDERR, 2))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /*
     * setup callback
     */

    if(NULL != cbfunc) {
        rc = orte_rmgr_base_proc_stage_gate_subscribe(*jobid, orte_rmgr_urm_callback, (void*)cbfunc, ORTE_STAGE_GATE_ALL);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    /*
     * launch the job
     */
    if (ORTE_SUCCESS != (rc = orte_rmgr_urm_launch(*jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    orte_ns.free_name(&name);
    return ORTE_SUCCESS;
}


static int orte_rmgr_urm_finalize(void)
{
    int rc;

    OPAL_TRACE(1);
    
    /**
     * Finalize Process Launch Subsystem (PLS)
     */
    if (ORTE_SUCCESS != (rc = orte_pls_base_finalize())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /**
     * Finalize Resource Mapping Subsystem (RMAPS)
     */
    if (ORTE_SUCCESS != (rc = orte_rmaps_base_finalize())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /**
     * Finalize Resource Allocation Subsystem (RAS)
     */
    if (ORTE_SUCCESS != (rc = orte_ras_base_finalize())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /**
     * Finalize Resource Discovery Subsystem (RDS)
     */
    if (ORTE_SUCCESS != (rc = orte_rds_base_finalize())) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* Cancel pending receive. */

    orte_rml.recv_cancel(ORTE_RML_NAME_ANY, ORTE_RML_TAG_RMGR_SVC);

    return ORTE_SUCCESS;
}

#if 0
static void orte_rmgr_urm_recv(
    int status,
    orte_process_name_t* peer,
    orte_buffer_t* req,
    orte_rml_tag_t tag,
    void* cbdata)
{
    return;
}
#endif

