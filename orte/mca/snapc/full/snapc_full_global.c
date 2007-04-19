/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University.
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

#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */

#include "opal/threads/mutex.h"
#include "opal/threads/condition.h"
#include "opal/util/output.h"
#include "opal/util/show_help.h"
#include "opal/util/argv.h"
#include "opal/util/opal_environ.h"
#include "opal/util/basename.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/mca/crs/crs.h"
#include "opal/mca/crs/base/base.h"

#include "orte/orte_constants.h"
#include "orte/runtime/params.h"
#include "orte/dss/dss.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/rmgr/rmgr.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/filem/filem.h"
#include "orte/mca/pls/pls.h"
#include "orte/mca/snapc/snapc.h"
#include "orte/mca/snapc/base/base.h"

#include "snapc_full.h"

/************************************
 * Locally Global vars & functions :)
 ************************************/
/* RML Callback */
static void snapc_full_global_recv(int status, 
                                   orte_process_name_t* sender,
                                   orte_buffer_t *buffer, 
                                   orte_rml_tag_t tag,
                                   void* cbdata);
static int  snapc_full_global_checkpoint(orte_jobid_t jobid, 
                                         bool term, 
                                         char **global_snapshot_handle, 
                                         int *ckpt_status);
static int  snapc_full_reg_vpid_state_updates( orte_jobid_t jobid, 
                                               orte_vpid_t *vpid_start, 
                                               orte_vpid_t *vpid_range);
static int  snapc_full_get_vpid_range( orte_jobid_t jobid, 
                                       orte_vpid_t *vpid_start, 
                                       orte_vpid_t *vpid_range);
static void job_ckpt_request_callback(orte_gpr_notify_data_t *data, void *cbdata);
static void vpid_ckpt_state_callback( orte_gpr_notify_data_t *data, void *cbdata);
static int  snapc_full_global_notify_checkpoint( char *       global_snapshot_handle,
                                                 orte_jobid_t jobid, 
                                                 orte_vpid_t  vpid_start, 
                                                 orte_vpid_t  vpid_range,
                                                 bool term);
static int snapc_full_global_check_for_done(orte_jobid_t jobid);

static int  snapc_full_global_gather_all_files(void);
static bool snapc_full_global_is_done_yet(void);

static opal_mutex_t global_coord_mutex;

static orte_snapc_base_global_snapshot_t global_snapshot;
static orte_process_name_t orte_checkpoint_sender;
static bool updated_job_to_running;

/************************
 * Function Definitions
 ************************/
int global_coord_init(void) {
    /* Create a lock so that we don't try to do this multiple times */
    OBJ_CONSTRUCT(&global_coord_mutex, opal_mutex_t);

    return ORTE_SUCCESS;
}

int global_coord_finalize(void) {

    OBJ_DESTRUCT(&global_coord_mutex);

    return ORTE_SUCCESS;
}

int global_coord_setup_job(orte_jobid_t jobid) {
    int ret, exit_status = ORTE_SUCCESS;
    orte_vpid_t vpid_start = 0, vpid_range = 0;
    orte_std_cntr_t i;

    /*
     * Start out with a sequence number just below the first
     * This will be incremented when we checkpoint
     */
    orte_snapc_base_snapshot_seq_number = -1;

    /*
     * Setup the checkpoint request callbacks
     */
    if(ORTE_SUCCESS != (ret = orte_snapc_base_global_init_request(jobid,
                                                                  snapc_full_global_recv,    NULL, /* RML */
                                                                  job_ckpt_request_callback, NULL) /* GPR */
                        ) ) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Setup GPR Callbacks triggered by Local Snapshot Coordinator.
     * This indicates that a checkpoint has been completed locally on 
     *  a node.
     */
    if( ORTE_SUCCESS != (ret = snapc_full_reg_vpid_state_updates(jobid, &vpid_start, &vpid_range))) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Allocate the snapshot structures
     */
    OBJ_CONSTRUCT(&global_snapshot, orte_snapc_base_global_snapshot_t);
    global_snapshot.component_name = strdup(mca_snapc_full_component.super.snapc_version.mca_component_name);
    for(i = vpid_start; i < vpid_start + vpid_range; ++i) {
        orte_snapc_base_snapshot_t *vpid_snapshot;
        
        vpid_snapshot = OBJ_NEW(orte_snapc_base_snapshot_t);

        vpid_snapshot->process_name.cellid = 0;
        vpid_snapshot->process_name.jobid  = jobid;
        vpid_snapshot->process_name.vpid   = i;
        vpid_snapshot->term = false;

        opal_list_append(&global_snapshot.snapshots, &(vpid_snapshot->crs_snapshot_super.super));
    }
    
    opal_output_verbose(10, mca_snapc_full_component.super.output_handle,
                        "global [%d]) Setup job (%d) with vpid [%d, %d]\n", getpid(), jobid, vpid_start, vpid_range);

 cleanup:
    return exit_status;
}

