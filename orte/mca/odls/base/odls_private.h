/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
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

#ifndef MCA_ODLS_PRIVATE_H
#define MCA_ODLS_PRIVATE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/types.h"

#include "opal/class/opal_list.h"
#include "opal/class/opal_pointer_array.h"
#include "opal/threads/mutex.h"
#include "opal/threads/condition.h"
#include "opal/dss/dss_types.h"

#include "orte/mca/grpcomm/grpcomm_types.h"
#include "orte/mca/plm/plm_types.h"
#include "orte/mca/rmaps/rmaps_types.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/runtime/orte_globals.h"

#include "orte/mca/odls/odls_types.h"

BEGIN_C_DECLS

/*
 * General ODLS types
 */

/*
 * List object to locally store the process names and pids of
 * our children. This can subsequently be used to order termination
 * or pass signals without looking the info up again.
 */
typedef struct {
    opal_list_item_t super;      /* required to place this on a list */
    orte_process_name_t *name;   /* the OpenRTE name of the proc */
    orte_vpid_t local_rank;      /* local rank of the proc on this node */
    pid_t pid;                   /* local pid of the proc */
    orte_std_cntr_t app_idx;     /* index of the app_context for this proc */
    bool alive;                  /* is this proc alive? */
    bool coll_recvd;             /* collective operation recvd */
    orte_proc_state_t state;     /* the state of the process */
    orte_exit_code_t exit_code;  /* process exit code */
    char *rml_uri;               /* contact info for this child */
    char *slot_list;             /* list of slots for this child */
} orte_odls_child_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(orte_odls_child_t);

/*
 * List object to locally store job related info
 */
typedef struct orte_odls_job_t {
    opal_list_item_t    super;                  /* required to place this on a list */
    orte_jobid_t        jobid;                  /* jobid for this data */
    orte_app_context_t  **apps;                 /* app_contexts for this job */
    orte_std_cntr_t     num_apps;               /* number of app_contexts */
    orte_std_cntr_t     total_slots_alloc;
    orte_vpid_t         num_procs;
    int32_t             num_local_procs;
    orte_pmap_t         *procmap;               /* map of procs/node, local ranks */
    opal_byte_object_t  *pmap;                  /* byte object version of procmap */
    opal_buffer_t       collection_bucket;
    opal_buffer_t       local_collection;
    orte_grpcomm_coll_t collective_type;
    int32_t             num_contributors;
    int                 num_participating;
    int                 num_collected;
} orte_odls_job_t;
ORTE_DECLSPEC OBJ_CLASS_DECLARATION(orte_odls_job_t);



typedef struct {
    /** Verbose/debug output stream */
    int output;
    /** Time to allow process to forcibly die */
    int timeout_before_sigkill;
    /* mutex */
    opal_mutex_t mutex;
    /* condition variable */
    opal_condition_t cond;
    /* list of children for this orted */
    opal_list_t children;
    /* list of job data for this orted */
    opal_list_t jobs;
    /* byte object to store daemon map for later xmit to procs */
    opal_byte_object_t *dmap;
} orte_odls_globals_t;

ORTE_DECLSPEC extern orte_odls_globals_t orte_odls_globals;


/*
 * Default functions that are common to most environments - can
 * be overridden by specific environments if they need something
 * different (e.g., bproc)
 */
ORTE_DECLSPEC int
orte_odls_base_default_get_add_procs_data(opal_buffer_t *data,
                                          orte_jobid_t job);

ORTE_DECLSPEC int
orte_odls_base_default_construct_child_list(opal_buffer_t *data,
                                            orte_jobid_t *job);

/* define a function that will fork a local proc */
typedef int (*orte_odls_base_fork_local_proc_fn_t)(orte_app_context_t *context,
                                                   orte_odls_child_t *child,
                                                   char **environ_copy);

ORTE_DECLSPEC int
orte_odls_base_default_launch_local(orte_jobid_t job,
                                    orte_odls_base_fork_local_proc_fn_t fork_local);

ORTE_DECLSPEC int
orte_odls_base_default_deliver_message(orte_jobid_t job, opal_buffer_t *buffer, orte_rml_tag_t tag);

ORTE_DECLSPEC void odls_base_default_wait_local_proc(pid_t pid, int status, void* cbdata);

/* define a function type to signal a local proc */
typedef int (*orte_odls_base_signal_local_fn_t)(pid_t pid, int signum);

ORTE_DECLSPEC int
orte_odls_base_default_signal_local_procs(const orte_process_name_t *proc, int32_t signal,
                                          orte_odls_base_signal_local_fn_t signal_local);

/* define a function type for killing a local proc */
typedef int (*orte_odls_base_kill_local_fn_t)(pid_t pid, int signum);

/* define a function type to detect that a child died */
typedef bool (*orte_odls_base_child_died_fn_t)(pid_t pid, unsigned int timeout, int *exit_status);

ORTE_DECLSPEC int
orte_odls_base_default_kill_local_procs(orte_jobid_t job, bool set_state,
                                        orte_odls_base_kill_local_fn_t kill_local,
                                        orte_odls_base_child_died_fn_t child_died);

ORTE_DECLSPEC int orte_odls_base_default_require_sync(orte_process_name_t *proc,
                                                      opal_buffer_t *buffer,
                                                      bool drop_nidmap);

/*
 * Preload binary/files functions
 */
ORTE_DECLSPEC int orte_odls_base_preload_files_app_context(orte_app_context_t* context);

/*
 * Collect data to support collective operations across the procs
 */
ORTE_DECLSPEC int orte_odls_base_default_collect_data(orte_process_name_t *proc, opal_buffer_t *buf);

/*
 * Retrive the daemon map
 */
ORTE_DECLSPEC opal_pointer_array_t* orte_odls_base_get_daemon_map(void);

END_C_DECLS

#endif
