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
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include "opal/util/if.h"

#include "btl_mx.h"
#include "btl_mx_frag.h" 
#include "btl_mx_proc.h"
#include "btl_mx_endpoint.h"
#include "ompi/datatype/convertor.h" 
#include "opal/prefetch.h"

/**
 *
 */
int mca_btl_mx_add_procs( struct mca_btl_base_module_t* btl, 
                          size_t nprocs, 
                          struct ompi_proc_t** ompi_procs, 
                          struct mca_btl_base_endpoint_t** peers, 
                          ompi_bitmap_t* reachable )
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*)btl;
    int i, rc;

    for( i = 0; i < (int) nprocs; i++ ) {

        struct ompi_proc_t* ompi_proc = ompi_procs[i];
        mca_btl_mx_proc_t* mx_proc;
        mca_btl_base_endpoint_t* mx_endpoint;

        /**
         * By default don't allow communications with self nor with any
         * other processes on the same node. The BTL self and sm are
         * supposed to take care of such communications.
         */
        if( ompi_procs[i]->proc_flags & OMPI_PROC_FLAG_LOCAL ) {
            if( ompi_procs[i] == ompi_proc_local_proc ) {
                if( 0 == mca_btl_mx_component.mx_support_self )
                    continue;
            } else {
                if( 0 == mca_btl_mx_component.mx_support_sharedmem )
                    continue;
            }
        }
        if( NULL == (mx_proc = mca_btl_mx_proc_create(ompi_proc)) ) {
            continue;
        }

        OPAL_THREAD_LOCK(&mx_proc->proc_lock);

        /* The btl_proc datastructure is shared by all MX BTL
         * instances that are trying to reach this destination. 
         * Cache the peer instance on the btl_proc.
         */
        mx_endpoint = OBJ_NEW(mca_btl_mx_endpoint_t);
        if(NULL == mx_endpoint) {
            OPAL_THREAD_UNLOCK(&mx_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        mx_endpoint->endpoint_btl = mx_btl;
        rc = mca_btl_mx_proc_insert( mx_proc, mx_endpoint );
        if( rc != OMPI_SUCCESS ) {
            OBJ_RELEASE(mx_endpoint);
            OBJ_RELEASE(mx_proc);
            OPAL_THREAD_UNLOCK(&mx_proc->proc_lock);
            continue;
        }
        ompi_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&mx_proc->proc_lock);
        peers[i] = mx_endpoint;
    }

    return OMPI_SUCCESS;
}

int mca_btl_mx_del_procs( struct mca_btl_base_module_t* btl, 
                          size_t nprocs, 
                          struct ompi_proc_t** procs, 
                          struct mca_btl_base_endpoint_t** peers )
{
    /* TODO */
    return OMPI_SUCCESS;
}


/**
 * Register callback function to support send/recv semantics
 */

int mca_btl_mx_register( struct mca_btl_base_module_t* btl, 
                         mca_btl_base_tag_t tag, 
                         mca_btl_base_module_recv_cb_fn_t cbfunc, 
                         void* cbdata )
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*) btl; 

    mx_btl->mx_reg[tag].cbfunc = cbfunc; 
    mx_btl->mx_reg[tag].cbdata = cbdata; 

    if( (NULL != cbfunc) && ( 0 == mca_btl_mx_component.mx_use_unexpected) ) {
        mca_btl_mx_frag_t* frag;
        mx_return_t mx_return;
        mx_segment_t mx_segment;
        int i, rc;
        /* Post the receives if there is no unexpected handler */
        for( i = 0; i < mca_btl_mx_component.mx_max_posted_recv; i++ ) {
            MCA_BTL_MX_FRAG_ALLOC_EAGER( mx_btl, frag, rc );
            if( NULL == frag ) {
                opal_output( 0, "mca_btl_mx_register: unable to allocate more eager fragments\n" );
                if( 0 == i ) {
                    return OMPI_ERROR;
                }
                break;  /* some fragments are already registered. Try to continue... */
            }
            frag->base.des_dst     = frag->segment;
            frag->base.des_dst_cnt = 1;
            frag->base.des_src     = NULL;
            frag->base.des_src_cnt = 0;
            frag->mx_frag_list     = NULL;
            frag->tag              = tag;
            
            mx_segment.segment_ptr    = (void*)(frag+1);
            mx_segment.segment_length = mx_btl->super.btl_eager_limit;
            mx_return = mx_irecv( mx_btl->mx_endpoint, &mx_segment, 1, (uint64_t)tag,
                                  BTL_MX_RECV_MASK,
                                  frag, &(frag->mx_request) );
            if( MX_SUCCESS != mx_return ) {
                opal_output( 0, "mca_btl_mx_register: mx_irecv failed with status %d (%s)\n",
                             mx_return, mx_strerror(mx_return) );
                MCA_BTL_MX_FRAG_RETURN( mx_btl, frag );
                return OMPI_ERROR;
            }
        }
    }

    return OMPI_SUCCESS;
}


