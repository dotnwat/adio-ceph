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

#include "ompi_config.h"
#include <string.h>
#include "opal/util/output.h"
#include "opal/util/if.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"

#include "btl_mvapi.h"
#include "btl_mvapi_frag.h" 
#include "btl_mvapi_proc.h"
#include "btl_mvapi_endpoint.h"
#include "datatype/convertor.h" 
#include "mca/mpool/base/base.h" 
#include "mca/mpool/mpool.h" 
#include "mca/mpool/mvapi/mpool_mvapi.h" 
#include "mca/btl/base/btl_base_error.h" 
#include <vapi_types.h> 
#include <math.h> /* for log2 */ 

mca_btl_mvapi_module_t mca_btl_mvapi_module = {
    {
        &mca_btl_mvapi_component.super,
        0, /* max size of first fragment */
        0, /* min send fragment size */
        0, /* max send fragment size */
        0, /* min rdma fragment size */
        0, /* max rdma fragment size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        0,  /* TODO this should be PUT btl flags */
        mca_btl_mvapi_add_procs,
        mca_btl_mvapi_del_procs,
        mca_btl_mvapi_register, 
        mca_btl_mvapi_finalize,
        /* we need alloc free, pack */ 
        mca_btl_mvapi_alloc, 
        mca_btl_mvapi_free, 
        mca_btl_mvapi_prepare_src,
        mca_btl_mvapi_prepare_dst,
        mca_btl_mvapi_send,
        mca_btl_mvapi_put,
        mca_btl_mvapi_get
    }
};

/* 
 *  add a proc to this btl module 
 *    creates an endpoint that is setup on the
 *    first send to the endpoint
 */ 
int mca_btl_mvapi_add_procs(
    struct mca_btl_base_module_t* btl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_procs, 
    struct mca_btl_base_endpoint_t** peers, 
    ompi_bitmap_t* reachable)
{
    mca_btl_mvapi_module_t* mvapi_btl = (mca_btl_mvapi_module_t*)btl;
    int i, rc;

    for(i = 0; i < (int) nprocs; i++) {
        
        struct ompi_proc_t* ompi_proc = ompi_procs[i];
        mca_btl_mvapi_proc_t* ib_proc;
        mca_btl_base_endpoint_t* ib_peer;

        if(NULL == (ib_proc = mca_btl_mvapi_proc_create(ompi_proc))) {
            continue;
        }

        /*
         * Check to make sure that the peer has at least as many interface 
         * addresses exported as we are trying to use. If not, then 
         * don't bind this PTL instance to the proc.
         */

        OPAL_THREAD_LOCK(&ib_proc->proc_lock);

        /* The btl_proc datastructure is shared by all IB PTL
         * instances that are trying to reach this destination. 
         * Cache the peer instance on the btl_proc.
         */
        ib_peer = OBJ_NEW(mca_btl_mvapi_endpoint_t);
        if(NULL == ib_peer) {
            OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        ib_peer->endpoint_btl = mvapi_btl;
        ib_peer->subnet = mvapi_btl->port_info.subnet; 
        rc = mca_btl_mvapi_proc_insert(ib_proc, ib_peer);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(ib_peer);
            OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
            continue;
        }

        ompi_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
        peers[i] = ib_peer;
    }
    
    /* currently we only scale the srq the first time 
       add_procs is called, subsequent calls are ignored, 
       we should be able to change this to modify the SRQ but 
       I am unsure as to what this entails
    */ 

    if( 0 == mvapi_btl->num_peers ) { 
        mvapi_btl->num_peers += nprocs; 
        if(mca_btl_mvapi_component.use_srq) { 
            mvapi_btl->rd_buf_max = mca_btl_mvapi_component.ib_rr_buf_max + log2(nprocs) * mca_btl_mvapi_component.rd_per_peer; 
        }
    }
    return OMPI_SUCCESS;
}

/* 
 * delete the proc as reachable from this btl module 
 */
int mca_btl_mvapi_del_procs(struct mca_btl_base_module_t* btl, 
        size_t nprocs, 
        struct ompi_proc_t **procs, 
        struct mca_btl_base_endpoint_t ** peers)
{
    /* Stub */
    BTL_VERBOSE(("Stub\n"));
    return OMPI_SUCCESS;
}

