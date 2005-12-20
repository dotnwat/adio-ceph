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
#include <string.h>
#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#include <fcntl.h>
#include <signal.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#include "opal/mca/base/mca_base_param.h"
#include "opal/util/if.h"
#include "opal/util/if.h"
#include "opal/util/path.h"
#include "opal/event/event.h"
#include "opal/util/show_help.h"
#include "opal/util/argv.h"
#include "opal/util/opal_environ.h"
#include "opal/util/output.h"
#include "orte/include/orte_constants.h"
#include "orte/util/univ_info.h"
#include "orte/util/session_dir.h"
#include "orte/runtime/orte_wait.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/pls/pls.h"
#include "orte/mca/pls/base/base.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/ras/base/ras_base_node.h"
#include "orte/mca/rmaps/base/rmaps_base_map.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/soh/soh.h"
#include "orte/mca/soh/base/base.h"
#include "orte/mca/pls/rsh/pls_rsh.h"
#include "orte/util/sys_info.h"

extern char **environ;


#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS
static int orte_pls_rsh_launch_threaded(orte_jobid_t jobid);
#endif


orte_pls_base_module_1_0_0_t orte_pls_rsh_module = {
#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS
    orte_pls_rsh_launch_threaded,
#else
    orte_pls_rsh_launch,
#endif
    orte_pls_rsh_terminate_job,
    orte_pls_rsh_terminate_proc,
    orte_pls_rsh_finalize
};

/* struct used to have enough information to clean up the state of the
   universe if a daemon aborts */
struct rsh_daemon_info_t {
    opal_object_t super;
    orte_ras_node_t* node;
    orte_jobid_t jobid;
};
typedef struct rsh_daemon_info_t rsh_daemon_info_t;
static OBJ_CLASS_INSTANCE(rsh_daemon_info_t,
                          opal_object_t,
                          NULL, NULL);
static void set_handler_default(int sig);

enum {
    ORTE_PLS_RSH_SHELL_BASH = 0,
    ORTE_PLS_RSH_SHELL_TCSH,
    ORTE_PLS_RSH_SHELL_CSH,
    ORTE_PLS_RSH_SHELL_KSH,
    ORTE_PLS_RSH_SHELL_UNKNOWN
};

typedef int orte_pls_rsh_shell;

static const char * orte_pls_rsh_shell_name[] = {
    "bash",
    "tcsh",       /* tcsh has to be first otherwise strstr finds csh */
    "csh",
    "ksh",
    "unknown"
};

/**
 * Check the Shell variable on the specified node
 */

