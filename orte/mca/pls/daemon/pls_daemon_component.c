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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"

#include "include/orte_constants.h"
#include "mca/pls/pls.h"
#include "pls_daemon.h"
#include "mca/base/mca_base_param.h"


/*
 * Public string showing the pls ompi_daemon component version number
 */
const char *mca_pls_daemon_component_version_string =
  "Open MPI daemon pls MCA component version " ORTE_VERSION;


/*
 * Local variable
 */
static int param_priority = -1;


/*
 * Local function
 */
static int pls_daemon_open(void);
static struct orte_pls_base_module_1_0_0_t *pls_daemon_init(int *priority);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

orte_pls_base_component_1_0_0_t mca_pls_daemon_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a pls v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_PLS_BASE_VERSION_1_0_0,

        /* Component name and version */

        "daemon",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */

        pls_daemon_open,
        NULL
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* Whether the component is checkpointable or not */

        true
    },

    /* Initialization / querying functions */

    pls_daemon_init
};


static int pls_daemon_open(void)
{
    /* Use a low priority, but allow other components to be lower */
    
    param_priority = 
        mca_base_param_register_int("pls", "daemon", "priority", NULL, 10);

    return ORTE_SUCCESS;
}


static struct orte_pls_base_module_1_0_0_t *pls_daemon_init(int *priority)
{
    /* Do we have daemons? */

    /* JMS: Add check to see if daemons */

    bool have_daemons = false;
    if (have_daemons) {
        mca_base_param_lookup_int(param_priority, priority);

        return &orte_pls_daemon_module;
    }

    /* Sadly, no */

    return NULL;
}
