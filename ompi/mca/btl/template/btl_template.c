/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2008 The University of Tennessee and The University
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
#include <string.h>
#include "opal/class/opal_bitmap.h"
#include "ompi/mca/btl/btl.h"

#include "btl_template.h"
#include "btl_template_frag.h" 
#include "btl_template_proc.h"
#include "btl_template_endpoint.h"
#include "opal/datatype/opal_convertor.h" 
#include "ompi/mca/mpool/base/base.h" 
#include "ompi/mca/mpool/mpool.h" 


mca_btl_template_module_t mca_btl_template_module = {
    {
        &mca_btl_template_component.super,
        0, /* max size of first fragment */
        0, /* min send fragment size */
        0, /* max send fragment size */
        0, /* rdma pipeline offset */
        0, /* rdma pipeline frag size */
        0, /* min rdma pipeline size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        0, /* flags */
        mca_btl_template_add_procs,
        mca_btl_template_del_procs,
        mca_btl_template_register,
        mca_btl_template_finalize,
        mca_btl_template_alloc, 
        mca_btl_template_free, 
        mca_btl_template_prepare_src,
        mca_btl_template_prepare_dst,
        mca_btl_template_send,
        NULL, /* send immediate */
        mca_btl_template_put,
        NULL, /* get */ 
        NULL, /*dump */
        NULL, /* mpool */
        NULL, /* register error cb */
        mca_btl_template_ft_event
    }
};

/**
 *
 */

int mca_btl_template_add_procs(
    struct mca_btl_base_module_t* btl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_procs, 
    struct mca_btl_base_endpoint_t** peers, 
    opal_bitmap_t* reachable)
{
    mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*)btl;
    int i, rc;

    for(i = 0; i < (int) nprocs; i++) {

        struct ompi_proc_t* ompi_proc = ompi_procs[i];
        mca_btl_template_proc_t* template_proc;
        mca_btl_base_endpoint_t* template_endpoint;

#if 0
        /* If the BTL doesn't support heterogeneous operations, be
           sure to say we can't reach that proc */
        if (ompi_proc_local()->proc_arch != ompi_proc->proc_arch) {
            continue;
        }
#endif

        if(NULL == (template_proc = mca_btl_template_proc_create(ompi_proc))) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        /*
         * Check to make sure that the peer has at least as many interface 
         * addresses exported as we are trying to use. If not, then 
         * don't bind this PTL instance to the proc.
         */

        OPAL_THREAD_LOCK(&template_proc->proc_lock);

        /* The btl_proc datastructure is shared by all TEMPLATE PTL
         * instances that are trying to reach this destination. 
         * Cache the peer instance on the btl_proc.
         */
        template_endpoint = OBJ_NEW(mca_btl_template_endpoint_t);
        if(NULL == template_endpoint) {
            OPAL_THREAD_UNLOCK(&template_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        template_endpoint->endpoint_btl = template_btl;
        rc = mca_btl_template_proc_insert(template_proc, template_endpoint);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(template_endpoint);
            OPAL_THREAD_UNLOCK(&template_proc->proc_lock);
            continue;
        }

        opal_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&template_proc->proc_lock);
        peers[i] = template_endpoint;
    }

    return OMPI_SUCCESS;
}

int mca_btl_template_del_procs(struct mca_btl_base_module_t* btl, 
        size_t nprocs, 
        struct ompi_proc_t **procs, 
        struct mca_btl_base_endpoint_t ** peers)
{
    /* TODO */
    return OMPI_SUCCESS;
}


/**
 * Register callback function to support send/recv semantics
 */

int mca_btl_template_register(
                        struct mca_btl_base_module_t* btl, 
                        mca_btl_base_tag_t tag, 
                        mca_btl_base_module_recv_cb_fn_t cbfunc, 
                        void* cbdata)
{
    return OMPI_SUCCESS;
}


/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
 */

