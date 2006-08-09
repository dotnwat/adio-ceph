/*
 * Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
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
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "orte_config.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <signal.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#include <time.h>
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_NETDB_H
#include <netdb.h>
#endif

#include "orte/orte_constants.h"
#include "opal/event/event.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "opal/mca/paffinity/base/base.h"
#include "opal/util/show_help.h"
#include "opal/util/path.h"
#include "opal/util/basename.h"
#include "opal/class/opal_value_array.h"
#include "orte/util/sys_info.h"
#include "orte/util/univ_info.h"
#include "opal/util/opal_environ.h"
#include "orte/util/session_dir.h"
#include "orte/runtime/orte_wait.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/errmgr/base/base.h"
#include "orte/mca/iof/iof.h"
#include "orte/mca/iof/base/iof_base_setup.h"
#include "opal/mca/base/mca_base_param.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/sds/base/base.h"
#include "orte/mca/pls/pls.h"
#include "orte/mca/pls/base/base.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/mca/rmaps/base/rmaps_base_map.h"
#include "orte/mca/soh/soh.h"
#include "orte/mca/soh/base/base.h"
#include "orte/mca/pls/fork/pls_fork.h"

extern char **environ;

#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS
static int orte_pls_fork_launch_threaded(orte_jobid_t);
#endif


orte_pls_base_module_1_0_0_t orte_pls_fork_module = {
#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS
    orte_pls_fork_launch_threaded,
#else
    orte_pls_fork_launch,
#endif
    orte_pls_fork_terminate_job,
    orte_pls_fork_terminate_proc,
    orte_pls_fork_signal_job,
    orte_pls_fork_signal_proc,
    orte_pls_fork_finalize
};

static void set_handler_default(int sig);


static bool orte_pls_fork_child_died(pid_t pid, unsigned int timeout, int *exit_status)
{
    time_t end;
    pid_t ret;

    end = time(NULL) + timeout;
    do {
        ret = waitpid(pid, exit_status, WNOHANG);
        if (pid == ret) {
            /* It died -- return success */
            return true;
        } else if (-1 == ret && ECHILD == errno) {
            /* The pid no longer exists, so we'll call this "good
               enough for government work" */
            return true;
        }

        /* Sleep for a second */
        sleep(1);
    } while (time(NULL) < end);

    /* The child didn't die, so return false */
    return false;
}

static void orte_pls_fork_kill_processes(opal_value_array_t *pids, opal_value_array_t *procs)
{
    size_t i;
    pid_t pid;
    orte_process_name_t proc;
    int rc, exit_status;

    for (i = 0; i < opal_value_array_get_size(pids); ++i) {
        pid = OPAL_VALUE_ARRAY_GET_ITEM(pids, pid_t, i);
        
        /* de-register the SIGCHILD callback for this pid */
        orte_wait_cb_cancel(pid);

        /* Send a sigterm to the process.  If we get ESRCH back, that
           means the process is already dead, so just proceed on to
           the reaping of it.  If we get any other error back, just
           skip it and go on to the next process. */
        if (0 != kill(pid, SIGTERM) && ESRCH != errno) {
            int err = errno;
            char hostname[MAXHOSTNAMELEN];
            gethostname(hostname, sizeof(hostname));

            opal_show_help("help-orte-pls-fork.txt",
                           "orte-pls-fork:could-not-send-kill",
                           true, hostname, pid, err);
            
            continue;
        }

        /* The kill succeeded.  Wait up to timeout_before_sigkill
           seconds to see if it died. */

        if (!orte_pls_fork_child_died(pid, mca_pls_fork_component.timeout_before_sigkill, &exit_status)) {
            kill(pid, SIGKILL);
            /* Double check that it actually died */
            if (!orte_pls_fork_child_died(pid, mca_pls_fork_component.timeout_before_sigkill, &exit_status)) {
                char hostname[MAXHOSTNAMELEN];
                gethostname(hostname, sizeof(hostname));

                opal_show_help("help-orte-pls-fork.txt",
                               "orte-pls-fork:could-not-kill",
                               true, hostname, pid);
            }
        }
        
        /* update the process state on the registry */
        proc = OPAL_VALUE_ARRAY_GET_ITEM(procs, orte_process_name_t, i);
        if (ORTE_SUCCESS != (rc = orte_soh.set_proc_soh(&proc, ORTE_PROC_STATE_TERMINATED, exit_status))) {
            ORTE_ERROR_LOG(rc);
            /* don't exit out even if this didn't work - we still might need to kill more
             * processes, so just keep trucking
             */
        }
    }

    /* Release any waiting threads from this process */
    OPAL_THREAD_LOCK(&mca_pls_fork_component.lock);
    mca_pls_fork_component.num_children = 0;
    opal_condition_signal(&mca_pls_fork_component.cond);
    OPAL_THREAD_UNLOCK(&mca_pls_fork_component.lock);
}

