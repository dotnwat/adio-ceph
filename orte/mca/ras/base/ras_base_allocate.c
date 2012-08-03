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
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#include "orte/constants.h"
#include "orte/types.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/class/opal_list.h"
#include "opal/util/output.h"
#include "opal/dss/dss.h"

#include "orte/util/show_help.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"
#include "orte/util/hostfile/hostfile.h"
#include "orte/util/dash_host/dash_host.h"
#include "orte/util/proc_info.h"
#include "orte/util/comm/comm.h"
#include "orte/mca/state/state.h"
#include "orte/runtime/orte_quit.h"

#include "orte/mca/ras/base/ras_private.h"

/* static function to display allocation */
static void display_alloc(void)
{
    char *tmp=NULL, *tmp2, *tmp3, *pfx=NULL;
    int i;
    orte_node_t *alloc;
    
    if (orte_xml_output) {
        asprintf(&tmp, "<allocation>\n");
        pfx = "\t";
    } else {
        asprintf(&tmp, "\n======================   ALLOCATED NODES   ======================\n");
    }
    for (i=0; i < orte_node_pool->size; i++) {
        if (NULL == (alloc = (orte_node_t*)opal_pointer_array_get_item(orte_node_pool, i))) {
            continue;
        }
        opal_dss.print(&tmp2, pfx, alloc, ORTE_NODE);
        if (NULL == tmp) {
            tmp = tmp2;
        } else {
            asprintf(&tmp3, "%s%s", tmp, tmp2);
            free(tmp);
            free(tmp2);
            tmp = tmp3;
        }
    }
    if (orte_xml_output) {
        fprintf(orte_xml_fp, "%s</allocation>\n", tmp);
        fflush(orte_xml_fp);
    } else {
        opal_output(orte_clean_output, "%s\n\n=================================================================\n", tmp);
    }
    free(tmp);    
}

/*
 * Function for selecting one component from all those that are
 * available.
 */