static int orte_pls_rsh_probe(orte_ras_node_t * node, orte_pls_rsh_shell * shell)
{
    char ** argv;
    int argc, rc, nfds, i;
    int fd[2];
    pid_t pid;
    fd_set readset;
    fd_set errset;
    char outbuf[4096];

    if (mca_pls_rsh_component.debug) {
        opal_output(0, "pls:rsh: going to check SHELL variable on node %s\n",
                    node->node_name);
    }
    *shell = ORTE_PLS_RSH_SHELL_UNKNOWN;
    /*
     * Build argv array
     */
    argv = opal_argv_copy(mca_pls_rsh_component.argv);
    argc = mca_pls_rsh_component.argc;
    opal_argv_append(&argc, &argv, node->node_name);
    opal_argv_append(&argc, &argv, "echo $SHELL");
    if (pipe(fd)) {
        opal_output(0, "pls:rsh: pipe failed with errno=%d\n", errno);
        return ORTE_ERR_IN_ERRNO;
    }
    if ((pid = fork()) < 0) {
        opal_output(0, "pls:rsh: fork failed with errno=%d\n", errno);
        return ORTE_ERR_IN_ERRNO;
    }
    else if (pid == 0) {          /* child */
        if (dup2(fd[1], 1) < 0) {
            opal_output(0, "pls:rsh: dup2 failed with errno=%d\n", errno);
            return ORTE_ERR_IN_ERRNO;
        }
        execvp(argv[0], argv);
        exit(errno);
    }
    if (close(fd[1])) {
        opal_output(0, "pls:rsh: close failed with errno=%d\n", errno);
        return ORTE_ERR_IN_ERRNO;
    }
    /* Monitor stdout */
    FD_ZERO(&readset);
    nfds = fd[0]+1;

    memset (outbuf, 0, sizeof (outbuf));
    rc = ORTE_SUCCESS;;
    while (ORTE_SUCCESS == rc) {
        int err;
        FD_SET (fd[0], &readset);
        errset = readset;
        err = select(nfds, &readset, NULL, &errset, NULL);
        if (err == -1) {
            if (errno == EINTR)
                continue;
            else {
                rc = ORTE_ERR_IN_ERRNO;
                break;
            }
        }
        if (FD_ISSET(fd[0], &errset) != 0)
            rc = ORTE_ERR_FATAL;
        /* In case we have something valid to read on stdin */
        if (FD_ISSET(fd[0], &readset) != 0) {
            ssize_t ret = 1;
            char temp[4096];
            char * ptr = outbuf;
            ssize_t outbufsize = sizeof(outbuf);

            memset (temp, 0, sizeof(temp));

            while (ret != 0) {
                ret = read (fd[0], temp, 256);
                if (ret < 0) {
                    if (errno == EINTR)
                        continue;
                    else {
                        rc = ORTE_ERR_IN_ERRNO;
                        break;
                    }
                }
                else {
                    if (outbufsize > 0) {
                        memcpy (ptr, temp, (ret > outbufsize) ? outbufsize : ret);
                        outbufsize -= ret;
                        ptr += ret;
                        if (outbufsize > 0)
                            *ptr = '\0';
                    }
                }
            }
            /* After reading complete string (aka read returns 0), we just break */
            break;
        }
    }

    /* Search for the substring of known shell-names */
    for (i = 0; i < (int)(sizeof (orte_pls_rsh_shell_name)/
                          sizeof(orte_pls_rsh_shell_name[0])); i++) {
        if (NULL != strstr (outbuf, orte_pls_rsh_shell_name[i])) {
          *shell = i;
          break;
        }
    }
    if (mca_pls_rsh_component.debug) {
        opal_output(0, "pls:rsh: node:%s has SHELL:%s\n",
                    node->node_name, orte_pls_rsh_shell_name[*shell]);
    }
    return rc;
}

/**
 * Fill the exec_path variable with the directory to the orted
 */

static int orte_pls_rsh_fill_exec_path ( char ** exec_path)
{
    struct stat buf;

    asprintf(exec_path, "%s/orted", OMPI_BINDIR);
    if (0 != stat(*exec_path, &buf)) {
        char *path = getenv("PATH");
        if (NULL == path) {
            path = ("PATH is empty!");
        }
        opal_show_help("help-pls-rsh.txt", "no-local-orted",
                        true, path, OMPI_BINDIR);
        return ORTE_ERR_NOT_FOUND;
    }
   return ORTE_SUCCESS;
}

/**
 * Callback on daemon exit.
 */