/*
 *  Wait for a callback indicating the child has completed.
 */

static void orte_pls_fork_wait_proc(pid_t pid, int status, void* cbdata)
{
    orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)cbdata;
    int rc;

    /* Clean up the session directory as if we were the process
       itself.  This covers the case where the process died abnormally
       and didn't cleanup its own session directory. */
    orte_session_dir_finalize(&proc->proc_name);
    orte_iof.iof_flush();

    /* set the state of this process */
    if(WIFEXITED(status)) {
        rc = orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_TERMINATED, status);
    } else {
        rc = orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_ABORTED, status);
    }
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_RELEASE(proc);

    /* release any waiting threads */
    OPAL_THREAD_LOCK(&mca_pls_fork_component.lock);
    mca_pls_fork_component.num_children--;
    opal_condition_signal(&mca_pls_fork_component.cond);
    OPAL_THREAD_UNLOCK(&mca_pls_fork_component.lock);
}

/**
 *  Fork/exec the specified processes
 */

static int orte_pls_fork_proc(
    orte_app_context_t* context,
    orte_rmaps_base_proc_t* proc,
    orte_vpid_t vpid_start,
    orte_vpid_t vpid_range,
    bool want_processor,
    size_t processor)
{
    pid_t pid;
    orte_iof_base_io_conf_t opts;
    int rc;
    sigset_t sigs;
    orte_vpid_t vpid;
    int i = 0, p[2];

    /* should pull this information from MPIRUN instead of going with
       default */
    opts.usepty = OMPI_ENABLE_PTY_SUPPORT;

    /* BWB - Fix post beta.  Should setup stdin in orterun and
       make part of the app_context */
    if (ORTE_SUCCESS == orte_ns.get_vpid(&vpid, &proc->proc_name) &&
        vpid == 0) {
        opts.connect_stdin = true;
    } else {
        opts.connect_stdin = false;
    }

    rc = orte_iof_base_setup_prefork(&opts);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

#if 0
 {
    /* Do fork the windows way: see opal_few() for example */
    HANDLE new_process;
    STARTUPINFO si;
    PROCESS_INFORMATION pi;
    DWORD process_id;

    ZeroMemory (&si, sizeof(si));
    ZeroMemory (&pi, sizeof(pi));

    GetStartupInfo (&si);
    if (!CreateProcess (NULL,
                        "new process",
                        NULL,
                        NULL,
                        TRUE,
                        0,
                        NULL,
                        NULL,
                        &si,
                        &pi)){
       /* actual error can be got by simply calling GetLastError() */
       return ORTE_ERROR;
    }
    /* get child pid */
    process_id = GetProcessId(&pi);
    pid = (int) process_id;
 }
#endif
    /* A pipe is used to communicate between the parent and child to
       indicate whether the exec ultiimately succeeded or failed.  The
       child sets the pipe to be close-on-exec; the child only ever
       writes anything to the pipe if there is an error (e.g.,
       executable not found, exec() fails, etc.).  The parent does a
       blocking read on the pipe; if the pipe closed with no data,
       then the exec() succeeded.  If the parent reads something from
       the pipe, then the child was letting us know that it failed. */
    if (pipe(p) < 0) {
        ORTE_ERROR_LOG(ORTE_ERR_IN_ERRNO);
        return ORTE_ERR_IN_ERRNO;
    }

    /* Fork off the child */
    pid = fork();
    if(pid < 0) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    if (pid == 0) {
        char *param, *param2;
        char *uri;
        char **environ_copy;
        long fd, fdmax = sysconf(_SC_OPEN_MAX);

        /* Setup the pipe to be close-on-exec */
        close(p[0]);
        fcntl(p[1], F_SETFD, FD_CLOEXEC);

        /* setup stdout/stderr so that any error messages that we may
           print out will get displayed back at orterun */
        orte_iof_base_setup_child(&opts);

        /* Try to change to the context cwd and check that the app
           exists and is executable */
        if (ORTE_SUCCESS != orte_pls_base_check_context_cwd(context, true) ||
            ORTE_SUCCESS != orte_pls_base_check_context_app(context)) {
            /* Tell the parent that Badness happened */
            write(p[1], &i, sizeof(int));
            exit(-1);
        }

        /* setup base environment: copy the current environ and merge
           in the app context environ */
        if (NULL != context->env) {
            environ_copy = opal_environ_merge(environ, context->env);
        } else {
            environ_copy = opal_argv_copy(environ);
        }

        /* special case handling for --prefix: this is somewhat icky,
           but at least some users do this.  :-\ It is possible that
           when using --prefix, the user will also "-x PATH" and/or
           "-x LD_LIBRARY_PATH", which would therefore clobber the
           work that was done in the prior pls to ensure that we have
           the prefix at the beginning of the PATH and
           LD_LIBRARY_PATH.  So examine the context->env and see if we
           find PATH or LD_LIBRARY_PATH.  If found, that means the
           prior work was clobbered, and we need to re-prefix those
           variables. */
        for (i = 0; NULL != context->env && NULL != context->env[i]; ++i) {
            char *newenv;

            /* Reset PATH */
            if (0 == strncmp("PATH=", context->env[i], 5)) {
                asprintf(&newenv, "%s/bin:%s",
                         context->prefix_dir, context->env[i] + 5);
                opal_setenv("PATH", newenv, true, &environ_copy);
                free(newenv);
            }

            /* Reset LD_LIBRARY_PATH */
            else if (0 == strncmp("LD_LIBRARY_PATH=", context->env[i], 16)) {
                asprintf(&newenv, "%s/lib:%s",
                         context->prefix_dir, context->env[i] + 16);
                opal_setenv("LD_LIBRARY_PATH", newenv, true, &environ_copy);
                free(newenv);
            }
        }

        param = mca_base_param_environ_variable("rmgr","bootproxy","jobid");
        opal_unsetenv(param, &environ_copy);
        free(param);

        /* Set the relative vpid */

        if (want_processor) {
            param = mca_base_param_environ_variable("mpi", NULL,
                                                    "paffinity_processor");
            asprintf(&param2, "%lu", (unsigned long) processor);
            opal_setenv(param, param2, true, &environ_copy);
            free(param);
            free(param2);
        }

        /* setup universe info */
        if (NULL != orte_universe_info.name) {
            param = mca_base_param_environ_variable("universe", NULL, NULL);
            asprintf(&uri, "%s@%s:%s", orte_universe_info.uid,
                                       orte_universe_info.host,
                                       orte_universe_info.name);
            opal_setenv(param, uri, true, &environ_copy);
            free(param);
            free(uri);
        }

        /* setup ns contact info */
        if(NULL != orte_process_info.ns_replica_uri) {
            uri = strdup(orte_process_info.ns_replica_uri);
        } else {
            uri = orte_rml.get_uri();
        }
        param = mca_base_param_environ_variable("ns","replica","uri");
        opal_setenv(param, uri, true, &environ_copy);
        free(param);
        free(uri);

        /* setup gpr contact info */
        if(NULL != orte_process_info.gpr_replica_uri) {
            uri = strdup(orte_process_info.gpr_replica_uri);
        } else {
            uri = orte_rml.get_uri();
        }
        param = mca_base_param_environ_variable("gpr","replica","uri");
        opal_setenv(param, uri, true, &environ_copy);
        free(param);
        free(uri);

        /* use same nodename as the starting daemon (us) */
        param = mca_base_param_environ_variable("orte", "base", "nodename");
        opal_setenv(param, orte_system_info.nodename, true, &environ_copy);
        free(param);

        /* push name into environment */
        orte_ns_nds_env_put(&proc->proc_name, vpid_start, vpid_range,
                            &environ_copy);

        /* close all file descriptors w/ exception of stdin/stdout/stderr */
        for(fd=3; fd<fdmax; fd++)
            close(fd);

        if (context->argv == NULL) {
            context->argv = malloc(sizeof(char*)*2);
            context->argv[0] = strdup(context->app);
            context->argv[1] = NULL;
        }

        /* Set signal handlers back to the default.  Do this close to
           the exev() because the event library may (and likely will)
           reset them.  If we don't do this, the event library may
           have left some set that, at least on some OS's, don't get
           reset via fork() or exec().  Hence, the launched process
           could be unkillable (for example). */

        set_handler_default(SIGTERM);
        set_handler_default(SIGINT);
        set_handler_default(SIGHUP);
        set_handler_default(SIGPIPE);
        set_handler_default(SIGCHLD);

        /* Unblock all signals, for many of the same reasons that we
           set the default handlers, above.  This is noticable on
           Linux where the event library blocks SIGTERM, but we don't
           want that blocked by the launched process. */
        sigprocmask(0, 0, &sigs);
        sigprocmask(SIG_UNBLOCK, &sigs, 0);

        /* Exec the new executable */

        execve(context->app, context->argv, environ_copy);
        opal_show_help("help-orte-pls-fork.txt", "orte-pls-fork:execv-error",
                       true, context->app, strerror(errno));
        exit(-1);
    } else {

        /* connect endpoints IOF */
        rc = orte_iof_base_setup_parent(&proc->proc_name, &opts);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        /* Wait to read something from the pipe or close */
        close(p[1]);
        while (1) {
            rc = read(p[0], &i, sizeof(int));
            if (rc < 0) {
                /* Signal interrupts are ok */
                if (errno == EINTR) {
                    continue;
                }
                /* Other errno's are bad */
                return ORTE_ERR_IN_ERRNO;
                break;
            } else if (0 == rc) {
                /* Child was successful in exec'ing! */
                break;
            } else {
                /* Doh -- child failed.  The child already printed a
                   suitable error message, so disable all
                   ORTE_ERROR_LOG reporting after this. Must also
                   report the failure to launch this process through
                   the SOH or else everyone else will hang. Don't bother
                   checking whether or not this worked - just fire and forget
                */
                orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_ABORTED, rc);
                return ORTE_ERR_FATAL;
                break;
            }
        }

        /* save the pid in the registry */
        if (ORTE_SUCCESS !=
            (rc = orte_pls_base_set_proc_pid(&proc->proc_name, pid))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        /* wait for the child process - dont register for wait
         * callback until after I/O is setup and the pid registered -
         * otherwise can receive the wait callback before the above is
         * ever completed
         */
        OPAL_THREAD_LOCK(&mca_pls_fork_component.lock);
        mca_pls_fork_component.num_children++;
        OPAL_THREAD_UNLOCK(&mca_pls_fork_component.lock);
        OBJ_RETAIN(proc);
        orte_wait_cb(pid, orte_pls_fork_wait_proc, proc);
    }
    return ORTE_SUCCESS;
}


