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
#include <inttypes.h>
#include "opal/util/output.h"
#include "opal/util/if.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"

#include "btl_openib.h"
#include "btl_openib_frag.h" 
#include "btl_openib_proc.h"
#include "btl_openib_endpoint.h"
#include "datatype/convertor.h" 
#include "mca/mpool/base/base.h" 
#include "mca/mpool/mpool.h" 
#include "mca/mpool/openib/mpool_openib.h" 
#include <errno.h> 
#include <string.h> 
extern int errno; 

mca_btl_openib_module_t mca_btl_openib_module = {
    {
        &mca_btl_openib_component.super,
        0, /* max size of first fragment */
        0, /* min send fragment size */
        0, /* max send fragment size */
        0, /* min rdma fragment size */
        0, /* max rdma fragment size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        0,  /* TODO this should be PUT btl flags */
        mca_btl_openib_add_procs,
        mca_btl_openib_del_procs,
        mca_btl_openib_register, 
        mca_btl_openib_finalize,
        /* we need alloc free, pack */ 
        mca_btl_openib_alloc, 
        mca_btl_openib_free, 
        mca_btl_openib_prepare_src,
        mca_btl_openib_prepare_dst,
        mca_btl_openib_send,
        mca_btl_openib_put,
        NULL /* get */ 
    }
};



int mca_btl_openib_add_procs(
    struct mca_btl_base_module_t* btl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_procs, 
    struct mca_btl_base_endpoint_t** peers, 
    ompi_bitmap_t* reachable)
{
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*)btl;
    int i, rc;

    for(i = 0; i < (int) nprocs; i++) {
        
        struct ompi_proc_t* ompi_proc = ompi_procs[i];
        mca_btl_openib_proc_t* ib_proc;
        mca_btl_base_endpoint_t* ib_peer;

        if(NULL == (ib_proc = mca_btl_openib_proc_create(ompi_proc))) {
            return OMPI_ERR_OUT_OF_RESOURCE;
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
        ib_peer = OBJ_NEW(mca_btl_openib_endpoint_t);
        if(NULL == ib_peer) {
            OPAL_THREAD_UNLOCK(&module_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        ib_peer->endpoint_btl = openib_btl;
        rc = mca_btl_openib_proc_insert(ib_proc, ib_peer);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(ib_peer);
            OPAL_THREAD_UNLOCK(&module_proc->proc_lock);
            continue;
        }

        ompi_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&module_proc->proc_lock);
        peers[i] = ib_peer;
    }

    return OMPI_SUCCESS;
}

int mca_btl_openib_del_procs(struct mca_btl_base_module_t* btl, 
        size_t nprocs, 
        struct ompi_proc_t **procs, 
        struct mca_btl_base_endpoint_t ** peers)
{
    /* Stub */
    DEBUG_OUT("Stub\n");
    return OMPI_SUCCESS;
}

int mca_btl_openib_register(
                        struct mca_btl_base_module_t* btl, 
                        mca_btl_base_tag_t tag, 
                        mca_btl_base_module_recv_cb_fn_t cbfunc, 
                        void* cbdata)
{
    /* TODO add register stuff here... */ 
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl; 
    

    OPAL_THREAD_LOCK(&ib->btl.ib_lock); 
    openib_btl->ib_reg[tag].cbfunc = cbfunc; 
    openib_btl->ib_reg[tag].cbdata = cbdata; 
    OPAL_THREAD_UNLOCK(&ib->btl.ib_lock); 
    return OMPI_SUCCESS;
}


/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
 */
mca_btl_base_descriptor_t* mca_btl_openib_alloc(
    struct mca_btl_base_module_t* btl,
    size_t size)
{
    mca_btl_openib_frag_t* frag;
    mca_btl_openib_module_t* openib_btl; 
    int rc;
    openib_btl = (mca_btl_openib_module_t*) btl; 
    
    if(size <= mca_btl_openib_component.eager_limit){ 
        MCA_BTL_IB_FRAG_ALLOC_EAGER(btl, frag, rc); 
        frag->segment.seg_len = 
            size <= mca_btl_openib_component.eager_limit ? 
            size: mca_btl_openib_component.eager_limit ; 
    } else { 
        MCA_BTL_IB_FRAG_ALLOC_MAX(btl, frag, rc); 
        frag->segment.seg_len = 
            size <= mca_btl_openib_component.max_send_size ? 
            size: mca_btl_openib_component.max_send_size ; 
    }
    
    frag->segment.seg_len = size <= openib_btl->super.btl_eager_limit ? size : openib_btl->super.btl_eager_limit;  
    frag->base.des_flags = 0; 
    
    return (mca_btl_base_descriptor_t*)frag;
}

