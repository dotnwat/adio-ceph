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
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "opal/util/trace.h"

#include "orte/mca/gpr/replica/transition_layer/gpr_replica_tl.h"

int orte_gpr_replica_find_seg(orte_gpr_replica_segment_t **seg,
                              bool create, char *segment)
{
    size_t len;
    int rc=ORTE_SUCCESS;
    size_t i, cntri;
    orte_gpr_replica_segment_t **ptr;

    OPAL_TRACE(3);

    /* initialize to nothing */
    *seg = NULL;
    
    len = strlen(segment);

    /* search the registry segments to find which one is being referenced */
    ptr = (orte_gpr_replica_segment_t**)(orte_gpr_replica.segments->addr);
    cntri = 0;
    for (i=0; cntri < orte_gpr_replica.num_segs &&
              i < (orte_gpr_replica.segments)->size; i++) {
        if (NULL != ptr[i]) {
            cntri++;
            if (0 == strncmp(segment, ptr[i]->name, len)) {
                *seg = ptr[i];
                return ORTE_SUCCESS;
            }
        }
    }
    
    if (!create) {
        /* couldn't find it and don't want it created - just return NULL */
        return ORTE_ERR_BAD_PARAM;
    }
    
    /* add the segment to the registry */
    *seg = OBJ_NEW(orte_gpr_replica_segment_t);
    (*seg)->name = strdup(segment);
    if (0 > orte_pointer_array_add(&i, orte_gpr_replica.segments, (void*)(*seg))) {
        OBJ_RELEASE(*seg);
        return rc;
    }
    (*seg)->itag = i;
    (orte_gpr_replica.num_segs)++;
    
    return ORTE_SUCCESS;
}
