/*
 * Copyright (c) 2004-2010 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2012 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2010-2012 Los Alamos National Security, LLC.
 *                         All rights reserved.
 * Copyright (c) 2011-2012 University of Houston. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"
#include "orte/types.h"

#include "opal/class/opal_pointer_array.h"
#include "opal/runtime/opal_info_support.h"

#include "orte/mca/db/base/base.h"
#include "orte/mca/errmgr/base/base.h"
#include "orte/mca/ess/base/base.h"
#include "orte/mca/grpcomm/base/base.h"
#include "orte/mca/iof/base/base.h"
#include "orte/mca/odls/base/base.h"
#include "orte/mca/oob/base/base.h"
#include "orte/mca/plm/base/base.h"
#include "orte/mca/ras/base/ras_private.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/mca/rml/base/base.h"
#include "orte/mca/routed/base/base.h"
#if OPAL_ENABLE_FT_CR == 1
#include "orte/mca/snapc/base/base.h"
#include "orte/mca/sstore/base/base.h"
#endif
#if ORTE_ENABLE_SENSORS
#include "orte/mca/sensor/base/base.h"
#endif
#include "orte/mca/filem/base/base.h"
#include "orte/mca/state/base/base.h"
#include "orte/util/proc_info.h"
#include "orte/runtime/runtime.h"

#include "orte/runtime/orte_info_support.h"

const char *orte_info_type_orte = "orte";

void orte_info_register_types(opal_pointer_array_t *mca_types)
{
    /* frameworks */
    opal_pointer_array_add(mca_types, "db");
    opal_pointer_array_add(mca_types, "errmgr");
    opal_pointer_array_add(mca_types, "ess");
    opal_pointer_array_add(mca_types, "filem");
    opal_pointer_array_add(mca_types, "grpcomm");
    opal_pointer_array_add(mca_types, "iof");
    opal_pointer_array_add(mca_types, "odls");
    opal_pointer_array_add(mca_types, "oob");
    opal_pointer_array_add(mca_types, "orte");
    opal_pointer_array_add(mca_types, "plm");
    opal_pointer_array_add(mca_types, "ras");
    opal_pointer_array_add(mca_types, "rmaps");
    opal_pointer_array_add(mca_types, "rml");
    opal_pointer_array_add(mca_types, "routed");
#if ORTE_ENABLE_SENSORS
    opal_pointer_array_add(mca_types, "sensor");
#endif
#if OPAL_ENABLE_FT_CR == 1
    opal_pointer_array_add(mca_types, "snapc");
    opal_pointer_array_add(mca_types, "sstore");
#endif
    opal_pointer_array_add(mca_types, "state");
}

