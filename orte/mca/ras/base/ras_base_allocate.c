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

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/util/output.h"
#include "orte/include/orte_constants.h"
#include "orte/mca/ras/base/base.h"
#include "orte/mca/ras/base/ras_base_node.h"
#include "orte/mca/errmgr/errmgr.h"


/*
 * Function for selecting one component from all those that are
 * available.
 */
int orte_ras_base_allocate(orte_jobid_t jobid,
                           orte_ras_base_module_t **module)
{
    int ret;
    opal_list_item_t *item;
    orte_ras_base_cmp_t *cmp;

    /* If the list is empty, return NULL */

    if (opal_list_is_empty(&orte_ras_base.ras_available)) {
        opal_output(orte_ras_base.ras_output,
                    "orte:ras:base:select: no components available!");
        ret = ORTE_ERR_NOT_FOUND;
        ORTE_ERROR_LOG(ret);
        return ret;
    }

    /* Otherwise, go through the [already sorted in priority order]
       list and initialize them until one of them puts something on
       the node segment */

    for (item = opal_list_get_first(&orte_ras_base.ras_available);
         item != opal_list_get_end(&orte_ras_base.ras_available);
         item = opal_list_get_next(item)) {
        cmp = (orte_ras_base_cmp_t *) item;
        opal_output(orte_ras_base.ras_output,
                    "orte:ras:base:allocate: attemping to allocate using module: %s",
                    cmp->component->ras_version.mca_component_name);

        if (NULL != cmp->module->allocate) {
            ret = cmp->module->allocate(jobid);
            if (ORTE_SUCCESS == ret) {
                bool empty;

                if (ORTE_SUCCESS != 
                    (ret = orte_ras_base_node_segment_empty(&empty))) {
                    ORTE_ERROR_LOG(ret);
                    return ret;
                }

                /* If this module put something on the node segment,
                   we're done */

                if (!empty) {
                    opal_output(orte_ras_base.ras_output,
                                "orte:ras:base:allocate: found good module: %s",
                                cmp->component->ras_version.mca_component_name);
                    *module = cmp->module;
                    return ORTE_SUCCESS;
                }
            }
        }
    }

    /* We didn't find anyone who put anything on the node segment */

    opal_output(orte_ras_base.ras_output,
                "orte:ras:base:allocate: no module put anything in the node segment");
    ret = ORTE_ERR_NOT_FOUND;
    ORTE_ERROR_LOG(ret);
    return ret;
}