/* 
 *Register callback function to support send/recv semantics 
 */ 
int mca_btl_mvapi_register(
                        struct mca_btl_base_module_t* btl, 
                        mca_btl_base_tag_t tag, 
                        mca_btl_base_module_recv_cb_fn_t cbfunc, 
                        void* cbdata)
{
    
    mca_btl_mvapi_module_t* mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    

    OPAL_THREAD_LOCK(&mvapi_btl->ib_lock); 
    mvapi_btl->ib_reg[tag].cbfunc = cbfunc; 
    mvapi_btl->ib_reg[tag].cbdata = cbdata; 
    OPAL_THREAD_UNLOCK(&mvapi_btl->ib_lock); 
    return OMPI_SUCCESS;
}



/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
 * 
 * When allocating a segment we pull a pre-alllocated segment 
 * from one of two free lists, an eager list and a max list
 */
mca_btl_base_descriptor_t* mca_btl_mvapi_alloc(
    struct mca_btl_base_module_t* btl,
    size_t size)
{
    mca_btl_mvapi_frag_t* frag;
    mca_btl_mvapi_module_t* mvapi_btl; 
    int rc;
    mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    
    if(size <= mca_btl_mvapi_component.eager_limit){ 
        MCA_BTL_IB_FRAG_ALLOC_EAGER(btl, frag, rc); 
        frag->segment.seg_len = 
            size <= mca_btl_mvapi_component.eager_limit ? 
            size: mca_btl_mvapi_component.eager_limit ; 
    } else { 
        MCA_BTL_IB_FRAG_ALLOC_MAX(btl, frag, rc); 
        frag->segment.seg_len = 
            size <= mca_btl_mvapi_component.max_send_size ? 
            size: mca_btl_mvapi_component.max_send_size ; 
    }
    
    frag->segment.seg_len = size <= mvapi_btl->super.btl_eager_limit ? size : mvapi_btl->super.btl_eager_limit;  
    frag->base.des_flags = 0; 
    
    return (mca_btl_base_descriptor_t*)frag;
}

/** 
 * Return a segment 
 * 
 * Return the segment to the appropriate 
 *  preallocated segment list 
 */  
int mca_btl_mvapi_free(
                    struct mca_btl_base_module_t* btl, 
                    mca_btl_base_descriptor_t* des) 
{
    mca_btl_mvapi_frag_t* frag = (mca_btl_mvapi_frag_t*)des; 

    if(frag->size == 0) {
        btl->btl_mpool->mpool_release(btl->btl_mpool, (mca_mpool_base_registration_t*) frag->vapi_reg); 
        MCA_BTL_IB_FRAG_RETURN_FRAG(btl, frag);
    } else if(frag->size == mca_btl_mvapi_component.max_send_size){ 
        MCA_BTL_IB_FRAG_RETURN_MAX(btl, frag); 
    } else if(frag->size == mca_btl_mvapi_component.eager_limit){ 
        MCA_BTL_IB_FRAG_RETURN_EAGER(btl, frag); 
    } else { 
        BTL_ERROR(("invalid descriptor")); 
    }
    return OMPI_SUCCESS; 
}

/**
 * register user buffer or pack 
 * data into pre-registered buffer and return a 
 * descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param endpoint (IN)     BTL peer addressing
 *  
 * prepare source's behavior depends on the following: 
 * Has a valid memory registration been passed to prepare_src? 
 *    if so we attempt to use the pre-registred user-buffer, if the memory registration 
 *    is to small (only a portion of the user buffer) then we must reregister the user buffer 
 * Has the user requested the memory to be left pinned? 
 *    if so we insert the memory registration into a memory tree for later lookup, we 
 *    may also remove a previous registration if a MRU (most recently used) list of 
 *    registions is full, this prevents resources from being exhausted.
 * Is the requested size larger than the btl's max send size? 
 *    if so and we aren't asked to leave the registration pinned than we register the memory if 
 *    the users buffer is contiguous 
 * Otherwise we choose from two free lists of pre-registered memory in which to pack the data into. 
 * 
 */
