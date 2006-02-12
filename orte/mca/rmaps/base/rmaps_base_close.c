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
 */

#include "orte_config.h"

#include <stdio.h>

#include "orte/orte_constants.h"
#include "opal/util/output.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"

#include "orte/mca/rmaps/base/base.h"


int orte_rmaps_base_finalize(void)
{
    opal_list_item_t* item;

    /* Finalize all available modules */

    while (NULL != 
           (item = opal_list_remove_first(&orte_rmaps_base.rmaps_available))) {
        orte_rmaps_base_cmp_t* cmp = (orte_rmaps_base_cmp_t*) item;
        opal_output(orte_rmaps_base.rmaps_output,
                    "orte:base:close: finalizing module %s",
                    cmp->component->rmaps_version.mca_component_name);
        if (NULL != cmp->module->finalize) {
            cmp->module->finalize();
        }
        OBJ_RELEASE(cmp);
    }
    return ORTE_SUCCESS;
}

int orte_rmaps_base_close(void)
{
    /* Close all remaining open components */

    mca_base_components_close(orte_rmaps_base.rmaps_output, 
                              &orte_rmaps_base.rmaps_opened, NULL);

    return ORTE_SUCCESS;
}

