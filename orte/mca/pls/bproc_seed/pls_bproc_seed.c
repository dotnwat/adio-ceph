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

#include "orte_config.h"
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */
#include <errno.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif  /* HAVE_SYS_STAT_H */
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif  /* HAVE_FCNTL_H */
#include <sys/wait.h>

#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "opal/util/opal_environ.h"
#include "orte/util/proc_info.h"
#include "opal/event/event.h"
#include "orte/runtime/orte_wait.h"
#include "orte/runtime/runtime.h"
#include "orte/mca/ns/base/base.h"
#include "orte/mca/sds/base/base.h"
#include "orte/mca/pls/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "orte/mca/iof/iof.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/soh/soh.h"
#include "orte/mca/soh/base/base.h"
#include "orte/mca/ras/base/base.h"
#include "orte/mca/rmaps/base/rmaps_base_map.h"

#include "pls_bproc_seed.h"

#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS
int orte_pls_bproc_seed_launch_threaded(orte_jobid_t);
#endif


orte_pls_base_module_t orte_pls_bproc_seed_module = {
#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS
    orte_pls_bproc_seed_launch_threaded,
#else
    orte_pls_bproc_seed_launch,
#endif
    orte_pls_bproc_seed_terminate_job,
    orte_pls_bproc_seed_terminate_proc,
    orte_pls_bproc_seed_signal_job,
    orte_pls_bproc_seed_signal_proc,
    orte_pls_bproc_seed_finalize
};



static int orte_pls_bproc_nodelist(orte_rmaps_base_map_t* map, int** nodelist, size_t* num_nodes)
{
    opal_list_item_t* item;
    size_t count = opal_list_get_size(&map->nodes);
    size_t index = 0;

    /* build the node list */
    *nodelist = (int*)malloc(sizeof(int) * count);
    if(NULL == *nodelist)
        return ORTE_ERR_OUT_OF_RESOURCE;

    for(item =  opal_list_get_first(&map->nodes);
        item != opal_list_get_end(&map->nodes);
        item =  opal_list_get_next(item)) {
        orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)item;
        (*nodelist)[index++] = atol(node->node->node_name);
    }
    *num_nodes = count;
    return ORTE_SUCCESS;
}

/*
 *  Execute/dump a process and read the image into memory.
 */