mca_btl_base_descriptor_t* mca_btl_mvapi_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    size_t reserve,
    size_t* size
)
{
    mca_btl_mvapi_module_t* mvapi_btl; 
    mca_btl_mvapi_frag_t* frag; 
    mca_mpool_mvapi_registration_t * vapi_reg; 
    struct iovec iov; 
    int32_t iov_count = 1; 
    size_t max_data = *size; 
    int32_t free_after; 
    int rc; 
    
    
    mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    vapi_reg = (mca_mpool_mvapi_registration_t*) registration; 
    
    if(NULL != vapi_reg &&  0 == ompi_convertor_need_buffers(convertor)){ 
        size_t reg_len; 
        /* the memory is already pinned and we have contiguous user data */ 
        MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc); 
        if(NULL == frag){
            return NULL; 
        } 
        
        iov.iov_len = max_data; 
        iov.iov_base = NULL; 
        
        ompi_convertor_pack(convertor, &iov, &iov_count, &max_data, &free_after); 
        
        frag->segment.seg_len = max_data; 
        frag->segment.seg_addr.pval = iov.iov_base; 
        
        reg_len = (unsigned char*)vapi_reg->base_reg.bound - (unsigned char*)iov.iov_base + 1; 

        frag->sg_entry.len = max_data; 
        frag->sg_entry.lkey = vapi_reg->l_key; 
        frag->sg_entry.addr = (VAPI_virt_addr_t) (MT_virt_addr_t) iov.iov_base; 
        
        frag->segment.seg_key.key32[0] = (uint32_t) vapi_reg->l_key; 
        
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->base.des_flags = 0; 
        frag->vapi_reg = vapi_reg; 
        
        btl->btl_mpool->mpool_retain(btl->btl_mpool, (mca_mpool_base_registration_t*) vapi_reg); 
        if(vapi_reg->base_reg.flags & MCA_MPOOL_FLAGS_CACHE) { 
            assert(vapi_reg->base_reg.ref_count >= 4); 
        } else { 
            assert(vapi_reg->base_reg.ref_count >= 2); 
        }

        return &frag->base;
        
    } else if( max_data > btl->btl_max_send_size && 
               ompi_convertor_need_buffers(convertor) == 0 && 
               reserve == 0)
    {
        /* The user buffer is contigous and we are asked to send more than the max send size.  */            

        MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc); 
        if(NULL == frag){
            return NULL; 
        } 
       
        iov.iov_len = max_data; 
        iov.iov_base = NULL; 
        
        ompi_convertor_pack(convertor, &iov, &iov_count, &max_data, &free_after); 
        
        
        frag->segment.seg_len = max_data; 
        frag->segment.seg_addr.pval = iov.iov_base; 
        frag->base.des_flags = 0; 

        btl->btl_mpool->mpool_register(btl->btl_mpool,
                                       iov.iov_base, 
                                       max_data, 
                                       0,
                                       (mca_mpool_base_registration_t**) &vapi_reg); 
        
        
        frag->sg_entry.len = max_data; 
        frag->sg_entry.lkey = vapi_reg->l_key; 
        frag->sg_entry.addr = (VAPI_virt_addr_t) (MT_virt_addr_t) iov.iov_base; 
        
        frag->segment.seg_key.key32[0] = (uint32_t) vapi_reg->l_key; 
            
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->vapi_reg = vapi_reg; 
        
        return &frag->base;

    } else if (max_data+reserve <=  btl->btl_eager_limit) { 
        /* the data is small enough to fit in the eager frag and 
           either we received no prepinned memory or leave pinned is 
           not set
        */
        MCA_BTL_IB_FRAG_ALLOC_EAGER(btl, frag, rc); 
        if(NULL == frag) { 
            return NULL; 
        } 
        
        iov.iov_len = max_data; 
        iov.iov_base = frag->segment.seg_addr.pval + reserve; 
        
        rc = ompi_convertor_pack(convertor, &iov, &iov_count, &max_data, &free_after); 
        *size  = max_data; 
        if( rc < 0 ) { 
            MCA_BTL_IB_FRAG_RETURN_EAGER(btl, frag); 
            return NULL; 
        } 
        
        frag->segment.seg_len = max_data + reserve; 
        frag->segment.seg_key.key32[0] = (uint32_t) frag->sg_entry.lkey; 
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->base.des_flags = 0; 
        
        return &frag->base; 
        
    } else { 
       
        MCA_BTL_IB_FRAG_ALLOC_MAX(btl, frag, rc); 
        if(NULL == frag) { 
            return NULL; 
        } 
        if(max_data + reserve > btl->btl_max_send_size){ 
            max_data = btl->btl_max_send_size - reserve; 
        }
        iov.iov_len = max_data; 
        iov.iov_base = (unsigned char*) frag->segment.seg_addr.pval + reserve; 
        
        rc = ompi_convertor_pack(convertor, &iov, &iov_count, &max_data, &free_after); 
        *size  = max_data; 

        if( rc < 0 ) { 
            MCA_BTL_IB_FRAG_RETURN_MAX(btl, frag); 
            return NULL; 
        } 
        
        frag->segment.seg_len = max_data + reserve; 
        frag->segment.seg_key.key32[0] = (uint32_t) frag->sg_entry.lkey; 
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->base.des_flags=0; 
        
        return &frag->base; 
    }
    return NULL; 
}