mca_btl_base_descriptor_t* mca_btl_template_alloc(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    uint8_t order,
    size_t size,
    uint32_t flags)
{
    mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*) btl; 
    mca_btl_template_frag_t* frag = NULL;
    int rc;
    
    if(size <= btl->btl_eager_limit){ 
        MCA_BTL_TEMPLATE_FRAG_ALLOC_EAGER(template_btl, frag, rc); 
    } else { 
        MCA_BTL_TEMPLATE_FRAG_ALLOC_MAX(template_btl, frag, rc); 
    }
    if( OPAL_UNLIKELY(NULL != frag) ) {
        return NULL;
    }
    
    frag->segment.seg_len = size;
    frag->base.des_flags = 0; 
    return (mca_btl_base_descriptor_t*)frag;
}


/**
 * Return a segment
 */

int mca_btl_template_free(
    struct mca_btl_base_module_t* btl, 
    mca_btl_base_descriptor_t* des) 
{
    mca_btl_template_frag_t* frag = (mca_btl_template_frag_t*)des; 
    if(frag->size == 0) {
#if MCA_BTL_HAS_MPOOL
        OBJ_RELEASE(frag->registration);
#endif
        MCA_BTL_TEMPLATE_FRAG_RETURN_USER(btl, frag); 
    } else if(frag->size == btl->btl_eager_limit){ 
        MCA_BTL_TEMPLATE_FRAG_RETURN_EAGER(btl, frag); 
    } else if(frag->size == btl->btl_max_send_size) {
        MCA_BTL_TEMPLATE_FRAG_RETURN_EAGER(btl, frag); 
    }  else {
        return OMPI_ERR_BAD_PARAM;
    }
    return OMPI_SUCCESS; 
}

/**
 * Pack data and return a descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
mca_btl_base_descriptor_t* mca_btl_template_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_mpool_base_registration_t* registration,
    struct opal_convertor_t* convertor,
    uint8_t order,
    size_t reserve,
    size_t* size,
    uint32_t flags
)
{
    mca_btl_template_frag_t* frag;
    struct iovec iov;
    uint32_t iov_count = 1;
    size_t max_data = *size;
    int rc;
                                                                                                    

    /*
     * if we aren't pinning the data and the requested size is less
     * than the eager limit pack into a fragment from the eager pool
    */
    if (max_data+reserve <= btl->btl_eager_limit) {

        MCA_BTL_TEMPLATE_FRAG_ALLOC_EAGER(btl, frag, rc);
        if(OPAL_UNLIKELY(NULL == frag)) {
            return NULL;
        }

        iov.iov_len = max_data;
        iov.iov_base = (unsigned char*) frag->segment.seg_addr.pval + reserve;

        rc = opal_convertor_pack(convertor, &iov, &iov_count, &max_data );
        *size  = max_data;
        if( rc < 0 ) {
            MCA_BTL_TEMPLATE_FRAG_RETURN_EAGER(btl, frag);
            return NULL;
        }
        frag->segment.seg_len = max_data + reserve;
    }

    /* 
     * otherwise pack as much data as we can into a fragment
     * that is the max send size.
     */
    else {
                                                                                                    
        MCA_BTL_TEMPLATE_FRAG_ALLOC_MAX(btl, frag, rc);
        if(OPAL_UNLIKELY(NULL == frag)) {
            return NULL;
        }
        if(max_data + reserve > frag->size){
            max_data = frag->size - reserve;
        }
        iov.iov_len = max_data;
        iov.iov_base = (unsigned char*) frag->segment.seg_addr.pval + reserve;
                                                                                                    
        rc = opal_convertor_pack(convertor, &iov, &iov_count, &max_data );
        *size  = max_data;
                                                                                                    
        if( rc < 0 ) {
            MCA_BTL_TEMPLATE_FRAG_RETURN_MAX(btl, frag);
            return NULL;
        }
        frag->segment.seg_len = max_data + reserve;
    }

    frag->base.des_src = &frag->segment;
    frag->base.des_src_cnt = 1;
    frag->base.des_dst = NULL;
    frag->base.des_dst_cnt = 0;
    frag->base.des_flags = 0;
    return &frag->base;
}


