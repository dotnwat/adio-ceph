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
/** @file:
 *
 * The Open MPI general purpose registry - support functions.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "class/orte_pointer_array.h"
#include "opal/util/output.h"
#include "mca/errmgr/errmgr.h"

#include "mca/gpr/replica/functional_layer/gpr_replica_fn.h"

#include "gpr_replica_tl.h"

int
orte_gpr_replica_create_itag(orte_gpr_replica_itag_t *itag,
                             orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t **ptr, *new_dict;
    orte_gpr_replica_itag_t j;
    size_t i, len, len2;

    /* default to illegal value */
    *itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    /* if name or seg is NULL, error */
    if (NULL == name || NULL == seg) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }

    len = strlen(name);
    
    /* check seg's dictionary to ensure uniqueness */
    ptr = (orte_gpr_replica_dict_t**)(seg->dict)->addr;
    for (i=0, j=0; j < seg->num_dict_entries &&
                   i < (seg->dict)->size; i++) {
        if (NULL != ptr[i]) {
            j++;
            len2 = strlen(ptr[i]->entry);
            if ((len == len2 && 0 == strncmp(ptr[i]->entry, name, len))) {
                /* already present */
                *itag = ptr[i]->itag;
                return ORTE_SUCCESS;
            }
        }
    }

    /* okay, name is unique - create dictionary entry */
    
    /* first check to see if one is available */
    if (ORTE_GPR_REPLICA_ITAG_MAX-1 < seg->num_dict_entries) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    
    new_dict = (orte_gpr_replica_dict_t*)malloc(sizeof(orte_gpr_replica_dict_t));
    if (NULL == new_dict) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }
    new_dict->entry = strdup(name);
    if (0 > orte_pointer_array_add(&(new_dict->index), seg->dict, (void*)new_dict)) {
        *itag = ORTE_GPR_REPLICA_ITAG_MAX;
        free(new_dict->entry);
        free(new_dict);
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    *itag = seg->num_dict_entries;
    new_dict->itag = *itag;
    (seg->num_dict_entries)++;

    return ORTE_SUCCESS;
}


int orte_gpr_replica_delete_itag(orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t **ptr;
    orte_gpr_replica_itag_t itag;
    size_t index;
    int rc;

    /* check for errors */
    if (NULL == name || NULL == seg) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }

    /* find dictionary element to delete */
    if (ORTE_SUCCESS != (rc = orte_gpr_replica_dict_lookup(&itag, seg, name))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* found name in dictionary */
    /* need to search this segment's registry to find all instances
     * that name & delete them
     */
     if (ORTE_SUCCESS != (rc = orte_gpr_replica_purge_itag(seg, itag))) {
        ORTE_ERROR_LOG(rc);
        return rc;
     }

     /* free the dictionary element data */
     ptr = (orte_gpr_replica_dict_t**)((seg->dict)->addr);
     if (NULL == ptr[itag]) {  /* dict element no longer valid */
         return ORTE_ERR_NOT_FOUND;
     }
     index = ptr[itag]->index;
     if (NULL != ptr[itag]->entry) {
         free(ptr[itag]->entry);
     }
     free(ptr[itag]);
     
     /* remove itag from segment dictionary */
    orte_pointer_array_set_item(seg->dict, index, NULL);
    
    /* decrease the dict counter */
    (seg->num_dict_entries)--;
    
    return ORTE_SUCCESS;
}


