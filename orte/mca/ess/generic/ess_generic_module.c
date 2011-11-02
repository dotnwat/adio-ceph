/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2010-2011 Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <sys/types.h>
#include <stdio.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <stdlib.h>

#include "opal/mca/event/event.h"
#include "opal/runtime/opal.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/util/output.h"
#include "opal/util/malloc.h"
#include "opal/util/argv.h"
#include "opal/util/if.h"

#include "orte/util/show_help.h"
#include "orte/mca/rml/base/base.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/mca/routed/base/base.h"
#include "orte/mca/routed/routed.h"
#include "orte/mca/errmgr/base/base.h"
#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/iof/base/base.h"
#include "orte/mca/ess/base/base.h"
#include "orte/mca/ess/ess.h"
#include "orte/mca/ras/base/base.h"
#include "orte/mca/plm/base/base.h"

#include "orte/mca/rmaps/base/base.h"
#if OPAL_ENABLE_FT_CR == 1
#include "orte/mca/snapc/base/base.h"
#endif
#include "orte/mca/filem/base/base.h"
#include "orte/util/proc_info.h"
#include "orte/util/session_dir.h"
#include "orte/util/name_fns.h"
#include "orte/util/nidmap.h"

#include "orte/runtime/runtime.h"
#include "orte/runtime/orte_wait.h"
#include "orte/runtime/orte_globals.h"

#include "orte/runtime/orte_cr.h"
#include "orte/mca/ess/ess.h"
#include "orte/mca/ess/base/base.h"
#include "orte/mca/ess/generic/ess_generic.h"

static int rte_init(void);
static int rte_finalize(void);

orte_ess_base_module_t orte_ess_generic_module = {
    rte_init,
    rte_finalize,
    orte_ess_base_app_abort,
    orte_ess_base_proc_get_locality,
    orte_ess_base_proc_get_daemon,
    orte_ess_base_proc_get_hostname,
    orte_ess_base_proc_get_local_rank,
    orte_ess_base_proc_get_node_rank,
    orte_ess_base_proc_get_epoch,
    orte_ess_base_update_pidmap,
    orte_ess_base_update_nidmap,
    NULL
};

