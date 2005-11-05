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

#include "opal/class/opal_object.h"
#include "opal/util/output.h"
#include "opal/util/argv.h"
#include "opal/util/trace.h"

#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/gpr/replica/transition_layer/gpr_replica_tl.h"

#include "orte/mca/gpr/replica/functional_layer/gpr_replica_fn.h"


int orte_gpr_replica_find_containers(orte_gpr_replica_segment_t *seg,
                                     orte_gpr_replica_addr_mode_t addr_mode,
                                     orte_gpr_replica_itag_t *taglist, size_t num_tags)
{
    orte_gpr_replica_container_t **cptr;
    size_t i, j, index;
    
    OPAL_TRACE(3);

    /* ensure the search array is clear */
    orte_pointer_array_clear(orte_gpr_replica_globals.srch_cptr);
    orte_gpr_replica_globals.num_srch_cptr = 0;

    cptr = (orte_gpr_replica_container_t**)((seg->containers)->addr);
    for (i=0, j=0; j < seg->num_containers &&
                   i < (seg->containers)->size; i++) {
        if (NULL != cptr[i]) {
            j++;
            if (orte_gpr_replica_check_itag_list(addr_mode,
                                             num_tags, taglist,
                                             cptr[i]->num_itags, cptr[i]->itags)) {
                if (0 > orte_pointer_array_add(&index, orte_gpr_replica_globals.srch_cptr, cptr[i])) {
                    ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                    orte_pointer_array_clear(orte_gpr_replica_globals.srch_cptr);
                    return ORTE_ERR_OUT_OF_RESOURCE;
                }
                (orte_gpr_replica_globals.num_srch_cptr)++;
            }
        }
    }
    return ORTE_SUCCESS;
}


int orte_gpr_replica_create_container(orte_gpr_replica_container_t **cptr,
                                      orte_gpr_replica_segment_t *seg,
                                      size_t num_itags,
                                      orte_gpr_replica_itag_t *itags)
{
    int rc;
    size_t index;
    
    OPAL_TRACE(3);

    *cptr = OBJ_NEW(orte_gpr_replica_container_t);
    if (NULL == *cptr) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    if (ORTE_SUCCESS !=
          (rc = orte_gpr_replica_copy_itag_list(&((*cptr)->itags), itags, num_itags))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(*cptr);
        return rc;
    }
    
    (*cptr)->num_itags = num_itags;
    
    if (0 > orte_pointer_array_add(&index, seg->containers, (void*)(*cptr))) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (seg->num_containers)++;
    
    (*cptr)->index = index;
    return ORTE_SUCCESS;
}


int orte_gpr_replica_release_container(orte_gpr_replica_segment_t *seg,
                                       orte_gpr_replica_container_t *cptr)
{
    orte_gpr_replica_itagval_t **iptr;
    size_t i;
    int rc;
    
    OPAL_TRACE(3);

    /* delete all the itagvals in the container */
    iptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    for (i=0; i < (cptr->itagvals)->size; i++) {
        if (NULL != iptr[i]) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_delete_itagval(seg, cptr, iptr[i]))) {
                ORTE_ERROR_LOG(rc);
                return rc;
            }
        }
    }

    /* remove container from segment and release it */
    i = cptr->index;
    OBJ_RELEASE(cptr);
    orte_pointer_array_set_item(seg->containers, i, NULL);
    (seg->num_containers)--;
    
    /* if the segment is now empty of containers, release it too */
    if (0 == seg->num_containers) {
        if (ORTE_SUCCESS != (rc = orte_gpr_replica_release_segment(&seg))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_add_keyval(orte_gpr_replica_itagval_t **ivalptr,
                                orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_container_t *cptr,
                                orte_gpr_keyval_t *kptr)
{
    orte_gpr_replica_itagval_t *iptr;
    int rc;
    
    OPAL_TRACE(3);

    iptr = OBJ_NEW(orte_gpr_replica_itagval_t);
    if (NULL == iptr) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&(iptr->itag),
                                            seg, kptr->key))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(iptr);
        return rc;
    }
    
    iptr->type = kptr->type;
    if (ORTE_SUCCESS != (rc = orte_gpr_base_xfer_payload(&(iptr->value),
                                               &(kptr->value), kptr->type))) {
        ORTE_ERROR_LOG(rc);
        OBJ_RELEASE(iptr);
        return rc;
    }
    
    if (0 > orte_pointer_array_add(&(iptr->index), cptr->itagvals, (void*)iptr)) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        OBJ_RELEASE(iptr);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    (cptr->num_itagvals)++;
    
    if (0 > (rc = orte_value_array_append_item(&(cptr->itaglist), (void*)(&(iptr->itag))))) {
        ORTE_ERROR_LOG(rc);
        orte_pointer_array_set_item(cptr->itagvals, iptr->index, NULL);
        OBJ_RELEASE(iptr);
        return rc;
    }
    
    *ivalptr = iptr;
    return ORTE_SUCCESS;
}


