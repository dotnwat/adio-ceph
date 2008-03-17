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
 *
 */

#include "orte_config.h"
#include "orte/constants.h"

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "opal/util/output.h"
#include "opal/util/argv.h"
#include "opal/util/show_help.h"
#include "opal/util/trace.h"

#include "opal/dss/dss.h"
#include "orte/mca/odls/odls_types.h"
#include "orte/mca/grpcomm/grpcomm.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"
#include "orte/runtime/orte_globals.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_wait.h"
#include "orte/orted/orted.h"

#include "orte/mca/plm/base/base.h"
#include "orte/mca/plm/base/plm_private.h"

static opal_event_t *ev;
static orte_vpid_t num_reported, num_being_sent;
static bool done_reporting;

static void failed_send(int fd, short event, void *arg)
{
    /* we get called if the sends in an abnormal term
     * don't get sent in time - set the done flag
     * so we can return the error
     */
    OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                         "%s plm:base:orted_cmd command messages timed out with num_sent %ld",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), (long)num_reported));
    done_reporting = true;
}

static void send_callback(int status,
                          orte_process_name_t* peer,
                          opal_buffer_t* req,
                          orte_rml_tag_t tag,
                          void* cbdata)
{
    num_reported++;
    if (num_reported == num_being_sent) {
        /* cancel the timer */
        opal_event_del(ev);
        /* mark as done */
        done_reporting = true;
        
        OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                             "%s plm:base:orted_cmd all messages sent",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
        
    }
}

int orte_plm_base_orted_exit(void)
{
    int rc;
    opal_buffer_t cmd;
    orte_daemon_cmd_flag_t command;
    orte_job_t *daemons;
    orte_proc_t **procs;
    
    OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                         "%s plm:base:orted_cmd sending orted_exit commands",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    OBJ_CONSTRUCT(&cmd, opal_buffer_t);
    
    /* since the orteds are being ordered to exit, and we are
     * technically a member of that "job", first update our
     * information to indicate we are "terminated". This will
     * ensure that we can exit correctly when all daemons report
     * in as "terminated"
     */
    
    /* get the job object for the daemons */
    if (NULL == (daemons = orte_get_job_data_object(ORTE_PROC_MY_NAME->jobid))) {
        ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
        return ORTE_ERR_NOT_FOUND;
    }
    procs = (orte_proc_t**)daemons->procs->addr;
    
    procs[0]->state = ORTE_PROC_STATE_TERMINATED;
    daemons->num_terminated++;
    
    /* just to be sure - it could be that we are the only daemon in
     * the job, and/or that all the other daemons reported termination
     * due to some other influence, so check to see if we are all done.
     * This will wake us up if everything is done
     */
    orte_plm_base_check_job_completed(daemons);

    command = ORTE_DAEMON_EXIT_CMD;
    
    /* pack the command */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &command, 1, ORTE_DAEMON_CMD))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* if we are abnormally ordering the termination, then
     * we do -not- want to use a collective operation to send the
     * command out as some of the daemons may not be alive and thus
     * any daemon beyond that in the collective wouldn't get the
     * command - use an alternative approach
     */
    if (orte_abnormal_term_ordered) {
        orte_vpid_t v;
        orte_process_name_t peer;
        
        OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                             "%s plm:base:orted_cmd:orted_exit abnormal term ordered",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
        
        /* since we cannot know which daemons may/may not be alive,
         * setup an event so we will time out after giving the send
         * our best attempt
         */
        ORTE_DETECT_TIMEOUT(&ev, orte_process_info.num_procs,
                            orte_timeout_usec_per_proc,
                            orte_max_timeout, failed_send);
        /* if I am the HNP, I need to get this message too, but just set things
         * up so the cmd processor gets called.
         * We don't want to message ourselves as this can create circular logic
         * in the RML. Instead, this macro will set a zero-time event which will
         * cause the buffer to be processed by the cmd processor - probably will
         * fire right away, but that's okay
         * The macro makes a copy of the buffer, so it's okay to release it here
         */
        if (orte_process_info.hnp) {
            ORTE_MESSAGE_EVENT(ORTE_PROC_MY_NAME, &cmd, ORTE_RML_TAG_DAEMON, orte_daemon_cmd_processor);
        }
        
        /* now send the command one daemon at a time using a non-blocking
         * send - let the callback function keep track of how many
         * complete - it will delete the event if they all do.
         * Start with vpid=1 as the HNP gets it another way
         */
        done_reporting = false;
        num_reported = 0;
        num_being_sent = orte_process_info.num_procs-1;
        peer.jobid = ORTE_PROC_MY_NAME->jobid;
        for(v=1; v < orte_process_info.num_procs; v++) {
            peer.vpid = v;
            /* don't worry about errors on the send here - just
             * issue it and keep going
             */
            orte_rml.send_buffer_nb(&peer, &cmd, ORTE_RML_TAG_DAEMON, 0,
                                    send_callback, 0);
        }
        /* wait for completion or timeout */
        while (!done_reporting) {
            opal_progress();
        }
        OBJ_DESTRUCT(&cmd);
        
        /* if all the sends didn't go, report that */
        if (num_reported != num_being_sent) {
            opal_show_help("help-plm-base.txt", "incomplete-exit-cmd", true);
            return ORTE_ERR_SILENT;
        }
        
        /* if all sends went out, return success */
        return ORTE_SUCCESS;
    }
    
    /* we are not abnormally terminating - send it express delivery! */
    if (ORTE_SUCCESS != (rc = orte_grpcomm.xcast(ORTE_PROC_MY_NAME->jobid, &cmd, ORTE_RML_TAG_DAEMON))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_DESTRUCT(&cmd);
    
    return rc;
}


