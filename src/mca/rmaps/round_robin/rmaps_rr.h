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
 */
/**
 * @file
 *
 * Resource Mapping 
 */
#ifndef ORTE_RMAPS_RR_H
#define ORTE_RMAPS_RR_H

#include "mca/rmaps/rmaps.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/**
 * RMGR Component 
 */
struct orte_rmaps_round_robin_component_t {
    orte_rmaps_base_component_t super;
    int debug;
    int priority;
};
typedef struct orte_rmaps_round_robin_component_t orte_rmaps_round_robin_component_t;

OMPI_COMP_EXPORT extern orte_rmaps_round_robin_component_t mca_rmaps_round_robin_component;
OMPI_COMP_EXPORT extern orte_rmaps_base_module_t orte_rmaps_round_robin_module;


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