/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
 */

mca_btl_base_descriptor_t* mca_btl_mx_alloc( struct mca_btl_base_module_t* btl,
                                             size_t size )
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*) btl; 
    mca_btl_mx_frag_t* frag;
    int rc;
    
    MCA_BTL_MX_FRAG_ALLOC_EAGER(mx_btl, frag, rc);
    if( OPAL_UNLIKELY(NULL == frag) ) {
        return NULL;
    }
    frag->segment[0].seg_len = 
        size <= mx_btl->super.btl_eager_limit ? 
        size : mx_btl->super.btl_eager_limit ;
    frag->segment[0].seg_addr.pval = (void*)(frag+1);
    frag->base.des_src = frag->segment;
    frag->base.des_src_cnt = 1;
    frag->base.des_flags = 0;
    return (mca_btl_base_descriptor_t*)frag;
}


/**
 * Return a segment
 */
int mca_btl_mx_free( struct mca_btl_base_module_t* btl, 
                     mca_btl_base_descriptor_t* des )
{
    mca_btl_mx_frag_t* frag = (mca_btl_mx_frag_t*)des; 

    assert( 0xff == frag->tag );
    MCA_BTL_MX_FRAG_RETURN(btl, frag);

    return OMPI_SUCCESS; 
}

/**
 * Pack data and return a descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
mca_btl_base_descriptor_t*
mca_btl_mx_prepare_src( struct mca_btl_base_module_t* btl,
                        struct mca_btl_base_endpoint_t* endpoint,
                        struct mca_mpool_base_registration_t* registration,
                        struct ompi_convertor_t* convertor,
                        size_t reserve,
                        size_t* size )
{
    mca_btl_mx_frag_t* frag;
    struct iovec iov;
    uint32_t iov_count = 1;
    size_t max_data;
    int rc;

    max_data = btl->btl_eager_limit - reserve;
    if( (*size) < max_data ) {
        max_data = *size;
    }
    /* If the data is contiguous we can use directly the pointer
     * to the user memory.
     */
    if( 0 == ompi_convertor_need_buffers(convertor) ) {
        /**
         * let the convertor figure out the correct pointer depending
         * on the data layout
         */
        iov.iov_base = NULL;
        if( 0 == reserve ) {
            MCA_BTL_MX_FRAG_ALLOC_USER(btl, frag, rc);
            if( OPAL_UNLIKELY(NULL == frag) ) {
                return NULL;
            }
            max_data = *size;
            frag->base.des_src_cnt = 1;
        } else {
            MCA_BTL_MX_FRAG_ALLOC_EAGER( mx_btl, frag, rc );
            if( OPAL_UNLIKELY(NULL == frag) ) {
                return NULL;
            }
            frag->base.des_src_cnt = 2;
        }
    } else {
        MCA_BTL_MX_FRAG_ALLOC_EAGER( mx_btl, frag, rc );
        if( OPAL_UNLIKELY(NULL == frag) ) {
            return NULL;
        }
        frag->base.des_src_cnt = 1;
        iov.iov_base = (void*)((unsigned char*)frag->segment[0].seg_addr.pval + reserve);
    }

    iov.iov_len = max_data;
    (void)ompi_convertor_pack(convertor, &iov, &iov_count, &max_data );
    *size = max_data;

    if( 1 == frag->base.des_src_cnt ) {
        frag->segment[0].seg_len = reserve + max_data;
        if( 0 == reserve )
            frag->segment[0].seg_addr.pval = iov.iov_base;
    } else {
        frag->segment[0].seg_len       = reserve;
        frag->segment[1].seg_len       = max_data;
        frag->segment[1].seg_addr.pval = iov.iov_base;
    }
    frag->base.des_src = frag->segment;
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

