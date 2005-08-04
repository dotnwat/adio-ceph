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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"
#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "include/orte_constants.h"
#include "opal/util/argv.h"
#include "opal/util/path.h"
#include "opal/util/basename.h"
#include "opal/util/show_help.h"
#include "mca/pls/pls.h"
#include "mca/pls/rsh/pls_rsh.h"
#include "mca/base/mca_base_param.h"
#include "mca/rml/rml.h"


/*
 * Public string showing the pls ompi_rsh component version number
 */
const char *mca_pls_rsh_component_version_string =
  "Open MPI rsh pls MCA component version " ORTE_VERSION;


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

orte_pls_rsh_component_t mca_pls_rsh_component = {
    {
    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a pls v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_PLS_BASE_VERSION_1_0_0,

        /* Component name and version */

        "rsh",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */

        orte_pls_rsh_component_open,
        orte_pls_rsh_component_close
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* Whether the component is checkpointable or not */

        true
    },

    /* Initialization / querying functions */

    orte_pls_rsh_component_init
    }
};



int orte_pls_rsh_component_open(void)
{
    char* param;
    char *bname;
    size_t i;
    int tmp;
    mca_base_component_t *c = &mca_pls_rsh_component.super.pls_version;

    /* initialize globals */
    OBJ_CONSTRUCT(&mca_pls_rsh_component.lock, opal_mutex_t);
    OBJ_CONSTRUCT(&mca_pls_rsh_component.cond, opal_condition_t);
    mca_pls_rsh_component.num_children = 0;

    /* lookup parameters */
    mca_base_param_reg_int(c, "debug",
                           "Whether or not to enable debugging output for the rsh pls component (0 or 1)",
                           false, false, false, &tmp);
    mca_pls_rsh_component.debug = tmp;
    mca_base_param_reg_int(c, "num_concurrent",
                           "How many pls_rsh_agent instances to invoke concurrently (must be > 0)",
                           false, false, 128, &tmp);
    if (tmp <= 0) {
        opal_show_help("help-pls-rsh.txt", "concurrency-less-than-zero",
                       true, tmp);
        tmp = 1;
    }
    mca_pls_rsh_component.num_concurrent = tmp;
    if (mca_pls_rsh_component.debug == 0) {
        mca_base_param_reg_int_name("orte", "debug",
                                    "Whether or not to enable debugging output for all ORTE components (0 or 1)",
                                    false, false, false, &tmp);
        mca_pls_rsh_component.debug = tmp;
    }

    mca_base_param_reg_string(c, "orted",
                              "The command name that the rsh pls component will invoke for the ORTE daemon",
                              false, false, "orted", 
                              &mca_pls_rsh_component.orted);
    mca_base_param_reg_int(c, "priority",
                           "Priority of the rsh pls component",
                           false, false, 10,
                           &mca_pls_rsh_component.priority);
    mca_base_param_reg_int(c, "delay",
                           "Delay (in seconds) between invocations of the remote agent, but only used when the \"debug\" MCA parameter is true, or the top-level MCA debugging is enabled (otherwise this value is ignored)",
                           false, false, 1,
                           &mca_pls_rsh_component.delay);
    mca_base_param_reg_int(c, "reap",
                           "If set to 1, wait for all the processes to complete before exiting.  Otherwise, quit immediately -- without waiting for confirmation that all other processes in the job have completed.",
                           false, false, 1, &tmp);
    mca_pls_rsh_component.reap = tmp;
    mca_base_param_reg_int(c, "assume_same_shell",
                           "If set to 1, assume that the shell on the remote node is the same as the shell on the local node.  Otherwise, probe for what the remote shell is (PROBE IS NOT CURRENTLY IMPLEMENTED!).",
                           false, false, 1, &tmp);
    mca_pls_rsh_component.assume_same_shell = tmp;
    /* JMS: To be removed when probe is implemented */
    if (!mca_pls_rsh_component.assume_same_shell) {
        opal_show_help("help-pls-rsh.txt", "assume-same-shell-probe-not-implemented", true);
        mca_pls_rsh_component.assume_same_shell = true;
    }

    mca_base_param_reg_string(c, "agent",
                              "The command used to launch executables on remote nodes (typically either \"rsh\" or \"ssh\")",
                              false, false, "ssh",
                              &param);
    mca_pls_rsh_component.argv = opal_argv_split(param, ' ');
    mca_pls_rsh_component.argc = opal_argv_count(mca_pls_rsh_component.argv);
    if (mca_pls_rsh_component.argc > 0) {
        /* If the agent is ssh, and debug was not selected, then
           automatically add "-x" */

        bname = opal_basename(mca_pls_rsh_component.argv[0]);
        if (NULL != bname && 0 == strcmp(bname, "ssh") &&
            mca_pls_rsh_component.debug == 0) {
            for (i = 1; NULL != mca_pls_rsh_component.argv[i]; ++i) {
                if (0 == strcasecmp("-x", mca_pls_rsh_component.argv[i])) {
                    break;
                }
            }
            if (NULL == mca_pls_rsh_component.argv[i]) {
                opal_argv_append(&mca_pls_rsh_component.argc, 
                                 &mca_pls_rsh_component.argv, "-x");
            }
        }
        if (NULL != bname) {
            free(bname);
        }

        mca_pls_rsh_component.path = strdup(mca_pls_rsh_component.argv[0]);
        return ORTE_SUCCESS;
    } else {
        mca_pls_rsh_component.path = NULL;
        return ORTE_ERR_BAD_PARAM;
    }
}


orte_pls_base_module_t *orte_pls_rsh_component_init(int *priority)
{
    extern char **environ;

    /* If we didn't find the agent in the path, then don't use this component */
    if (NULL == mca_pls_rsh_component.argv || NULL == mca_pls_rsh_component.argv[0]) {
        return NULL;
    }
    mca_pls_rsh_component.path = opal_path_findv(mca_pls_rsh_component.argv[0], 0, environ, NULL);
    if (NULL == mca_pls_rsh_component.path) {
        return NULL;
    }
    *priority = mca_pls_rsh_component.priority;
    return &orte_pls_rsh_module;
}


int orte_pls_rsh_component_close(void)
{
    /* cleanup state */
    OBJ_DESTRUCT(&mca_pls_rsh_component.lock);
    OBJ_DESTRUCT(&mca_pls_rsh_component.cond);
    if(NULL != mca_pls_rsh_component.argv)
        opal_argv_free(mca_pls_rsh_component.argv);
    if(NULL != mca_pls_rsh_component.path)
        free(mca_pls_rsh_component.path);
    return ORTE_SUCCESS;
}