/**
 * Launch all processes allocated to the current node.
 */

int orte_pls_fork_launch(orte_jobid_t jobid)
{
    opal_list_t map;
    opal_list_item_t* item;
    orte_vpid_t vpid_start;
    orte_vpid_t vpid_range;
    int rc;
    size_t num_processors, num_processes;

    /* query the allocation for this node */
    OBJ_CONSTRUCT(&map, opal_list_t);
    rc = orte_rmaps_base_get_node_map(
        orte_process_info.my_name->cellid,jobid,orte_system_info.nodename,&map);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    rc = orte_rmaps_base_get_vpid_range(jobid, &vpid_start, &vpid_range);
    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* are we oversubscribing? */
    opal_paffinity_base_get_num_processors(&rc);
    num_processors = (size_t) rc;
    for (num_processes = 0, item =  opal_list_get_first(&map);
         item != opal_list_get_end(&map);
         item =  opal_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        num_processes += map->num_procs;
    }

    /* attempt to launch each of the apps */
    for (item =  opal_list_get_first(&map);
         item != opal_list_get_end(&map);
         item =  opal_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        size_t i;
        for (i=0; i<map->num_procs; i++) {
            rc = orte_pls_fork_proc(map->app, map->procs[i], vpid_start,
                                    vpid_range,
                                    (num_processes > num_processors) ?
                                    false : true, i);

            if (ORTE_SUCCESS != rc) {
                /* Set the state of this process, and all remaining
                   processes to be launched to ABORTED.  This will
                   cause the entire job to abort. */
                for (; i < map->num_procs; ++i) {
                    orte_soh.set_proc_soh(&map->procs[i]->proc_name,
                                          ORTE_PROC_STATE_ABORTED, 0);
                }

                /* Propagate the error up the stack */
                ORTE_ERROR_LOG(rc);
                goto cleanup;
            }
        }
    }

cleanup:
    while(NULL != (item = opal_list_remove_first(&map))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&map);
    return rc;
}

