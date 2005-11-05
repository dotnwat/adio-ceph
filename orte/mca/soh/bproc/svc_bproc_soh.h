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
/**
 * @file
 */
#ifndef _MCA_SVC_BPROC_SOH_
#define _MCA_SVC_BPROC_SOH_

#include "mca/svc/svc.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


/**
 * Component open/close/init
 */
int mca_svc_bproc_soh_component_open(void);
int mca_svc_bproc_soh_component_close(void);
mca_svc_base_module_t* mca_svc_bproc_soh_component_init(void);

/**
 * Module init/fini
 */
int mca_svc_bproc_soh_module_init(mca_svc_base_module_t*);
int mca_svc_bproc_soh_module_fini(mca_svc_base_module_t*);

struct mca_svc_bproc_soh_component_t {
    mca_svc_base_component_t base;
    int debug;
};
typedef struct mca_svc_bproc_soh_component_t mca_svc_bproc_soh_component_t;                                                                                                                                               

extern mca_svc_base_module_t mca_svc_bproc_soh_module;
extern mca_svc_soh_component_t mca_svc_bproc_soh_component;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif

