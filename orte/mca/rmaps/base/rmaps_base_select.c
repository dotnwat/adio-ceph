/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
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

#include <string.h>

#include "orte/orte_constants.h"
#include "opal/util/output.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "orte/mca/rmaps/base/base.h"


/*
 * Local functions
 */
static orte_rmaps_base_module_t *select_preferred(char *name);
static orte_rmaps_base_module_t *select_any(void);


/*
 * Function for selecting one component from all those that are
 * available.
 */
orte_rmaps_base_module_t* orte_rmaps_base_select(char *preferred)
{
    if (NULL != preferred) {
        return select_preferred(preferred);
    } else {
        return select_any();
    }
}


static orte_rmaps_base_module_t *select_preferred(char *name)
{
    opal_list_item_t *item;
    orte_rmaps_base_cmp_t *cmp;

    /* Look for a matching selected name */

    opal_output(orte_rmaps_base.rmaps_output,
                "orte:base:select: looking for component %s", name);
    for (item = opal_list_get_first(&orte_rmaps_base.rmaps_available);
         item != opal_list_get_end(&orte_rmaps_base.rmaps_available);
         item = opal_list_get_next(item)) {
        cmp = (orte_rmaps_base_cmp_t *) item;

        if (0 == strcmp(name, 
                        cmp->component->rmaps_version.mca_component_name)) {
            opal_output(orte_rmaps_base.rmaps_output,
                        "orte:base:select: found module for compoent %s", name);
            return cmp->module;
        }
    }

    /* Didn't find a matching name */

    opal_output(orte_rmaps_base.rmaps_output,
                "orte:base:select: did not find module for compoent %s", name);
    return NULL;
}


static orte_rmaps_base_module_t *select_any(void)
{
    opal_list_item_t *item;
    orte_rmaps_base_cmp_t *cmp;

    /* If the list is empty, return NULL */

    if (true == opal_list_is_empty(&orte_rmaps_base.rmaps_available)) {
        opal_output(orte_rmaps_base.rmaps_output,
                    "orte:base:select: no components available!");
        return NULL;
    }

    /* Otherwise, return the first item (it's already sorted in
       priority order) */

    item = opal_list_get_first(&orte_rmaps_base.rmaps_available);
    cmp = (orte_rmaps_base_cmp_t *) item;
    opal_output(orte_rmaps_base.rmaps_output,
                "orte:base:select: highest priority component: %s",
                cmp->component->rmaps_version.mca_component_name);
    return cmp->module;
}