void orte_ras_base_allocate(int fd, short args, void *cbdata)
{
    int rc;
    orte_job_t *jdata;
    opal_list_t nodes;
    orte_node_t *node;
    orte_std_cntr_t i;
    orte_app_context_t *app;
    orte_state_caddy_t *caddy = (orte_state_caddy_t*)cbdata;

    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:allocate",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    /* convenience */
    jdata = caddy->jdata;

    /* if we already did this, don't do it again - the pool of
     * global resources is set. 
     */
    if (orte_ras_base.allocation_read) {
        
        OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                             "%s ras:base:allocate allocation already read",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
        goto next_state;
    }
    orte_ras_base.allocation_read = true;

    /* Otherwise, we have to create
     * the initial set of resources that will delineate all
     * further operations serviced by this HNP. This list will
     * contain ALL nodes that can be used by any subsequent job.
     *
     * In other words, if a node isn't found in this step, then
     * no job launched by this HNP will be able to utilize it.
     */
    
    /* construct a list to hold the results */
    OBJ_CONSTRUCT(&nodes, opal_list_t);

    /* if a component was selected, then we know we are in a managed
     * environment.  - the active module will return a list of what it found
     */
    if (NULL != orte_ras_base.active_module)  {
        /* read the allocation */
        if (ORTE_SUCCESS != (rc = orte_ras_base.active_module->allocate(&nodes))) {
            if (ORTE_ERR_SYSTEM_WILL_BOOTSTRAP == rc) {
                /* this module indicates that nodes will be discovered
                 * on a bootstrap basis, so all we do here is add our
                 * own node to the list
                 */
                goto addlocal;
            }
            ORTE_ERROR_LOG(rc);
            OBJ_DESTRUCT(&nodes);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
    } 
    /* If something came back, save it and we are done */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
            OBJ_DESTRUCT(&nodes);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
        OBJ_DESTRUCT(&nodes);
        /* default to no-oversubscribe-allowed for managed systems */
        if (!(ORTE_MAPPING_SUBSCRIBE_GIVEN & ORTE_GET_MAPPING_DIRECTIVE(orte_rmaps_base.mapping))) {
            ORTE_SET_MAPPING_DIRECTIVE(orte_rmaps_base.mapping, ORTE_MAPPING_NO_OVERSUBSCRIBE);
        }
        goto DISPLAY;
    } else if (orte_allocation_required) {
        /* if nothing was found, and an allocation is
         * required, then error out
         */
        OBJ_DESTRUCT(&nodes);
        orte_show_help("help-ras-base.txt", "ras-base:no-allocation", true);
        ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
        OBJ_RELEASE(caddy);
        return;
    }
    
    
    
    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:allocate nothing found in module - proceeding to hostfile",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    /* nothing was found, or no active module was alive. Our next
     * option is to look for a hostfile and assign our global
     * pool from there. First, we check for a default hostfile
     * as set by an mca param.
     *
     * Note that any relative node syntax found in the hostfile will
     * generate an error in this scenario, so only non-relative syntax
     * can be present
     */
    if (NULL != orte_default_hostfile) {
        OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                             "%s ras:base:allocate parsing default hostfile %s",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             orte_default_hostfile));
        
        /* a default hostfile was provided - parse it */
        if (ORTE_SUCCESS != (rc = orte_util_add_hostfile_nodes(&nodes,
                                                               orte_default_hostfile))) {
            ORTE_ERROR_LOG(rc);
            OBJ_DESTRUCT(&nodes);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
    }
    /* if something was found in the default hostfile, we use that as our global
     * pool - set it and we are done
     */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
        /* cleanup */
        OBJ_DESTRUCT(&nodes);
        goto DISPLAY;
    }
    
    /* Individual hostfile names, if given, are included
     * in the app_contexts for this job. We therefore need to
     * retrieve the app_contexts for the job, and then cycle
     * through them to see if anything is there. The parser will
     * add the nodes found in each hostfile to our list - i.e.,
     * the resulting list contains the UNION of all nodes specified
     * in hostfiles from across all app_contexts
     *
     * Note that any relative node syntax found in the hostfiles will
     * generate an error in this scenario, so only non-relative syntax
     * can be present
     */
    
    for (i=0; i < jdata->apps->size; i++) {
        if (NULL == (app = (orte_app_context_t*)opal_pointer_array_get_item(jdata->apps, i))) {
            continue;
        }
        if (NULL != app->hostfile) {
            OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                                 "%s ras:base:allocate checking hostfile %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 app->hostfile));
            
            /* hostfile was specified - parse it and add it to the list */
            if (ORTE_SUCCESS != (rc = orte_util_add_hostfile_nodes(&nodes,
                                                                   app->hostfile))) {
                ORTE_ERROR_LOG(rc);
                OBJ_DESTRUCT(&nodes);
                /* set an error event */
                ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
                OBJ_RELEASE(caddy);
                return;
            }
        }
    }

    /* if something was found in the hostfile(s), we use that as our global
     * pool - set it and we are done
     */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
        /* cleanup */
        OBJ_DESTRUCT(&nodes);
        goto DISPLAY;
    }
    
    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:allocate nothing found in hostfiles - checking dash-host options",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    /* Our next option is to look for hosts provided via the -host
     * command line option. If they are present, we declare this
     * to represent not just a mapping, but to define the global
     * resource pool in the absence of any other info.
     *
     * -host lists are provided as part of the app_contexts for
     * this job. We therefore need to retrieve the app_contexts
     * for the job, and then cycle through them to see if anything
     * is there. The parser will add the -host nodes to our list - i.e.,
     * the resulting list contains the UNION of all nodes specified
     * by -host across all app_contexts
     *
     * Note that any relative node syntax found in the -host lists will
     * generate an error in this scenario, so only non-relative syntax
     * can be present
     */
    for (i=0; i < jdata->apps->size; i++) {
        if (NULL == (app = (orte_app_context_t*)opal_pointer_array_get_item(jdata->apps, i))) {
            continue;
        }
        if (NULL != app->dash_host) {
            if (ORTE_SUCCESS != (rc = orte_util_add_dash_host_nodes(&nodes,
                                                                    app->dash_host))) {
                ORTE_ERROR_LOG(rc);
                OBJ_DESTRUCT(&nodes);
                ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
                OBJ_RELEASE(caddy);
                return;
            }
        }
    }

    /* if something was found in -host, we use that as our global
     * pool - set it and we are done
     */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
        /* cleanup */
        OBJ_DESTRUCT(&nodes);
        goto DISPLAY;
    }

    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:allocate nothing found in dash-host - checking for rankfile",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
    /* Our next option is to look for a rankfile - if one was provided, we
     * will use its nodes to create a default allocation pool
     */
    if (NULL != orte_rankfile) {
        /* check the rankfile for node information */
        if (ORTE_SUCCESS != (rc = orte_util_add_hostfile_nodes(&nodes,
                                                               orte_rankfile))) {
            ORTE_ERROR_LOG(rc);
            OBJ_DESTRUCT(&nodes);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return ;
        }
    }
    /* if something was found in rankfile, we use that as our global
     * pool - set it and we are done
     */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
            return;
        }
        /* rankfile is considered equivalent to an RM allocation */
        if (!(ORTE_MAPPING_SUBSCRIBE_GIVEN & ORTE_GET_MAPPING_DIRECTIVE(orte_rmaps_base.mapping))) {
            ORTE_SET_MAPPING_DIRECTIVE(orte_rmaps_base.mapping, ORTE_MAPPING_NO_OVERSUBSCRIBE);
        }
        /* cleanup */
        OBJ_DESTRUCT(&nodes);
        goto DISPLAY;
    }
    
    
    OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                         "%s ras:base:allocate nothing found in rankfile - inserting current node",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    
 addlocal:
    /* if nothing was found by any of the above methods, then we have no
     * earthly idea what to do - so just add the local host
     */
    node = OBJ_NEW(orte_node_t);
    if (NULL == node) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_DESTRUCT(&nodes);
        ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
        OBJ_RELEASE(caddy);
        return;
    }
    /* use the same name we got in orte_process_info so we avoid confusion in
     * the session directories
     */
    node->name = strdup(orte_process_info.nodename);
    node->state = ORTE_NODE_STATE_UP;
    node->slots_inuse = 0;
    node->slots_max = 0;
    node->slots = 1;
    opal_list_append(&nodes, &node->super);
    
    /* store the results in the global resource pool - this removes the
     * list items
     */
    if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
        ORTE_ERROR_LOG(rc);
        OBJ_DESTRUCT(&nodes);
        ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
        OBJ_RELEASE(caddy);
        return;
    }
    OBJ_DESTRUCT(&nodes);

 DISPLAY:
    /* shall we display the results? */
    if (4 < opal_output_get_verbosity(orte_ras_base.ras_output) || orte_ras_base.display_alloc) {
        display_alloc();
    }

 next_state:
    /* are we to report this event? */
    if (orte_report_events) {
        if (ORTE_SUCCESS != (rc = orte_util_comm_report_event(ORTE_COMM_EVENT_ALLOCATE))) {
            ORTE_ERROR_LOG(rc);
            ORTE_TERMINATE(ORTE_ERROR_DEFAULT_EXIT_CODE);
            OBJ_RELEASE(caddy);
        }
    }
    
    /* set total slots alloc */
    jdata->total_slots_alloc = orte_ras_base.total_slots_alloc;

    /* set the job state to the next position */
    ORTE_ACTIVATE_JOB_STATE(jdata, ORTE_JOB_STATE_ALLOCATION_COMPLETE);

    /* cleanup */
    OBJ_RELEASE(caddy);
}

