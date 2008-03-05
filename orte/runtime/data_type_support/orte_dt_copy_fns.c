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
 */

#include "orte_config.h"

#include <sys/types.h>

#include "opal/util/argv.h"

#include "orte/mca/errmgr/errmgr.h"
#include "opal/dss/dss.h"
#include "orte/runtime/data_type_support/orte_dt_support.h"

/* ORTE_STD_CNTR */
int orte_dt_copy_std_cntr(orte_std_cntr_t **dest, orte_std_cntr_t *src, opal_data_type_t type) 
{
    orte_std_cntr_t *val;
    
    val = (orte_std_cntr_t*)malloc(sizeof(orte_std_cntr_t));
    if (NULL == val) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *val = *src;
    *dest = val;
    
    return ORTE_SUCCESS;
}

/* PROCESS NAME */
int orte_dt_copy_name(orte_process_name_t **dest, orte_process_name_t *src, opal_data_type_t type)
{
    orte_process_name_t *val;
    
    val = (orte_process_name_t*)malloc(sizeof(orte_process_name_t));
    if (NULL == val) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    val->jobid = src->jobid;
    val->vpid = src->vpid;
    
    *dest = val;
    return ORTE_SUCCESS;
}

/*
 * JOBID
 */
int orte_dt_copy_jobid(orte_jobid_t **dest, orte_jobid_t *src, opal_data_type_t type)
{
    orte_jobid_t *val;
    
    val = (orte_jobid_t*)malloc(sizeof(orte_jobid_t));
    if (NULL == val) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *val = *src;
    *dest = val;
    
    return ORTE_SUCCESS;
}

/*
 * VPID
 */
int orte_dt_copy_vpid(orte_vpid_t **dest, orte_vpid_t *src, opal_data_type_t type)
{
    orte_vpid_t *val;
    
    val = (orte_vpid_t*)malloc(sizeof(orte_vpid_t));
    if (NULL == val) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *val = *src;
    *dest = val;
    
    return ORTE_SUCCESS;
}

/**
 * JOB
 */
int orte_dt_copy_job(orte_job_t **dest, orte_job_t *src, opal_data_type_t type)
{
    (*dest) = src;
    OBJ_RETAIN(src);
    
#if 0
    orte_std_cntr_t i;
    int rc;
    
    /* create the new object */
    *dest = OBJ_NEW(orte_job_t);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    /* copy data into it */
    (*dest)->jobid = src->jobid;
    
    if (0 < src->num_apps) {
        (*dest)->apps = (orte_app_context_t**)malloc(src->num_apps * sizeof(orte_app_context_t*));
        if (NULL == (*dest)->apps) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        for (i=0; i < src->num_apps; i++) {
            if (ORTE_SUCCESS != (rc = opal_dss.copy((void*)&((*dest)->apps[i]), src->apps[i], ORTE_APP_CONTEXT))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        }
        (*dest)->num_apps = src->num_apps;
    }
    
    (*dest)->total_slots_alloc = src->total_slots_alloc;
    
    (*dest)->num_procs = src->num_procs;
    for (i=0; i < src->procs->size; i++) {
        if (NULL != src->procs->addr[i]) {
            /* need to use pointer_array_set_item here */
            (*dest)->procs->addr[i] = src->procs->addr[i];
            OBJ_RETAIN(src->procs->addr[i]);  /* keep the instance count correct */
        }
    }
    
    (*dest)->map = src->map;
    OBJ_RETAIN(src->map); /* keep the instance count correct */
    
    (*dest)->bookmark = src->bookmark;
    (*dest)->oversubscribe_override = src->oversubscribe_override;
    (*dest)->state = src->state;
    
    (*dest)->num_terminated = src->num_terminated;
    (*dest)->abort = src->abort;
#if OPAL_ENABLE_FT == 1
    (*dest)->ckpt_state = src->ckpt_state;
    if (NULL != src->ckpt_snapshot_ref) {
        (*dest)->ckpt_snapshot_ref = strdup(src->ckpt_snapshot_ref);
    }
    if (NULL != src->ckpt_snapshot_loc) {
        (*dest)->ckpt_snapshot_loc = strdup(src->ckpt_snapshot_loc);
    }
#endif
#endif
    return ORTE_SUCCESS;
}

/**
* NODE
 */
int orte_dt_copy_node(orte_node_t **dest, orte_node_t *src, opal_data_type_t type)
{
    (*dest) = src;
    OBJ_RETAIN(src);
#if 0
    orte_std_cntr_t i;
    
    /* create the new object */
    *dest = OBJ_NEW(orte_node_t);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* copy data into it */
    if (NULL != src->name) {
        (*dest)->name = strdup(src->name);
    }
    (*dest)->nodeid = src->nodeid;
    
    (*dest)->index = src->index;
    if (NULL != src->daemon) {
        (*dest)->daemon = src->daemon;
        OBJ_RETAIN(src->daemon);  /* keep the instance count correct */
    }
    (*dest)->launch_id = src->launch_id;
    
    (*dest)->num_procs = src->num_procs;
    for (i=0; i < src->procs->size; i++) {
        if (NULL != src->procs->addr[i]) {
            (*dest)->procs->addr[i] = src->procs->addr[i];
            OBJ_RETAIN(src->procs->addr[i]);  /* keep the instance count correct */
        }
    }

    (*dest)->oversubscribed = src->oversubscribed;
    (*dest)->arch = src->arch;
    (*dest)->state = src->state;
    (*dest)->slots = src->slots;
    (*dest)->slots_inuse = src->slots_inuse;
    (*dest)->slots_alloc = src->slots_alloc;
    (*dest)->slots_max = src->slots_max;
    if (NULL != src->username) {
        (*dest)->username = strdup(src->username);
    }
    (*dest)->oversubscribe_override = src->oversubscribe_override;
#endif
    return ORTE_SUCCESS;
}

/**
 * PROC
 */
int orte_dt_copy_proc(orte_proc_t **dest, orte_proc_t *src, opal_data_type_t type)
{
    (*dest) = src;
    OBJ_RETAIN(src);
#if 0
    /* create the new object */
    *dest = OBJ_NEW(orte_proc_t);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* copy data into it */
    (*dest)->name = src->name;
    (*dest)->pid = src->pid;
    (*dest)->local_rank = src->local_rank;
    (*dest)->state = src->state;
    (*dest)->app_idx = src->app_idx;
    if (NULL != src->slot_list) {
        (*dest)->slot_list = strdup(src->slot_list);
    }
    
    if (NULL != src->node) {
        (*dest)->node = src->node;
        OBJ_RETAIN(src->node); /* keep the instance count correct */
    }

#if OPAL_ENABLE_FT == 1
    (*dest)->ckpt_state = src->ckpt_state;
    if (NULL != src->ckpt_snapshot_ref) {
        (*dest)->ckpt_snapshot_ref = strdup(src->ckpt_snapshot_ref);
    }
    if (NULL != src->ckpt_snapshot_loc) {
        (*dest)->ckpt_snapshot_loc = strdup(src->ckpt_snapshot_loc);
    }
#endif
#endif
    return ORTE_SUCCESS;
}

/*
 * APP CONTEXT
 */
int orte_dt_copy_app_context(orte_app_context_t **dest, orte_app_context_t *src, opal_data_type_t type)
{
    /* create the new object */
    *dest = OBJ_NEW(orte_app_context_t);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* copy data into it */
    (*dest)->idx = src->idx;
    if (NULL != src->app) {
        (*dest)->app = strdup(src->app);
    }
    (*dest)->num_procs = src->num_procs;
    (*dest)->argv = opal_argv_copy(src->argv);
    (*dest)->env = opal_argv_copy(src->env);
    if (NULL != src->cwd) {
        (*dest)->cwd = strdup(src->cwd);
    }
    (*dest)->user_specified_cwd = src->user_specified_cwd;
    
    if (NULL != src->hostfile) {
        (*dest)->hostfile = strdup(src->hostfile);
    }
    
    if (NULL != src->add_hostfile) {
        (*dest)->add_hostfile = strdup(src->add_hostfile);
    }
    
    (*dest)->num_map = src->num_map;
    (*dest)->preload_binary = src->preload_binary;
    
    if( NULL != src->preload_files)
        (*dest)->preload_files  = strdup(src->preload_files);
    else 
        (*dest)->preload_files = NULL;
    
    if( NULL != src->preload_files_dest_dir)
        (*dest)->preload_files_dest_dir  = strdup(src->preload_files_dest_dir);
    else 
        (*dest)->preload_files_dest_dir = NULL;
   
    (*dest)->dash_host = opal_argv_copy(src->dash_host);
    if (NULL != src->prefix_dir) {
        (*dest)->prefix_dir = strdup(src->prefix_dir);
    }
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_proc_state(orte_proc_state_t **dest, orte_proc_state_t *src, opal_data_type_t type)
{
    orte_proc_state_t *ps;
    
    ps = (orte_proc_state_t*)malloc(sizeof(orte_proc_state_t));
    if (NULL == ps) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *ps = *src;
    *dest = ps;
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_job_state(orte_job_state_t **dest, orte_job_state_t *src, opal_data_type_t type)
{
    orte_job_state_t *ps;
    
    ps = (orte_job_state_t*)malloc(sizeof(orte_job_state_t));
    if (NULL == ps) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *ps = *src;
    *dest = ps;
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_node_state(orte_node_state_t **dest, orte_node_state_t *src, opal_data_type_t type)
{
    orte_node_state_t *ps;
    
    ps = (orte_node_state_t*)malloc(sizeof(orte_node_state_t));
    if (NULL == ps) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *ps = *src;
    *dest = ps;
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_exit_code(orte_exit_code_t **dest, orte_exit_code_t *src, opal_data_type_t type)
{
    orte_exit_code_t *ps;
    
    ps = (orte_exit_code_t*)malloc(sizeof(orte_exit_code_t));
    if (NULL == ps) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    *ps = *src;
    *dest = ps;
    
    return ORTE_SUCCESS;
}

/*
 * JOB_MAP
 */
int orte_dt_copy_map(orte_job_map_t **dest, orte_job_map_t *src, opal_data_type_t type)
{
    orte_std_cntr_t i;
    
    if (NULL == src) {
        *dest = NULL;
        return ORTE_SUCCESS;
    }
    
    /* create the new object */
    *dest = OBJ_NEW(orte_job_map_t);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* copy data into it */
    (*dest)->policy = src->policy;
    (*dest)->no_use_local = src->no_use_local;
    (*dest)->pernode = src->pernode;
    (*dest)->npernode = src->npernode;
    (*dest)->oversubscribe = src->oversubscribe;
    (*dest)->display_map = src->display_map;
    (*dest)->num_new_daemons = src->num_new_daemons;
    (*dest)->daemon_vpid_start = src->daemon_vpid_start;
    (*dest)->num_nodes = src->num_nodes;
    
    /* copy the pointer array - have to do this manually
        * as no dss.copy function is setup for that object
        */
    (*dest)->nodes->lowest_free = src->nodes->lowest_free;
    (*dest)->nodes->number_free = src->nodes->number_free;
    (*dest)->nodes->size = src->nodes->size;
    (*dest)->nodes->max_size = src->nodes->max_size;
    (*dest)->nodes->block_size = src->nodes->block_size;
    for (i=0; i < src->nodes->size; i++) {
        (*dest)->nodes->addr[i] = src->nodes->addr[i];
    }
    
    return ORTE_SUCCESS;
}

/*
 * RML tag
 */
int orte_dt_copy_tag(orte_rml_tag_t **dest, orte_rml_tag_t *src, opal_data_type_t type)
{
    orte_rml_tag_t *tag;
    
    if (NULL == src) {
        *dest = NULL;
        return ORTE_SUCCESS;
    }
    
    /* create the new space */
    tag = (orte_rml_tag_t*)malloc(sizeof(orte_rml_tag_t));
    if (NULL == tag) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    /* copy data into it */
    *tag = *src;
    *dest = tag;
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_daemon_cmd(orte_daemon_cmd_flag_t **dest, orte_daemon_cmd_flag_t *src, opal_data_type_t type)
{
    size_t datasize;
    
    datasize = sizeof(orte_daemon_cmd_flag_t);
    
    *dest = (orte_daemon_cmd_flag_t*)malloc(datasize);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    memcpy(*dest, src, datasize);
    
    return ORTE_SUCCESS;
}

int orte_dt_copy_grpcomm_mode(orte_grpcomm_mode_t **dest, orte_grpcomm_mode_t *src, opal_data_type_t type)
{
    size_t datasize;
    
    datasize = sizeof(orte_grpcomm_mode_t);
    
    *dest = (orte_grpcomm_mode_t*)malloc(datasize);
    if (NULL == *dest) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    memcpy(*dest, src, datasize);
    
    return ORTE_SUCCESS;
}

