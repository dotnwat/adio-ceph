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

#include <string.h>

#include "include/orte_constants.h"
#include "opal/util/output.h"

#include "mca/errmgr/errmgr.h"
#include "mca/soh/soh_types.h"
#include "mca/gpr/gpr.h"
#include "mca/ns/ns.h"
#include "mca/ras/base/ras_base_node.h"

static void orte_ras_base_node_construct(orte_ras_node_t* node)
{
    node->node_name = NULL;
    node->node_arch = NULL;
    node->node_cellid = 0;
    node->node_state = ORTE_NODE_STATE_UNKNOWN;
    node->node_slots = 0;
    node->node_slots_inuse = 0;
    node->node_slots_alloc = 0;
    node->node_slots_max = 0;
    node->node_username = NULL;
}

static void orte_ras_base_node_destruct(orte_ras_node_t* node)
{
    if (NULL != node->node_name) {
        free(node->node_name);
    }
    if (NULL != node->node_arch) {
        free(node->node_arch);
    }
    if (NULL != node->node_username) {
        free(node->node_username);
    }
}


OBJ_CLASS_INSTANCE(
    orte_ras_node_t,
    opal_list_item_t,
    orte_ras_base_node_construct,
    orte_ras_base_node_destruct);


/*
 * Query the registry for all available nodes 
 */

int orte_ras_base_node_query(opal_list_t* nodes)
{
    size_t i, cnt;
    orte_gpr_value_t** values;
    int rc;
    
    /* query all node entries */
    rc = orte_gpr.get(
        ORTE_GPR_KEYS_OR|ORTE_GPR_TOKENS_OR,
        ORTE_NODE_SEGMENT,
        NULL,
        NULL,
        &cnt,
        &values);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* parse the response */
    for(i=0; i<cnt; i++) {
        orte_gpr_value_t* value = values[i];
        orte_ras_node_t* node = OBJ_NEW(orte_ras_node_t);
        size_t k;

        for(k=0; k<value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            if(strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                node->node_name = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_ARCH_KEY) == 0) {
                node->node_arch = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_STATE_KEY) == 0) {
                node->node_state = keyval->value.node_state;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_KEY) == 0) {
                node->node_slots = keyval->value.size;
                continue;
            }
            if(strncmp(keyval->key, ORTE_NODE_SLOTS_ALLOC_KEY, strlen(ORTE_NODE_SLOTS_ALLOC_KEY)) == 0) {
                node->node_slots_inuse += keyval->value.size;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_MAX_KEY) == 0) {
                node->node_slots_max = keyval->value.size;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_USERNAME_KEY) == 0) {
                node->node_username = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_CELLID_KEY) == 0) {
                node->node_cellid = keyval->value.cellid;
                continue;
            }
        }
        opal_list_append(nodes, &node->super);
        OBJ_RELEASE(value);
    }
    if (NULL != values) free(values);
    
    return ORTE_SUCCESS;
}

/*
 * Query the registry for all nodes allocated to a specified job
 */
int orte_ras_base_node_query_alloc(opal_list_t* nodes, orte_jobid_t jobid)
{
    char* keys[] = {
        ORTE_NODE_NAME_KEY,
        ORTE_NODE_ARCH_KEY,
        ORTE_NODE_STATE_KEY,
        ORTE_NODE_SLOTS_KEY,
        ORTE_NODE_SLOTS_ALLOC_KEY,
        ORTE_NODE_SLOTS_MAX_KEY,
        ORTE_NODE_USERNAME_KEY,
        ORTE_CELLID_KEY,
        NULL
    };
    size_t i, cnt, keys_len;
    orte_gpr_value_t** values;
    char* jobid_str;
    int rc;

    if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_str, jobid))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    asprintf(&keys[4], "%s-%s", ORTE_NODE_SLOTS_ALLOC_KEY, jobid_str);
    keys_len = strlen(keys[4]);
    free(jobid_str);

    /* query selected node entries */
    rc = orte_gpr.get(
        ORTE_GPR_KEYS_OR|ORTE_GPR_TOKENS_OR,
        ORTE_NODE_SEGMENT,
        NULL,
        keys,
        &cnt,
        &values);
    if(ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* parse the response */
    for(i=0; i<cnt; i++) {
        orte_gpr_value_t* value = values[i];
        orte_ras_node_t* node;
        size_t k;
        bool found;

        found = false;
        for (k = 0; k < value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];

            if(0 == strcmp(keyval->key, keys[4])) {
                found = true;
                break;
            }
        }
        if (!found)
            continue;

        node = OBJ_NEW(orte_ras_node_t);

        for(k=0; k < value->cnt; k++) {
            orte_gpr_keyval_t* keyval = value->keyvals[k];
            if(strcmp(keyval->key, ORTE_NODE_NAME_KEY) == 0) {
                node->node_name = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_ARCH_KEY) == 0) {
                node->node_arch = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_STATE_KEY) == 0) {
                node->node_state = keyval->value.node_state;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_KEY) == 0) {
                node->node_slots = keyval->value.size;
                continue;
            }
            if(strncmp(keyval->key, keys[4], keys_len) == 0) {
                node->node_slots_inuse += keyval->value.size;
                node->node_slots_alloc += keyval->value.size;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_SLOTS_MAX_KEY) == 0) {
                node->node_slots_max = keyval->value.size;
                continue;
            }
            if(strcmp(keyval->key, ORTE_NODE_USERNAME_KEY) == 0) {
                node->node_username = strdup(keyval->value.strptr);
                continue;
            }
            if(strcmp(keyval->key, ORTE_CELLID_KEY) == 0) {
                node->node_cellid = keyval->value.cellid;
                continue;
            }
        }
        /* in case we get back more than we asked for */
        if(node->node_slots_inuse == 0) {
            OBJ_RELEASE(node);
            continue;
        }
        opal_list_append(nodes, &node->super);
        OBJ_RELEASE(value);
    }

    free (keys[4]);
    if (NULL != values) free(values);
    return ORTE_SUCCESS;
}