int global_coord_release_job(orte_jobid_t jobid) {
    int exit_status = ORTE_SUCCESS;

    /*
     * Make sure we are not waiting on a checkpoint to complete
     */

    /*
     * Clean up RML Callback
     */
    
    /*
     * Cleanup GPR Callbacks
     */

    OBJ_DESTRUCT(&global_snapshot);
    
    return exit_status;
}

/******************
 * Local functions
 ******************/
static void 
snapc_full_global_recv(int status, orte_process_name_t* sender,
                       orte_buffer_t *buffer, orte_rml_tag_t tag,
                       void* cbdata) {
    int ret, exit_status = ORTE_SUCCESS;
    size_t command;
    orte_std_cntr_t n = 1;
    bool term = false;
    int ckpt_status = ORTE_SUCCESS;
    char *global_snapshot_handle = NULL;
    orte_jobid_t jobid;

    OPAL_THREAD_LOCK(&global_coord_mutex);

    n = 1;
    if (ORTE_SUCCESS != (ret = orte_dss.unpack(buffer, &command, &n, ORTE_CKPT_CMD))) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * orte_checkpoint has requested that a checkpoint be taken
     */
    if (ORTE_SNAPC_GLOBAL_INIT_CMD == command) {
        /********************
         * Do the basic handshake with the orte_checkpoint command
         ********************/
        if( ORTE_SUCCESS != (ret = orte_snapc_base_global_coord_ckpt_init_cmd(sender, &term, &jobid)) ) {
            exit_status = ret;
            goto cleanup;
        }

        /* Save things */
        orte_checkpoint_sender = *sender;
        
        /*************************
         * Kick of the checkpoint
         *************************/
        if( ORTE_SUCCESS != (ret = snapc_full_global_checkpoint(jobid, term, &global_snapshot_handle, &ckpt_status) ) ) {
            exit_status = ret;
            /* We don't want to terminate here, becase orte_checkpoint may be waiting for
             * us to come back with something, so just send back the empty values, and
             * it will know what to do
             */
        }
        
    }
    else if (ORTE_SNAPC_GLOBAL_TERM_CMD == command) {
        /* Something must have happened so we are forced to terminate */
        goto cleanup;
    }

    /*
     * Unknown command
     */
    else {
        goto cleanup;
    }
    
 cleanup:

    return;
}

