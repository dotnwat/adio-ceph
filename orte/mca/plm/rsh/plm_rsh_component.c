/*
 * Copyright (c) 2004-2008 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2009      Cisco Systems, Inc.  All rights reserved.
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

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include "opal/util/opal_environ.h"
#include "opal/util/argv.h"
#include "opal/util/path.h"
#include "opal/mca/base/mca_base_param.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"
#include "orte/util/name_fns.h"
#include "orte/util/show_help.h"

#include "orte/mca/plm/plm.h"
#include "orte/mca/plm/base/plm_private.h"
#include "orte/mca/plm/rsh/plm_rsh.h"


/*
 * Public string showing the plm ompi_rsh component version number
 */
const char *mca_plm_rsh_component_version_string =
  "Open MPI rsh plm MCA component version " ORTE_VERSION;


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

orte_plm_rsh_component_t mca_plm_rsh_component = {
    {
    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        ORTE_PLM_BASE_VERSION_2_0_0,

        /* Component name and version */
        "rsh",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */
        orte_plm_rsh_component_open,
        orte_plm_rsh_component_close,
        orte_plm_rsh_component_query
    },
    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    }
    }
};



int orte_plm_rsh_component_open(void)
{
    int tmp;
    mca_base_component_t *c = &mca_plm_rsh_component.super.base_version;

    /* initialize globals */
    OBJ_CONSTRUCT(&mca_plm_rsh_component.lock, opal_mutex_t);
    OBJ_CONSTRUCT(&mca_plm_rsh_component.cond, opal_condition_t);
    mca_plm_rsh_component.num_children = 0;
    OBJ_CONSTRUCT(&mca_plm_rsh_component.children, opal_list_t);

    /* lookup parameters */
    mca_base_param_reg_int(c, "num_concurrent",
                           "How many plm_rsh_agent instances to invoke concurrently (must be > 0)",
                           false, false, 128, &tmp);
    if (tmp <= 0) {
        orte_show_help("help-plm-rsh.txt", "concurrency-less-than-zero",
                       true, tmp);
        tmp = 1;
    }
    mca_plm_rsh_component.num_concurrent = tmp;

    mca_base_param_reg_int(c, "force_rsh",
                           "Force the launcher to always use rsh",
                           false, false, false, &tmp);
    mca_plm_rsh_component.force_rsh = OPAL_INT_TO_BOOL(tmp);
    mca_base_param_reg_int(c, "disable_qrsh",
                           "Disable the launcher to use qrsh when under the SGE parallel environment",
                           false, false, false, &tmp);
    mca_plm_rsh_component.disable_qrsh = OPAL_INT_TO_BOOL(tmp);  
    
    mca_base_param_reg_int(c, "priority",
                           "Priority of the rsh plm component",
                           false, false, 10,
                           &mca_plm_rsh_component.priority);
    mca_base_param_reg_int(c, "delay",
                           "Delay (in seconds) between invocations of the remote agent, but only used when the \"debug\" MCA parameter is true, or the top-level MCA debugging is enabled (otherwise this value is ignored)",
                           false, false, 1,
                           &mca_plm_rsh_component.delay);
    mca_base_param_reg_int(c, "assume_same_shell",
                           "If set to 1, assume that the shell on the remote node is the same as the shell on the local node.  Otherwise, probe for what the remote shell.",
                           false, false, 1, &tmp);
    mca_plm_rsh_component.assume_same_shell = OPAL_INT_TO_BOOL(tmp);
    
    mca_base_param_reg_int(c, "tree_spawn",
                           "If set to 1, launch via a tree-based topology",
                           false, false, (int)false, &tmp);
    mca_plm_rsh_component.tree_spawn = OPAL_INT_TO_BOOL(tmp);
    
    return ORTE_SUCCESS;
}


int orte_plm_rsh_component_query(mca_base_module_t **module, int *priority)
{
    /* To be absolutely sure that we are under an SGE parallel env */
    if (!mca_plm_rsh_component.disable_qrsh &&
        NULL != getenv("SGE_ROOT") && NULL != getenv("ARC") &&
        NULL != getenv("PE_HOSTFILE") && NULL != getenv("JOB_ID")) {
        /* setting exec_argv and exec_path for qrsh */
        asprintf(&orte_plm_globals.rsh_agent_path, "%s/bin/%s", getenv("SGE_ROOT"), getenv("ARC"));
        asprintf(&orte_plm_globals.rsh_agent_argv[0], "%s/bin/%s/qrsh", getenv("SGE_ROOT"), getenv("ARC"));
        /* double check that we have access and permissions for the qrsh agent */
        if (NULL == opal_path_findv(orte_plm_globals.rsh_agent_argv[0], X_OK,
                                    environ, NULL)) {
            OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                                 "%s plm:rsh: unable to be used: cannot find path "
                                 "or execution permissions not set for launching agent \"%s\"\n", 
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 orte_plm_globals.rsh_agent_argv[0]));
            *module = NULL;
            return ORTE_ERROR;
        }
        /* automatically add -inherit and grid engine PE related flags */
        opal_argv_append_nosize(&orte_plm_globals.rsh_agent_argv, "-inherit");
        /* Don't use the "-noshell" flag as qrsh would have a problem 
         * swallowing a long command */
        opal_argv_append_nosize(&orte_plm_globals.rsh_agent_argv, "-nostdin");
        opal_argv_append_nosize(&orte_plm_globals.rsh_agent_argv, "-V");
        if (0 < opal_output_get_verbosity(orte_plm_globals.output)) {
            opal_argv_append_nosize(&orte_plm_globals.rsh_agent_argv, "-verbose");
            opal_output_verbose(1, orte_plm_globals.output,
                                "%s plm:rsh: using %s for launching\n",
                                ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                orte_plm_globals.rsh_agent_argv[0]);
        }
        *priority = mca_plm_rsh_component.priority;
        *module = (mca_base_module_t *) &orte_plm_rsh_module;
        return ORTE_SUCCESS;
    }
    
    /* if this isn't an SGE environment, see if rsh/ssh is available */
    
    if (ORTE_SUCCESS != orte_plm_base_rsh_launch_agent_setup()) {
        /* this isn't an error - we just cannot be selected */
        OPAL_OUTPUT_VERBOSE((1, orte_plm_globals.output,
                             "%s plm:rsh: unable to be used: cannot find path "
                             "for launching agent \"%s\"\n", 
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                             orte_plm_globals.rsh_agent_argv[0]));
        *module = NULL;
        return ORTE_ERROR;
    }
    
    /* we are good - make ourselves available */
    *priority = mca_plm_rsh_component.priority;
    *module = (mca_base_module_t *) &orte_plm_rsh_module;
    return ORTE_SUCCESS;
}


int orte_plm_rsh_component_close(void)
{
    /* cleanup state */
    OBJ_DESTRUCT(&mca_plm_rsh_component.lock);
    OBJ_DESTRUCT(&mca_plm_rsh_component.cond);
    OBJ_DESTRUCT(&mca_plm_rsh_component.children);

    return ORTE_SUCCESS;
}