static int orte_pls_bproc_dump(orte_app_context_t* app, uint8_t** image, size_t* image_len)
{
    pid_t pid;
    int pfd[2];
    size_t cur_offset, tot_offset, num_buffers;
    uint8_t *image_buffer;
    int rc = ORTE_SUCCESS;

    if (pipe(pfd)) {
        opal_output(0, "orte_pls_bproc_seed: pipe() failed errno=%d\n",errno);
        return ORTE_ERROR;
    }

    if ((pid = fork ()) < 0) {
        opal_output(0, "orte_pls_bproc_seed: fork() failed errno=%d\n",errno);
        return ORTE_ERROR;
    }

    if (pid == 0) {
        close(pfd[0]);  /* close the read end - we are write only */
        chdir(app->cwd);
        bproc_execdump(pfd[1], BPROC_DUMP_EXEC | BPROC_DUMP_OTHER, app->app, app->argv, app->env);
        exit(0);
    }

    /* this is the parent - I will read the
     * info coming from the pipe
     */

    close(pfd[1]); /* close the sending end - we are read only */
    image_buffer = (uint8_t*)malloc(mca_pls_bproc_seed_component.image_frag_size);
    if (!image_buffer) {
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    tot_offset = 0;
    cur_offset = 0;
    num_buffers = 1;
    while (1) {
        int num_bytes = read(pfd[0], image_buffer + tot_offset, mca_pls_bproc_seed_component.image_frag_size - cur_offset);
        if (0 > num_bytes) {  /* got an error - abort process */
            free(image_buffer);
            rc = ORTE_ERR_OUT_OF_RESOURCE;
            ORTE_ERROR_LOG(rc);
            goto cleanup;
        } else if (0 == num_bytes) {
            break;
        }

        tot_offset += num_bytes;
        cur_offset += num_bytes;
        if (mca_pls_bproc_seed_component.image_frag_size == cur_offset) {  /* filled the current buffer -  need to realloc */
            num_buffers++;
            image_buffer = (uint8_t*)realloc(image_buffer, num_buffers * mca_pls_bproc_seed_component.image_frag_size);
            if(NULL == image_buffer) {
                rc = ORTE_ERR_OUT_OF_RESOURCE;
                ORTE_ERROR_LOG(rc);
                goto cleanup;
            }
            cur_offset = 0;
        }
    }
    *image = image_buffer;
    *image_len = tot_offset;

    if(tot_offset == 0) {
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        goto cleanup;
    }

cleanup:
    close(pfd[0]);
    waitpid(pid,0,0);
    return rc;
}

/*
 *  Spew out a new child based on the in-memory process image.
 */

static int orte_pls_bproc_undump(
    orte_rmaps_base_proc_t* proc,
    orte_vpid_t vpid_start,
    orte_vpid_t vpid_range,
    uint8_t* image,
    size_t image_len,
    pid_t* pid)
{
    int p_name[2];
    int p_stdout[2];
    int p_stderr[2];
    int p_image[2];
    int rc = ORTE_SUCCESS;
    size_t bytes_writen = 0;

    if(pipe(p_name)   < 0 ||
       pipe(p_stdout) < 0 ||
       pipe(p_stderr) < 0 ||
       pipe(p_image)  < 0) {
       rc = ORTE_ERR_OUT_OF_RESOURCE;
       ORTE_ERROR_LOG(rc);
       return rc;
    }

    /* fork a child process which is overwritten with the process image */
    *pid = fork();
    if (*pid == 0) {

        /* child is read-only */
        close(p_image[1]);
        close(p_name[1]);

        /* child is write-only */
        close(p_stdout[0]);
        close(p_stderr[0]);

        /* setup stdout/stderr */
        dup2(p_stdout[1], 1);
        close(p_stdout[1]);
        dup2(p_stderr[1], 2);
        close(p_stderr[1]);

        /* verify that the name file descriptor is free */
        if(p_image[0] == mca_pls_bproc_seed_component.name_fd) {
            int fd = dup(p_image[0]);
            close(p_image[0]);
            p_image[0] = fd;
        }
        if(p_name[0] != mca_pls_bproc_seed_component.name_fd) {
            dup2(p_name[0], mca_pls_bproc_seed_component.name_fd);
            close(p_name[0]);
        }

        bproc_undump(p_image[0]);  /* child is now executing */
        opal_output(0, "orte_pls_bproc: bproc_undump(%d) failed errno=%d\n", p_image[0], errno);
        exit(1);
    }

    if (*pid < 0) {
        close(p_image[0]);
        close(p_image[1]);
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* parent is write-only */
    close(p_image[0]);
    close(p_name[0]);

    /* parent is read-only */
    close(p_stdout[1]);
    close(p_stderr[1]);

    if(*pid < 0) {
        close(p_image[1]);
        close(p_name[1]);
        close(p_stdout[0]);
        close(p_stderr[0]);
        rc = ORTE_ERR_OUT_OF_RESOURCE;
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* connect the app to the IOF framework */
    rc = orte_iof.iof_publish(&proc->proc_name, ORTE_IOF_SOURCE, ORTE_IOF_STDOUT, p_stdout[0]);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    rc = orte_iof.iof_publish(&proc->proc_name, ORTE_IOF_SOURCE, ORTE_IOF_STDERR, p_stderr[0]);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* set the process status in the registery - child is not running yet */
    rc = orte_pls_base_set_proc_pid(&proc->proc_name, *pid);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* write the process image to the app */
    while(bytes_writen < image_len) {
        rc = write(p_image[1], image+bytes_writen, image_len-bytes_writen);
        if(rc < 0) {
            opal_output(0, "orte_pls_bproc_undump: write failed errno=%d\n", errno);
            return ORTE_ERROR;
        }
        bytes_writen += rc;
    }
    close(p_image[1]);

    /* write the process name */
    orte_ns_nds_pipe_put(&proc->proc_name, vpid_start, vpid_range, p_name[1]);
    close(p_name[1]);
    return ORTE_SUCCESS;
}


/*
 *  Wait for a callback indicating the child has completed.
 */

static void orte_pls_bproc_wait_proc(pid_t pid, int status, void* cbdata)
{
    orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)cbdata;

    /* set the state of this process */
    if(NULL != proc) {
        int rc = orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_TERMINATED, status);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
        }
        OBJ_RELEASE(proc);
    }

    /* release any waiting threads */
    OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
    mca_pls_bproc_seed_component.num_children--;
    opal_condition_signal(&mca_pls_bproc_seed_component.condition);
    OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);
}


/*
 *  Wait for a callback indicating the daemon has exited.
 */