int orte_gpr_replica_delete_itagval(orte_gpr_replica_segment_t *seg,
                                   orte_gpr_replica_container_t *cptr,
                                   orte_gpr_replica_itagval_t *iptr)
{
    size_t i;
    int rc;
    
    OPAL_TRACE(3);

    /* record that we are going to do this
     * NOTE: it is important that we make the record BEFORE doing the release.
     * The record_action function will do a RETAIN on the object so it
     * doesn't actually get released until we check subscriptions to see
     * if someone wanted to be notified if/when this object was released
     */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_record_action(seg, cptr, iptr,
                            ORTE_GPR_REPLICA_ENTRY_DELETED))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    /* remove the itag value from the container's list */
    for (i=0; i < orte_value_array_get_size(&(cptr->itaglist)); i++) {
        if (iptr->itag == ORTE_VALUE_ARRAY_GET_ITEM(&(cptr->itaglist), orte_gpr_replica_itag_t, i)) {
            orte_value_array_remove_item(&(cptr->itaglist), i);
            goto MOVEON;
        }
    }
    ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
    return ORTE_ERR_NOT_FOUND;
    
MOVEON:
    /* release the data storage */
    i = iptr->index;
    OBJ_RELEASE(iptr);
    
    /* remove the entry from the container's itagval array */
    orte_pointer_array_set_item(cptr->itagvals, i, NULL);
    (cptr->num_itagvals)--;
    
    /* NOTE: If the container is now empty, *don't* remove it here
     * This is cause improper recursion if called from orte_gpr_replica_release_container
     */
    
    return ORTE_SUCCESS;
}


int orte_gpr_replica_update_keyval(orte_gpr_replica_itagval_t **iptr2,
                                   orte_gpr_replica_segment_t *seg,
                                   orte_gpr_replica_container_t *cptr,
                                   orte_gpr_keyval_t *kptr)
{
    size_t i, j, k;
    int rc;
    orte_pointer_array_t *ptr;
    orte_gpr_replica_itagval_t *iptr;

    OPAL_TRACE(3);

    ptr = orte_gpr_replica_globals.srch_ival;
    
    /* record the error value */
    *iptr2 = NULL;
    
    /* for each item in the search array, delete it */
    for (i=0; i < ptr->size; i++) {
        if (NULL != ptr->addr[i]) {
            iptr = (orte_gpr_replica_itagval_t*)ptr->addr[i];
            /* release the data storage */
            j = iptr->index;
            /* DON'T RECORD THE ACTION - THIS WILL PREVENT US FROM SENDING
             * BOTH THE OLD AND THE NEW DATA BACK ON A SUBSCRIPTION
             * REQUEST
             */
            /* remove the itag value from the container's list */
            for (k=0; k < orte_value_array_get_size(&(cptr->itaglist)); k++) {
                if (iptr->itag == ORTE_VALUE_ARRAY_GET_ITEM(&(cptr->itaglist), orte_gpr_replica_itag_t, k)) {
                    orte_value_array_remove_item(&(cptr->itaglist), k);
                    goto MOVEON;
                }
            }
            ORTE_ERROR_LOG(ORTE_ERR_NOT_FOUND);
            return ORTE_ERR_NOT_FOUND;
    
MOVEON:
            OBJ_RELEASE(iptr);
            /* remove the entry from the container's itagval array */
            orte_pointer_array_set_item(cptr->itagvals, j, NULL);
            (cptr->num_itagvals)--;
        }
    }
    
    /* now add new item in their place */
   if (ORTE_SUCCESS != (rc = orte_gpr_replica_add_keyval(&iptr, seg, cptr, kptr))) {
       ORTE_ERROR_LOG(rc);
       return rc;
   }
   
    /* record that we did this */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_record_action(seg, cptr, iptr,
                                    ORTE_GPR_REPLICA_ENTRY_CHANGED |
                                    ORTE_GPR_REPLICA_ENTRY_CHG_TO))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

   
   /* update any storage locations that were pointing to these items */
   if (ORTE_SUCCESS != (rc = orte_gpr_replica_update_storage_locations(iptr))) {
       ORTE_ERROR_LOG(rc);
       return rc;
   }
   
   /* return the location of the new iptr */
   *iptr2 = iptr;
   
   return ORTE_SUCCESS;
}