static int rte_init(void)
{
    int ret;
    char *error = NULL;
    char **nodes = NULL, **ppnlist = NULL;
    char *envar;
    int32_t jobfam;
    int i, j, *ppn;
    orte_nid_t *node;
    orte_jmap_t *jmap;
    orte_pmap_t *pmap;
    orte_vpid_t vpid;
    bool byslot;

    /* run the prolog */
    if (ORTE_SUCCESS != (ret = orte_ess_base_std_prolog())) {
        error = "orte_ess_base_std_prolog";
        goto error;
    }
    
    /* Only application procs can use this module. Since we
     * were directly launched by someone, we need to bootstrap
     * our own global info so we can startup.
     */
    
    /* ensure that static ports were assigned - otherwise, we cant
     * work since we won't know how to talk to anyone else
     */
    if (NULL == getenv("OMPI_MCA_oob_tcp_static_ports") &&
        NULL == getenv("OMPI_MCA_oob_tcp_static_ports_v6")) {
        error = "static ports were not assigned";
        goto error;
    }

    /* declare ourselves to be standalone - i.e., not launched by orted */
    orte_standalone_operation = true;
    
    /* extract a jobid from the environment - can be totally
     * arbitrary. if one isn't provided, just fake it
     */
    if (NULL != (envar = getenv("OMPI_MCA_orte_jobid"))) {
        jobfam = strtol(envar, NULL, 10);
    } else {
        jobfam = 1;
    }
    ORTE_PROC_MY_NAME->jobid = ORTE_CONSTRUCT_LOCAL_JOBID(0, jobfam);
    
    /* extract a rank from the environment */
    if (NULL == (envar = getenv("OMPI_MCA_orte_rank"))) {
        error = "could not get process rank";
        goto error;
    }
    ORTE_PROC_MY_NAME->vpid = strtol(envar, NULL, 10);
    ORTE_EPOCH_SET(ORTE_PROC_MY_NAME->epoch,ORTE_EPOCH_MIN);

    OPAL_OUTPUT_VERBOSE((1, orte_ess_base_output,
                         "%s completed name definition",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    /* get the number of procs in this job */
    if (NULL == (envar = getenv("OMPI_MCA_orte_num_procs"))) {
        error = "could not get number of processes in job";
        goto error;
    }
    orte_process_info.num_procs = strtol(envar, NULL, 10);

    if (orte_process_info.max_procs < orte_process_info.num_procs) {
        orte_process_info.max_procs = orte_process_info.num_procs;
    }
    
    /* set the app_num so that MPI attributes get set correctly */
    orte_process_info.app_num = 1;

    /* get the list of nodes */
    if (NULL == (envar = getenv("OMPI_MCA_orte_nodes"))) {
        error = "could not get list of nodes";
        goto error;
    }
    /* break this down */
    nodes = opal_argv_split(envar, ',');
    orte_process_info.num_nodes = opal_argv_count(nodes);

    /* get the ppn */
    if (NULL == (envar = getenv("OMPI_MCA_orte_ppn"))) {
        error = "could not get ppn";
        goto error;
    }
    ppnlist = opal_argv_split(envar, ',');
    ppn = (int*)malloc(orte_process_info.num_nodes * sizeof(int));
    if (1 == opal_argv_count(ppnlist)) {
        /* constant ppn */
        j = strtol(ppnlist[0], NULL, 10);
        for (i=0; i < orte_process_info.num_nodes; i++) {
            ppn[i] = j;
        }
    } else {
        for (i=0; i < orte_process_info.num_nodes; i++) {
            ppn[i] = strtol(ppnlist[i], NULL, 10);
        }
    }
    opal_argv_free(ppnlist);

    /* get the mapping mode - default to byslot */
    byslot = true;
    if (NULL != (envar = getenv("OMPI_MCA_mapping")) &&
        0 == strcmp(envar, "bynode")) {
        byslot = false;
    }

    /* setup the nidmap arrays */
    if (ORTE_SUCCESS != (ret = orte_util_nidmap_init(NULL))) {
        ORTE_ERROR_LOG(ret);
        error = "orte_util_nidmap_init";
        goto error;
    }
    
    /* set the size of the nidmap storage so we minimize realloc's */
    if (ORTE_SUCCESS != (ret = opal_pointer_array_set_size(&orte_nidmap, orte_process_info.num_nodes))) {
        error = "could not set pointer array size for nidmap";
        goto error;
    }
    
    /* construct the nidmap */
    for (i=0; i < orte_process_info.num_nodes; i++) {
        node = OBJ_NEW(orte_nid_t);
        if (0 == strcmp(nodes[i], orte_process_info.nodename) || opal_ifislocal(nodes[i])) {
            node->name = strdup(orte_process_info.nodename);
        } else {
            node->name = strdup(nodes[i]);
        }
        node->daemon = i;
        node->index = i;
        opal_pointer_array_set_item(&orte_nidmap, i, node);
    }
    opal_argv_free(nodes);

    /* create a job map for this job */
    jmap = OBJ_NEW(orte_jmap_t);
    jmap->job = ORTE_PROC_MY_NAME->jobid;
    opal_pointer_array_add(&orte_jobmap, jmap);
    /* update the num procs */
    jmap->num_procs = orte_process_info.num_procs;
    /* set the size of the pidmap storage so we minimize realloc's */
    if (ORTE_SUCCESS != (ret = opal_pointer_array_set_size(&jmap->pmap, jmap->num_procs))) {
        ORTE_ERROR_LOG(ret);
        error = "could not set pointer array size for pidmap";
        goto error;
    }

    /* construct the pidmap */
    if (byslot) {
        vpid = 0;
        for (i=0; i < orte_process_info.num_nodes; i++) {
            node = (orte_nid_t*)opal_pointer_array_get_item(&orte_nidmap, i);
            /* for each node, cycle through the ppn */
            for (j=0; j < ppn[i]; j++) {
                pmap = OBJ_NEW(orte_pmap_t);
                pmap->node = i;
                pmap->local_rank = j;
                pmap->node_rank = j;
                if (ORTE_SUCCESS != (ret = opal_pointer_array_set_item(&jmap->pmap, vpid, pmap))) {
                    ORTE_ERROR_LOG(ret);
                    error = "could not set pmap values";
                    goto error;
                }
                /* if this is me, then define the daemon's vpid to 
                 * be the node number
                 */
                if (vpid == ORTE_PROC_MY_NAME->vpid) {
                    ORTE_PROC_MY_DAEMON->jobid = 0;
                    ORTE_PROC_MY_DAEMON->vpid = i;
                    ORTE_EPOCH_SET(ORTE_PROC_MY_DAEMON->epoch,ORTE_PROC_MY_NAME->epoch);
                }
                OPAL_OUTPUT_VERBOSE((1, orte_ess_base_output,
                                     "%s node %d name %s rank %s",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     (int) node->index, node->name, ORTE_VPID_PRINT(vpid)));
                vpid++;
            }
        }
    } else {
        /* cycle across the nodes */
        vpid = 0;
        while (vpid < orte_process_info.num_procs) {
            for (i=0; i < orte_process_info.num_nodes && vpid < orte_process_info.num_procs; i++) {
                node = (orte_nid_t*)opal_pointer_array_get_item(&orte_nidmap, i);
                if (0 < ppn[i]) {
                    pmap = OBJ_NEW(orte_pmap_t);
                    pmap->node = i;
                    pmap->local_rank = ppn[i]-1;
                    pmap->node_rank = ppn[i]-1;
                    if (ORTE_SUCCESS != (ret = opal_pointer_array_set_item(&jmap->pmap, vpid, pmap))) {
                        ORTE_ERROR_LOG(ret);
                        error = "could not set pmap values";
                        goto error;
                    }
                    /* if this is me, then define the daemon's vpid to 
                     * be the node number
                     */
                    if (vpid == ORTE_PROC_MY_NAME->vpid) {
                        ORTE_PROC_MY_DAEMON->jobid = 0;
                        ORTE_PROC_MY_DAEMON->vpid = i;
                        ORTE_EPOCH_SET(ORTE_PROC_MY_DAEMON->epoch,ORTE_PROC_MY_NAME->epoch);
                    }
                    OPAL_OUTPUT_VERBOSE((1, orte_ess_base_output,
                                         "%s node %d name %s rank %d",
                                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                         (int) node->index, node->name, (int)vpid));
                    vpid++;
                    --ppn[i];
                }
            }
        }
    }
    free(ppn);

    /* ensure we pick the correct critical components */
    putenv("OMPI_MCA_grpcomm=hier");
    putenv("OMPI_MCA_routed=direct");

    /* use the default procedure to finish my setup */
    if (ORTE_SUCCESS != (ret = orte_ess_base_app_setup())) {
        ORTE_ERROR_LOG(ret);
        error = "orte_ess_base_app_setup";
        goto error;
    }

    if (0 < opal_output_get_verbosity(orte_ess_base_output)) {
        orte_nidmap_dump();
        orte_jobmap_dump();
    }

    return ORTE_SUCCESS;

 error:
    orte_show_help("help-orte-runtime.txt",
                   "orte_init:startup:internal-failure",
                   true, error, ORTE_ERROR_NAME(ret), ret);
    
    return ret;
}

static int rte_finalize(void)
{
    int ret;
    
    /* use the default procedure to finish */
    if (ORTE_SUCCESS != (ret = orte_ess_base_app_finalize())) {
        ORTE_ERROR_LOG(ret);
    }
    
    /* remove the envars that we pushed into environ
     * so we leave that structure intact
     */
    unsetenv("OMPI_MCA_grpcomm");
    unsetenv("OMPI_MCA_routed");

    /* deconstruct my nidmap and jobmap arrays - this
     * function protects itself from being called
     * before things were initialized
     */
    orte_util_nidmap_finalize();

    return ret;    
}