static void orte_pls_bproc_wait_node(pid_t pid, int status, void* cbdata)
{
    orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)cbdata;
    opal_list_item_t* item;

    /* set state of all processes associated with the daemon as terminated */
    for(item =  opal_list_get_first(&node->node_procs);
        item != opal_list_get_end(&node->node_procs);
        item =  opal_list_get_next(item)) {
        orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)item;

        int rc = orte_soh.set_proc_soh(&proc->proc_name, ORTE_PROC_STATE_TERMINATED, 0);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
        }
    }
    OBJ_RELEASE(node);

    /* release any waiting threads */
    OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
    mca_pls_bproc_seed_component.num_children--;
    opal_condition_signal(&mca_pls_bproc_seed_component.condition);
    OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);
}


/*
 *  (1) Execute/dump the process image and read into memory.
 *  (2) Fork a daemon across the allocated set of nodes.
 *  (3) Fork/undump the required number of copies of the process
 *      on each of the nodes.
 */

static int orte_pls_bproc_launch_app(
    orte_jobid_t jobid,
    orte_rmaps_base_map_t* map,
    orte_vpid_t vpid_start,
    orte_vpid_t vpid_range)
{
    uint8_t* image = NULL;
    size_t image_len;
    int* node_list = NULL;
    int* daemon_pids = NULL;
    size_t num_nodes;
    orte_vpid_t daemon_vpid_start = 0;
    int rc, index;
    char* uri;
    char *var;
    int num_env;

    /* convert node names to bproc nodelist */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_nodelist(map, &node_list, &num_nodes))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    if(NULL == (daemon_pids = (int*)malloc(sizeof(int) * num_nodes))) {
        goto cleanup;
    }

    /* append mca parameters to our environment */
    num_env = opal_argv_count(map->app->env);
    if(ORTE_SUCCESS != (rc = mca_base_param_build_env(&map->app->env, &num_env, true))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* overwrite seed setting */
    var = mca_base_param_environ_variable("seed",NULL,NULL);
    opal_setenv(var, "0", true, &map->app->env);

    /* set name discovery mode */
    var = mca_base_param_environ_variable("ns","nds",NULL);
    opal_setenv(var, "pipe", true, &map->app->env);
    free(var);

    /* ns replica contact info */
    if(NULL == orte_process_info.ns_replica) {
        rc = orte_ns.copy_process_name(&orte_process_info.ns_replica,orte_process_info.my_name);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            goto cleanup;
        }
        orte_process_info.ns_replica_uri = orte_rml.get_uri();
    }
    var = mca_base_param_environ_variable("ns","replica","uri");
    opal_setenv(var,orte_process_info.ns_replica_uri, true, &map->app->env);
    free(var);

    /* gpr replica contact info */
    if(NULL == orte_process_info.gpr_replica) {
        rc = orte_ns.copy_process_name(&orte_process_info.gpr_replica,orte_process_info.my_name);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            goto cleanup;
        }
        orte_process_info.gpr_replica_uri = orte_rml.get_uri();
    }
    var = mca_base_param_environ_variable("gpr","replica","uri");
    opal_setenv(var,orte_process_info.gpr_replica_uri, true, &map->app->env);
    free(var);

    /* read process image */
    if(ORTE_SUCCESS != (rc = orte_pls_bproc_dump(map->app, &image, &image_len))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* allocate a range of vpids for the daemons */
    if(ORTE_SUCCESS != (rc = orte_ns.reserve_range(0, num_nodes, &daemon_vpid_start))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* save our contact information - push out to daemons */
    if(NULL == (uri = orte_rml.get_uri())) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* replicate the process image to all nodes */
    rc = bproc_vrfork(num_nodes, node_list, daemon_pids);
    if(rc < 0) {
        ORTE_ERROR_LOG(rc);
        return ORTE_ERROR;
    }

    /* return is the rank of the child or number of nodes in the parent */
    if(rc < (int)num_nodes) {

        opal_list_item_t* item;
        orte_rmaps_base_node_t* node = NULL;
        orte_process_name_t* daemon_name;
        int fd;
        int rank = rc;

        /* connect stdin to /dev/null */
        fd = open("/dev/null", O_RDONLY);
        if(fd >= 0) {
            if(fd != 0) {
                dup2(fd, 0);
                close(fd);
            }
        }

        /* connect stdout/stderr to a file */
        fd = open("/dev/null", O_CREAT|O_WRONLY|O_TRUNC, 0666);
        if(fd >= 0) {
            if(fd != 1) {
                dup2(fd,1);
            }
            if(fd != 2) {
                dup2(fd,2);
            }
            if(fd > 2) {
                close(fd);
            }
        } else {
            _exit(-1);
        }

        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: rank=%d\n", rank);
        }

        /* find this node */
        index = 0;
        for(item =  opal_list_get_first(&map->nodes);
            item != opal_list_get_end(&map->nodes);
            item =  opal_list_get_next(item)) {
            if(index++ == rank) {
                node = (orte_rmaps_base_node_t*)item;
                break;
            }
        }
        if(NULL == node) {
            rc = ORTE_ERR_NOT_FOUND;
            ORTE_ERROR_LOG(rc);
            _exit(-1);
        }

        /* setup the daemons process name */
        rc = orte_ns.create_process_name(
            &daemon_name, orte_process_info.my_name->cellid, 0, daemon_vpid_start + rank);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            _exit(-1);
        }
        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: node=%s name=%d.%d.%d procs=%d\n",
            node->node->node_name,
                orte_process_info.my_name->cellid, 0,
                daemon_vpid_start+rank,
                opal_list_get_size(&node->node_procs));
        }

        /* restart the daemon w/ the new process name */
        rc = orte_restart(daemon_name, uri);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            _exit(-1);
        }

        /* save the daemons pid in the registry */
        rc = orte_pls_base_set_node_pid(node->node->node_cellid, node->node->node_name, jobid, getpid());
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            _exit(-1);
        }

        /* start the required number of copies of the application */
        index = 0;
        for(item =  opal_list_get_first(&node->node_procs);
            item != opal_list_get_end(&node->node_procs);
            item =  opal_list_get_next(item)) {
            orte_rmaps_base_proc_t* proc = (orte_rmaps_base_proc_t*)item;
            pid_t pid;

            if(mca_pls_bproc_seed_component.debug) {
                opal_output(0, "orte_pls_bproc: starting: [%lu,%lu,%lu]\n", ORTE_NAME_ARGS(&proc->proc_name));
            }
            rc = orte_pls_bproc_undump(proc, vpid_start, vpid_range, image, image_len, &pid);
            if(ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
                _exit(1);
            }

            OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
            mca_pls_bproc_seed_component.num_children++;
            OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);
            OBJ_RETAIN(proc);
            orte_wait_cb(pid, orte_pls_bproc_wait_proc, proc);

            if(mca_pls_bproc_seed_component.debug) {
                opal_output(0, "orte_pls_bproc: started: [%lu,%lu,%lu]\n", ORTE_NAME_ARGS(&proc->proc_name));
            }
        }

        /* free memory associated with the process image */
        free(image);

        /* wait for all children to complete */
        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: waiting for %d children",  mca_pls_bproc_seed_component.num_children);
        }
        OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
        while(mca_pls_bproc_seed_component.num_children > 0) {
            opal_condition_wait(
                &mca_pls_bproc_seed_component.condition,
                &mca_pls_bproc_seed_component.lock);
        }
        OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);

        /* daemon is done when all children have completed */
        orte_finalize();
        _exit(0);

    } else {
        opal_list_item_t* item;

        /* post wait callback the daemons to complete */
        index = 0;
        while(NULL != (item = opal_list_remove_first(&map->nodes))) {
            orte_rmaps_base_node_t* node = (orte_rmaps_base_node_t*)item;

            OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
            mca_pls_bproc_seed_component.num_children++;
            OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);
            orte_wait_cb(daemon_pids[index++], orte_pls_bproc_wait_node, node);
        }

        /* release resources */
        rc = ORTE_SUCCESS;
    }