int orte_gpr_replica_search_container(orte_gpr_replica_addr_mode_t addr_mode,
                                      orte_gpr_replica_itag_t *itags, size_t num_itags,
                                      orte_gpr_replica_container_t *cptr)
{
    orte_gpr_replica_itagval_t **ptr;
    size_t i, j, index;
    
    OPAL_TRACE(3);

    /* ensure the search array is clear */
    orte_pointer_array_clear(orte_gpr_replica_globals.srch_ival);
    orte_gpr_replica_globals.num_srch_ival = 0;
    
    /* check list of itags in container to see if there is a match according
     * to addr_mode spec
     */
    if (orte_gpr_replica_check_itag_list(addr_mode, num_itags, itags,
            orte_value_array_get_size(&(cptr->itaglist)),
            ORTE_VALUE_ARRAY_GET_BASE(&(cptr->itaglist), orte_gpr_replica_itag_t))) {
        /* there is! so now collect those values into the search array */
        ptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
        for (i=0, j=0; j < cptr->num_itagvals &&
                       i < (cptr->itagvals)->size; i++) {
            if (NULL != ptr[i]) {
                j++;
                if (orte_gpr_replica_check_itag_list(ORTE_GPR_REPLICA_OR,
                                                 num_itags, itags,
                                                 1, &(ptr[i]->itag))) {
        
                    if (0 > orte_pointer_array_add(&index, orte_gpr_replica_globals.srch_ival, ptr[i])) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        orte_pointer_array_clear(orte_gpr_replica_globals.srch_ival);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                    (orte_gpr_replica_globals.num_srch_ival)++;
                }
            }
        }
    }
    
    return ORTE_SUCCESS;
}


bool orte_gpr_replica_value_in_container(orte_gpr_replica_container_t *cptr,
                                      orte_gpr_replica_itagval_t *iptr)
{
    orte_gpr_replica_itagval_t **ptr;
    size_t i, j;
    int cmp, rc;

    ptr = (orte_gpr_replica_itagval_t**)((cptr->itagvals)->addr);
    for (i=0, j=0; j < cptr->num_itagvals &&
                   i < (cptr->itagvals)->size; i++) {
        if (NULL != ptr[i]) {
            j++;
            if ((ptr[i]->itag == iptr->itag) && (ptr[i]->type == iptr->type)) {
                if (ORTE_SUCCESS != (rc = orte_gpr_replica_compare_values(&cmp, ptr[i], iptr))) {
                    ORTE_ERROR_LOG(rc);
                    return false;
                }
                if (0 == cmp) return true;
            }
        }
    }
    
    return false;
}


int orte_gpr_replica_get_value(void *value, orte_gpr_replica_itagval_t *ival)
{
    orte_gpr_value_union_t *src;
    
    OPAL_TRACE(3);

    src = &(ival->value);
    
    switch(ival->type) {

        case ORTE_STRING:
            value = strdup(src->strptr);
            break;
            
        case ORTE_SIZE:
            *((size_t*)value) = src->size;
            break;
            
        case ORTE_PID:
            *((pid_t*)value) = src->pid;
            break;
            
        case ORTE_UINT8:
            *((uint8_t*)value) = src->ui8;
            break;
            
        case ORTE_UINT16:
            *((uint16_t*)value) = src->ui16;
            break;
            
        case ORTE_UINT32:
            *((uint32_t*)value) = src->ui32;
            break;
            
#ifdef HAVE_INT64_T
        case ORTE_UINT64:
            *((uint64_t*)value) = src->ui64;
            break;
#endif

        case ORTE_INT8:
            *((int8_t*)value) = src->i8;
            break;
        
        case ORTE_INT16:
            *((int16_t*)value) = src->i16;
            break;
        
        case ORTE_INT32:
            *((int32_t*)value) = src->i32;
            break;
        
#ifdef HAVE_INT64_T
        case ORTE_INT64:
            *((int64_t*)value) = src->i64;
            break;
#endif

        case ORTE_JOBID:
            *((orte_jobid_t*)value) = src->jobid;
            break;
            
        case ORTE_CELLID:
            *((orte_cellid_t*)value) = src->cellid;
            break;
            
        case ORTE_VPID:
            *((orte_vpid_t*)value) = src->vpid;
            break;
            
        case ORTE_NODE_STATE:
            *((orte_node_state_t*)value) = src->node_state;
            break;
            
        case ORTE_PROC_STATE:
            *((orte_proc_state_t*)value) = src->proc_state;
            break;
            
        case ORTE_JOB_STATE:
            *((orte_job_state_t*)value) = src->job_state;
            break;
            
        case ORTE_EXIT_CODE:
            *((orte_exit_code_t*)value) = src->exit_code;
            break;
            
        case ORTE_NULL:
            *((uint8_t*)value) = (uint8_t)NULL;
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
            break;
    }
    return ORTE_SUCCESS;
}


