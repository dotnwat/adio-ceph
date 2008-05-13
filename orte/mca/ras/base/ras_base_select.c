/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
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
#include "orte/constants.h"

#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "orte/util/output.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/util/name_fns.h"
#include "orte/util/proc_info.h"
#include "orte/runtime/orte_globals.h"

#include "orte/mca/ras/base/ras_private.h"
#include "orte/mca/ras/base/base.h"


/*
 * Select one RAS component from all those that are available.
 */
int orte_ras_base_select(void)
{
#ifdef ORTE_WANT_NO_RAS_SUPPORT
    /* some systems require no allocation support - they handle
    * the allocation internally themselves. In those cases, memory
    * footprint is often a consideration. Hence, we provide a means
    * for someone to transparently configure out all RAS support.
    */
    return ORTE_SUCCESS;
    
#else
    /* For all other systems, provide the following support */

    int ret, exit_status = OPAL_SUCCESS;
    orte_ras_base_component_t *best_component = NULL;
    orte_ras_base_module_t *best_module = NULL;

    /*
     * Select the best component
     */
    if( OPAL_SUCCESS != (ret = mca_base_select("ras", orte_ras_base.ras_output,
                                               &orte_ras_base.ras_opened,
                                               (mca_base_module_t **) &best_module,
                                               (mca_base_component_t **) &best_component) ) ) {
        /* This will only happen if no component was selected */
        /* If we didn't find one to select, that is okay */
        exit_status = ORTE_SUCCESS;
        goto cleanup;
    }

    /* Save the winner */
    /* No component saved */
    orte_ras_base.active_module = best_module;

 cleanup:
    return exit_status;
#endif
}