/**
 * Prepare the dst buffer
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 * prepare dest's behavior depends on the following: 
 * Has a valid memory registration been passed to prepare_src? 
 *    if so we attempt to use the pre-registred user-buffer, if the memory registration 
 *    is to small (only a portion of the user buffer) then we must reregister the user buffer 
 * Has the user requested the memory to be left pinned? 
 *    if so we insert the memory registration into a memory tree for later lookup, we 
 *    may also remove a previous registration if a MRU (most recently used) list of 
 *    registions is full, this prevents resources from being exhausted.
 */
mca_btl_base_descriptor_t* mca_btl_mvapi_prepare_dst(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    size_t reserve,
    size_t* size)
{
    mca_btl_mvapi_module_t* mvapi_btl; 
    mca_btl_mvapi_frag_t* frag; 
    mca_mpool_mvapi_registration_t * vapi_reg; 
    int rc; 
    
    mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    vapi_reg = (mca_mpool_mvapi_registration_t*) registration; 
    
    MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc); 

    if(NULL == frag){
        return NULL; 
    }
    
    
    frag->segment.seg_len = *size; 
    frag->segment.seg_addr.pval = convertor->pBaseBuf + convertor->bConverted; 
    frag->base.des_flags = 0; 

    if(NULL!= vapi_reg){ 
        /* the memory is already pinned- use it*/ 
        btl->btl_mpool->mpool_retain(btl->btl_mpool, (mca_mpool_base_registration_t*) vapi_reg); 
        if(vapi_reg->base_reg.flags & MCA_MPOOL_FLAGS_CACHE) { 
            assert(vapi_reg->base_reg.ref_count >= 4); 
        } else { 
            assert(vapi_reg->base_reg.ref_count >= 2); 
        }
    }  else { 
        /* we didn't get a memory registration passed in, so we have to register the region
         * ourselves 
         */ 
        btl->btl_mpool->mpool_register(btl->btl_mpool,
                                       frag->segment.seg_addr.pval,
                                       *size, 
                                       0,
                                       (mca_mpool_base_registration_t**) &vapi_reg);
    }
    
    frag->sg_entry.len = *size; 
    frag->sg_entry.lkey = vapi_reg->l_key; 
    frag->sg_entry.addr = (VAPI_virt_addr_t) (MT_virt_addr_t) frag->segment.seg_addr.pval; 
    
    frag->segment.seg_key.key32[0] = (uint32_t) vapi_reg->r_key; 
    
    frag->base.des_dst = &frag->segment; 
    frag->base.des_dst_cnt = 1; 
    frag->base.des_src = NULL; 
    frag->base.des_src_cnt = 0; 
    frag->vapi_reg = vapi_reg; 
    
    return &frag->base; 
    
}