int orte_gpr_replica_compare_values(int *cmp, orte_gpr_replica_itagval_t *ival1,
                                    orte_gpr_replica_itagval_t *ival2)
{
    OPAL_TRACE(3);

    /* sanity check */
    if (ival1->type != ival2->type) {  /* can't compare mismatch */
        ORTE_ERROR_LOG(ORTE_ERR_TYPE_MISMATCH);
        return ORTE_ERR_TYPE_MISMATCH;
    }
    
    switch(ival1->type) {

        case ORTE_STRING:
            *cmp = strcmp(ival1->value.strptr, ival2->value.strptr);
            break;
            
        case ORTE_SIZE:
            if (ival1->value.size == ival2->value.size) {
                *cmp = 0;
            } else if (ival1->value.size < ival2->value.size) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_PID:
            if (ival1->value.pid == ival2->value.pid) {
                *cmp = 0;
            } else if (ival1->value.pid < ival2->value.pid) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_UINT8:
            if (ival1->value.ui8 == ival2->value.ui8) {
                *cmp = 0;
            } else if (ival1->value.ui8 < ival2->value.ui8) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_UINT16:
            if (ival1->value.ui16 == ival2->value.ui16) {
                *cmp = 0;
            } else if (ival1->value.ui16 < ival2->value.ui16) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_UINT32:
            if (ival1->value.ui32 == ival2->value.ui32) {
                *cmp = 0;
            } else if (ival1->value.ui32 < ival2->value.ui32) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
#ifdef HAVE_INT64_T
        case ORTE_UINT64:
            if (ival1->value.ui64 == ival2->value.ui64) {
                *cmp = 0;
            } else if (ival1->value.ui64 < ival2->value.ui64) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_INT64:
            if (ival1->value.i64 == ival2->value.i64) {
                *cmp = 0;
            } else if (ival1->value.i64 < ival2->value.i64) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
#endif
        case ORTE_INT8:
            if (ival1->value.i8 == ival2->value.i8) {
                *cmp = 0;
            } else if (ival1->value.i8 < ival2->value.i8) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_INT16:
            if (ival1->value.i16 == ival2->value.i16) {
                *cmp = 0;
            } else if (ival1->value.i16 < ival2->value.i16) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_INT32:
            if (ival1->value.i32 == ival2->value.i32) {
                *cmp = 0;
            } else if (ival1->value.i32 < ival2->value.i32) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_JOBID:
            if (ival1->value.jobid == ival2->value.jobid) {
                *cmp = 0;
            } else if (ival1->value.jobid < ival2->value.jobid) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
           
        case ORTE_CELLID:
            if (ival1->value.cellid == ival2->value.cellid) {
                *cmp = 0;
            } else if (ival1->value.cellid < ival2->value.cellid) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_VPID:
            if (ival1->value.vpid == ival2->value.vpid) {
                *cmp = 0;
            } else if (ival1->value.vpid < ival2->value.vpid) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_NODE_STATE:
            if (ival1->value.node_state == ival2->value.node_state) {
                *cmp = 0;
            } else if (ival1->value.node_state < ival2->value.node_state) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_PROC_STATE:
            if (ival1->value.proc_state == ival2->value.proc_state) {
                *cmp = 0;
            } else if (ival1->value.proc_state < ival2->value.proc_state) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_JOB_STATE:
            if (ival1->value.job_state == ival2->value.job_state) {
                *cmp = 0;
            } else if (ival1->value.job_state < ival2->value.job_state) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_EXIT_CODE:
            if (ival1->value.exit_code == ival2->value.exit_code) {
                *cmp = 0;
            } else if (ival1->value.exit_code < ival2->value.exit_code) {
                *cmp = -1;
            } else {
                *cmp = 1;
            }
            break;
            
        case ORTE_NULL:
            *cmp = 0;
            break;
            
        default:
            return ORTE_ERR_BAD_PARAM;
            break;
    }
    return ORTE_SUCCESS;
}


int orte_gpr_replica_release_segment(orte_gpr_replica_segment_t **seg)
{
    int rc;
    size_t i;
    
    OPAL_TRACE(3);

    i = (*seg)->itag;
    OBJ_RELEASE(*seg);
    
    if (0 > (rc = orte_pointer_array_set_item(orte_gpr_replica.segments, i, NULL))) {
        return rc;
    }
    (orte_gpr_replica.num_segs)--;
    
    return ORTE_SUCCESS;
}

int orte_gpr_replica_purge_itag(orte_gpr_replica_segment_t *seg,
                                orte_gpr_replica_itag_t itag)
{
    OPAL_TRACE(3);

     /*
     * Begin by looping through the segment's containers and check
     * their descriptions first - if removing this name leaves that
     * list empty, then remove the container.
     * If the container isn't to be removed, then loop through all
     * the container's keyvalue pairs and check the "key" - if
     * it matches, then remove that pair. If all pairs are removed,
     * then remove the container
     * */

    return ORTE_SUCCESS;
}