static void orte_pls_rsh_wait_daemon(pid_t pid, int status, void* cbdata)
{
    rsh_daemon_info_t *info = (rsh_daemon_info_t*) cbdata;
    opal_list_t map;
    opal_list_item_t* item;
    int rc;

    /* if ssh exited abnormally, set the child processes to aborted
       and print something useful to the user.  The usual reasons for
       ssh to exit abnormally all are a pretty good indication that
       the child processes aren't going to start up properly.

       This should somehow be pushed up to the calling level, but we
       don't really have a way to do that just yet.
    */
#ifdef __WINDOWS__
    printf("This is not implemented yet for windows\n");
    ORTE_ERROR_LOG(ORTE_ERROR);
    return;
#else
    if (! WIFEXITED(status) || ! WEXITSTATUS(status) == 0) {
        /* get the mapping for our node so we can cancel the right things */
        OBJ_CONSTRUCT(&map, opal_list_t);
        rc = orte_rmaps_base_get_node_map(orte_process_info.my_name->cellid,
                                          info->jobid,
                                          info->node->node_name,
                                          &map);
        if (ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            goto cleanup;
        }

        /* set state of all processes associated with the daemon as
           terminated */
        for(item =  opal_list_get_first(&map);
            item != opal_list_get_end(&map);
            item =  opal_list_get_next(item)) {
            orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*) item;
            size_t i;

            for (i = 0 ; i < map->num_procs ; ++i) {
                /* Clean up the session directory as if we were the
                   process itself.  This covers the case where the
                   process died abnormally and didn't cleanup its own
                   session directory. */

                orte_session_dir_finalize(&(map->procs[i])->proc_name);

                rc = orte_soh.set_proc_soh(&(map->procs[i]->proc_name),
                                           ORTE_PROC_STATE_ABORTED, status);
            }
            if (ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
            }
        }
        OBJ_DESTRUCT(&map);

 cleanup:
        /* tell the user something went wrong */
        opal_output(0, "ERROR: A daemon on node %s failed to start as expected.",
                    info->node->node_name);
        opal_output(0, "ERROR: There may be more information available from");
        opal_output(0, "ERROR: the remote shell (see above).");

        if (WIFEXITED(status)) {
            opal_output(0, "ERROR: The daemon exited unexpectedly with status %d.",
                   WEXITSTATUS(status));
        } else if (WIFSIGNALED(status)) {
#ifdef WCOREDUMP
            if (WCOREDUMP(status)) {
                opal_output(0, "The daemon received a signal %d (with core).",
                            WTERMSIG(status));
            } else {
                opal_output(0, "The daemon received a signal %d.", WTERMSIG(status));
            }
#else
            opal_output(0, "The daemon received a signal %d.", WTERMSIG(status));
#endif /* WCOREDUMP */
        } else {
            opal_output(0, "No extra status information is available: %d.", status);
        }
    }
#endif /* __WINDOWS__ */

    /* release any waiting threads */
    OPAL_THREAD_LOCK(&mca_pls_rsh_component.lock);
    if (mca_pls_rsh_component.num_children-- >=
        mca_pls_rsh_component.num_concurrent ||
        mca_pls_rsh_component.num_children == 0) {
        opal_condition_signal(&mca_pls_rsh_component.cond);
    }
    OPAL_THREAD_UNLOCK(&mca_pls_rsh_component.lock);

    /* cleanup */
    OBJ_RELEASE(info->node);
    OBJ_RELEASE(info);
}

/**
 * Launch a daemon (bootproxy) on each node. The daemon will be responsible
 * for launching the application.
 */

