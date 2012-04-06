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
 * Copyright (c) 2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2011      Los Alamos National Security, LLC.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
/** @file:
 * rmaps framework base functionality.
 */

#ifndef ORTE_MCA_RMAPS_BASE_H
#define ORTE_MCA_RMAPS_BASE_H

/*
 * includes
 */
#include "orte_config.h"
#include "orte/types.h"

#include "opal/class/opal_list.h"
#include "opal/mca/mca.h"

#include "orte/runtime/orte_globals.h"

#include "orte/mca/rmaps/rmaps.h"

BEGIN_C_DECLS

/**
 * Open the rmaps framework
 */
ORTE_DECLSPEC int orte_rmaps_base_open(void);

#if !ORTE_DISABLE_FULL_SUPPORT

/*
 * Global functions for MCA overall collective open and close
 */

/**
 * Struct to hold data global to the rmaps framework
 */
typedef struct {
    /** Verbose/debug output stream */
    int rmaps_output;
    /** List of available components */
    opal_list_t available_components;
    /* list of selected modules */
    opal_list_t selected_modules;
    /* default ppr */
    char *ppr;
    /* cpus per rank */
    int cpus_per_rank;
    /* display the map after it is computed */
    bool display_map;
    /* slot list, if provided by user */
    char *slot_list;
    /* default mapping directives */
    orte_mapping_policy_t mapping;
    orte_ranking_policy_t ranking;
} orte_rmaps_base_t;

/**
 * Global instance of rmaps-wide framework data
 */
ORTE_DECLSPEC extern orte_rmaps_base_t orte_rmaps_base;

/**
 * Select an rmaps component / module
 */
typedef struct {
    opal_list_item_t super;
    int pri;
    orte_rmaps_base_module_t *module;
    mca_base_component_t *component;
} orte_rmaps_base_selected_module_t;
OBJ_CLASS_DECLARATION(orte_rmaps_base_selected_module_t);

ORTE_DECLSPEC int orte_rmaps_base_select(void);

/*
 * Map a job
 */
ORTE_DECLSPEC void orte_rmaps_base_map_job(int fd, short args, void *cbdata);

/**
 * Utility routines to get/set vpid mapping for the job
 */ 
                                                                  
ORTE_DECLSPEC int orte_rmaps_base_get_vpid_range(orte_jobid_t jobid, 
    orte_vpid_t *start, orte_vpid_t *range);
ORTE_DECLSPEC int orte_rmaps_base_set_vpid_range(orte_jobid_t jobid, 
    orte_vpid_t start, orte_vpid_t range);

/* pretty-print functions */
ORTE_DECLSPEC char* orte_rmaps_base_print_mapping(orte_mapping_policy_t mapping);
ORTE_DECLSPEC char* orte_rmaps_base_print_ranking(orte_ranking_policy_t ranking);

/**
 * Close down the rmaps framework
 */
ORTE_DECLSPEC int orte_rmaps_base_close(void);

#if OPAL_HAVE_HWLOC
ORTE_DECLSPEC int orte_rmaps_base_prep_topology(hwloc_topology_t topo);
#endif

ORTE_DECLSPEC int orte_rmaps_base_filter_nodes(orte_app_context_t *app,
                                               opal_list_t *nodes,
                                               bool remove);

#endif /* ORTE_DISABLE_FULL_SUPPORT */

END_C_DECLS

#endif