int
orte_gpr_replica_dict_lookup(orte_gpr_replica_itag_t *itag,
                             orte_gpr_replica_segment_t *seg, char *name)
{
    orte_gpr_replica_dict_t **ptr;
    size_t i;
    orte_gpr_replica_itag_t j;
    size_t len, len2;
    
    /* initialize to illegal value */
    *itag = ORTE_GPR_REPLICA_ITAG_MAX;
    
    /* protect against error */
    if (NULL == seg) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (NULL == name) { /* just want segment token-itag pair */
        *itag = seg->itag;
        return ORTE_SUCCESS;
	}

    len = strlen(name);
    
    /* want specified token-itag pair in that segment's dictionary */
    ptr = (orte_gpr_replica_dict_t**)((seg->dict)->addr);
    for (i=0, j=0; j < seg->num_dict_entries &&
                   i < (seg->dict)->size; i++) {
        if (NULL != ptr[i]) {
            j++;
            len2 = strlen(ptr[i]->entry);
    	       if (len == len2 && 0 == strncmp(ptr[i]->entry, name, len)) {
                *itag = ptr[i]->itag;
                return ORTE_SUCCESS;
            }
        }
	}

    return ORTE_ERR_NOT_FOUND; /* couldn't find the specified entry */
}


int orte_gpr_replica_dict_reverse_lookup(char **name,
        orte_gpr_replica_segment_t *seg, orte_gpr_replica_itag_t itag)
{
    orte_gpr_replica_dict_t **ptr;
    orte_gpr_replica_segment_t **segptr;
    size_t i;
    orte_gpr_replica_itag_t j;


    /* initialize to nothing */
    *name = NULL;

    /* protect against error (shouldn't happen) */
    if (ORTE_GPR_REPLICA_ITAG_MAX == itag) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    if (NULL == seg) {
	   /* return the segment name
        * note that itag is the index of the segment in that array
        */
        segptr = (orte_gpr_replica_segment_t**)(orte_gpr_replica.segments->addr);
        if (NULL == segptr[itag]) { /* this segment is no longer alive */
            return ORTE_ERR_NOT_FOUND;
        }
	   *name = strdup(segptr[itag]->name);
	   return ORTE_SUCCESS;
    }

    /* seg is provided - find the matching token for this itag
     * note again that itag is the index into this segment's
     * dictionary array
     */
    ptr = (orte_gpr_replica_dict_t**)((seg->dict)->addr);
    for (i=0, j=0; j < seg->num_dict_entries &&
                   i < (seg->dict)->size; i++) {
        if (NULL != ptr[i]) {
            j++;
            if (itag == ptr[i]->itag) { /* entry found! */
                *name = strdup(ptr[i]->entry);
                return ORTE_SUCCESS;
            }
        }
    }
    /* get here if entry not found */
    
    return ORTE_ERR_NOT_FOUND;
}

int
orte_gpr_replica_get_itag_list(orte_gpr_replica_itag_t **itaglist,
                    orte_gpr_replica_segment_t *seg, char **names,
                    size_t *num_names)
{
    char **namptr;
    int rc;
    size_t i;

    *itaglist = NULL;

    /* check for errors */
    if (NULL == seg) {
        ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
        return ORTE_ERR_BAD_PARAM;
    }
    
    /* check for wild-card case */
    if (NULL == names) {
	   return ORTE_SUCCESS;
    }

    if (0 >= (*num_names)) { /* NULL-terminated list - count them */
        *num_names = 0;
        namptr = names;
        while (NULL != *namptr) {
	       *num_names = (*num_names) + 1;
	       namptr++;
        }
    }

    *itaglist = (orte_gpr_replica_itag_t*)malloc((*num_names)*sizeof(orte_gpr_replica_itag_t));
    if (NULL == *itaglist) {
        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
        return ORTE_ERR_OUT_OF_RESOURCE;
    }

    namptr = names;

    for (i=0; i < (*num_names); i++) {  /* traverse array of names - ignore any NULL's */
        if (NULL != names[i]) {
            if (ORTE_SUCCESS != (rc = orte_gpr_replica_create_itag(&((*itaglist)[i]), seg, names[i]))) {
                ORTE_ERROR_LOG(rc);
                free(*itaglist);
                *itaglist = NULL;
                return rc;
            }
        }
    }
    return ORTE_SUCCESS;
}