int orte_pls_rsh_launch(orte_jobid_t jobid)
{
    opal_list_t mapping;
    opal_list_item_t* m_item, *n_item;
    size_t num_nodes;
    orte_vpid_t vpid;
    int node_name_index1;
    int node_name_index2;
    int proc_name_index;
    int local_exec_index, local_exec_index_end;
    int call_yield_index;
    char *jobid_string;
    char *uri, *param;
    char **argv, **tmp;
    int argc;
    int rc;
    sigset_t sigs;
    struct passwd *p;
    bool remote_bash = false, remote_csh = false;
    bool local_bash = false, local_csh = false;

    /* Query the list of nodes allocated and mapped to this job.
     * We need the entire mapping for a couple of reasons:
     *  - need the prefix to start with.
     *  - need to know if we are launching on a subset of the allocated nodes
     * All other mapping responsibilities fall to orted in the fork PLS
     */
    OBJ_CONSTRUCT(&mapping, opal_list_t);
    rc = orte_rmaps_base_get_map(jobid, &mapping);
    if (ORTE_SUCCESS != rc) {
        goto cleanup;
    }

    num_nodes = 0;
    for(m_item = opal_list_get_first(&mapping);
        m_item != opal_list_get_end(&mapping);
        m_item = opal_list_get_next(m_item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)m_item;
        num_nodes += opal_list_get_size(&map->nodes);
    }

    /*
     * Allocate a range of vpids for the daemons.
     */
    if (num_nodes == 0) {
        return ORTE_ERR_BAD_PARAM;
    }
    rc = orte_ns.reserve_range(0, num_nodes, &vpid);
    if (ORTE_SUCCESS != rc) {
        goto cleanup;
    }

    /* need integer value for command line parameter */
    if (ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_string, jobid))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* What is our local shell? */
    p = getpwuid(getuid());
    if (NULL != p) {
        local_csh = (strstr(p->pw_shell, "csh") != 0) ? true : false;
        if ((strstr(p->pw_shell, "bash") != 0) ||
            (strstr(p->pw_shell, "zsh") != 0)) {
            local_bash = true;
        } else {
            local_bash = false;
        }
        if (mca_pls_rsh_component.debug) {
            opal_output(0, "pls:rsh: local csh: %d, local bash: %d\n",
                        local_csh, local_bash);
        }
    }

    /* What is our remote shell? */
    if (mca_pls_rsh_component.assume_same_shell) {
        remote_bash = local_bash;
        remote_csh = local_csh;
        if (mca_pls_rsh_component.debug) {
            opal_output(0, "pls:rsh: assuming same remote shell as local shell");
        }
    } else {
        orte_pls_rsh_shell shell;
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)opal_list_get_first(&mapping);
        orte_rmaps_base_node_t* rmaps_node = 
            (orte_rmaps_base_node_t*)opal_list_get_first(&map->nodes);
        orte_ras_node_t* node = rmaps_node->node;

        rc = orte_pls_rsh_probe(node, &shell);

        if (ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        switch (shell) {
        case ORTE_PLS_RSH_SHELL_KSH: /* fall through */
        case ORTE_PLS_RSH_SHELL_BASH: remote_bash = true; break;
        case ORTE_PLS_RSH_SHELL_TCSH: /* fall through */
        case ORTE_PLS_RSH_SHELL_CSH:  remote_csh = true; break;
        default:
            opal_output(0, "WARNING: rsh probe returned unhandled shell:%s assuming bash\n",
                        orte_pls_rsh_shell_name[shell]);
            remote_bash = true;
        }
    }
    if (mca_pls_rsh_component.debug) {
        opal_output(0, "pls:rsh: remote csh: %d, remote bash: %d\n",
                    remote_csh, remote_bash);
    }

    /*
     * Build argv array
     */
    argv = opal_argv_copy(mca_pls_rsh_component.argv);
    argc = mca_pls_rsh_component.argc;
    node_name_index1 = argc;
    opal_argv_append(&argc, &argv, "<template>");

    /* Do we need to source .profile on the remote side? */

    if (!(remote_csh || remote_bash)) {
        int i;
        tmp = opal_argv_split("( ! [ -e ./.profile ] || . ./.profile;", ' ');
        if (NULL == tmp) {
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        for (i = 0; NULL != tmp[i]; ++i) {
            opal_argv_append(&argc, &argv, tmp[i]);
        }
        opal_argv_free(tmp);
    }

    /* add the daemon command (as specified by user) */
    local_exec_index = argc;
    opal_argv_append(&argc, &argv, mca_pls_rsh_component.orted);

    /* check for debug flags */
    orte_pls_base_proxy_mca_argv(&argc, &argv);

    opal_argv_append(&argc, &argv, "--bootproxy");
    opal_argv_append(&argc, &argv, jobid_string);
    opal_argv_append(&argc, &argv, "--name");
    proc_name_index = argc;
    opal_argv_append(&argc, &argv, "<template>");

    /* tell the daemon how many procs are in the daemon's job */
    opal_argv_append(&argc, &argv, "--num_procs");
    asprintf(&param, "%lu", (unsigned long)(vpid + num_nodes));
    opal_argv_append(&argc, &argv, param);
    free(param);
    /* tell the daemon the starting vpid of the daemon's job */
    opal_argv_append(&argc, &argv, "--vpid_start");
    opal_argv_append(&argc, &argv, "0");

    opal_argv_append(&argc, &argv, "--nodename");
    node_name_index2 = argc;
    opal_argv_append(&argc, &argv, "<template>");

    /* pass along the universe name and location info */
    opal_argv_append(&argc, &argv, "--universe");
    asprintf(&param, "%s@%s:%s", orte_universe_info.uid,
                orte_universe_info.host, orte_universe_info.name);
    opal_argv_append(&argc, &argv, param);
    free(param);

    /* setup ns contact info */
    opal_argv_append(&argc, &argv, "--nsreplica");
    if (NULL != orte_process_info.ns_replica_uri) {
        uri = strdup(orte_process_info.ns_replica_uri);
    } else {
        uri = orte_rml.get_uri();
    }
    asprintf(&param, "\"%s\"", uri);
    opal_argv_append(&argc, &argv, param);
    free(uri);
    free(param);

    /* setup gpr contact info */
    opal_argv_append(&argc, &argv, "--gprreplica");
    if (NULL != orte_process_info.gpr_replica_uri) {
        uri = strdup(orte_process_info.gpr_replica_uri);
    } else {
        uri = orte_rml.get_uri();
    }
    asprintf(&param, "\"%s\"", uri);
    opal_argv_append(&argc, &argv, param);
    free(uri);
    free(param);

    opal_argv_append(&argc, &argv, "--mpi-call-yield");
    call_yield_index = argc;
    opal_argv_append(&argc, &argv, "0");

    local_exec_index_end = argc;
    if (!(remote_csh || remote_bash)) {
        opal_argv_append(&argc, &argv, ")");
    }
    if (mca_pls_rsh_component.debug) {
        param = opal_argv_join(argv, ' ');
        if (NULL != param) {
            opal_output(0, "pls:rsh: final template argv:");
            opal_output(0, "pls:rsh:     %s", param);
            free(param);
        }
    }

    /*
     * Iterate through each of the contexts
     */
    for(m_item = opal_list_get_first(&mapping);
        m_item != opal_list_get_end(&mapping);
        m_item = opal_list_get_next(m_item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)m_item;
        char * prefix_dir = map->app->prefix_dir;

        /*
         * For each of the contexts - iterate through the nodes.
         */
        for(n_item =  opal_list_get_first(&map->nodes);
            n_item != opal_list_get_end(&map->nodes);
            n_item =  opal_list_get_next(n_item)) {
            orte_rmaps_base_node_t* rmaps_node = (orte_rmaps_base_node_t*)n_item;
            orte_ras_node_t* ras_node = rmaps_node->node;
            orte_process_name_t* name;
            pid_t pid;
            char *exec_path;
            char **exec_argv;

            /* already launched on this node */
            if(ras_node->node_launched++ != 0)
                continue;

            /* setup node name */
            free(argv[node_name_index1]);
            if (NULL != ras_node->node_username &&
                0 != strlen (ras_node->node_username)) {
                asprintf (&argv[node_name_index1], "%s@%s",
                          ras_node->node_username, ras_node->node_name);
            } else {
                argv[node_name_index1] = strdup(ras_node->node_name);
            }

            free(argv[node_name_index2]);
            argv[node_name_index2] = strdup(ras_node->node_name);

            /* initialize daemons process name */
            rc = orte_ns.create_process_name(&name, ras_node->node_cellid, 0, vpid);
            if (ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
                goto cleanup;
            }

            /* rsh a child to exec the rsh/ssh session */
    #ifdef __WINDOWS__
            printf("Unimplemented feature for windows\n");
            return;
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
                    return OMPI_ERROR;
                }
                /* get child pid */
                process_id = GetProcessId(&pi);
                pid = (int) process_id;
            }
    #endif
    #else
            pid = fork();
    #endif
            if (pid < 0) {
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                goto cleanup;
            }

            /* child */
            if (pid == 0) {
                char* name_string;
                char** env;
                char* var;
                long fd, fdmax = sysconf(_SC_OPEN_MAX);

                chdir("/tmp");
                if (mca_pls_rsh_component.debug) {
                    opal_output(0, "pls:rsh: launching on node %s\n",
                                ras_node->node_name);
                }

                /* set the progress engine schedule for this node.
                 * if node_slots is set to zero, then we default to
                 * NOT being oversubscribed
                 */
                if (ras_node->node_slots > 0 &&
                    opal_list_get_size(&rmaps_node->node_procs) > ras_node->node_slots) {
                    if (mca_pls_rsh_component.debug) {
                        opal_output(0, "pls:rsh: oversubscribed -- setting mpi_yield_when_idle to 1 (%d %d)",
                                    ras_node->node_slots, opal_list_get_size(&rmaps_node->node_procs));
                    }
                    free(argv[call_yield_index]);
                    argv[call_yield_index] = strdup("1");
                } else {
                    if (mca_pls_rsh_component.debug) {
                        opal_output(0, "pls:rsh: not oversubscribed -- setting mpi_yield_when_idle to 0");
                    }
                    free(argv[call_yield_index]);
                    argv[call_yield_index] = strdup("0");
                }

                /* Is this a local launch?
                 *
                 * Not all node names may be resolvable (if we found
                 * localhost in the hostfile, for example).  So first
                 * check trivial case of node_name being same as the
                 * current nodename, which must be local.  If that doesn't
                 * match, check using ifislocal().
                 */
                if (0 == strcmp(ras_node->node_name, orte_system_info.nodename) ||
                    opal_ifislocal(ras_node->node_name)) {
                    if (mca_pls_rsh_component.debug) {
                        opal_output(0, "pls:rsh: %s is a LOCAL node\n",
                                    ras_node->node_name);
                    }
                    exec_argv = &argv[local_exec_index];
                    exec_path = opal_path_findv(exec_argv[0], 0, environ, NULL);

                    if (NULL == exec_path && NULL == prefix_dir) {
                        rc = orte_pls_rsh_fill_exec_path (&exec_path);
                        if (ORTE_SUCCESS != rc) {
                            return rc;
                        }
                    } else {
                        if (NULL != prefix_dir) {
                            asprintf(&exec_path, "%s/bin/orted", prefix_dir);
                        }
                        /* If we yet did not fill up the execpath, do so now */
                        if (NULL == exec_path) {
                            rc = orte_pls_rsh_fill_exec_path (&exec_path);
                            if (ORTE_SUCCESS != rc) {
                                return rc;
                            }
                        }
                    }

                    /* If we have a prefix, then modify the PATH and
                       LD_LIBRARY_PATH environment variables.  We're
                       already in the child process, so it's ok to modify
                       environ. */
                    if (NULL != prefix_dir) {
                        char *oldenv, *newenv;

                        /* Reset PATH */
                        oldenv = getenv("PATH");
                        if (NULL != oldenv) {
                            asprintf(&newenv, "%s/bin:%s\n", prefix_dir, oldenv);
                        } else {
                            asprintf(&newenv, "%s/bin", prefix_dir);
                        }
                        opal_setenv("PATH", newenv, true, &environ);
                        if (mca_pls_rsh_component.debug) {
                            opal_output(0, "pls:rsh: reset PATH: %s", newenv);
                        }
                        free(newenv);

                        /* Reset LD_LIBRARY_PATH */
                        oldenv = getenv("LD_LIBRARY_PATH");
                        if (NULL != oldenv) {
                            asprintf(&newenv, "%s/lib:%s\n", prefix_dir, oldenv);
                        } else {
                            asprintf(&newenv, "%s/lib", prefix_dir);
                        }
                        opal_setenv("LD_LIBRARY_PATH", newenv, true, &environ);
                        if (mca_pls_rsh_component.debug) {
                            opal_output(0, "pls:rsh: reset LD_LIBRARY_PATH: %s",
                                        newenv);
                        }
                        free(newenv);
                    }

                    /* Since this is a local execution, we need to
                       potentially whack the final ")" in the argv (if
                       sh/csh conditionals, from above).  Note that we're
                       modifying the argv[] in the child process, so
                       there's no need to save this and restore it
                       afterward -- the parent's argv[] is unmodified. */
                    if (NULL != argv[local_exec_index_end]) {
                        free(argv[local_exec_index_end]);
                        argv[local_exec_index_end] = NULL;
                    }
                } else {
                    if (mca_pls_rsh_component.debug) {
                        opal_output(0, "pls:rsh: %s is a REMOTE node\n",
                                    ras_node->node_name);
                    }
                    exec_argv = argv;
                    exec_path = strdup(mca_pls_rsh_component.path);

                    if (NULL != prefix_dir) {
                        if (remote_bash) {
                            asprintf (&argv[local_exec_index],
                                    "PATH=%s/bin:$PATH ; export PATH ; "
                                    "LD_LIBRARY_PATH=%s/lib:$LD_LIBRARY_PATH ; export LD_LIBRARY_PATH ; "
                                    "%s/bin/%s",
                                    prefix_dir,
                                    prefix_dir,
                                    prefix_dir, mca_pls_rsh_component.orted);
                        }
                        /*
                         * This needs cleanup:
                         * Only if the LD_LIBRARY_PATH is set, prepend our prefix/lib to it....
                         */
                        if (remote_csh) {
                            asprintf (&argv[local_exec_index],
                                    "set path = ( %s/bin $path ) ; "
                                    "if ( \"$?LD_LIBRARY_PATH\" == 1 ) "
                                    "setenv LD_LIBRARY_PATH %s/lib:$LD_LIBRARY_PATH ; "
                                    "if ( \"$?LD_LIBRARY_PATH\" == 0 ) "
                                    "setenv LD_LIBRARY_PATH %s/lib ; "
                                    "%s/bin/%s",
                                    prefix_dir,
                                    prefix_dir,
                                    prefix_dir,
                                    prefix_dir, mca_pls_rsh_component.orted);
                        }
                    }
                }

                /* setup process name */
                rc = orte_ns.get_proc_name_string(&name_string, name);
                if (ORTE_SUCCESS != rc) {
                    opal_output(0, "orte_pls_rsh: unable to create process name");
                    exit(-1);
                }
                free(argv[proc_name_index]);
                argv[proc_name_index] = strdup(name_string);

                if (!mca_pls_rsh_component.debug) {
                     /* setup stdin */
                    int fd = open("/dev/null", O_RDWR);
                    dup2(fd, 0);
                    close(fd);
                }

                /* close all file descriptors w/ exception of stdin/stdout/stderr */
                for(fd=3; fd<fdmax; fd++)
                    close(fd);

                /* Set signal handlers back to the default.  Do this close
                   to the execve() because the event library may (and likely
                   will) reset them.  If we don't do this, the event
                   library may have left some set that, at least on some
                   OS's, don't get reset via fork() or exec().  Hence, the
                   orted could be unkillable (for example). */

                set_handler_default(SIGTERM);
                set_handler_default(SIGINT);
    #ifndef __WINDOWS__
                set_handler_default(SIGHUP);
                set_handler_default(SIGPIPE);
    #endif
                set_handler_default(SIGCHLD);

                /* Unblock all signals, for many of the same reasons that
                   we set the default handlers, above.  This is noticable
                   on Linux where the event library blocks SIGTERM, but we
                   don't want that blocked by the orted (or, more
                   specifically, we don't want it to be blocked by the
                   orted and then inherited by the ORTE processes that it
                   forks, making them unkillable by SIGTERM). */
    #ifndef __WINDOWS__
                sigprocmask(0, 0, &sigs);
                sigprocmask(SIG_UNBLOCK, &sigs, 0);
    #endif

                /* setup environment */
                env = opal_argv_copy(environ);
                var = mca_base_param_environ_variable("seed",NULL,NULL);
                opal_setenv(var, "0", true, &env);

                /* exec the daemon */
                if (mca_pls_rsh_component.debug) {
                    param = opal_argv_join(exec_argv, ' ');
                    if (NULL != param) {
                        opal_output(0, "pls:rsh: executing: %s", param);
                        free(param);
                    }
                }
                execve(exec_path, exec_argv, env);
                opal_output(0, "pls:rsh: execv failed with errno=%d\n", errno);
                exit(-1);

            } else { /* father */
                rsh_daemon_info_t *daemon_info;

                OPAL_THREAD_LOCK(&mca_pls_rsh_component.lock);
                if (mca_pls_rsh_component.num_children++ >=
                    mca_pls_rsh_component.num_concurrent) {
                    opal_condition_wait(&mca_pls_rsh_component.cond, &mca_pls_rsh_component.lock);
                }
                OPAL_THREAD_UNLOCK(&mca_pls_rsh_component.lock);

                /* save the daemons name on the node */
                if (ORTE_SUCCESS != (rc = orte_pls_base_proxy_set_node_name(ras_node,jobid,name))) {
                    ORTE_ERROR_LOG(rc);
                    goto cleanup;
                }

                /* setup callback on sigchild - wait until setup above is complete
                 * as the callback can occur in the call to orte_wait_cb
                 */
                daemon_info = OBJ_NEW(rsh_daemon_info_t);
                OBJ_RETAIN(ras_node);
                daemon_info->node = ras_node;
                daemon_info->jobid = jobid;
                orte_wait_cb(pid, orte_pls_rsh_wait_daemon, daemon_info);

                /* if required - add delay to avoid problems w/ X11 authentication */
                if (mca_pls_rsh_component.debug && mca_pls_rsh_component.delay) {
                    sleep(mca_pls_rsh_component.delay);
                }
                vpid++;
            }
            free(name);
        }
    }

