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
 * Resource Allocation (POE)
 */
#ifndef ORTE_RAS_POE_H
#define ORTE_RAS_POE_H

#include "mca/ras/ras.h"
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/**
 * RAS Component 
 */
struct orte_ras_poe_component_t {
    orte_ras_base_component_t super;
    int debug;
    int priority;
};
typedef struct orte_ras_poe_component_t orte_ras_poe_component_t;

OMPI_COMP_EXPORT extern orte_ras_poe_component_t mca_ras_poe_component;
OMPI_COMP_EXPORT extern orte_ras_base_module_t orte_ras_poe_module;


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