/** 
 * 
 * 
 */ 
int mca_btl_openib_free(
                    struct mca_btl_base_module_t* btl, 
                    mca_btl_base_descriptor_t* des) 
{
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*)des; 

    if(frag->size == 0) {
        MCA_BTL_IB_FRAG_RETURN_FRAG(btl, frag);
     
        OBJ_RELEASE(frag->openib_reg); 
        
            
    } 
    else if(frag->size == mca_btl_openib_component.max_send_size){ 
        MCA_BTL_IB_FRAG_RETURN_MAX(btl, frag); 
    } else if(frag->size == mca_btl_openib_component.eager_limit){ 
        MCA_BTL_IB_FRAG_RETURN_EAGER(btl, frag); 
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
mca_btl_base_descriptor_t* mca_btl_openib_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    size_t reserve,
    size_t* size
)
{
    mca_btl_openib_module_t* openib_btl; 
    mca_btl_openib_frag_t* frag; 
    mca_mpool_openib_registration_t * openib_reg; 
    struct iovec iov; 
    int32_t iov_count = 1; 
    size_t max_data = *size; 
    int32_t free_after; 
    int rc; 
    
    
    openib_btl = (mca_btl_openib_module_t*) btl; 
    openib_reg = (mca_mpool_openib_registration_t*) registration; 

    /** if the data fits in the eager limit and we aren't told to pinn then we 
        simply pack, if the data fits in the eager limit and the data is non contiguous 
        then we pack **/ 

    
    if(NULL != openib_reg &&  0 == ompi_convertor_need_buffers(convertor)){ 
        bool is_leave_pinned = openib_reg->base_reg.is_leave_pinned; 
        MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc); 
        if(NULL == frag){
            return NULL; 
        } 
        
        iov.iov_len = max_data; 
        iov.iov_base = NULL; 
        
        ompi_convertor_pack(convertor, &iov, &iov_count, &max_data, &free_after); 
        
        /* first we will try to find this address in the memory tree (from MPI_Alloc_mem) */ 
        
        frag->segment.seg_len = max_data; 
        frag->segment.seg_addr.pval = iov.iov_base; 
        
        size_t reg_len; 
        reg_len = (unsigned char*)openib_reg->base_reg.bound - (unsigned char*)iov.iov_base + 1; 
        if(frag->segment.seg_len > reg_len) { 
            size_t new_len = openib_reg->base_reg.bound - openib_reg->base_reg.base + 1 
                + frag->segment.seg_len - reg_len; 
            void * base_addr = openib_reg->base_reg.base; 
            
            rc = mca_mpool_base_remove((void*) openib_reg->base_reg.base); 
            if(OMPI_SUCCESS != rc) { 
                opal_output(0, "%s:%d:%s error removing memory region from memory pool tree", __FILE__, __LINE__,  __func__); 
                return NULL; 
            } 

            if(is_leave_pinned) { 
                if(NULL == opal_list_remove_item(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg)){ 
                    opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                }
            } 
            OBJ_RELEASE(openib_reg); 
            
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool, 
                                                base_addr, 
                                                new_len, 
                                                (mca_mpool_base_registration_t**) &openib_reg);
            
            rc = mca_mpool_base_insert(openib_reg->base_reg.base, 
                                       openib_reg->base_reg.bound - openib_reg->base_reg.base + 1, 
                                       openib_btl->ib_pool, 
                                       (void*) (&openib_btl->super), 
                                       (mca_mpool_base_registration_t*) openib_reg); 
            
            
            if(rc != OMPI_SUCCESS) { 
                opal_output(0,"%s:%d:%s error inserting memory region into memory pool tree", __FILE__, __LINE__,  __func__); 
                return NULL; 
            } 

            OBJ_RETAIN(openib_reg); 
            if(is_leave_pinned) {
                openib_reg->base_reg.is_leave_pinned = is_leave_pinned; 
                opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg);
            } 
        }   
        else if(is_leave_pinned) { 
            if(NULL == opal_list_remove_item(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg)) { 
                opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                return NULL; 
            }
            opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg);
        }
        
        frag->mr = openib_reg->mr; 
        frag->sg_entry.length = max_data; 
        frag->sg_entry.lkey = frag->mr->lkey; 
        frag->sg_entry.addr = (uintptr_t) iov.iov_base; 
        
        frag->segment.seg_key.key32[0] = (uint32_t) frag->sg_entry.lkey; 
        
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->base.des_flags = 0; 
        frag->openib_reg= openib_reg; 
        OBJ_RETAIN(openib_reg); 

        return &frag->base;
        
    } else if((mca_btl_openib_component.leave_pinned || max_data > btl->btl_max_send_size) && 
               ompi_convertor_need_buffers(convertor) == 0 && 
               reserve == 0)
    {
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

        
        if(mca_btl_openib_component.leave_pinned) { 
            if(mca_btl_openib_component.reg_mru_len <= openib_btl->reg_mru_list.opal_list_length  ) {
                
                mca_mpool_openib_registration_t* old_reg =
                    (mca_mpool_openib_registration_t*)
                    opal_list_remove_last(&openib_btl->reg_mru_list);
                
                if( NULL == old_reg) { 
                    opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                }

                                
                rc = mca_mpool_base_remove((void*) old_reg->base_reg.base); 
                
                if(OMPI_SUCCESS != rc) { 
                    opal_output(0,"%s:%d:%s error removing memory region from memory pool tree", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                }
                
                               
                OBJ_RELEASE(old_reg);
            }
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool,
                                            iov.iov_base, 
                                            max_data, 
                                            (mca_mpool_base_registration_t**) &openib_reg); 
            
            rc = mca_mpool_base_insert(openib_reg->base_reg.base, 
                                       openib_reg->base_reg.bound - openib_reg->base_reg.base + 1, 
                                       openib_btl->ib_pool, 
                                       (void*) (&openib_btl->super), 
                                       (mca_mpool_base_registration_t*) openib_reg); 
            if(rc != OMPI_SUCCESS) 
                return NULL; 
            OBJ_RETAIN(openib_reg); 
            
            openib_reg->base_reg.is_leave_pinned = true; 
                    
            opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg);
            
        } else { 
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool,
                                            iov.iov_base, 
                                            max_data, 
                                            (mca_mpool_base_registration_t**) &openib_reg); 
            
            openib_reg->base_reg.is_leave_pinned = false; 
        } 
        frag->mr = openib_reg->mr; 
        frag->sg_entry.length = max_data; 
        frag->sg_entry.lkey = openib_reg->mr->lkey;
        frag->sg_entry.addr = (uintptr_t) iov.iov_base; 
        
        frag->segment.seg_key.key32[0] = (uint32_t) frag->mr->lkey; 
            
        frag->base.des_src = &frag->segment;
        frag->base.des_src_cnt = 1;
        frag->base.des_dst = NULL;
        frag->base.des_dst_cnt = 0;
        frag->openib_reg = openib_reg; 
        return &frag->base;

    } else if (max_data+reserve <=  btl->btl_eager_limit) { 
           
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
        
    }
       /** if the data fits in the max limit and we aren't told to pinn then we 
           simply pack, if the data  is non contiguous then we pack **/ 
       
       else if(max_data + reserve <= openib_btl->super.btl_max_send_size) { 
           
        MCA_BTL_IB_FRAG_ALLOC_MAX(btl, frag, rc); 
        if(NULL == frag) { 
            return NULL; 
        } 
        if(max_data + reserve > frag->size){ 
            max_data = frag->size - reserve; 
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
 * Pack data
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
mca_btl_base_descriptor_t* mca_btl_openib_prepare_dst(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    size_t reserve,
    size_t* size)
{
    mca_btl_openib_module_t* openib_btl; 
    mca_btl_openib_frag_t* frag; 
    mca_mpool_openib_registration_t * openib_reg; 
    int rc; 
    size_t reg_len; 

    openib_btl = (mca_btl_openib_module_t*) btl; 
    openib_reg = (mca_mpool_openib_registration_t*) registration; 
    
    MCA_BTL_IB_FRAG_ALLOC_FRAG(btl, frag, rc); 

    if(NULL == frag){
        return NULL; 
    }
    
    
    frag->segment.seg_len = *size; 
    frag->segment.seg_addr.pval = convertor->pBaseBuf + convertor->bConverted; 
    frag->base.des_flags = 0; 

    if(NULL!= openib_reg){ 
        reg_len = (unsigned char*)openib_reg->base_reg.bound - (unsigned char*)frag->segment.seg_addr.pval + 1; 
        bool is_leave_pinned = openib_reg->base_reg.is_leave_pinned; 

        if(frag->segment.seg_len > reg_len ) { 
            size_t new_len = openib_reg->base_reg.bound - openib_reg->base_reg.base + 1 
                + frag->segment.seg_len - reg_len; 
            void * base_addr = openib_reg->base_reg.base; 

            rc = mca_mpool_base_remove((void*) openib_reg->base_reg.base); 
            if(OMPI_SUCCESS != rc) { 
                opal_output(0,"%s:%d:%s error removing memory region from memory pool tree", __FILE__, __LINE__,  __func__); 
                return NULL; 
            } 

            if(is_leave_pinned) { 
                if(NULL == opal_list_remove_item(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg)) { 
                    opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                }
            }
            OBJ_RELEASE(openib_reg); 
            
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool, 
                                            base_addr, 
                                            new_len,
                                            (mca_mpool_base_registration_t**) &openib_reg);
            
        
            rc = mca_mpool_base_insert(openib_reg->base_reg.base, 
                                       openib_reg->base_reg.bound - openib_reg->base_reg.base + 1, 
                                       openib_btl->ib_pool, 
                                       (void*) (&openib_btl->super), 
                                       (mca_mpool_base_registration_t*) openib_reg); 
            
            if(OMPI_SUCCESS != rc) {
                opal_output(0,"%s:%d:%s error inserting memory region into memory pool tree", __FILE__, __LINE__,  __func__); 
                return NULL;
            }
            OBJ_RETAIN(openib_reg); 
            
            if(is_leave_pinned) { 
                openib_reg->base_reg.is_leave_pinned = is_leave_pinned; 
                opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg);
            } 

        } 
        else if(is_leave_pinned){ 
            if(NULL == opal_list_remove_item(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg)) { 
                opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                return NULL; 
            }    
            opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg); 
        }
    }  else { 
           
        if(mca_btl_openib_component.leave_pinned) { 
            
        
            if( mca_btl_openib_component.reg_mru_len <= openib_btl->reg_mru_list.opal_list_length  ) {
               
                mca_mpool_openib_registration_t* old_reg =
                    (mca_mpool_openib_registration_t*)
                    opal_list_remove_last(&openib_btl->reg_mru_list);
                
                if( NULL == old_reg) { 
                    opal_output(0,"%s:%d:%s error removing item from reg_mru_list", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                }
                
                rc = mca_mpool_base_remove((void*) old_reg->base_reg.base); 
                if(OMPI_SUCCESS !=rc ) { 
                    opal_output(0,"%s:%d:%s error removing memory region from memory pool tree", __FILE__, __LINE__,  __func__); 
                    return NULL; 
                } 
                
                OBJ_RELEASE(old_reg);
                
            }
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool,
                                        frag->segment.seg_addr.pval,
                                        *size, 
                                        (mca_mpool_base_registration_t**) &openib_reg);
            
            openib_reg->base_reg.is_leave_pinned = true;
            
            rc = mca_mpool_base_insert(openib_reg->base_reg.base,  
                                       openib_reg->base_reg.bound - openib_reg->base_reg.base + 1, 
                                       openib_btl->ib_pool, 
                                       (void*) (&openib_btl->super), 
                                       (mca_mpool_base_registration_t*)  openib_reg); 
            if(OMPI_SUCCESS != rc){ 
                opal_output(0,"%s:%d:%s error inserting memory region into memory pool", __FILE__, __LINE__,  __func__); 
                return NULL;
            } 

            OBJ_RETAIN(openib_reg); 
            opal_list_append(&openib_btl->reg_mru_list, (opal_list_item_t*) openib_reg);
            
        } else { 
            openib_btl->ib_pool->mpool_register(openib_btl->ib_pool,
                                            frag->segment.seg_addr.pval,
                                            *size, 
                                            (mca_mpool_base_registration_t**) &openib_reg);
            openib_reg->base_reg.is_leave_pinned=false; 
        }
        
        
    }

    
    frag->mr = openib_reg->mr; 
    frag->sg_entry.length = *size; 
    frag->sg_entry.lkey = openib_reg->mr->lkey; 
    frag->sg_entry.addr = (uintptr_t) frag->segment.seg_addr.pval; 
    
    frag->segment.seg_key.key32[0] = (uint32_t) frag->mr->lkey; 
    
    frag->base.des_dst = &frag->segment; 
    frag->base.des_dst_cnt = 1; 
    frag->base.des_src = NULL; 
    frag->base.des_src_cnt = 0; 
    frag->openib_reg = openib_reg; 
    OBJ_RETAIN(openib_reg); 
    return &frag->base; 
    
}