int mca_btl_mvapi_finalize(struct mca_btl_base_module_t* btl)
{
    mca_btl_mvapi_module_t* mvapi_btl; 
    mvapi_btl = (mca_btl_mvapi_module_t*) btl; 

#if 0     
    if(mvapi_btl->send_free_eager.fl_num_allocated != 
       mvapi_btl->send_free_eager.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_eager frags: %d allocated %d returned \n", 
                    mvapi_btl->send_free_eager.fl_num_allocated, 
                    mvapi_btl->send_free_eager.super.opal_list_length); 
    }
    if(mvapi_btl->send_free_max.fl_num_allocated != 
      mvapi_btl->send_free_max.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_max frags: %d allocated %d returned \n", 
                    mvapi_btl->send_free_max.fl_num_allocated, 
                    mvapi_btl->send_free_max.super.opal_list_length); 
    }
    if(mvapi_btl->send_free_frag.fl_num_allocated != 
       mvapi_btl->send_free_frag.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_frag frags: %d allocated %d returned \n", 
                    mvapi_btl->send_free_frag.fl_num_allocated, 
                    mvapi_btl->send_free_frag.super.opal_list_length); 
    }
    
    if(mvapi_btl->recv_free_eager.fl_num_allocated != 
       mvapi_btl->recv_free_eager.super.opal_list_length){ 
        opal_output(0, "btl ib recv_free_eager frags: %d allocated %d returned \n", 
                    mvapi_btl->recv_free_eager.fl_num_allocated, 
                    mvapi_btl->recv_free_eager.super.opal_list_length); 
    }

    if(mvapi_btl->recv_free_max.fl_num_allocated != 
       mvapi_btl->recv_free_max.super.opal_list_length){ 
        opal_output(0, "btl ib recv_free_max frags: %d allocated %d returned \n", 
                    mvapi_btl->recv_free_max.fl_num_allocated, 
                    mvapi_btl->recv_free_max.super.opal_list_length); 
    }
#endif 

    return OMPI_SUCCESS;
}

/*
 *  Initiate a send. 
 */

int mca_btl_mvapi_send( 
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_btl_base_descriptor_t* descriptor, 
    mca_btl_base_tag_t tag)
   
{
    mca_btl_mvapi_frag_t* frag = (mca_btl_mvapi_frag_t*)descriptor; 
    frag->endpoint = endpoint; 
    frag->hdr->tag = tag; 
    frag->rc = mca_btl_mvapi_endpoint_send(endpoint, frag);
    return frag->rc;
}

/*
 * RDMA local buffer to remote buffer address.
 */

int mca_btl_mvapi_put( mca_btl_base_module_t* btl,
                       mca_btl_base_endpoint_t* endpoint,
                       mca_btl_base_descriptor_t* descriptor)
{
    int rc; 
    mca_btl_mvapi_module_t* mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    mca_btl_mvapi_frag_t* frag = (mca_btl_mvapi_frag_t*) descriptor; 

    assert(endpoint->endpoint_state == MCA_BTL_IB_CONNECTED || 
            endpoint->endpoint_state == MCA_BTL_IB_WAITING_ACK);
    frag->sr_desc.opcode = VAPI_RDMA_WRITE; 
    /* atomically test and acquire a token */
    if(OPAL_THREAD_ADD32(&endpoint->wr_sq_tokens_lp,-1) < 0) { 
        BTL_VERBOSE(("Queing because no rdma write tokens \n"));
        BTL_MVAPI_INSERT_PENDING(frag, endpoint->pending_frags_lp, 
                                 endpoint->wr_sq_tokens_lp, endpoint->endpoint_lock, rc); 
    } else { 
        
        frag->endpoint = endpoint;
        
    
        frag->sr_desc.remote_qp = endpoint->rem_info.rem_qp_num_low; 
        frag->sr_desc.remote_addr = (VAPI_virt_addr_t) (MT_virt_addr_t) frag->base.des_dst->seg_addr.pval; 
        frag->sr_desc.r_key = frag->base.des_dst->seg_key.key32[0]; 
        frag->sg_entry.addr = (VAPI_virt_addr_t) (MT_virt_addr_t) frag->base.des_src->seg_addr.pval; 
        frag->sg_entry.len  = frag->base.des_src->seg_len; 
        
        frag->ret = VAPI_post_sr(mvapi_btl->nic, 
                                 endpoint->lcl_qp_hndl_low, 
                                 &frag->sr_desc); 
        if(VAPI_OK != frag->ret){ 
            rc =  OMPI_ERROR; 
        } else { 
            rc = OMPI_SUCCESS; 
        }
        if(mca_btl_mvapi_component.use_srq) { 
            MCA_BTL_MVAPI_POST_SRR_HIGH(mvapi_btl, 1); 
            MCA_BTL_MVAPI_POST_SRR_LOW(mvapi_btl, 1); 
        } else { 
            MCA_BTL_MVAPI_ENDPOINT_POST_RR_HIGH(endpoint, 1); 
            MCA_BTL_MVAPI_ENDPOINT_POST_RR_LOW(endpoint, 1); 
        }
    }
    
    return rc; 
        
}