static int 
snapc_full_global_checkpoint(orte_jobid_t jobid, bool term, char **global_snapshot_handle, int *ckpt_status) {
    int ret, exit_status = ORTE_SUCCESS;
    orte_vpid_t vpid_start, vpid_range;

    opal_output_verbose(10, mca_snapc_full_component.super.output_handle,
                        "global) Checkpoint of job (%d) has been requested\n", jobid);

    /*********************
     * Generate the global snapshot directory, and unique global snapshot handle
     *********************/
    ++orte_snapc_base_snapshot_seq_number;
    *global_snapshot_handle = strdup( orte_snapc_base_unique_global_snapshot_name( getpid() ) );

    global_snapshot.seq_num = orte_snapc_base_snapshot_seq_number;
    global_snapshot.reference_name = strdup(*global_snapshot_handle);
    global_snapshot.local_location = opal_dirname(orte_snapc_base_get_global_snapshot_directory(global_snapshot.reference_name));

    /* Creates the directory (with metadata files):
     *   /tmp/ompi_global_snapshot_PID.ckpt/seq_num
     */
    if( ORTE_SUCCESS != (ret = orte_snapc_base_init_global_snapshot_directory(*global_snapshot_handle))) {
        exit_status = ret;
        goto cleanup;
    }

    /***********************************
     * Do an update handshake with the orte_checkpoint command
     ***********************************/
    updated_job_to_running = false;
    if( ORTE_SUCCESS != (ret = orte_snapc_base_global_coord_ckpt_update_cmd(&orte_checkpoint_sender,
                                                                            global_snapshot.reference_name,
                                                                            global_snapshot.seq_num,
                                                                            ORTE_SNAPC_CKPT_STATE_REQUEST) ) ) {
        exit_status = ret;
        goto cleanup;
    }

    opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                        "global) Using the checkpoint directory (%s)\n", *global_snapshot_handle);

    /**********************
     * Notify the Local Snapshot Coordinators by putting necessary values into the GPR 
     **********************/
    if( ORTE_SUCCESS != (ret = snapc_full_get_vpid_range(jobid, &vpid_start, &vpid_range) ) ) {
        exit_status = ret;
        goto cleanup;
    }

    opal_output_verbose(15, mca_snapc_full_component.super.output_handle,
                        "global) Notifying the individual processes (%d, %d)\n", vpid_start, vpid_range);

    if( ORTE_SUCCESS != (ret = snapc_full_global_notify_checkpoint(*global_snapshot_handle, 
                                                                   jobid, 
                                                                   vpid_start, 
                                                                   vpid_range,
                                                                   term))) {
        exit_status = ret;
        goto cleanup;
    }

 cleanup:

    *ckpt_status = exit_status;
    return exit_status; 
}

static void job_ckpt_request_callback(orte_gpr_notify_data_t *data, void *cbdata) {
    int ret, exit_status = ORTE_SUCCESS;
    orte_gpr_value_t **values;
    orte_jobid_t jobid;
    int ckpt_status = ORTE_SUCCESS;
    char *global_snapshot_handle = NULL;
    bool term = false;
    orte_std_cntr_t i;
    size_t job_ckpt_state = ORTE_SNAPC_CKPT_STATE_NONE;
    size_t *size_ptr;

    /*
     * Get jobid from the segment name in the first value
     */
    values = (orte_gpr_value_t**)(data->values)->addr;
    if (ORTE_SUCCESS != (ret = orte_schema.extract_jobid_from_segment_name(&jobid,
                                                                           values[0]->segment))) {
        exit_status = ret;
        goto cleanup;
    }
    
    /*
     * Get the state change (ORTE_JOB_CKPT_STATE_KEY)
     */
    for( i = 0; i < values[0]->cnt; ++i) {
        orte_gpr_keyval_t* keyval = values[0]->keyvals[i];
        if(strcmp(keyval->key, ORTE_JOB_CKPT_STATE_KEY) == 0) {
            if (ORTE_SUCCESS != (ret = orte_dss.get((void**)&(size_ptr), keyval->value, ORTE_SIZE))) {
                exit_status = ret;
                goto cleanup;
            }
            job_ckpt_state = *size_ptr;
        }
    }
    
    if(ORTE_SNAPC_CKPT_STATE_REQUEST == job_ckpt_state ) {
        /*
         * Start the checkpoint, now that we have the jobid
         */
        if( ORTE_SUCCESS != (ret = snapc_full_global_checkpoint(jobid, term, &global_snapshot_handle, &ckpt_status) ) ) {
            exit_status = ret;
            goto cleanup;
        }
    }
    else if( ORTE_SNAPC_CKPT_STATE_FINISHED != job_ckpt_state) {
        /*
         * Update the orte-checkpoint cmd
         */
        if( ORTE_SUCCESS != (ret = orte_snapc_base_global_coord_ckpt_update_cmd(&orte_checkpoint_sender,
                                                                                global_snapshot.reference_name,
                                                                                global_snapshot.seq_num,
                                                                                job_ckpt_state) ) ) {
            exit_status = ret;
            goto cleanup;
        }
    }

 cleanup:
    return;
}