int mca_btl_openib_finalize(struct mca_btl_base_module_t* btl)
{
    mca_btl_openib_module_t* openib_btl; 
    openib_btl = (mca_btl_openib_module_t*) btl; 
    
    if(openib_btl->send_free_eager.fl_num_allocated != 
       openib_btl->send_free_eager.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_eager frags: %d allocated %d returned \n", 
                    openib_btl->send_free_eager.fl_num_allocated, 
                    openib_btl->send_free_eager.super.opal_list_length); 
    }
    if(openib_btl->send_free_max.fl_num_allocated != 
      openib_btl->send_free_max.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_max frags: %d allocated %d returned \n", 
                    openib_btl->send_free_max.fl_num_allocated, 
                    openib_btl->send_free_max.super.opal_list_length); 
    }
    if(openib_btl->send_free_frag.fl_num_allocated != 
       openib_btl->send_free_frag.super.opal_list_length){ 
        opal_output(0, "btl ib send_free_frag frags: %d allocated %d returned \n", 
                    openib_btl->send_free_frag.fl_num_allocated, 
                    openib_btl->send_free_frag.super.opal_list_length); 
    }
    
    if(openib_btl->recv_free_eager.fl_num_allocated != 
       openib_btl->recv_free_eager.super.opal_list_length){ 
        opal_output(0, "btl ib recv_free_eager frags: %d allocated %d returned \n", 
                    openib_btl->recv_free_eager.fl_num_allocated, 
                    openib_btl->recv_free_eager.super.opal_list_length); 
    }

    if(openib_btl->recv_free_max.fl_num_allocated != 
       openib_btl->recv_free_max.super.opal_list_length){ 
        opal_output(0, "btl ib recv_free_max frags: %d allocated %d returned \n", 
                    openib_btl->recv_free_max.fl_num_allocated, 
                    openib_btl->recv_free_max.super.opal_list_length); 
    }

    return OMPI_SUCCESS;
}