/* 
 * RDMA read remote buffer to local buffer address. 
 */ 

int mca_btl_mvapi_get( mca_btl_base_module_t* btl, 
                       mca_btl_base_endpoint_t* endpoint,
                       mca_btl_base_descriptor_t* descriptor)
{
    int rc; 
    mca_btl_mvapi_module_t* mvapi_btl = (mca_btl_mvapi_module_t*) btl; 
    mca_btl_mvapi_frag_t* frag = (mca_btl_mvapi_frag_t*) descriptor; 
    
    assert(endpoint->endpoint_state == MCA_BTL_IB_CONNECTED ||
           endpoint->endpoint_state == MCA_BTL_IB_WAITING_ACK);
    frag->sr_desc.opcode = VAPI_RDMA_READ; 
    
    /* atomically test and acquire a token */
    if(OPAL_THREAD_ADD32(&endpoint->wr_sq_tokens_lp,-1) < 0) { 
        BTL_VERBOSE(("Queing because no rdma write tokens \n"));
        BTL_MVAPI_INSERT_PENDING(frag, endpoint->pending_frags_lp, 
                                 endpoint->wr_sq_tokens_lp, endpoint->endpoint_lock, rc); 
    } else { 
        frag->endpoint = endpoint;
        frag->sr_desc.remote_qp = endpoint->rem_info.rem_qp_num_low; 
        frag->sr_desc.remote_addr = (VAPI_virt_addr_t) (MT_virt_addr_t) frag->base.des_src->seg_addr.pval; 
        frag->sr_desc.r_key = frag->base.des_src->seg_key.key32[0]; 
        frag->sg_entry.addr = (VAPI_virt_addr_t) (MT_virt_addr_t) frag->base.des_dst->seg_addr.pval; 
        frag->sg_entry.len  = frag->base.des_dst->seg_len; 

        frag->ret = VAPI_post_sr(mvapi_btl->nic, 
                                 endpoint->lcl_qp_hndl_low, 
                                 &frag->sr_desc); 
        if(VAPI_OK != frag->ret){ 
            rc =  OMPI_ERROR; 
        } else { 
            rc = OMPI_SUCCESS; 
        }
        if(mca_btl_mvapi_component.use_srq) { 
            MCA_BTL_MVAPI_POST_SRR_HIGH(mvapi_btl, 1); 
            MCA_BTL_MVAPI_POST_SRR_LOW(mvapi_btl, 1); 
        } else { 
            MCA_BTL_MVAPI_ENDPOINT_POST_RR_HIGH(endpoint, 1); 
            MCA_BTL_MVAPI_ENDPOINT_POST_RR_LOW(endpoint, 1); 
        }
    }
    return rc; 
    
}

                       

/*
 * Asynchronous event handler to detect unforseen
 * events. Usually, such events are catastrophic.
 * Should have a robust mechanism to handle these
 * events and abort the OMPI application if necessary.
 *
 */
static void async_event_handler(VAPI_hca_hndl_t hca_hndl,
        VAPI_event_record_t * event_p,
        void *priv_data)
{
    switch (event_p->type) {
    case VAPI_QP_PATH_MIGRATED:
    case VAPI_EEC_PATH_MIGRATED:
    case VAPI_QP_COMM_ESTABLISHED:
    case VAPI_EEC_COMM_ESTABLISHED:
    case VAPI_SEND_QUEUE_DRAINED:
    case VAPI_PORT_ACTIVE:
        {
            BTL_VERBOSE(("Got an asynchronous event: %s\n", VAPI_event_record_sym(event_p->type)));
            break;
        }
    case VAPI_CQ_ERROR:
    case VAPI_LOCAL_WQ_INV_REQUEST_ERROR:
    case VAPI_LOCAL_WQ_ACCESS_VIOL_ERROR:
    case VAPI_LOCAL_WQ_CATASTROPHIC_ERROR:
    case VAPI_PATH_MIG_REQ_ERROR:
    case VAPI_LOCAL_EEC_CATASTROPHIC_ERROR:
    case VAPI_LOCAL_CATASTROPHIC_ERROR:
    case VAPI_PORT_ERROR:
        {
            BTL_ERROR(("Got an asynchronous event: %s (%s)",
                       VAPI_event_record_sym(event_p->type),
                       VAPI_event_syndrome_sym(event_p->syndrome)));
            break;
        }
            
    case VAPI_SRQ_LIMIT_REACHED: 
        { 
            
            BTL_VERBOSE(("SRQ limit is reached, posting more buffers %s\n", VAPI_event_record_sym(event_p->type))); 
        }
        
    case VAPI_RECEIVE_QUEUE_DRAINED: { 
        

    }
    default:
        BTL_ERROR(("Warning!! Got an undefined "
                   "asynchronous event %s", VAPI_event_record_sym(event_p->type)));
    }

}


