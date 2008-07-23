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
 * Copyright (c) 2006-2007 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2008      Institut National de Recherche en Informatique
 *                         et Automatique. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "orte_config.h"
#include "orte/constants.h"

#include <lsf/lsbatch.h>

#include "opal/mca/base/mca_base_param.h"
#include "opal/util/argv.h"

#include "orte/util/show_help.h"
#include "orte/util/proc_info.h"
#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/plm/plm.h"
#include "orte/mca/plm/base/base.h"
#include "orte/mca/plm/base/plm_private.h"
#include "plm_lsf.h"


/*
 * Public string showing the plm lsf component version number
 */
const char *mca_plm_lsf_component_version_string =
  "Open MPI lsf plm MCA component version " ORTE_VERSION;



/*
 * Local function
 */
static int plm_lsf_open(void);
static int plm_lsf_close(void);
static int orte_plm_lsf_component_query(mca_base_module_t **module, int *priority);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

orte_plm_lsf_component_t mca_plm_lsf_component = {
    {
        /* First, the mca_component_t struct containing meta information
           about the component itself */

        {
            /* Indicate that we are a plm v1.0.0 component (which also
               implies a specific MCA version) */
            ORTE_PLM_BASE_VERSION_1_0_0,

            /* Component name and version */
            "lsf",
            ORTE_MAJOR_VERSION,
            ORTE_MINOR_VERSION,
            ORTE_RELEASE_VERSION,

            /* Component open and close functions */
            plm_lsf_open,
            plm_lsf_close,
            orte_plm_lsf_component_query
        },

        /* Next the MCA v1.0.0 component meta data */
        {
            /* The component is checkpoint ready */
            MCA_BASE_METADATA_PARAM_CHECKPOINT
        }
    }
};


static int plm_lsf_open(void)
{
    mca_base_component_t *comp = &mca_plm_lsf_component.super.base_version;

    mca_base_param_reg_int(comp, "priority", "Default selection priority",
                           false, false, 75, &mca_plm_lsf_component.priority);

    mca_base_param_reg_string(comp, "orted",
                              "Command to use to start proxy orted",
                              false, false, "orted",
                              &mca_plm_lsf_component.orted);

    return ORTE_SUCCESS;
}


static int plm_lsf_close(void)
{
    return ORTE_SUCCESS;
}


static int orte_plm_lsf_component_query(mca_base_module_t **module, int *priority)
{
    
    /* check if lsf is running here */
    if (NULL == getenv("LSB_JOBID") || lsb_init("ORTE launcher") < 0) {
        /* nope, not here */
        opal_output_verbose(10, orte_plm_globals.output,
                            "plm:lsf: NOT available for selection");
        *module = NULL;
        return ORTE_ERROR;
    }
    
    *priority = mca_plm_lsf_component.priority;
    *module = (mca_base_module_t *) &orte_plm_lsf_module;
    return ORTE_SUCCESS;
}