mca_btl_base_descriptor_t* mca_btl_mx_prepare_dst( struct mca_btl_base_module_t* btl,
                                                   struct mca_btl_base_endpoint_t* endpoint,
                                                   struct mca_mpool_base_registration_t* registration,
                                                   struct ompi_convertor_t* convertor,
                                                   size_t reserve,
                                                   size_t* size)
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*)btl;
    mca_btl_mx_frag_t* frag;
    mx_return_t mx_return;
    mx_segment_t mx_segment;
    int rc;

    MCA_BTL_MX_FRAG_ALLOC_USER(btl, frag, rc);
    if( OPAL_UNLIKELY(NULL == frag) ) {
        return NULL;
    }

    frag->segment[0].seg_len       = *size;
    frag->segment[0].seg_addr.pval = convertor->pBaseBuf + convertor->bConverted;
    frag->segment[0].seg_key.key64 = (uint64_t)(intptr_t)frag;

    mx_segment.segment_ptr    = frag->segment[0].seg_addr.pval;
    mx_segment.segment_length = frag->segment[0].seg_len;
    mx_return = mx_irecv( mx_btl->mx_endpoint, &mx_segment, 1, frag->segment[0].seg_key.key64, 
                          BTL_MX_PUT_MASK, NULL, &(frag->mx_request) );
    if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) {
        opal_output( 0, "Fail to re-register a fragment with the MX NIC ...\n" );
        MCA_BTL_MX_FRAG_RETURN( btl, frag );
        return NULL;
    }

    frag->base.des_dst = frag->segment;
    frag->base.des_dst_cnt = 1;

    return &frag->base;
}


/**
 * Initiate an asynchronous put. 
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transferred
 */
static int mca_btl_mx_put( struct mca_btl_base_module_t* btl,
                           struct mca_btl_base_endpoint_t* endpoint,
                           struct mca_btl_base_descriptor_t* descriptor )
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*)btl;
    mca_btl_mx_frag_t* frag = (mca_btl_mx_frag_t*)descriptor;
    mx_segment_t mx_segment[2];
    mx_return_t mx_return;
    uint32_t i = 0;

    if( OPAL_UNLIKELY(MCA_BTL_MX_CONNECTED != ((mca_btl_mx_endpoint_t*)endpoint)->status) ) {
        if( MCA_BTL_MX_NOT_REACHEABLE == ((mca_btl_mx_endpoint_t*)endpoint)->status )
            return OMPI_ERROR;
        if( MCA_BTL_MX_CONNECTION_PENDING == ((mca_btl_mx_endpoint_t*)endpoint)->status )
            return OMPI_ERR_OUT_OF_RESOURCE;
        if( OMPI_SUCCESS != mca_btl_mx_proc_connect( (mca_btl_mx_endpoint_t*)endpoint ) )
            return OMPI_ERROR;
    }

    frag->endpoint  = endpoint;
    frag->tag       = 0xff;

    do {
        mx_segment[i].segment_ptr    = descriptor->des_src[i].seg_addr.pval;
        mx_segment[i].segment_length = descriptor->des_src[i].seg_len;
    } while (++i < descriptor->des_src_cnt);

    mx_return = mx_isend( mx_btl->mx_endpoint, mx_segment, descriptor->des_src_cnt,
                          endpoint->mx_peer_addr,
                          descriptor->des_dst[0].seg_key.key64, frag, &frag->mx_request );
    if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) {
        opal_output( 0, "mx_isend fails with error %s\n", mx_strerror(mx_return) );
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}


/**
 * Initiate an asynchronous send.
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transfered
 * @param tag (IN)         The tag value used to notify the peer.
 */

