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
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 *
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */


#include "opal_config.h"

#include "opal/constants.h"
#include "opal/mca/paffinity/paffinity.h"
#include "opal/mca/paffinity/base/base.h"

int opal_paffinity_base_set(opal_paffinity_base_cpu_set_t cpumask)
{
    if (!opal_paffinity_base_selected) {
        return OPAL_ERR_NOT_FOUND;
    }
    return opal_paffinity_base_module->paff_module_set(cpumask);
}

int opal_paffinity_base_get(opal_paffinity_base_cpu_set_t *cpumask)
{
    if (!opal_paffinity_base_selected) {
        if(NULL != cpumask) {
            OPAL_PAFFINITY_CPU_ZERO(*cpumask);
        }
        return OPAL_ERR_NOT_FOUND;
    }
    if(NULL == cpumask) {
        return OPAL_ERR_BAD_PARAM;
    }
    return opal_paffinity_base_module->paff_module_get(cpumask);
}