/*
 * Add the specified node definitions to the registry
 */
int orte_ras_base_node_insert(opal_list_t* nodes)
{
    opal_list_item_t* item;
    orte_gpr_value_t **values;
    int rc;
    size_t num_values, i, j;
    orte_ras_node_t* node;
    
    num_values = opal_list_get_size(nodes);
    if (0 >= num_values) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    values = (orte_gpr_value_t**)malloc(num_values * sizeof(orte_gpr_value_t*));
    if (NULL == values) {
       ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
       return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    for (i=0; i < num_values; i++) {
        orte_gpr_value_t* value = values[i] = OBJ_NEW(orte_gpr_value_t);
        if (NULL == value) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        value->addr_mode = ORTE_GPR_OVERWRITE | ORTE_GPR_TOKENS_AND;
        value->segment = strdup(ORTE_NODE_SEGMENT);
        value->cnt = 7;             /* Seven, at max (including node_username) */
        value->keyvals = (orte_gpr_keyval_t**)malloc(value->cnt*sizeof(orte_gpr_keyval_t*));
        if (NULL == value->keyvals) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        for (j=0; j < value->cnt; j++) {
            value->keyvals[j] = OBJ_NEW(orte_gpr_keyval_t);
            if (NULL == value->keyvals[j]) {
                for (j=0; j <= i; j++) {
                    OBJ_RELEASE(values[j]);
                }
                free(values);
                ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                return ORTE_ERR_OUT_OF_RESOURCE;
            }
        }
    }
    
    for(i=0, item =  opal_list_get_first(nodes);
        i < num_values && item != opal_list_get_end(nodes);
        i++, item =  opal_list_get_next(item)) {
        orte_gpr_value_t* value = values[i];
        node = (orte_ras_node_t*)item;

        j = 0;
        (value->keyvals[j])->key = strdup(ORTE_NODE_NAME_KEY);
        (value->keyvals[j])->type = ORTE_STRING;
        (value->keyvals[j])->value.strptr = strdup(node->node_name);
        
        ++j;
        (value->keyvals[j])->key = strdup(ORTE_NODE_ARCH_KEY);
        (value->keyvals[j])->type = ORTE_STRING;
        if (NULL != node->node_arch) {
            (value->keyvals[j])->value.strptr = strdup(node->node_arch);
        } else {
            (value->keyvals[j])->value.strptr = strdup("");
        }
        
        ++j;
        (value->keyvals[j])->key = strdup(ORTE_NODE_STATE_KEY);
        (value->keyvals[j])->type = ORTE_NODE_STATE;
        (value->keyvals[j])->value.node_state = node->node_state;
        
        ++j;
        (value->keyvals[j])->key = strdup(ORTE_CELLID_KEY);
        (value->keyvals[j])->type = ORTE_CELLID;
        (value->keyvals[j])->value.cellid = node->node_cellid;
        
        ++j;
        (value->keyvals[j])->key = strdup(ORTE_NODE_SLOTS_KEY);
        (value->keyvals[j])->type = ORTE_SIZE;
        (value->keyvals[j])->value.size = node->node_slots;
        
        ++j;
        (value->keyvals[j])->key = strdup(ORTE_NODE_SLOTS_MAX_KEY);
        (value->keyvals[j])->type = ORTE_SIZE;
        (value->keyvals[j])->value.size = node->node_slots_max;

        ++j;
        (value->keyvals[j])->key = strdup(ORTE_NODE_USERNAME_KEY);
        (value->keyvals[j])->type = ORTE_STRING;
        if (NULL != node->node_username) {
            (value->keyvals[j])->value.strptr = strdup(node->node_username);
        } else {
            (value->keyvals[j])->value.strptr = strdup("");
        }

        /* setup index/keys for this node */
        rc = orte_schema.get_node_tokens(&value->tokens, &value->num_tokens, node->node_cellid, node->node_name);
        if (ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            for (j=0; j <= i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            return rc;
        }
    }
    
    /* try the insert */
    if (ORTE_SUCCESS != (rc = orte_gpr.put(num_values, values))) {
        ORTE_ERROR_LOG(rc);
    }

    for (j=0; j < num_values; j++) {
          OBJ_RELEASE(values[j]);
    }
    if (NULL != values) free(values);
    return rc;
}

/*
 * Delete the specified nodes from the registry
 */
int orte_ras_base_node_delete(opal_list_t* nodes)
{
    opal_list_item_t* item;
    int rc;
    size_t i, num_values, num_tokens;
    orte_ras_node_t* node;
    char** tokens;

    num_values = opal_list_get_size(nodes);
    if (0 >= num_values) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }

    for(item =  opal_list_get_first(nodes);
        item != opal_list_get_end(nodes);
        item =  opal_list_get_next(item)) {
        node = (orte_ras_node_t*)item;

        /* setup index/keys for this node */
        rc = orte_schema.get_node_tokens(&tokens, &num_tokens, node->node_cellid, node->node_name);
        if (ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        rc = orte_gpr.delete_entries(
            ORTE_GPR_TOKENS_AND,
            ORTE_NODE_SEGMENT,
            tokens,
            NULL);
        if(ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        for (i=0; i < num_tokens; i++) {
            free(tokens[i]);
            tokens[i] = NULL;
        }
        if (NULL != tokens) free(tokens);
    }
    return ORTE_SUCCESS;
}

/*
 * Assign the allocated slots on the specified nodes to the  
 * indicated jobid.
 */
int orte_ras_base_node_assign(opal_list_t* nodes, orte_jobid_t jobid)
{
    opal_list_item_t* item;
    orte_gpr_value_t **values;
    int rc;
    size_t num_values, i, j;
    orte_ras_node_t* node;
    char* jobid_str;
    
    num_values = opal_list_get_size(nodes);
    if (0 >= num_values) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    values = (orte_gpr_value_t**)malloc(num_values * sizeof(orte_gpr_value_t*));
    if (NULL == values) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    for (i=0; i < num_values; i++) {
        values[i] = OBJ_NEW(orte_gpr_value_t);
        if (NULL == values[i]) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->addr_mode = ORTE_GPR_OVERWRITE | ORTE_GPR_TOKENS_AND;
        values[i]->segment = strdup(ORTE_NODE_SEGMENT);
        values[i]->cnt = 1;
        values[i]->keyvals = (orte_gpr_keyval_t**)malloc(sizeof(orte_gpr_keyval_t*));
        if (NULL == values[i]->keyvals) {
            for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        
        values[i]->keyvals[0] = OBJ_NEW(orte_gpr_keyval_t);
        if (NULL == values[i]->keyvals[0]) {
           for (j=0; j < i; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
    }
    
    for(i=0, item =  opal_list_get_first(nodes);
        i < num_values && item != opal_list_get_end(nodes);
        i++, item = opal_list_get_next(item)) {
        int rc;
        node = (orte_ras_node_t*)item;

        if(node->node_slots_alloc == 0)
            continue;
        if(ORTE_SUCCESS != (rc = orte_ns.convert_jobid_to_string(&jobid_str, jobid))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }

        /* setup index/keys for this node */
        rc = orte_schema.get_node_tokens(&values[i]->tokens, &values[i]->num_tokens, node->node_cellid, node->node_name);
        if (ORTE_SUCCESS != rc) {
            ORTE_ERROR_LOG(rc);
           for (j=0; j < num_values; j++) {
                OBJ_RELEASE(values[j]);
            }
            free(values);
            return rc;
        }

        /* setup node key/value pairs */
        asprintf(&((values[i]->keyvals[0])->key), "%s-%s", ORTE_NODE_SLOTS_ALLOC_KEY, jobid_str);
        free(jobid_str);
        
        (values[i]->keyvals[0])->type = ORTE_SIZE; 
        (values[i]->keyvals[0])->value.size = node->node_slots_alloc;
    }
    
    /* try the insert */
    if (ORTE_SUCCESS != (rc = orte_gpr.put(num_values, values))) {
        ORTE_ERROR_LOG(rc);
    }
    
    for (j=0; j < num_values; j++) {
        OBJ_RELEASE(values[j]);
    }
    if (NULL != values) free(values);

    return rc;
}