static void vpid_ckpt_state_callback(orte_gpr_notify_data_t *data, void *cbdata) {
    int ret, exit_status = ORTE_SUCCESS;
    orte_process_name_t *proc;
    size_t ckpt_state;
    char *ckpt_ref, *ckpt_loc;
    opal_list_item_t* item = NULL;

    /*
     * Now we know which vpid changed, now we must figure out what to
     */
    if( ORTE_SUCCESS != (ret = orte_snapc_base_extract_gpr_vpid_ckpt_info(data, 
                                                                          &proc,
                                                                          &ckpt_state,
                                                                          &ckpt_ref,
                                                                          &ckpt_loc) ) ) {
        exit_status = ret;
        goto cleanup;
    }

    opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                        "global) Process (%d,%d,%d): Changed to state to:\n",
                         proc->cellid, 
                         proc->jobid, 
                         proc->vpid);
    opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                        "global) State:            %d\n", (int)ckpt_state);
    opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                        "global) Snapshot Ref:    (%s)\n", ckpt_ref);
    opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                        "global) Remote Location: (%s)\n", ckpt_loc);

    /*
     * Find this process and update it's information
     */
    for(item  = opal_list_get_first(&global_snapshot.snapshots);
        item != opal_list_get_end(&global_snapshot.snapshots);
        item  = opal_list_get_next(item) ) {
        orte_snapc_base_snapshot_t *vpid_snapshot;
        vpid_snapshot = (orte_snapc_base_snapshot_t*)item;

        if(vpid_snapshot->process_name.cellid == proc->cellid &&
           vpid_snapshot->process_name.jobid  == proc->jobid &&
           vpid_snapshot->process_name.vpid   == proc->vpid) {

            vpid_snapshot->state               = ckpt_state;
            vpid_snapshot->crs_snapshot_super.reference_name  = strdup(ckpt_ref);
            vpid_snapshot->crs_snapshot_super.remote_location = strdup(ckpt_loc);
            
            if(ckpt_state == ORTE_SNAPC_CKPT_STATE_FINISHED) {
                snapc_full_global_check_for_done(vpid_snapshot->process_name.jobid);
            }
            break;
        }
    }

    if( !updated_job_to_running) {
        char * global_dir = NULL;
        global_dir = orte_snapc_base_get_global_snapshot_directory(global_snapshot.reference_name);

        if( ORTE_SUCCESS != (ret = orte_snapc_base_set_job_ckpt_info(proc->jobid,
                                                                     ORTE_SNAPC_CKPT_STATE_RUNNING,
                                                                     global_snapshot.reference_name,
                                                                     global_dir) ) ) {
            free(global_dir);
            exit_status = ret;
            goto cleanup;
        }

        free(global_dir);
        updated_job_to_running = true;
    }

 cleanup:

    return;
}

static int snapc_full_get_vpid_range( orte_jobid_t jobid, orte_vpid_t *vpid_start, orte_vpid_t *vpid_range) {
    int ret, exit_status = ORTE_SUCCESS;
    char *segment = NULL;
    char *keys[] = {
        ORTE_JOB_VPID_START_KEY,
        ORTE_JOB_VPID_RANGE_KEY,
        NULL
    };
    orte_gpr_value_t** values = NULL;
    orte_std_cntr_t i, k, num_values = 0;

    *vpid_start = 0;
    *vpid_range = 0;
    
    /*
     * Get job segment
     */
    if(ORTE_SUCCESS != (ret = orte_schema.get_job_segment_name(&segment, jobid))) {
        exit_status = ret;
        goto cleanup;
    }
    
    /*
     * Get values from GPR
     */
    if(ORTE_SUCCESS != (ret = orte_gpr.get(ORTE_GPR_KEYS_OR | ORTE_GPR_TOKENS_OR,
                                           segment,
                                           NULL,
                                           keys,
                                           &num_values,
                                           &values ) )) {
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Parse out the values
     */
    for( i = 0; i < num_values; ++i) {
        orte_gpr_value_t* value = values[i];
        orte_vpid_t *loc_vpid;

        for ( k = 0; k < value->cnt; ++k) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            
            if(strcmp(keyval->key, ORTE_JOB_VPID_START_KEY) == 0) {
                if (ORTE_SUCCESS != (ret = orte_dss.get((void**)&(loc_vpid), keyval->value, ORTE_VPID))) {
                    exit_status = ret;
                    goto cleanup;
                }
                *vpid_start = *loc_vpid;
            }
            else if(strcmp(keyval->key, ORTE_JOB_VPID_RANGE_KEY) == 0) {
                if (ORTE_SUCCESS != (ret = orte_dss.get((void**)&(loc_vpid), keyval->value, ORTE_VPID))) {
                    exit_status = ret;
                    goto cleanup;
                }
                *vpid_range = *loc_vpid;
            }
            
        }
    }
    
 cleanup:
    if( NULL != segment) 
        free(segment);

    return exit_status;
}