int orte_plm_base_orted_kill_local_procs(orte_jobid_t job)
{
    int rc;
    opal_buffer_t cmd;
    orte_daemon_cmd_flag_t command=ORTE_DAEMON_KILL_LOCAL_PROCS;
    
    OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                         "%s plm:base:orted_cmd sending kill_local_procs cmds",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    OBJ_CONSTRUCT(&cmd, opal_buffer_t);

    /* pack the command */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &command, 1, ORTE_DAEMON_CMD))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* pack the jobid */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &job, 1, ORTE_JOBID))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* if we are abnormally ordering the termination, then
     * we do -not- want to use a collective operation to send the
     * command out as some of the daemons may not be alive and thus
     * any daemon beyond that in the collective wouldn't get the
     * command - use an alternative approach
     */
    if (orte_abnormal_term_ordered) {
        orte_vpid_t v;
        orte_process_name_t peer;
        
        OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                             "%s plm:base:orted_cmd:kill_local_procs abnormal term ordered",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
        
        /* since we cannot know which daemons may/may not be alive,
         * setup an event so we will time out after giving the send
         * our best attempt
         */
        ORTE_DETECT_TIMEOUT(&ev, orte_process_info.num_procs,
                            orte_timeout_usec_per_proc,
                            orte_max_timeout, failed_send);
       
        /* if I am the HNP, I need to get this message too, but just set things
         * up so the cmd processor gets called.
         * We don't want to message ourselves as this can create circular logic
         * in the RML. Instead, this macro will set a zero-time event which will
         * cause the buffer to be processed by the cmd processor - probably will
         * fire right away, but that's okay
         * The macro makes a copy of the buffer, so it's okay to release it here
         */
        if (orte_process_info.hnp) {
            ORTE_MESSAGE_EVENT(ORTE_PROC_MY_NAME, &cmd, ORTE_RML_TAG_DAEMON, orte_daemon_cmd_processor);
        }
        
        /* now send the command one daemon at a time using a non-blocking
         * send - let the callback function keep track of how many
         * complete - it will delete the event if they all do.
         * Start with vpid=1 as the HNP gets it another way
         */
        done_reporting = false;
        num_reported = 0;
        num_being_sent = orte_process_info.num_procs-1;
        peer.jobid = ORTE_PROC_MY_NAME->jobid;
        for(v=1; v < orte_process_info.num_procs; v++) {
            peer.vpid = v;
            /* don't worry about errors on the send here - just
             * issue it and keep going
             */
            orte_rml.send_buffer_nb(&peer, &cmd, ORTE_RML_TAG_DAEMON, 0,
                                send_callback, 0);
        }
        /* wait for completion or timeout */
        while (!done_reporting) {
            opal_progress();
        }
        OBJ_DESTRUCT(&cmd);
        
        /* if all the sends didn't go, report that */
        if (num_reported != num_being_sent) {
            opal_show_help("help-plm-base.txt", "incomplete-kill-procs-cmd", true);
            return ORTE_ERR_SILENT;
        }
        
        /* if all sends went out, return success */
        return ORTE_SUCCESS;
    }
    
    /* we are not abnormally terminating - send it express delivery! */
    if (ORTE_SUCCESS != (rc = orte_grpcomm.xcast(ORTE_PROC_MY_NAME->jobid, &cmd, ORTE_RML_TAG_DAEMON))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_DESTRUCT(&cmd);
    
    /* we're done! */
    return rc;
}



int orte_plm_base_orted_signal_local_procs(orte_jobid_t job, int32_t signal)
{
    int rc;
    opal_buffer_t cmd;
    orte_daemon_cmd_flag_t command=ORTE_DAEMON_SIGNAL_LOCAL_PROCS;
    
    OPAL_OUTPUT_VERBOSE((5, orte_plm_globals.output,
                         "%s plm:base:orted_cmd sending signal_local_procs cmds",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    OBJ_CONSTRUCT(&cmd, opal_buffer_t);
    
    /* pack the command */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &command, 1, ORTE_DAEMON_CMD))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* pack the jobid */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &job, 1, ORTE_JOBID))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* pack the signal */
    if (ORTE_SUCCESS != (rc = opal_dss.pack(&cmd, &signal, 1, OPAL_INT32))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&cmd);
        return rc;
    }
    
    /* send it! */
    if (ORTE_SUCCESS != (rc = orte_grpcomm.xcast(ORTE_PROC_MY_NAME->jobid, &cmd, ORTE_RML_TAG_DAEMON))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_DESTRUCT(&cmd);
    
    /* we're done! */
    return ORTE_SUCCESS;
}