cleanup:
    while (NULL != (m_item = opal_list_remove_first(&mapping))) {
        OBJ_RELEASE(m_item);
    }
    OBJ_DESTRUCT(&mapping);

    free(jobid_string);  /* done with this variable */
    opal_argv_free(argv);

    return rc;
}


/**
 * Query the registry for all nodes participating in the job
 */
int orte_pls_rsh_terminate_job(orte_jobid_t jobid)
{
    return orte_pls_base_proxy_terminate_job(jobid);
}

int orte_pls_rsh_terminate_proc(const orte_process_name_t* proc)
{
    return orte_pls_base_proxy_terminate_proc(proc);
}

int orte_pls_rsh_finalize(void)
{
    if (mca_pls_rsh_component.reap) {
        OPAL_THREAD_LOCK(&mca_pls_rsh_component.lock);
        while (mca_pls_rsh_component.num_children > 0) {
            opal_condition_wait(&mca_pls_rsh_component.cond, &mca_pls_rsh_component.lock);
        }
        OPAL_THREAD_UNLOCK(&mca_pls_rsh_component.lock);
    }

    /* cleanup any pending recvs */
    orte_rml.recv_cancel(ORTE_RML_NAME_ANY, ORTE_RML_TAG_RMGR_CLNT);
    return ORTE_SUCCESS;
}