static int snapc_full_reg_vpid_state_updates( orte_jobid_t jobid, orte_vpid_t *vpid_start, orte_vpid_t *vpid_range ) {
    int ret, exit_status = ORTE_SUCCESS;
    char *segment = NULL, *trig_name = NULL, **tokens = NULL;
    orte_gpr_subscription_id_t id;
    char* keys[] = {
        ORTE_PROC_CKPT_STATE_KEY,
        ORTE_PROC_CKPT_SNAPSHOT_REF_KEY,
        ORTE_PROC_CKPT_SNAPSHOT_LOC_KEY,
        NULL
    };
    char* trig_names[] = {
        ORTE_PROC_CKPT_STATE_TRIGGER,
        NULL
    };
    orte_std_cntr_t num_tokens;
    orte_process_name_t proc;
    orte_vpid_t vpid;


    /*
     * Get the vpid range for this job
     */
    if( ORTE_SUCCESS != (ret = snapc_full_get_vpid_range(jobid, 
                                                         vpid_start, 
                                                         vpid_range)) ) {
        exit_status = ret;
        goto cleanup;
    }
    
    /*
     * Identify the segment for this job
     */
    if( ORTE_SUCCESS != (ret = orte_schema.get_job_segment_name(&segment, jobid))) {
        exit_status = ret;
        goto cleanup;
    }
    
    for ( vpid = *vpid_start; vpid < *vpid_start + *vpid_range; ++vpid) {
        proc.cellid = 0;
        proc.jobid  = jobid;
        proc.vpid   = vpid;

        /*
         * Setup the tokens
         */
        if (ORTE_SUCCESS != (ret = orte_schema.get_proc_tokens(&tokens,
                                                               &num_tokens,
                                                               &proc) )) {
            exit_status = ret;
            goto cleanup;
        }
        
        /*
         * Attach to the standard trigger
         */
        if( ORTE_SUCCESS != (ret = orte_schema.get_std_trigger_name(&trig_name, trig_names[0], jobid))) {
            exit_status = ret;
            goto cleanup;
        }
        
        /*
         * Subscribe to the GPR
         */
        if( ORTE_SUCCESS != (ret = orte_gpr.subscribe_N(&id,
                                                        trig_name,
                                                        NULL,
                                                        ORTE_GPR_NOTIFY_VALUE_CHG,
                                                        ORTE_GPR_TOKENS_OR | ORTE_GPR_KEYS_OR,
                                                        segment,
                                                        tokens,
                                                        3,
                                                        keys,
                                                        vpid_ckpt_state_callback,
                                                        NULL))) {
            exit_status = ret;
            goto cleanup;
        }
        
    }
    

 cleanup:
    if(NULL != segment)
        free(segment);
    if(NULL != trig_name)
        free(trig_name);

    return exit_status;
}