int orte_ras_base_add_hosts(orte_job_t *jdata)
{
    int rc;
    opal_list_t nodes;
    int i;
    orte_app_context_t *app;

    /* construct a list to hold the results */
    OBJ_CONSTRUCT(&nodes, opal_list_t);
    
    /* Individual add-hostfile names, if given, are included
     * in the app_contexts for this job. We therefore need to
     * retrieve the app_contexts for the job, and then cycle
     * through them to see if anything is there. The parser will
     * add the nodes found in each add-hostfile to our list - i.e.,
     * the resulting list contains the UNION of all nodes specified
     * in add-hostfiles from across all app_contexts
     *
     * Note that any relative node syntax found in the add-hostfiles will
     * generate an error in this scenario, so only non-relative syntax
     * can be present
     */
    
    for (i=0; i < jdata->apps->size; i++) {
        if (NULL == (app = (orte_app_context_t*)opal_pointer_array_get_item(jdata->apps, i))) {
            continue;
        }
        if (NULL != app->add_hostfile) {
            OPAL_OUTPUT_VERBOSE((5, orte_ras_base.ras_output,
                                 "%s ras:base:add_hosts checking add-hostfile %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 app->add_hostfile));
            
            /* hostfile was specified - parse it and add it to the list */
            if (ORTE_SUCCESS != (rc = orte_util_add_hostfile_nodes(&nodes,
                                                                   app->add_hostfile))) {
                ORTE_ERROR_LOG(rc);
                OBJ_DESTRUCT(&nodes);
                return rc;
            }
        }
    }

    /* We next check for and add any add-host options. Note this is
     * a -little- different than dash-host in that (a) we add these
     * nodes to the global pool regardless of what may already be there,
     * and (b) as a result, any job and/or app_context can access them.
     *
     * Note that any relative node syntax found in the add-host lists will
     * generate an error in this scenario, so only non-relative syntax
     * can be present
     */
    for (i=0; i < jdata->apps->size; i++) {
        if (NULL == (app = (orte_app_context_t*)opal_pointer_array_get_item(jdata->apps, i))) {
            continue;
        }
        if (NULL != app->add_host) {
            if (ORTE_SUCCESS != (rc = orte_util_add_dash_host_nodes(&nodes,
                                                                    app->add_host))) {
                ORTE_ERROR_LOG(rc);
                OBJ_DESTRUCT(&nodes);
                return rc;
            }
        }
    }
    
    /* if something was found, we add that to our global pool */
    if (!opal_list_is_empty(&nodes)) {
        /* store the results in the global resource pool - this removes the
         * list items
         */
        if (ORTE_SUCCESS != (rc = orte_ras_base_node_insert(&nodes, jdata))) {
            ORTE_ERROR_LOG(rc);
        }
        /* cleanup */
        OBJ_DESTRUCT(&nodes);
    }
    
    /* shall we display the results? */
    if (0 < opal_output_get_verbosity(orte_ras_base.ras_output) || orte_ras_base.display_alloc) {
        display_alloc();
    }
    
    return ORTE_SUCCESS;
}