/**
 * Handle threading issues.
 */

#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS && OMPI_ENABLE_PROGRESS_THREADS

struct orte_pls_rsh_stack_t {
    opal_condition_t cond;
    opal_mutex_t mutex;
    bool complete;
    orte_jobid_t jobid;
    int rc;
};
typedef struct orte_pls_rsh_stack_t orte_pls_rsh_stack_t;

static void orte_pls_rsh_stack_construct(orte_pls_rsh_stack_t* stack)
{
    OBJ_CONSTRUCT(&stack->mutex, opal_mutex_t);
    OBJ_CONSTRUCT(&stack->cond, opal_condition_t);
    stack->rc = 0;
    stack->complete = false;
}

static void orte_pls_rsh_stack_destruct(orte_pls_rsh_stack_t* stack)
{
    OBJ_DESTRUCT(&stack->mutex);
    OBJ_DESTRUCT(&stack->cond);
}

static OBJ_CLASS_INSTANCE(
    orte_pls_rsh_stack_t,
    opal_object_t,
    orte_pls_rsh_stack_construct,
    orte_pls_rsh_stack_destruct);

static void orte_pls_rsh_launch_cb(int fd, short event, void* args)
{
    orte_pls_rsh_stack_t *stack = (orte_pls_rsh_stack_t*)args;
    OPAL_THREAD_LOCK(&stack->mutex);
    stack->rc = orte_pls_rsh_launch(stack->jobid);
    stack->complete = true;
    opal_condition_signal(&stack->cond);
    OPAL_THREAD_UNLOCK(&stack->mutex);
}

static int orte_pls_rsh_launch_threaded(orte_jobid_t jobid)
{
    struct timeval tv = { 0, 0 };
    struct opal_event event;
    struct orte_pls_rsh_stack_t stack;

    OBJ_CONSTRUCT(&stack, orte_pls_rsh_stack_t);

    stack.jobid = jobid;
    if( opal_event_progress_thread() ) {
        stack.rc = orte_pls_rsh_launch( jobid );
    } else {
        opal_evtimer_set(&event, orte_pls_rsh_launch_cb, &stack);
        opal_evtimer_add(&event, &tv);

        OPAL_THREAD_LOCK(&stack.mutex);
        while (stack.complete == false) {
            opal_condition_wait(&stack.cond, &stack.mutex);
        }
        OPAL_THREAD_UNLOCK(&stack.mutex);
    }
    OBJ_DESTRUCT(&stack);
    return stack.rc;
}

#endif


static void set_handler_default(int sig)
{
#ifndef __WINDOWS__
    struct sigaction act;

    act.sa_handler = SIG_DFL;
    act.sa_flags = 0;
    sigemptyset(&act.sa_mask);

    sigaction(sig, &act, (struct sigaction *)0);
#endif
}
