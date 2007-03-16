/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University.
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
 * Coord CRCP component
 *
 */

#ifndef MCA_CRCP_COORD_EXPORT_H
#define MCA_CRCP_COORD_EXPORT_H

#include "ompi_config.h"

#include "opal/mca/mca.h"
#include "ompi/mca/crcp/crcp.h"
#include "ompi/communicator/communicator.h"
#include "orte/mca/ns/ns.h"
#include "opal/runtime/opal_cr.h"
#include "opal/threads/mutex.h"
#include "opal/threads/condition.h"


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

    /*
     * Local Component structures
     */
    struct ompi_crcp_coord_component_t {
        ompi_crcp_base_component_t super;  /** Base CRCP component */
    };
    typedef struct ompi_crcp_coord_component_t ompi_crcp_coord_component_t;
    extern ompi_crcp_coord_component_t mca_crcp_coord_component;

    /*
     * Module functions
     */
    ompi_crcp_base_module_1_0_0_t *
        ompi_crcp_coord_component_query(int *priority);
    int ompi_crcp_coord_module_init(void);
    int ompi_crcp_coord_module_finalize(void);

    int ompi_crcp_coord_pml_init(void);
    int ompi_crcp_coord_pml_finalize(void);

    int ompi_crcp_coord_btl_init(void);
    int ompi_crcp_coord_btl_finalize(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* MCA_CRCP_COORD_EXPORT_H */