/*
 *  Initiate a send. If this is the first fragment, use the fragment
 *  descriptor allocated with the send requests, otherwise obtain
 *  one from the free list. Initialize the fragment and foward
 *  on to the peer.
 */

int mca_btl_openib_send( 
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_btl_base_descriptor_t* descriptor, 
    mca_btl_base_tag_t tag)
   
{
    
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*)descriptor; 
    frag->endpoint = endpoint; 
        
    frag->hdr->tag = tag; 
    frag->type = MCA_BTL_IB_FRAG_SEND; 
    frag->rc = mca_btl_openib_endpoint_send(endpoint, frag);
           
    return frag->rc;
}

/*
 * RDMA local buffer to remote buffer address.
 */

int mca_btl_openib_put( mca_btl_base_module_t* btl,
                    mca_btl_base_endpoint_t* endpoint,
                    mca_btl_base_descriptor_t* descriptor)
{
    struct ibv_send_wr* bad_wr; 
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*) descriptor; 
    frag->endpoint = endpoint;
    frag->sr_desc.opcode = IBV_WR_RDMA_WRITE; 
    frag->sr_desc.wr.rdma.remote_addr = (uintptr_t) frag->base.des_src->seg_addr.pval; 
    frag->sr_desc.wr.rdma.rkey = frag->base.des_dst->seg_key.key32[0]; 
    frag->sg_entry.addr = (uintptr_t) frag->base.des_src->seg_addr.pval; 
    frag->sg_entry.length  = frag->base.des_src->seg_len; 

    if(ibv_post_send(endpoint->lcl_qp_low, 
                     &frag->sr_desc, 
                     &bad_wr)){ 
        opal_output(0, "%s: error posting send request errno says %s\n", __func__, strerror(errno)); 
        return OMPI_ERROR; 
    }  

    mca_btl_openib_endpoint_post_rr(endpoint, 1); 

    return OMPI_SUCCESS; 

}



