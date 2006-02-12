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

#ifndef MCA_PML_BASE_H
#define MCA_PML_BASE_H

#include "ompi_config.h"

#include "opal/mca/mca.h"
#include "ompi/mca/pml/pml.h"


/*
 * Global functions for the PML
 */

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
OMPI_DECLSPEC  int mca_pml_base_open(void);
OMPI_DECLSPEC  int mca_pml_base_progress(void);
OMPI_DECLSPEC  int mca_pml_base_select(bool enable_progress_threads,
                                       bool enable_mpi_threads);
OMPI_DECLSPEC int mca_pml_base_close(void);


/*
 * Globals
 */
OMPI_DECLSPEC extern int mca_pml_base_output;
OMPI_DECLSPEC extern opal_list_t mca_pml_base_components_available;
OMPI_DECLSPEC extern mca_pml_base_component_t mca_pml_base_selected_component;
OMPI_DECLSPEC extern mca_pml_base_module_t mca_pml;
OMPI_DECLSPEC extern char* mca_pml_base_pml;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif /* MCA_PML_BASE_H */