/**
 *  Query for all processes allocated to the job and terminate
 *  those on the current node.
 */

int orte_pls_fork_terminate_job(orte_jobid_t jobid)
{
    /* query for the pids allocated on this node */
    char *segment;
    char *keys[] = {
        ORTE_PROC_NAME_KEY,
        ORTE_NODE_NAME_KEY,
        ORTE_PROC_PID_KEY,
        NULL
    };
    orte_gpr_value_t** values = NULL;
    size_t i, k, num_values = 0;
    int rc;
    opal_value_array_t pids, procs;
    orte_process_name_t proc, *procptr;

    /* setup the pid array */
    OBJ_CONSTRUCT(&pids, opal_value_array_t);
    opal_value_array_init(&pids, sizeof(pid_t));
    
    /* setup the process name array */
    OBJ_CONSTRUCT(&procs, opal_value_array_t);
    opal_value_array_init(&procs, sizeof(orte_process_name_t));
        
    /* query the job segment on the registry */
    if(ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&segment, jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    rc = orte_gpr.get(
        ORTE_GPR_KEYS_AND|ORTE_GPR_TOKENS_OR,
        segment,
        NULL,
        keys,
        &num_values,
        &values
        );
    if(rc != ORTE_SUCCESS) {
        free(segment);
        return rc;
    }

    free(segment);  /* done with this */
    
    for(i=0; i<num_values; i++) {
        orte_gpr_value_t* value = values[i];
        pid_t pid = 0, *pidptr;
        for(k=0; k<value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            if (strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                if(orte_dss.compare(keyval->value->data, orte_system_info.nodename, ORTE_STRING) != ORTE_EQUAL) {
                    break;
                }
            } else if (strcmp(keyval->key, ORTE_PROC_PID_KEY) == 0) {
                if (ORTE_SUCCESS != (rc = orte_dss.get((void**)&pidptr, keyval->value, ORTE_PID))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
                pid = *pidptr;
            } else if (strcmp(keyval->key, ORTE_PROC_NAME_KEY) == 0) {
                if (ORTE_SUCCESS != (rc = orte_dss.get((void**)&procptr, keyval->value, ORTE_NAME))) {
                    ORTE_ERROR_LOG(rc);
                    return rc;
                }
                proc = *procptr;
            }
        }
        if (0 != pid) {
            opal_value_array_append_item(&pids, &pid);
            opal_value_array_append_item(&procs, &proc);
        }
        OBJ_RELEASE(value);
    }

    /* If we have processes to kill, go kill them */
    if (opal_value_array_get_size(&pids) > 0) {
        orte_pls_fork_kill_processes(&pids, &procs);
    }
    OBJ_DESTRUCT(&pids);
    OBJ_DESTRUCT(&procs);

    if(NULL != values) {
        free(values);
    }
    free(segment);
    return ORTE_SUCCESS;
}


int orte_pls_fork_terminate_proc(const orte_process_name_t* proc)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

/**
 *  Query for all processes allocated to the job and signal
 *  those on the current node.
 */

int orte_pls_fork_signal_job(orte_jobid_t jobid, int32_t signal)
{
    /* query for the pids allocated on this node */
    char *segment;
    char *keys[3];
    orte_gpr_value_t** values = NULL;
    size_t i, k, num_values = 0;
    int rc;
    opal_value_array_t pids;
    pid_t pid;

    /* setup the pid array */
    OBJ_CONSTRUCT(&pids, opal_value_array_t);
    opal_value_array_init(&pids, sizeof(pid_t));

    /* query the job segment on the registry */
    if(ORTE_SUCCESS != (rc = orte_schema.get_job_segment_name(&segment, jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    keys[0] = ORTE_NODE_NAME_KEY;
    keys[1] = ORTE_PROC_PID_KEY;
    keys[2] = NULL;

    rc = orte_gpr.get(
            ORTE_GPR_KEYS_AND|ORTE_GPR_TOKENS_OR,
            segment,
            NULL,
            keys,
            &num_values,
            &values
            );
    if(rc != ORTE_SUCCESS) {
        free(segment);
        return rc;
    }

    for(i=0; i<num_values; i++) {
        orte_gpr_value_t* value = values[i];
        pid_t pid = 0, *pidptr;
        for(k=0; k<value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            if(strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                if(orte_dss.compare(keyval->value->data, orte_system_info.nodename, ORTE_STRING) != ORTE_EQUAL) {
                    break;
                }
            } else if (strcmp(keyval->key, ORTE_PROC_PID_KEY) == 0) {
                if (ORTE_SUCCESS != (rc = orte_dss.get((void**)&pidptr, keyval->value, ORTE_PID))) {
                    ORTE_ERROR_LOG(rc);
                    free(segment);
                    return rc;
                }
                pid = *pidptr;
            }
        }
        if (0 != pid) {
            opal_value_array_append_item(&pids, &pid);
        }
        OBJ_RELEASE(value);
    }

    rc = ORTE_SUCCESS;
    /* If we have processes to signal, go signal them */
    for (i = 0; i < opal_value_array_get_size(&pids); ++i) {
        pid = OPAL_VALUE_ARRAY_GET_ITEM(&pids, pid_t, i);
        if(kill(pid, (int)signal) != 0) {
            switch(errno) {
                case EINVAL:
                    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                    rc = ORTE_ERR_BAD_PARAM;
                    break;
                case ESRCH:
                    ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
                    rc = ORTE_ERR_NOT_FOUND;
                    break;
                case EPERM:
                    ORTE_ERROR_LOG(ORTE_ERR_PERM);
                    rc = ORTE_ERR_PERM;
                    break;
                default:
                    ORTE_ERROR_LOG(ORTE_ERROR);
                    rc = ORTE_ERROR;
            }
        }
    }

    OBJ_DESTRUCT(&pids);

    if(NULL != values) {
        free(values);
    }
    free(segment);

    return rc;
}


int orte_pls_fork_signal_proc(const orte_process_name_t* proc, int32_t signal)
{
    return ORTE_ERR_NOT_IMPLEMENTED;
}

int orte_pls_fork_finalize(void)
{
    if(mca_pls_fork_component.reap) {
        OPAL_THREAD_LOCK(&mca_pls_fork_component.lock);
        while(mca_pls_fork_component.num_children > 0) {
            opal_condition_wait(&mca_pls_fork_component.cond,
                &mca_pls_fork_component.lock);
        }
        OPAL_THREAD_UNLOCK(&mca_pls_fork_component.lock);
    }
    return ORTE_SUCCESS;
}


/**
 * Handle threading issues.
 */

#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS

struct orte_pls_fork_stack_t {
    opal_condition_t cond;
    opal_mutex_t mutex;
    bool complete;
    orte_jobid_t jobid;
    int rc;
};
typedef struct orte_pls_fork_stack_t orte_pls_fork_stack_t;

static void orte_pls_fork_stack_construct(orte_pls_fork_stack_t* stack)
{
    OBJ_CONSTRUCT(&stack->mutex, opal_mutex_t);
    OBJ_CONSTRUCT(&stack->cond, opal_condition_t);
    stack->rc = 0;
    stack->complete = false;
}

static void orte_pls_fork_stack_destruct(orte_pls_fork_stack_t* stack)
{
    OBJ_DESTRUCT(&stack->mutex);
    OBJ_DESTRUCT(&stack->cond);
}

static OBJ_CLASS_INSTANCE(
    orte_pls_fork_stack_t,
    opal_object_t,
    orte_pls_fork_stack_construct,
    orte_pls_fork_stack_destruct);


static void orte_pls_fork_launch_cb(int fd, short event, void* args)
{
    orte_pls_fork_stack_t *stack = (orte_pls_fork_stack_t*)args;
    OPAL_THREAD_LOCK(&stack->mutex);
    stack->rc = orte_pls_fork_launch(stack->jobid);
    stack->complete = true;
    opal_condition_signal(&stack->cond);
    OPAL_THREAD_UNLOCK(&stack->mutex);
}

static int orte_pls_fork_launch_threaded(orte_jobid_t jobid)
{

    struct timeval tv = { 0, 0 };
    struct opal_event event;
    struct orte_pls_fork_stack_t stack;

    OBJ_CONSTRUCT(&stack, orte_pls_fork_stack_t);

    stack.jobid = jobid;
    opal_evtimer_set(&event, orte_pls_fork_launch_cb, &stack);
    opal_evtimer_add(&event, &tv);

    OPAL_THREAD_LOCK(&stack.mutex);
    while(false == stack.complete) {
         opal_condition_wait(&stack.cond, &stack.mutex);
    }
    OPAL_THREAD_UNLOCK(&stack.mutex);
    OBJ_DESTRUCT(&stack);
    return stack.rc;
}

#endif



static void set_handler_default(int sig)
{
    struct sigaction act;

    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(sig, &act, (struct sigaction *)0);
}
