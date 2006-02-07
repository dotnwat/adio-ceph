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
/** @file:
 *
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "include/orte_constants.h"
#include "include/orte_types.h"
#include "dss/dss.h"
#include "mca/errmgr/errmgr.h"
#include "opal/util/output.h"

#include "mca/gpr/base/base.h"

int orte_gpr_base_print_dump(orte_buffer_t *buffer, int output_id)
{
    char *line;
    size_t n;
    orte_data_type_t type;
    int rc;

    n = 1;
    while (ORTE_SUCCESS == orte_dss.peek(buffer, &type, &n)) {
       if (ORTE_SUCCESS !=
           (rc = orte_dss.unpack(buffer, &line, &n, ORTE_STRING))) {
           ORTE_ERROR_LOG(rc);
           return rc;
       }
       opal_output(output_id, "%s", line);
       free(line);
       n=1;
    }

    return ORTE_SUCCESS;
}