cleanup:
    if(NULL != image)
        free(image);
    if(NULL != node_list)
        free(node_list);
    if(NULL != daemon_pids)
        free(daemon_pids);
    return rc;
}


/*
 * Query for the default mapping.  Launch each application context
 * w/ a distinct set of daemons.
 */

int orte_pls_bproc_seed_launch(orte_jobid_t jobid)
{
    opal_list_item_t* item;
    opal_list_t mapping;
    orte_vpid_t vpid_start;
    orte_vpid_t vpid_range;
    int rc;

    /* query for the application context and allocated nodes */
    OBJ_CONSTRUCT(&mapping, opal_list_t);
    if(ORTE_SUCCESS != (rc = orte_rmaps_base_get_map(jobid, &mapping))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    if(ORTE_SUCCESS != (rc = orte_rmaps_base_get_vpid_range(jobid, &vpid_start, &vpid_range))) {
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }

    /* for each application context - launch across the first n nodes required */
    for(item =  opal_list_get_first(&mapping);
        item != opal_list_get_end(&mapping);
        item =  opal_list_get_next(item)) {
        orte_rmaps_base_map_t* map = (orte_rmaps_base_map_t*)item;
        rc = orte_pls_bproc_launch_app(jobid, map, vpid_start, vpid_range);
        if(rc != ORTE_SUCCESS) {
            ORTE_ERROR_LOG(rc);
            goto cleanup;
        }
    }

cleanup:
    while(NULL != (item = opal_list_remove_first(&mapping)))
        OBJ_RELEASE(item);
    OBJ_DESTRUCT(&mapping);
    return rc;
}


/**
 * Terminate all processes associated with this job - including
 * daemons.
 */

int orte_pls_bproc_seed_terminate_job(orte_jobid_t jobid)
{
    pid_t* pids;
    pid_t my_pid = getpid();
    size_t i, num_pids;
    int rc;

    /* kill application process */
    if(ORTE_SUCCESS != (rc = orte_pls_base_get_proc_pids(jobid, &pids, &num_pids)))
        return rc;
    for(i=0; i<num_pids; i++) {
        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: killing proc: %d\n", pids[i]);
        }
        kill(pids[i], mca_pls_bproc_seed_component.terminate_sig);
    }
    if(NULL != pids)
        free(pids);

    /* kill daemons */
    if(ORTE_SUCCESS != (rc = orte_pls_base_get_node_pids(jobid, &pids, &num_pids)))
        return rc;
    for(i=0; i<num_pids; i++) {
        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: killing daemon: %d\n", pids[i]);
        }
        if(pids[i] != my_pid) {
            kill(pids[i], mca_pls_bproc_seed_component.terminate_sig);
        }
    }
    if(NULL != pids)
        free(pids);
    return ORTE_SUCCESS;
}


