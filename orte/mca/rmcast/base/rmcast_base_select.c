/*
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"

#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/rmcast/base/base.h"


/*
 * Select one RMCAST component from all those that are available.
 */
int orte_rmcast_base_select(void)
{
    mca_base_component_t *best_component = NULL;
    orte_rmcast_module_t *best_module = NULL;
    int rc;

    /*
     * Select the best component
     */
    if( OPAL_SUCCESS != mca_base_select("rmcast", orte_rmcast_base.rmcast_output,
                                        &orte_rmcast_base.rmcast_opened,
                                        (mca_base_module_t **) &best_module,
                                        (mca_base_component_t **) &best_component) ) {
        /* This will only happen if no component was selected */
        return ORTE_ERR_NOT_FOUND;
    }

    orte_rmcast = *best_module;
    
    /* init the selected module */
    if (NULL != orte_rmcast.init) {
        if (ORTE_SUCCESS != (rc = orte_rmcast.init())) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    return ORTE_SUCCESS;
}