int mca_btl_mx_send( struct mca_btl_base_module_t* btl,
                     struct mca_btl_base_endpoint_t* endpoint,
                     struct mca_btl_base_descriptor_t* descriptor, 
                     mca_btl_base_tag_t tag )
   
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*)btl;
    mca_btl_mx_frag_t* frag = (mca_btl_mx_frag_t*)descriptor;
    mx_segment_t mx_segment[2];
    mx_return_t mx_return;
    uint64_t total_length = 0;
    uint32_t i = 0;

    if( OPAL_UNLIKELY(MCA_BTL_MX_CONNECTED != ((mca_btl_mx_endpoint_t*)endpoint)->status) ) {
        if( MCA_BTL_MX_NOT_REACHEABLE == ((mca_btl_mx_endpoint_t*)endpoint)->status )
            return OMPI_ERROR;
        if( MCA_BTL_MX_CONNECTION_PENDING == ((mca_btl_mx_endpoint_t*)endpoint)->status )
            return OMPI_ERR_OUT_OF_RESOURCE;
        if( OMPI_SUCCESS != mca_btl_mx_proc_connect( (mca_btl_mx_endpoint_t*)endpoint ) )
            return OMPI_ERROR;
    }

    frag->endpoint  = endpoint;
    frag->tag       = 0xff;

    do {
        mx_segment[i].segment_ptr    = descriptor->des_src[i].seg_addr.pval;
        mx_segment[i].segment_length = descriptor->des_src[i].seg_len;
        total_length += descriptor->des_src[i].seg_len;
    } while (++i < descriptor->des_src_cnt);

    mx_return = mx_isend( mx_btl->mx_endpoint, mx_segment, descriptor->des_src_cnt, endpoint->mx_peer_addr,
                          (uint64_t)tag, frag, &frag->mx_request );
    if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) {
        opal_output( 0, "mx_isend fails with error %s\n", mx_strerror(mx_return) );
        return OMPI_ERROR;
    }

#if MX_HAVE_FORGET
    {
        uint32_t mx_result;
        mx_return = mx_ibuffered( mx_btl->mx_endpoint, &(frag->mx_request), &mx_result );
        if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) {
            opal_output( 0, "mx_ibuffered failed with error %d (%s)\n",
                         mx_return, mx_strerror(mx_return) );
            return OMPI_ERROR;
        }
        if( mx_result ) {
            mx_return = mx_forget( mx_btl->mx_endpoint, &(frag->mx_request) );
            frag->base.des_cbfunc( &(mx_btl->super), frag->endpoint, &(frag->base), OMPI_SUCCESS);
            if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) {
                opal_output( 0, "mx_forget failed with error %d (%s)\n",
                             mx_return, mx_strerror(mx_return) );
                return OMPI_ERROR;
            }
            return OMPI_SUCCESS;
        }
    }
#endif
    if( 4096 > total_length ) {
        mx_status_t mx_status;
        uint32_t mx_result;

        /* let's check for completness */
        mx_return = mx_test( mx_btl->mx_endpoint, &(frag->mx_request), &mx_status, &mx_result );
        if( OPAL_UNLIKELY(MX_SUCCESS != mx_return) ) 
            return OMPI_SUCCESS;
        /* call the completion callback */
        if( mx_result ) {
            frag->base.des_cbfunc( &(mx_btl->super), frag->endpoint, &(frag->base), OMPI_SUCCESS);
            return OMPI_SUCCESS;
        }
    }

    return OMPI_SUCCESS;
}


/*
 * Cleanup/release module resources.
 */

int mca_btl_mx_finalize( struct mca_btl_base_module_t* btl )
{
    mca_btl_mx_module_t* mx_btl = (mca_btl_mx_module_t*) btl; 

    mx_close_endpoint(mx_btl->mx_endpoint);
    
    OBJ_DESTRUCT( &mx_btl->mx_lock );
    OBJ_DESTRUCT( &mx_btl->mx_peers );
    free(mx_btl);
    return OMPI_SUCCESS;
}

mca_btl_mx_module_t mca_btl_mx_module = {
    {
        &mca_btl_mx_component.super,
        0, /* max size of first fragment */
        0, /* min send fragment size */
        0, /* max send fragment size */
        0, /* min rdma fragment size */
        0, /* max rdma fragment size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        MCA_BTL_FLAGS_SEND_INPLACE | MCA_BTL_FLAGS_PUT, /* flags */
        mca_btl_mx_add_procs,
        mca_btl_mx_del_procs,
        mca_btl_mx_register, 
        mca_btl_mx_finalize,
        mca_btl_mx_alloc, 
        mca_btl_mx_free, 
        mca_btl_mx_prepare_src,
        mca_btl_mx_prepare_dst,
        mca_btl_mx_send,
        mca_btl_mx_put, /* put */
        NULL, /* get */
        mca_btl_base_dump,
        NULL, /* mpool */
        NULL /* register error */
    }
};