/* 
 * Initialize the btl module by allocating a protection domain 
 *  and creating both the high and low priority completion queues 
 */ 
int mca_btl_mvapi_module_init(mca_btl_mvapi_module_t *mvapi_btl)
{

    /* Allocate Protection Domain */ 
    VAPI_ret_t ret;
    uint32_t cqe_cnt = 0;
    VAPI_srq_attr_t srq_attr, srq_attr_out; 
  
    ret = VAPI_alloc_pd(mvapi_btl->nic, &mvapi_btl->ptag);
    
    if(ret != VAPI_OK) {
        BTL_ERROR(("error in VAPI_alloc_pd: %s", VAPI_strerror(ret)));
        return OMPI_ERROR;
    }
    
    if(mca_btl_mvapi_component.use_srq) { 
        mvapi_btl->srr_posted_high = 0; 
        mvapi_btl->srr_posted_low = 0; 
        srq_attr.pd_hndl = mvapi_btl->ptag; 
        srq_attr.max_outs_wr = mca_btl_mvapi_component.ib_wq_size; 
        srq_attr.max_sentries = mca_btl_mvapi_component.ib_sg_list_size; 
        srq_attr.srq_limit =  mca_btl_mvapi_component.ib_wq_size; 
        
        ret = VAPI_create_srq(mvapi_btl->nic, 
                              &srq_attr, 
                              &mvapi_btl->srq_hndl_high, 
                              &srq_attr_out); 
        if(ret != VAPI_OK) {
            BTL_ERROR(("error in VAPI_create_srq: %s", VAPI_strerror(ret)));
            return OMPI_ERROR;
        }
        ret = VAPI_create_srq(mvapi_btl->nic, 
                              &srq_attr, 
                              &mvapi_btl->srq_hndl_low, 
                              &srq_attr_out); 
        if(ret != VAPI_OK) {
            BTL_ERROR(("error in VAPI_create_srq: %s", VAPI_strerror(ret)));
            return OMPI_ERROR;
        }
        
    } else { 
        mvapi_btl->srq_hndl_high = VAPI_INVAL_SRQ_HNDL; 
        mvapi_btl->srq_hndl_low = VAPI_INVAL_SRQ_HNDL; 
    } 
    ret = VAPI_create_cq(mvapi_btl->nic, mca_btl_mvapi_component.ib_cq_size,
                         &mvapi_btl->cq_hndl_low, &cqe_cnt);

    
    if( VAPI_OK != ret) {  
        BTL_ERROR(("error in VAPI_create_cq: %s", VAPI_strerror(ret)));
        return OMPI_ERROR;
    }
    
    ret = VAPI_create_cq(mvapi_btl->nic, mca_btl_mvapi_component.ib_cq_size,
                         &mvapi_btl->cq_hndl_high, &cqe_cnt);

    
    if( VAPI_OK != ret) {  
        BTL_ERROR(("error in VAPI_create_cq: %s", VAPI_strerror(ret)));
        return OMPI_ERROR;
    }

    
    if(cqe_cnt <= 0) { 
        BTL_ERROR(("error creating completion queue ")); 
        return OMPI_ERROR; 
    } 

    ret = EVAPI_set_async_event_handler(mvapi_btl->nic,
            async_event_handler, 0, &mvapi_btl->async_handler);

    if(VAPI_OK != ret) {
        BTL_ERROR(("error in EVAPI_set_async_event_handler: %s", VAPI_strerror(ret)));
        return OMPI_ERROR;
    }
    
    
    return OMPI_SUCCESS;
}
