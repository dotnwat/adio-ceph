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
#include "include/orte_constants.h"
#include "include/orte_types.h"

#include "mca/ras/base/base.h"
#include "mca/ras/base/ras_base_node.h"
#include "mca/ras/host/ras_host.h"

#if HAVE_STRING_H
#include <string.h>
#endif  /* HAVE_STRING_H */

/**
 *  Discover available (pre-allocated) nodes. Allocate the
 *  requested number of nodes/process slots to the job.
 *  
 */

static int orte_ras_host_allocate(orte_jobid_t jobid)
{
    opal_list_t nodes;
    opal_list_item_t* item;
    int rc;

    OBJ_CONSTRUCT(&nodes, opal_list_t);
    if(ORTE_SUCCESS != (rc = orte_ras_base_node_query(&nodes))) {
        goto cleanup;
    }

    if (0 == strcmp(mca_ras_host_component.schedule_policy, "node")) {
        if (ORTE_SUCCESS != 
            (rc = orte_ras_base_allocate_nodes_by_node(jobid, &nodes))) {
            goto cleanup;
        }
    } else {
        if (ORTE_SUCCESS != 
            (rc = orte_ras_base_allocate_nodes_by_slot(jobid, &nodes))) {
            goto cleanup;
        }
    }

cleanup:
    while(NULL != (item = opal_list_remove_first(&nodes))) {
        OBJ_RELEASE(item);
    }
    OBJ_DESTRUCT(&nodes);
    return rc;
}

static int orte_ras_host_node_insert(opal_list_t *nodes)
{
    return orte_ras_base_node_insert(nodes);
}

static int orte_ras_host_node_query(opal_list_t *nodes)
{
    return orte_ras_base_node_query(nodes);
}

static int orte_ras_host_deallocate(orte_jobid_t jobid)
{
    return ORTE_SUCCESS;
}


static int orte_ras_host_finalize(void)
{
    return ORTE_SUCCESS;
}


orte_ras_base_module_t orte_ras_host_module = {
    orte_ras_host_allocate,
    orte_ras_host_node_insert,
    orte_ras_host_node_query,
    orte_ras_host_deallocate,
    orte_ras_host_finalize
};