/**
 * Terminate a specific process.
 */
int orte_pls_bproc_seed_terminate_proc(const orte_process_name_t* proc_name)
{
    int rc;
    pid_t pid;
    if(ORTE_SUCCESS != (rc = orte_pls_base_get_proc_pid(proc_name, &pid)))
        return rc;
    if(kill(pid, mca_pls_bproc_seed_component.terminate_sig) != 0) {
        switch(errno) {
            case EINVAL:
                return ORTE_ERR_BAD_PARAM;
            case ESRCH:
                return ORTE_ERR_NOT_FOUND;
            case EPERM:
                return ORTE_ERR_PERM;
            default:
                return ORTE_ERROR;
        }
    }
    return ORTE_SUCCESS;
}

/**
 * Signal all processes associated with this job. Daemons are not included as this function
 * only applies to application processes.
 */

int orte_pls_bproc_seed_signal_job(orte_jobid_t jobid, int32_t signal)
{
    pid_t* pids;
    pid_t my_pid = getpid();
    size_t i, num_pids;
    int rc;

    /** signal application process */
    if(ORTE_SUCCESS != (rc = orte_pls_base_get_proc_pids(jobid, &pids, &num_pids)))
        return rc;
    for(i=0; i<num_pids; i++) {
        if(mca_pls_bproc_seed_component.debug) {
            opal_output(0, "orte_pls_bproc: killing proc: %d\n", pids[i]);
        }
        kill(pids[i], (int)signal);
    }
    if(NULL != pids)
        free(pids);

    return ORTE_SUCCESS;
}


/**
 * Signal a specific process.
 */
int orte_pls_bproc_seed_signal_proc(const orte_process_name_t* proc_name, int32_t signal)
{
    int rc;
    pid_t pid;
    if(ORTE_SUCCESS != (rc = orte_pls_base_get_proc_pid(proc_name, &pid)))
        return rc;
    if(kill(pid, (int)signal) != 0) {
        switch(errno) {
            case EINVAL:
                return ORTE_ERR_BAD_PARAM;
            case ESRCH:
                return ORTE_ERR_NOT_FOUND;
            case EPERM:
                return ORTE_ERR_PERM;
            default:
                return ORTE_ERROR;
        }
    }
    return ORTE_SUCCESS;
}

/**
 * Module cleanup
 */
int orte_pls_bproc_seed_finalize(void)
{
    return ORTE_SUCCESS;
}


/*
 * Handle threading issues.
 */

#if OMPI_HAVE_POSIX_THREADS && OMPI_THREADS_HAVE_DIFFERENT_PIDS