/**
 * Prepare a descriptor for send/rdma using the supplied
 * convertor. If the convertor references data that is contigous,
 * the descriptor may simply point to the user buffer. Otherwise,
 * this routine is responsible for allocating buffer space and
 * packing if required.
 *
 * @param btl (IN)          BTL module
 * @param endpoint (IN)     BTL peer addressing
 * @param convertor (IN)    Data type convertor
 * @param reserve (IN)      Additional bytes requested by upper layer to precede user data
 * @param size (IN/OUT)     Number of bytes to prepare (IN), number of bytes actually prepared (OUT)
 */

mca_btl_base_descriptor_t* mca_btl_template_prepare_dst(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_mpool_base_registration_t* registration,
    struct opal_convertor_t* convertor,
    uint8_t order,
    size_t reserve,
    size_t* size,
    uint32_t flags)
{
    mca_btl_template_frag_t* frag;
    int rc;

    MCA_BTL_TEMPLATE_FRAG_ALLOC_USER(btl, frag, rc);
    if(OPAL_UNLIKELY(NULL == frag)) {
        return NULL;
    }

    frag->segment.seg_len = *size;
    opal_convertor_get_current_pointer( convertor, (void**)&(frag->segment.seg_addr.pval) );

    frag->base.des_src = NULL;
    frag->base.des_src_cnt = 0;
    frag->base.des_dst = &frag->segment;
    frag->base.des_dst_cnt = 1;
    frag->base.des_flags = 0;
    return &frag->base;
}


/**
 * Initiate an asynchronous send.
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transfered
 * @param tag (IN)         The tag value used to notify the peer.
 */

int mca_btl_template_send( 
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_btl_base_descriptor_t* descriptor, 
    mca_btl_base_tag_t tag)
   
{
    /* mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*) btl; */
    mca_btl_template_frag_t* frag = (mca_btl_template_frag_t*)descriptor; 
    frag->endpoint = endpoint; 
    /* TODO */
    return OMPI_ERR_NOT_IMPLEMENTED;
}


/**
 * Initiate an asynchronous put.
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transferred
 */

int mca_btl_template_put( 
    mca_btl_base_module_t* btl,
    mca_btl_base_endpoint_t* endpoint,
    mca_btl_base_descriptor_t* descriptor)
{
    /* mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*) btl; */
    mca_btl_template_frag_t* frag = (mca_btl_template_frag_t*) descriptor; 
    frag->endpoint = endpoint;
    /* TODO */
    return OMPI_ERR_NOT_IMPLEMENTED; 
}


/**
 * Initiate an asynchronous get.
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transferred
 *
 */

int mca_btl_template_get( 
    mca_btl_base_module_t* btl,
    mca_btl_base_endpoint_t* endpoint,
    mca_btl_base_descriptor_t* descriptor)
{
    /* mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*) btl; */
    mca_btl_template_frag_t* frag = (mca_btl_template_frag_t*) descriptor; 
    frag->endpoint = endpoint;
    /* TODO */
    return OMPI_ERR_NOT_IMPLEMENTED; 
}


/*
 * Cleanup/release module resources.
 */

int mca_btl_template_finalize(struct mca_btl_base_module_t* btl)
{
    mca_btl_template_module_t* template_btl = (mca_btl_template_module_t*) btl; 
    OBJ_DESTRUCT(&template_btl->template_lock);
    OBJ_DESTRUCT(&template_btl->template_frag_eager);
    OBJ_DESTRUCT(&template_btl->template_frag_max);
    OBJ_DESTRUCT(&template_btl->template_frag_user);
    free(template_btl);
    return OMPI_SUCCESS;
}

int mca_btl_template_ft_event(int state) {
    if(OPAL_CRS_CHECKPOINT == state) {
        ;
    }
    else if(OPAL_CRS_CONTINUE == state) {
        ;
    }
    else if(OPAL_CRS_RESTART == state) {
        ;
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }

    return OMPI_SUCCESS;
}
