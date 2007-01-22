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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "orte_config.h"
#include "orte/orte_constants.h"

#include <stdlib.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <ctype.h>

#include "opal/util/argv.h"
#include "opal/util/path.h"
#include "opal/util/basename.h"
#include "opal/util/show_help.h"
#include "opal/mca/base/mca_base_param.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/rml/rml.h"

#include "orte/mca/pls/pls.h"
#include "orte/mca/pls/base/pls_private.h"
#include "orte/mca/pls/process/pls_process.h"

#if !defined(__WINDOWS__)
extern char **environ;
#endif  /* !defined(__WINDOWS__) */

/*
 * Local function
 */
static char **search(const char* agent_list);


/*
 * Public string showing the pls ompi_process component version number
 */
const char *mca_pls_process_component_version_string =
  "Open MPI process pls MCA component version " ORTE_VERSION;


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

orte_pls_process_component_t mca_pls_process_component = {
    {
    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a pls v1.3.0 component (which also
           implies a specific MCA version) */

        ORTE_PLS_BASE_VERSION_1_3_0,

        /* Component name and version */

        "process",
        ORTE_MAJOR_VERSION,
        ORTE_MINOR_VERSION,
        ORTE_RELEASE_VERSION,

        /* Component open and close functions */

        orte_pls_process_component_open,
        orte_pls_process_component_close
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* Whether the component is checkpointable or not */

        true
    },

    /* Initialization / querying functions */

    orte_pls_process_component_init
    }
};



int orte_pls_process_component_open(void)
{
    int tmp, value;
    mca_base_component_t *c = &mca_pls_process_component.super.pls_version;

    /* initialize globals */
    OBJ_CONSTRUCT(&mca_pls_process_component.lock, opal_mutex_t);
    OBJ_CONSTRUCT(&mca_pls_process_component.cond, opal_condition_t);
    mca_pls_process_component.num_children = 0;

    /* lookup parameters */
    mca_base_param_reg_int(c, "debug",
                           "Whether or not to enable debugging output for the process pls component (0 or 1)",
                           false, false, false, &tmp);
    mca_pls_process_component.debug = OPAL_INT_TO_BOOL(tmp);
    mca_base_param_reg_int(c, "num_concurrent",
                           "How many pls_process_agent instances to invoke concurrently (must be > 0)",
                           false, false, 128, &tmp);
    if (tmp <= 0) {
        opal_show_help("help-pls-process.txt", "concurrency-less-than-zero",
                       true, tmp);
        tmp = 1;
    }
    mca_pls_process_component.num_concurrent = tmp;

    mca_base_param_reg_int(c, "force_process",
                           "Force the launcher to always use process, even for local daemons",
                           false, false, false, &tmp);
    mca_pls_process_component.force_process = OPAL_INT_TO_BOOL(tmp);
    
    if (mca_pls_process_component.debug == 0) {
        mca_base_param_reg_int_name("orte", "debug",
                                    "Whether or not to enable debugging output for all ORTE components (0 or 1)",
                                    false, false, false, &tmp);
        mca_pls_process_component.debug = OPAL_INT_TO_BOOL(tmp);
    }
    mca_base_param_reg_int_name("orte", "debug_daemons",
                                "Whether or not to enable debugging of daemons (0 or 1)",
                                false, false, false, &tmp);
    mca_pls_process_component.debug_daemons = OPAL_INT_TO_BOOL(tmp);
    
    tmp = mca_base_param_reg_int_name("orte", "timing",
                                      "Request that critical timing loops be measured",
                                      false, false, 0, &value);
    if (value != 0) {
        mca_pls_process_component.timing = true;
    } else {
        mca_pls_process_component.timing = false;
    }

    mca_base_param_reg_string(c, "orted",
                              "The command name that the process pls component will invoke for the ORTE daemon",
                              false, false, "orted", 
                              &mca_pls_process_component.orted);
    
    mca_base_param_reg_int(c, "priority",
                           "Priority of the process pls component",
                           false, false, 10,
                           &mca_pls_process_component.priority);
    mca_base_param_reg_int(c, "delay",
                           "Delay (in seconds) between invocations of the remote agent, but only used when the \"debug\" MCA parameter is true, or the top-level MCA debugging is enabled (otherwise this value is ignored)",
                           false, false, 1,
                           &mca_pls_process_component.delay);
    mca_base_param_reg_int(c, "reap",
                           "If set to 1, wait for all the processes to complete before exiting.  Otherwise, quit immediately -- without waiting for confirmation that all other processes in the job have completed.",
                           false, false, 1, &tmp);
    mca_pls_process_component.reap = OPAL_INT_TO_BOOL(tmp);
    mca_base_param_reg_int(c, "assume_same_shell",
                           "If set to 1, assume that the shell on the remote node is the same as the shell on the local node.  Otherwise, probe for what the remote shell.",
                           false, false, 1, &tmp);
    mca_pls_process_component.assume_same_shell = OPAL_INT_TO_BOOL(tmp);

    return ORTE_SUCCESS;
}


#if !defined(__WINDOWS__)
extern char **environ;
#endif  /* !defined(__WINDOWS__) */

orte_pls_base_module_t *orte_pls_process_component_init(int *priority)
{
    /* if we are not an HNP, then don't select us */
    *priority = mca_pls_process_component.priority;
    
    return &orte_pls_process_module;
}


int orte_pls_process_component_close(void)
{
    /* cleanup state */
    OBJ_DESTRUCT(&mca_pls_process_component.lock);
    OBJ_DESTRUCT(&mca_pls_process_component.cond);
    if (NULL != mca_pls_process_component.orted) {
        free(mca_pls_process_component.orted);
    }
    return ORTE_SUCCESS;
}


/*
 * Take a colon-delimited list of agents and locate the first one that
 * we are able to find in the PATH.  Split that one into argv and
 * return it.  If nothing found, then return NULL.
 */
static char **search(const char* agent_list)
{
    int i, j;
    char *line, **lines = opal_argv_split(agent_list, ':');
    char **tokens, *tmp;
    char cwd[PATH_MAX];

    getcwd(cwd, PATH_MAX);
    for (i = 0; NULL != lines[i]; ++i) {
        line = lines[i];

        /* Trim whitespace at the beginning and end of the line */
        for (j = 0; '\0' != line[j] && isspace(line[j]); ++line) {
            continue;
        }
        for (j = strlen(line) - 2; j > 0 && isspace(line[j]); ++j) {
            line[j] = '\0';
        }
        if (strlen(line) <= 0) {
            continue;
        }

        /* Split it */
        tokens = opal_argv_split(line, ' ');

        /* Look for the first token in the PATH */
        tmp = opal_path_findv(tokens[0], X_OK, environ, cwd);
        if (NULL != tmp) {
            free(tokens[0]);
            tokens[0] = tmp;
            opal_argv_free(lines);
            return tokens;
        }

        /* Didn't find it */
        opal_argv_free(tokens);
    }

    /* Doh -- didn't find anything */
    opal_argv_free(lines);
    return NULL;
}
