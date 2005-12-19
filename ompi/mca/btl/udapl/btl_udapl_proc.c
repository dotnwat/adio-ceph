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

#include "ompi_config.h"

#include "class/opal_hash_table.h"
#include "mca/pml/base/pml_base_module_exchange.h"

#include "btl_udapl.h"
#include "btl_udapl_proc.h"

static void mca_btl_udapl_proc_construct(mca_btl_udapl_proc_t* proc);
static void mca_btl_udapl_proc_destruct(mca_btl_udapl_proc_t* proc);

OBJ_CLASS_INSTANCE(mca_btl_udapl_proc_t, 
        opal_list_item_t, mca_btl_udapl_proc_construct, 
        mca_btl_udapl_proc_destruct);

void mca_btl_udapl_proc_construct(mca_btl_udapl_proc_t* proc)
{
    proc->proc_ompi = 0;
    proc->proc_addr_count = 0;
    proc->proc_endpoints = 0;
    proc->proc_endpoint_count = 0;
    OBJ_CONSTRUCT(&proc->proc_lock, opal_mutex_t);
    /* add to list of all proc instance */
    OPAL_THREAD_LOCK(&mca_btl_udapl_component.udapl_lock);
    opal_list_append(&mca_btl_udapl_component.udapl_procs, &proc->super);
    OPAL_THREAD_UNLOCK(&mca_btl_udapl_component.udapl_lock);
}

/*
 * Cleanup uDAPL proc instance
 */

void mca_btl_udapl_proc_destruct(mca_btl_udapl_proc_t* proc)
{
    /* remove from list of all proc instances */
    OPAL_THREAD_LOCK(&mca_btl_udapl_component.udapl_lock);
    opal_list_remove_item(&mca_btl_udapl_component.udapl_procs, &proc->super);
    OPAL_THREAD_UNLOCK(&mca_btl_udapl_component.udapl_lock);

    /* release resources */
    if(NULL != proc->proc_endpoints) {
        free(proc->proc_endpoints);
    }
}


/*
 * Look for an existing uDAPL process instances based on the associated
 * ompi_proc_t instance.
 */
static mca_btl_udapl_proc_t* mca_btl_udapl_proc_lookup_ompi(ompi_proc_t* ompi_proc)
{
    mca_btl_udapl_proc_t* udapl_proc;

    OPAL_THREAD_LOCK(&mca_btl_udapl_component.udapl_lock);

    for(udapl_proc = (mca_btl_udapl_proc_t*)
            opal_list_get_first(&mca_btl_udapl_component.udapl_procs);
            udapl_proc != (mca_btl_udapl_proc_t*)
            opal_list_get_end(&mca_btl_udapl_component.udapl_procs);
            udapl_proc  = (mca_btl_udapl_proc_t*)opal_list_get_next(udapl_proc)) {

        if(udapl_proc->proc_ompi == ompi_proc) {
            OPAL_THREAD_UNLOCK(&mca_btl_udapl_component.udapl_lock);
            return udapl_proc;
        }

    }

    OPAL_THREAD_UNLOCK(&mca_btl_udapl_component.udapl_lock);

    return NULL;
}

/*
 * Create a uDAPL process structure. There is a one-to-one correspondence
 * between a ompi_proc_t and a mca_btl_udapl_proc_t instance. We cache
 * additional data (specifically the list of mca_btl_udapl_endpoint_t instances, 
 * and published addresses) associated w/ a given destination on this
 * datastructure.
 */

mca_btl_udapl_proc_t* mca_btl_udapl_proc_create(ompi_proc_t* ompi_proc)
{
    mca_btl_udapl_proc_t* module_proc = NULL;

    /* Check if we have already created a uDAPL proc
     * structure for this ompi process */
    module_proc = mca_btl_udapl_proc_lookup_ompi(ompi_proc);

    if(module_proc != NULL) {

        /* Gotcha! */
        return module_proc;
    }

    /* Oops! First time, gotta create a new uDAPL proc
     * out of the ompi_proc ... */

    module_proc = OBJ_NEW(mca_btl_udapl_proc_t);

    /* Initialize number of peer */
    module_proc->proc_endpoint_count = 0;

    module_proc->proc_ompi = ompi_proc;

    /* build a unique identifier (of arbitrary
     * size) to represent the proc */
    module_proc->proc_guid = ompi_proc->proc_name;

    /* uDAPL module doesn't have addresses exported at
     * initialization, so the addr_count is set to one. */
    module_proc->proc_addr_count = 1;

    /* XXX: Right now, there can be only 1 peer associated
     * with a proc. Needs a little bit change in 
     * mca_btl_udapl_proc_t to allow on demand increasing of
     * number of endpoints for this proc */

    module_proc->proc_endpoints = (mca_btl_base_endpoint_t**)
        malloc(module_proc->proc_addr_count * sizeof(mca_btl_base_endpoint_t*));

    if(NULL == module_proc->proc_endpoints) {
        OBJ_RELEASE(module_proc);
        return NULL;
    }
    return module_proc;
}


/*
 * Note that this routine must be called with the lock on the process
 * already held.  Insert a btl instance into the proc array and assign 
 * it an address.
 */
int mca_btl_udapl_proc_insert(mca_btl_udapl_proc_t* module_proc, 
        mca_btl_base_endpoint_t* module_endpoint)
{
    /* insert into endpoint array */
    module_endpoint->endpoint_proc = module_proc;
    module_proc->proc_endpoints[module_proc->proc_endpoint_count++] = module_endpoint;

    return OMPI_SUCCESS;
}
