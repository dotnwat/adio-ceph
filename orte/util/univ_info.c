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
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <stdlib.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <sys/stat.h>


#include "orte/include/orte_constants.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "orte/mca/ns/ns_types.h"
#include "orte/mca/schema/schema_types.h"
#include "opal/util/output.h"
#include "orte/util/proc_info.h"
#include "orte/util/sys_info.h"

#include "orte/util/univ_info.h"

orte_universe_t orte_universe_info = {
    /* .state =               */    ORTE_UNIVERSE_STATE_PRE_INIT,
    /* .name =                */    NULL,
    /* .host =                */    NULL,
    /* .uid =                 */    NULL,
    /* .persistence =         */    false,
    /* .scope =               */    NULL,
    /* .console =             */    false,
    /* .seed_uri =            */    NULL,
    /* .console_connected =   */    false,
    /* .scriptfile =          */    NULL,
};


int orte_univ_info(void)
{
    int id, tmp;
    char *tmpname=NULL, *tptr, *ptr;

    if (ORTE_UNIVERSE_STATE_PRE_INIT == orte_universe_info.state) {
        id = mca_base_param_register_string("universe", NULL, NULL, NULL, NULL);
        mca_base_param_lookup_string(id, &tmpname);

        if (NULL != tmpname) {
            /* Universe name info is passed as userid@hostname:univ_name */
            /* extract the userid from the universe option, if provided */
            tptr = tmpname;
            if (NULL != (ptr = strchr(tptr, '@'))) {
                *ptr = '\0';
                orte_universe_info.uid = strdup(tptr);
                ptr++;
                tptr = ptr;
            } else {
                if (NULL == orte_system_info.user) {
                    orte_sys_info();
                }
                orte_universe_info.uid = strdup(orte_system_info.user);
            }

            /* extract the hostname, if provided */
            if (NULL != (ptr = strchr(tptr, ':'))) {
                *ptr = '\0';
                orte_universe_info.host = strdup(tptr);
                ptr++;
                tptr = ptr;
            } else {
                orte_universe_info.host = strdup(orte_system_info.nodename);
            }

            /* now copy the universe name into the universe_info structure */
            orte_universe_info.name = strdup(tptr);
        } else {
            /* if nothing was provided, then initialize the user and nodename
             * to the local values
             */
            orte_universe_info.uid = strdup(orte_system_info.user);
            orte_universe_info.host = strdup(orte_system_info.nodename);
            /* and the universe name to default-universe */
            orte_universe_info.name = strdup(ORTE_DEFAULT_UNIVERSE);
        }

        id = mca_base_param_register_int("universe", "persistence", NULL, NULL, orte_universe_info.persistence);
        mca_base_param_lookup_int(id, &tmp);
        orte_universe_info.persistence = (tmp ? true : false);

        id = mca_base_param_register_string("universe", "scope", NULL, NULL, orte_universe_info.scope);
        mca_base_param_lookup_string(id, &(orte_universe_info.scope));

        id = mca_base_param_register_int("universe", "console", NULL, NULL, orte_universe_info.console);
        mca_base_param_lookup_int(id, &tmp);
        orte_universe_info.console = (tmp ? true : false);

        id = mca_base_param_register_string("universe", "uri", NULL, NULL, orte_universe_info.seed_uri);
        mca_base_param_lookup_string(id, &(orte_universe_info.seed_uri));

        /* console connected is set elsewhere */
        id = mca_base_param_register_string("universe", "script", NULL, NULL, orte_universe_info.scriptfile);
        mca_base_param_lookup_string(id, &(orte_universe_info.scriptfile));

        orte_universe_info.state = ORTE_UNIVERSE_STATE_INIT;
    }

    return(ORTE_SUCCESS);
}


int orte_univ_info_finalize(void)
{
    if (NULL != orte_universe_info.name) {
        free(orte_universe_info.name);
        orte_universe_info.name = NULL;
    }

    if (NULL != orte_universe_info.host) {
        free(orte_universe_info.host);
        orte_universe_info.host = NULL;
    }

    if (NULL != orte_universe_info.uid) {
        free(orte_universe_info.uid);
        orte_universe_info.uid = NULL;
    }

    if (NULL != orte_universe_info.scope) {
        free(orte_universe_info.scope);
        orte_universe_info.scope = NULL;
    }

    if (NULL != orte_universe_info.seed_uri) {
        free(orte_universe_info.seed_uri);
        orte_universe_info.seed_uri = NULL;
    }

    if (NULL != orte_universe_info.scriptfile) {
        free(orte_universe_info.scriptfile);
        orte_universe_info.scriptfile = NULL;
    }

    orte_universe_info.state = ORTE_UNIVERSE_STATE_PRE_INIT;
    orte_universe_info.persistence = false;
    orte_universe_info.console = false;
    orte_universe_info.console_connected = false;

    return ORTE_SUCCESS;
}