static int snapc_full_global_notify_checkpoint( char * global_snapshot_handle, 
                                                orte_jobid_t jobid, 
                                                orte_vpid_t  vpid_start, 
                                                orte_vpid_t  vpid_range,
                                                bool term) {
    int ret, exit_status = ORTE_SUCCESS;
    opal_list_item_t* item = NULL;
    char * global_dir = NULL;
    size_t ckpt_state = ORTE_SNAPC_CKPT_STATE_PENDING;

    global_dir = orte_snapc_base_get_global_snapshot_directory(global_snapshot_handle);

    if( term ) {
        ckpt_state = ORTE_SNAPC_CKPT_STATE_PENDING_TERM;
    }

    /*
     * By updating the job segment we tell the Local Coordinator to
     * checkpoint all their apps, so we don't need to do it explicitly here
     * Just update the global structure here...
     */
    for(item  = opal_list_get_first(&global_snapshot.snapshots);
        item != opal_list_get_end(&global_snapshot.snapshots);
        item  = opal_list_get_next(item) ) {
        orte_snapc_base_snapshot_t *vpid_snapshot;

        vpid_snapshot = (orte_snapc_base_snapshot_t*)item;

        vpid_snapshot->state   = ckpt_state;
        vpid_snapshot->term    = term;

        if( NULL != vpid_snapshot->crs_snapshot_super.reference_name)
            free(vpid_snapshot->crs_snapshot_super.reference_name);
        vpid_snapshot->crs_snapshot_super.reference_name = opal_crs_base_unique_snapshot_name(vpid_snapshot->process_name.vpid);

        if( NULL != vpid_snapshot->crs_snapshot_super.local_location)
            free(vpid_snapshot->crs_snapshot_super.local_location);
        asprintf(&(vpid_snapshot->crs_snapshot_super.local_location), "%s/%s", global_dir, vpid_snapshot->crs_snapshot_super.reference_name);

        if( NULL != vpid_snapshot->crs_snapshot_super.remote_location)
            free(vpid_snapshot->crs_snapshot_super.remote_location);
        asprintf(&(vpid_snapshot->crs_snapshot_super.remote_location), "%s/%s", global_dir, vpid_snapshot->crs_snapshot_super.reference_name);

#if 0
        /* JJH -- Redundant, but complete :/
         * When we set the job state, it will automaticly update the vpid information locally to the
         * correct data. The GPR will just be out of date for a short term (until the local coodinator
         * gets the update notification, changes the values locally, and puts them back in the GPR).
         */
        /* Update information in the GPR */
        if (ORTE_SUCCESS != (ret = orte_snapc_base_set_vpid_ckpt_info(vpid_snapshot->process_name,
                                                                      /* STATE_NONE Because we don't want to trigger the local daemon just yet */
                                                                      ORTE_SNAPC_CKPT_STATE_NONE,
                                                                      vpid_snapshot->crs_snapshot_super.reference_name,
                                                                      vpid_snapshot->crs_snapshot_super.local_location) ) ) {
            exit_status = ret;
            goto cleanup;
        }
#endif
    }

    /*
     * Update the job global segment
     */
    if( ORTE_SUCCESS != (ret = orte_snapc_base_set_job_ckpt_info(jobid,
                                                                 ckpt_state,
                                                                 global_snapshot_handle,
                                                                 global_dir) ) ) {
        exit_status = ret;
        goto cleanup;
    }

 cleanup:
    if( NULL != global_dir)
        free(global_dir);

    return exit_status;
}