int orte_info_register_components(opal_pointer_array_t *mca_types,
                                  opal_pointer_array_t *component_map)
{
    opal_info_component_map_t *map;
    char *env, *str;
    int i;
    char *target, *save, *type;
    char **env_save=NULL;

    /* Clear out the environment.  Use strdup() to orphan the resulting
     * strings because items are placed in the environment by reference,
     * not by value.
     */
    for (i = 0; i < mca_types->size; ++i) {
        if (NULL == (type = (char*)opal_pointer_array_get_item(mca_types, i))) {
            continue;
        }
        asprintf(&env, "OMPI_MCA_%s", type);
        if (NULL != (save = getenv(env))) {
            /* save this param so it can later be restored */
            asprintf(&str, "%s=%s", env, save);
            opal_argv_append_nosize(&env_save, str);
            free(str);
            /* can't manipulate it directly, so make a copy first */
            asprintf(&target, "%s=", env);
            putenv(target);
            free(target);
        }
        free(env);
    }

     /* Set orte_process_info.proc_type to HNP to force all frameworks to
     * open components
     */
    orte_process_info.proc_type = ORTE_PROC_HNP;

    /* Register the ORTE layer's MCA parameters */
    
    if (ORTE_SUCCESS != orte_register_params()) {
        return ORTE_ERROR;
    }
    
    if (ORTE_SUCCESS != orte_db_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("db");
    map->components = &orte_db_base.available_components;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_errmgr_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("errmgr");
    map->components = &orte_errmgr_base_components_available;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_ess_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("ess");
    map->components = &orte_ess_base_components_available;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_filem_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("filem");
    map->components = &orte_filem_base_components_available;
    opal_pointer_array_add(component_map, map);

    if (ORTE_SUCCESS != orte_grpcomm_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("grpcomm");
    map->components = &orte_grpcomm_base.components_available;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_iof_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("iof");
    map->components = &orte_iof_base.iof_components_opened;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_odls_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("odls");
    map->components = &orte_odls_base.available_components;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != mca_oob_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("oob");
    map->components = &mca_oob_base_components;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_plm_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("plm");
    map->components = &orte_plm_base.available_components;
    opal_pointer_array_add(component_map, map);

    if (ORTE_SUCCESS != orte_ras_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("ras");
    map->components = &orte_ras_base.ras_opened;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_rmaps_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("rmaps");
    map->components = &orte_rmaps_base.available_components;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_routed_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("routed");
    map->components = &orte_routed_base_components;
    opal_pointer_array_add(component_map, map);
    
    if (ORTE_SUCCESS != orte_rml_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("rml");
    map->components = &orte_rml_base_components;
    opal_pointer_array_add(component_map, map);
    
#if ORTE_ENABLE_SENSORS
    if (ORTE_SUCCESS != orte_sensor_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("sensor");
    map->components = &mca_sensor_base_components_available;
    opal_pointer_array_add(component_map, map);
#endif
    
#if OPAL_ENABLE_FT_CR == 1
    if (ORTE_SUCCESS != orte_snapc_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("snapc");
    map->components = &orte_snapc_base_components_available;
    opal_pointer_array_add(component_map, map);

    if (ORTE_SUCCESS != orte_sstore_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("sstore");
    map->components = &orte_sstore_base_components_available;
    opal_pointer_array_add(component_map, map);
#endif
    
    if (ORTE_SUCCESS != orte_state_base_open()) {
        return ORTE_ERROR;
    }
    map = OBJ_NEW(opal_info_component_map_t);
    map->type = strdup("state");
    map->components = &orte_state_base_components_available;
    opal_pointer_array_add(component_map, map);

    /* Restore the environment to what it was before we started so that
     * if users setenv OMPI_MCA_<framework name> to some value, they'll
     * see that value when it is shown via --param output.
     */
    
    if (NULL != env_save) {
        for (i = 0; i < opal_argv_count(env_save); ++i) {
            putenv(env_save[i]);
        }
    }
    return ORTE_SUCCESS;
}

void orte_info_close_components(void)
{
    (void) orte_db_base_close();
    (void) orte_errmgr_base_close();
    (void) orte_ess_base_close();
    (void) orte_filem_base_close();
    (void) orte_grpcomm_base_close();
    (void) orte_iof_base_close();
    (void) orte_odls_base_close();
    (void) mca_oob_base_close();
    (void) orte_plm_base_close();
    (void) orte_ras_base_close();
    (void) orte_rmaps_base_close();
    (void) orte_rml_base_close();
    (void) orte_routed_base_close();
#if OPAL_ENABLE_FT_CR == 1
    (void) orte_snapc_base_close();
    (void) orte_sstore_base_close();
#endif
    (void) orte_state_base_close();
}

void orte_info_show_orte_version(const char *scope)
{
    char *tmp, *tmp2;

    asprintf(&tmp, "%s:version:full", orte_info_type_orte);
    tmp2 = opal_info_make_version_str(scope, 
                                      ORTE_MAJOR_VERSION, ORTE_MINOR_VERSION, 
                                      ORTE_RELEASE_VERSION, 
                                      ORTE_GREEK_VERSION,
                                      ORTE_WANT_REPO_REV, ORTE_REPO_REV);
    opal_info_out("Open RTE", tmp, tmp2);
    free(tmp);
    free(tmp2);
    asprintf(&tmp, "%s:version:repo", orte_info_type_orte);
    opal_info_out("Open RTE repo revision", tmp, ORTE_REPO_REV);
    free(tmp);
    asprintf(&tmp, "%s:version:release_date", orte_info_type_orte);
    opal_info_out("Open RTE release date", tmp, ORTE_RELEASE_DATE);
    free(tmp);
}