/*
 * Asynchronous event handler to detect unforseen
 * events. Usually, such events are catastrophic.
 * Should have a robust mechanism to handle these
 * events and abort the OMPI application if necessary.
 *
 */
/* static void async_event_handler(VAPI_hca_hndl_t hca_hndl, */
/*         VAPI_event_record_t * event_p, */
/*         void *priv_data) */
/* { */
/*     switch (event_p->type) { */
/*         case VAPI_QP_PATH_MIGRATED: */
/*         case VAPI_EEC_PATH_MIGRATED: */
/*         case VAPI_QP_COMM_ESTABLISHED: */
/*         case VAPI_EEC_COMM_ESTABLISHED: */
/*         case VAPI_SEND_QUEUE_DRAINED: */
/*         case VAPI_PORT_ACTIVE: */
/*             { */
/*                 DEBUG_OUT("Got an asynchronous event: %s\n", */
/*                         VAPI_event_record_sym(event_p->type)); */
/*                 break; */
/*             } */
/*         case VAPI_CQ_ERROR: */
/*         case VAPI_LOCAL_WQ_INV_REQUEST_ERROR: */
/*         case VAPI_LOCAL_WQ_ACCESS_VIOL_ERROR: */
/*         case VAPI_LOCAL_WQ_CATASTROPHIC_ERROR: */
/*         case VAPI_PATH_MIG_REQ_ERROR: */
/*         case VAPI_LOCAL_EEC_CATASTROPHIC_ERROR: */
/*         case VAPI_LOCAL_CATASTROPHIC_ERROR: */
/*         case VAPI_PORT_ERROR: */
/*             { */
/*                 opal_output(0, "Got an asynchronous event: %s (%s)", */
/*                         VAPI_event_record_sym(event_p->type), */
/*                         VAPI_event_syndrome_sym(event_p-> */
/*                             syndrome)); */
/*                 break; */
/*             } */
/*         default: */
/*             opal_output(0, "Warning!! Got an undefined " */
/*                     "asynchronous event\n"); */
/*     } */