static int snapc_full_global_check_for_done(orte_jobid_t jobid) {
    int ret, exit_status = ORTE_SUCCESS;
    size_t ckpt_status = ORTE_SNAPC_CKPT_STATE_FINISHED;
    opal_list_item_t* item = NULL;
    char * global_dir = NULL;
    bool term_job  = false;

    /* If we are not done, then keep waiting */
    if(!snapc_full_global_is_done_yet()) {
        return exit_status;
    }
    
    /**********************
     * Gather all of the files locally
     **********************/
    if( ORTE_SUCCESS != (ret = snapc_full_global_gather_all_files()) ) {
        exit_status = ret;
        goto cleanup;
    }
    
    /**********************************
     * Update the job checkpoint state
     **********************************/
    global_dir = orte_snapc_base_get_global_snapshot_directory(global_snapshot.reference_name);

    if( ORTE_SUCCESS != (ret = orte_snapc_base_set_job_ckpt_info(jobid,
                                                                 ORTE_SNAPC_CKPT_STATE_FINISHED,
                                                                 global_snapshot.reference_name,
                                                                 global_dir) ) ) {
        exit_status = ret;
        goto cleanup;
    }

    /***********************************
     * Update the vpid checkpoint state
     ***********************************/
    for(item  = opal_list_get_first(&global_snapshot.snapshots);
        item != opal_list_get_end(&global_snapshot.snapshots);
        item  = opal_list_get_next(item) ) {
        orte_snapc_base_snapshot_t *vpid_snapshot;
        vpid_snapshot = (orte_snapc_base_snapshot_t*)item;

        vpid_snapshot->state = ORTE_SNAPC_CKPT_STATE_NONE;

        if( vpid_snapshot->term ){
            term_job = true;
        }

        if (ORTE_SUCCESS != (ret = orte_snapc_base_set_vpid_ckpt_info(vpid_snapshot->process_name,
                                                                      vpid_snapshot->state,
                                                                      vpid_snapshot->crs_snapshot_super.reference_name,
                                                                      vpid_snapshot->crs_snapshot_super.local_location) ) ) {
            exit_status = ret;
            goto cleanup;
        }
    }
    

    /************************
     * Do the final handshake with the orte_checkpoint command
     ************************/
    if( ORTE_SUCCESS != (ret = orte_snapc_base_global_coord_ckpt_update_cmd(&orte_checkpoint_sender, 
                                                                            global_snapshot.reference_name,
                                                                            global_snapshot.seq_num,
                                                                            ckpt_status)) ) {
        exit_status = ret;
        goto cleanup;
    }
    
    /************************
     * Set up the RML listener again
     *************************/
    if( ORTE_SUCCESS != (ret = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD,
                                                       ORTE_RML_TAG_CKPT,
                                                       0,
                                                       snapc_full_global_recv,
                                                       NULL)) ) {
        exit_status = ret;
    }

    /********************************
     * Terminate the job if requested
     *********************************/
    if( term_job ) {
        orte_pls.terminate_job(jobid, &orte_abort_timeout, NULL);
    }
            
    OPAL_THREAD_UNLOCK(&global_coord_mutex);
    
 cleanup:
    if( NULL != global_dir) 
        free(global_dir);

    return exit_status;
}

static bool snapc_full_global_is_done_yet(void) {
    opal_list_item_t* item = NULL;
    /* Be optimistic, we are talking about Fault Tolerance */
    bool done_yet = true;
    
    for(item  = opal_list_get_first(&global_snapshot.snapshots);
        item != opal_list_get_end(&global_snapshot.snapshots);
        item  = opal_list_get_next(item) ) {
        orte_snapc_base_snapshot_t *vpid_snapshot;
        vpid_snapshot = (orte_snapc_base_snapshot_t*)item;
        
        /* If they are working, then we are not done yet */
        if(ORTE_SNAPC_CKPT_STATE_FINISHED != vpid_snapshot->state) {
            done_yet = false;
            return done_yet;
        }
    }
    
    return done_yet;
}