struct orte_pls_bproc_stack_t {
    opal_condition_t cond;
    opal_mutex_t mutex;
    bool complete;
    orte_jobid_t jobid;
    int rc;
};
typedef struct orte_pls_bproc_stack_t orte_pls_bproc_stack_t;

static void orte_pls_bproc_stack_construct(orte_pls_bproc_stack_t* stack)
{
    OBJ_CONSTRUCT(&stack->mutex, opal_mutex_t);
    OBJ_CONSTRUCT(&stack->cond, opal_condition_t);
    stack->rc = 0;
    stack->complete = false;
}

static void orte_pls_bproc_stack_destruct(orte_pls_bproc_stack_t* stack)
{
    OBJ_DESTRUCT(&stack->mutex);
    OBJ_DESTRUCT(&stack->cond);
}

static OBJ_CLASS_INSTANCE(
    orte_pls_bproc_stack_t,
    opal_object_t,
    orte_pls_bproc_stack_construct,
    orte_pls_bproc_stack_destruct);


static void orte_pls_bproc_seed_launch_cb(int fd, short event, void* args)
{
    orte_pls_bproc_stack_t *stack = (orte_pls_bproc_stack_t*)args;
    orte_vpid_t child_vpid;
    orte_process_name_t* child_name;
    char* uri;
    int pid;
    int rc;

    /* setup the daemons process name */
    rc = orte_ns.reserve_range(orte_process_info.my_name->jobid,1,&child_vpid);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        stack->rc = rc;
        goto complete;
    }
    rc = orte_ns.create_process_name(
        &child_name, orte_process_info.my_name->cellid,
        orte_process_info.my_name->jobid,
        child_vpid);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        stack->rc = rc;
        goto complete;
    }
    uri = orte_rml.get_uri();

    /* fork the child */
    pid = fork();
    if(pid < 0) {
        opal_output(0, "orte_pls_bproc: fork failed with errno=%d\n", errno);
        stack->rc = ORTE_ERR_OUT_OF_RESOURCE;

    } else if (pid == 0) {

        pthread_kill_other_threads_np();
        opal_set_using_threads(false);
        if(NULL == orte_process_info.ns_replica) {
            rc = orte_ns.copy_process_name(&orte_process_info.ns_replica,orte_process_info.my_name);
            if(ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
                exit(rc);
            }
            orte_process_info.ns_replica_uri = orte_rml.get_uri();
        }

        if(NULL == orte_process_info.gpr_replica) {
            rc = orte_ns.copy_process_name(&orte_process_info.gpr_replica,orte_process_info.my_name);
            if(ORTE_SUCCESS != rc) {
                ORTE_ERROR_LOG(rc);
                exit(rc);
            }
            orte_process_info.gpr_replica_uri = orte_rml.get_uri();
        }

        /* restart the daemon w/ the new process name */
        rc = orte_restart(child_name, uri);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            exit(rc);
        }

        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            exit(rc);
        }
        rc = orte_pls_bproc_seed_launch(stack->jobid);
        orte_finalize();
        exit(rc);

    } else {

        OPAL_THREAD_LOCK(&mca_pls_bproc_seed_component.lock);
        mca_pls_bproc_seed_component.num_children++;
        OPAL_THREAD_UNLOCK(&mca_pls_bproc_seed_component.lock);
        orte_wait_cb(pid, orte_pls_bproc_wait_proc, NULL);

        stack->rc = ORTE_SUCCESS;
    }

complete:
    OPAL_THREAD_LOCK(&stack->mutex);
    stack->complete = true;
    opal_condition_signal(&stack->cond);
    OPAL_THREAD_UNLOCK(&stack->mutex);
}

int orte_pls_bproc_seed_launch_threaded(orte_jobid_t jobid)
{
    struct timeval tv = { 0, 0 };
    struct opal_event event;
    struct orte_pls_bproc_stack_t stack;

    OBJ_CONSTRUCT(&stack, orte_pls_bproc_stack_t);

    stack.jobid = jobid;
    opal_evtimer_set(&event, orte_pls_bproc_seed_launch_cb, &stack);
    opal_evtimer_add(&event, &tv);

    OPAL_THREAD_LOCK(&stack.mutex);
    while(stack.complete == false)
         opal_condition_wait(&stack.cond, &stack.mutex);
    OPAL_THREAD_UNLOCK(&stack.mutex);
    OBJ_DESTRUCT(&stack);
    return stack.rc;
}

#endif