/* } */




int mca_btl_openib_module_init(mca_btl_openib_module_t *openib_btl)
{

    /* Allocate Protection Domain */ 
    struct ibv_context *ctx; 
    
    
    ctx = openib_btl->ib_dev_context; 
    openib_btl->ib_pd = ibv_alloc_pd(ctx); 
    
    
    if(NULL == openib_btl->ib_pd) {
        opal_output(0, "%s: error allocating pd for %s errno says %s\n", 
                    __func__, 
                    ibv_get_device_name(openib_btl->ib_dev), 
                    strerror(errno)); 
        return OMPI_ERROR;
    }
    
    openib_btl->ib_cq_low = ibv_create_cq(ctx, mca_btl_openib_component.ib_cq_size, NULL); 
    
    if(NULL == openib_btl->ib_cq_low) {
        opal_output(0, "%s: error creating low priority cq for %s errno says %s\n",
                    __func__, 
                    ibv_get_device_name(openib_btl->ib_dev), 
                    strerror(errno)); 
        return OMPI_ERROR;
    }

    openib_btl->ib_cq_high = ibv_create_cq(ctx, mca_btl_openib_component.ib_cq_size, NULL); 
    
    if(NULL == openib_btl->ib_cq_high) {
        opal_output(0, "%s: error creating high priority cq for %s errno says %s\n", 
                    __func__, 
                    ibv_get_device_name(openib_btl->ib_dev), 
                    strerror(errno)); 
        return OMPI_ERROR;
    }
        
    /* TODO: EVAPI_set_qsync_event_handler? */ 

    return OMPI_SUCCESS;
}