static int snapc_full_global_gather_all_files(void) {
    int ret, exit_status = ORTE_SUCCESS;
    opal_list_item_t* item = NULL;
    char * local_dir = NULL;
    orte_filem_base_request_t *filem_request = OBJ_NEW(orte_filem_base_request_t);
    int tmp_argc = 0;


    /*
     * If it is stored in place, then we do not need to transfer anything
     */
    if( orte_snapc_base_store_in_place ) {
        for(item  = opal_list_get_first(&global_snapshot.snapshots);
            item != opal_list_get_end(&global_snapshot.snapshots);
            item  = opal_list_get_next(item) ) {
            orte_snapc_base_snapshot_t *vpid_snapshot;
            vpid_snapshot = (orte_snapc_base_snapshot_t*)item;

            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global) Updating Metadata - Files stored in place, no transfer required:\n");
            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global)   Remote Location: (%s)\n", vpid_snapshot->crs_snapshot_super.remote_location);
            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global)   Local Location:  (%s)\n", vpid_snapshot->crs_snapshot_super.local_location);

            /*
             * Update the metadata file
             */
            if(ORTE_SUCCESS != (ret = orte_snapc_base_add_vpid_metadata(&vpid_snapshot->process_name,
                                                                        global_snapshot.reference_name,
                                                                        vpid_snapshot->crs_snapshot_super.reference_name,
                                                                        vpid_snapshot->crs_snapshot_super.local_location))) {
                exit_status = ret;
                goto cleanup;
            }
        }
    }
    /*
     * If *not* stored in place then use FileM to transfer the files and cleanup
     */
    else {
        /*
         * Allocate the FileM request
         */
        filem_request->num_procs = 1;
        filem_request->proc_name = (orte_process_name_t*)malloc(sizeof(orte_process_name_t) * filem_request->num_procs);
        
        filem_request->num_targets = 1;
        
        filem_request->target_flags    = (int*)malloc(sizeof(int) * filem_request->num_targets);
        filem_request->target_flags[0] = ORTE_FILEM_TYPE_DIR;
        
        /*
         * Gather each file
         */
        for(item  = opal_list_get_first(&global_snapshot.snapshots);
            item != opal_list_get_end(&global_snapshot.snapshots);
            item  = opal_list_get_next(item) ) {
            orte_snapc_base_snapshot_t *vpid_snapshot;
            vpid_snapshot = (orte_snapc_base_snapshot_t*)item;

            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global) Getting remote directory:\n");
            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global)   Remote Location: (%s)\n", vpid_snapshot->crs_snapshot_super.remote_location);
            opal_output_verbose(20, mca_snapc_full_component.super.output_handle,
                                "global)   Local Location:  (%s)\n", vpid_snapshot->crs_snapshot_super.local_location);

            /*
             * Construct the process information
             */
            filem_request->proc_name[0].cellid = vpid_snapshot->process_name.cellid;
            filem_request->proc_name[0].jobid  = vpid_snapshot->process_name.jobid;
            filem_request->proc_name[0].vpid   = vpid_snapshot->process_name.vpid;

            /*
             * Construct the remote file name
             */
            tmp_argc = 0;
            opal_argv_append(&tmp_argc, &filem_request->remote_targets, vpid_snapshot->crs_snapshot_super.remote_location);

            /*
             * Construct the local file name
             */
            tmp_argc = 0;
            local_dir = strdup(vpid_snapshot->crs_snapshot_super.local_location);
            opal_argv_append(&tmp_argc, &filem_request->local_targets, opal_dirname(local_dir));

            /*
             * Do the transfer
             */
            if(ORTE_SUCCESS != (ret = orte_filem.get(filem_request) ) ) {
                exit_status = ret;
                /* Keep getting all the other files, eventually return an error */
                goto skip;
            }
            else {
                /*
                 * Update the metadata file
                 */
                if(ORTE_SUCCESS != (ret = orte_snapc_base_add_vpid_metadata(&vpid_snapshot->process_name,
                                                                            global_snapshot.reference_name,
                                                                            vpid_snapshot->crs_snapshot_super.reference_name,
                                                                            vpid_snapshot->crs_snapshot_super.local_location))) {
                    exit_status = ret;
                    goto cleanup;
                }
            }

            /*
             * Once we have brought it locally, then remove the remote copy
             */
            if(ORTE_SUCCESS != (ret = orte_filem.rm(filem_request)) ) {
                exit_status = ret;
                /* Keep getting all the other files, eventually return an error */
            }
        }
    skip:
        /* Do a bit of cleanup */
        opal_argv_free(filem_request->remote_targets);
        filem_request->remote_targets = NULL;
        opal_argv_free(filem_request->local_targets);
        filem_request->local_targets = NULL;
    }

    /*
     * Now that we gathered all the files, finish off the metadata file
     */
    orte_snapc_base_finalize_metadata(global_snapshot.reference_name);
    
 cleanup:
    if(NULL != local_dir)
        free(local_dir);

    return exit_status;
}
