/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2006-2007 Mellanox Technologies. All rights reserved.
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
#include "opal/util/show_help.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/btl/btl.h"
#include "ompi/mca/btl/base/btl_base_error.h"
#include "btl_openib.h"
#include "btl_openib_frag.h" 
#include "btl_openib_proc.h"
#include "btl_openib_endpoint.h"
#include "ompi/datatype/convertor.h" 
#include "ompi/datatype/datatype.h" 
#include "ompi/mca/mpool/base/base.h" 
#include "ompi/mca/mpool/mpool.h" 
#include "ompi/mca/mpool/rdma/mpool_rdma.h"
#include "ompi/runtime/params.h"
#include "orte/util/sys_info.h"
#include <errno.h> 
#include <string.h> 
#include <math.h>
#include <inttypes.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif


mca_btl_openib_module_t mca_btl_openib_module = {
    {
        &mca_btl_openib_component.super,
        0, /* max size of first fragment */
        0, /* min send fragment size */
        0, /* max send fragment size */
        0, /* btl_rdma_pipeline_send_length */
        0, /* btl_rdma_pipeline_frag_size */
        0, /* btl_min_rdma_pipeline_size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        0, /* TODO this should be PUT btl flags */
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
        mca_btl_openib_get,
        mca_btl_base_dump,
        NULL, /* mpool */
        mca_btl_openib_register_error_cb, /* error call back registration */
        mca_btl_openib_ft_event
    }
};

/*
 * Local functions
 */
static int mca_btl_openib_size_queues( struct mca_btl_openib_module_t* openib_btl, size_t nprocs);
static int mca_btl_openib_create_cq_srq(mca_btl_openib_module_t* openib_btl);
static int mca_btl_finalize_hca(struct mca_btl_openib_hca_t *hca);


static void show_init_error(const char *file, int line, 
                            const char *func, const char *dev) 
{
    if (ENOMEM == errno) {
        int ret;
        struct rlimit limit;
        char *str_limit = NULL;

        ret = getrlimit(RLIMIT_MEMLOCK, &limit);
        if (0 != ret) {
            asprintf(&str_limit, "Unknown");
        } else if (limit.rlim_cur == RLIM_INFINITY) {
            asprintf(&str_limit, "unlimited");
        } else {
            asprintf(&str_limit, "%ld", (long)limit.rlim_cur);
        }

        opal_show_help("help-mpi-btl-openib.txt", "init-fail-no-mem",
                       true, orte_system_info.nodename, 
                       file, line, func, dev, str_limit);

        if (NULL != str_limit) free(str_limit);
    } else {
        opal_show_help("help-mpi-btl-openib.txt", "init-fail-create-q",
                       true, orte_system_info.nodename, 
                       file, line, func, strerror(errno), errno, dev);
    }
}


/* 
 *  add a proc to this btl module 
 *    creates an endpoint that is setup on the
 *    first send to the endpoint
 */ 
int mca_btl_openib_add_procs(
    struct mca_btl_base_module_t* btl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_procs, 
    struct mca_btl_base_endpoint_t** peers, 
    ompi_bitmap_t* reachable)
{
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*)btl;
    int i,j, rc;
    int rem_subnet_id_port_cnt;
    int lcl_subnet_id_port_cnt = 0;
    int btl_rank = 0;
    mca_btl_base_endpoint_t* endpoint;
    
    for(j=0; j < mca_btl_openib_component.ib_num_btls; j++){ 
        if(mca_btl_openib_component.openib_btls[j]->port_info.subnet_id
           == openib_btl->port_info.subnet_id) { 
            if(openib_btl == mca_btl_openib_component.openib_btls[j]) { 
                btl_rank = lcl_subnet_id_port_cnt;
            }
            lcl_subnet_id_port_cnt++;
        }
    }

    for(i = 0; i < (int) nprocs; i++) {
        struct ompi_proc_t* ompi_proc = ompi_procs[i];
        mca_btl_openib_proc_t* ib_proc;
    /*     mca_btl_base_endpoint_t* endpoint; */
        
        if(NULL == (ib_proc = mca_btl_openib_proc_create(ompi_proc))) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        rem_subnet_id_port_cnt  = 0;
        /* check if the remote proc has a reachable subnet first */
        BTL_VERBOSE(("got %d port_infos \n", ib_proc->proc_port_count));
        for(j = 0; j < (int) ib_proc->proc_port_count; j++){
            BTL_VERBOSE(("got a subnet %016x\n",
                         ib_proc->proc_ports[j].subnet_id));
            if(ib_proc->proc_ports[j].subnet_id ==
               openib_btl->port_info.subnet_id) {
                BTL_VERBOSE(("Got a matching subnet!\n"));
                rem_subnet_id_port_cnt ++;
            }
        }
        
        if(!rem_subnet_id_port_cnt ) {
            /* no use trying to communicate with this endpointlater */
            BTL_VERBOSE(("No matching subnet id was found, moving on.. \n"));
            continue;
        }

#if 0
        num_endpoints = rem_subnet_id_port_cnt  / lcl_subnet_id_port_cnt + 
            (btl_rank < (rem_subnet_id_port_cnt  / lcl_subnet_id_port_cnt)) ? 1:0;
#endif 

        if(rem_subnet_id_port_cnt  < lcl_subnet_id_port_cnt && 
           btl_rank >= rem_subnet_id_port_cnt ) {
            BTL_VERBOSE(("Not enough remote ports on this subnet id, moving on.. \n"));
            continue;
            
        }
        OPAL_THREAD_LOCK(&ib_proc->proc_lock);

        /* The btl_proc datastructure is shared by all IB BTL
         * instances that are trying to reach this destination. 
         * Cache the peer instance on the btl_proc.
         */
        endpoint = OBJ_NEW(mca_btl_openib_endpoint_t);
        assert(((opal_object_t*)endpoint)->obj_reference_count == 1);
        if(NULL == endpoint) {
            OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
            
        endpoint->endpoint_btl = openib_btl;
        endpoint->use_eager_rdma = openib_btl->hca->use_eager_rdma &
            mca_btl_openib_component.use_eager_rdma;
        endpoint->subnet_id = openib_btl->port_info.subnet_id; 
        rc = mca_btl_openib_proc_insert(ib_proc, endpoint);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(endpoint);
            OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
            continue;
        }
        
        orte_pointer_array_add((orte_std_cntr_t*)&endpoint->index,
                               openib_btl->endpoints, (void*)endpoint);
        ompi_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&ib_proc->proc_lock);
        
        peers[i] = endpoint;
    }

    return mca_btl_openib_size_queues(openib_btl, nprocs);
}

static int mca_btl_openib_size_queues( struct mca_btl_openib_module_t* openib_btl, size_t nprocs) 
{
    int min_hp_cq_size = 0, min_lp_cq_size = 0;
    int first_time = (0 == openib_btl->num_peers);
    int rc;
    int qp;
    openib_btl->num_peers += nprocs;
    
    
    /* figure out reasonable sizes for completion queues */
    for(qp = 0; qp < mca_btl_openib_component.num_qps; qp++) { 
        if(MCA_BTL_OPENIB_SRQ_QP == mca_btl_openib_component.qp_infos[qp].type) { 
            if(mca_btl_openib_component.qp_infos[qp].size <=
               mca_btl_openib_component.eager_limit){
                min_hp_cq_size += mca_btl_openib_component.qp_infos[qp].rd_num + 
                    mca_btl_openib_component.qp_infos[qp].u.srq_qp.sd_max;
            } else {
                min_lp_cq_size += mca_btl_openib_component.qp_infos[qp].rd_num + 
                    mca_btl_openib_component.qp_infos[qp].u.srq_qp.sd_max;
            }
        }
        else { 
            if(mca_btl_openib_component.qp_infos[qp].size <=
               mca_btl_openib_component.eager_limit){
                min_hp_cq_size += mca_btl_openib_component.qp_infos[qp].rd_num 
                    * 2 * openib_btl->num_peers;
            } else {
                min_lp_cq_size += mca_btl_openib_component.qp_infos[qp].rd_num 
                    * 2 * openib_btl->num_peers;
            }
        }
    }
    
    /* make sure we don't exceed the maximum CQ size and that we 
     * don't size the queue smaller than otherwise requested 
     */
    if(min_lp_cq_size > (int32_t) mca_btl_openib_component.ib_lp_cq_size) { 
        mca_btl_openib_component.ib_lp_cq_size = 
            min_lp_cq_size > openib_btl->hca->ib_dev_attr.max_cq ? 
            openib_btl->hca->ib_dev_attr.max_cq : min_lp_cq_size;
    }
    if(min_hp_cq_size > (int32_t) mca_btl_openib_component.ib_hp_cq_size) { 
        mca_btl_openib_component.ib_hp_cq_size = 
            min_hp_cq_size > openib_btl->hca->ib_dev_attr.max_cq ? 
            openib_btl->hca->ib_dev_attr.max_cq : min_hp_cq_size;
    }

#ifdef HAVE_IBV_RESIZE_CQ
    if(!first_time) { 
        rc = ibv_resize_cq(openib_btl->ib_cq[BTL_OPENIB_LP_CQ], 
                           mca_btl_openib_component.ib_lp_cq_size);
        if(rc) {
            BTL_ERROR(("cannot resize low priority completion queue, error: %d", rc));
            return OMPI_ERROR;
        }
        rc = ibv_resize_cq(openib_btl->ib_cq[BTL_OPENIB_HP_CQ],
                           mca_btl_openib_component.ib_hp_cq_size);
        if(rc) {
            BTL_ERROR(("cannot resize high priority completion queue, error: %d", rc));
            return OMPI_ERROR;
        }
    }
#endif

    if(first_time) { 
        return mca_btl_openib_create_cq_srq(openib_btl); 
    }
    return OMPI_SUCCESS;
}
/* 
 * delete the proc as reachable from this btl module 
 */
int mca_btl_openib_del_procs(struct mca_btl_base_module_t* btl, 
        size_t nprocs, 
        struct ompi_proc_t **procs, 
        struct mca_btl_base_endpoint_t ** peers)
{
    int i,ep_index;
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl;
    mca_btl_openib_endpoint_t* endpoint;
        
    /* opal_output(0, "del_procs called!\n"); */
    for (i=0 ; i < (int) nprocs ; i++) {
        mca_btl_base_endpoint_t* del_endpoint = peers[i];
        for(ep_index=0;
            ep_index < orte_pointer_array_get_size(openib_btl->endpoints);
            ep_index++) {
            endpoint = 
                orte_pointer_array_get_item(openib_btl->endpoints,ep_index);
            if(!endpoint) {
                continue;
            }
            if (endpoint == del_endpoint) {
                opal_output(mca_btl_base_output,"in del_procs %d, setting another endpoint to null\n", 
                            ep_index); 
                orte_pointer_array_set_item(openib_btl->endpoints,ep_index,NULL);
                assert(((opal_object_t*)endpoint)->obj_reference_count == 1);
                OBJ_RELEASE(endpoint);
            }
        }
    }

    return OMPI_SUCCESS;
}

/* 
 *Register callback function to support send/recv semantics 
 */ 
int mca_btl_openib_register(
                        struct mca_btl_base_module_t* btl, 
                        mca_btl_base_tag_t tag, 
                        mca_btl_base_module_recv_cb_fn_t cbfunc, 
                        void* cbdata)
{
    
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl; 
    
    OPAL_THREAD_LOCK(&openib_btl->ib_lock); 
    openib_btl->ib_reg[tag].cbfunc = cbfunc; 
    openib_btl->ib_reg[tag].cbdata = cbdata; 
    OPAL_THREAD_UNLOCK(&openib_btl->ib_lock); 
    return OMPI_SUCCESS;
}


/* 
 *Register callback function for error handling..
 */ 
int mca_btl_openib_register_error_cb(
                        struct mca_btl_base_module_t* btl, 
                        mca_btl_base_module_error_cb_fn_t cbfunc)
{
    
    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl; 
    openib_btl->error_cb = cbfunc; /* stash for later */
    return OMPI_SUCCESS;
}

/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
  * @param size (IN) Size of segment to allocate    
 * 
 * When allocating a segment we pull a pre-alllocated segment 
 * from one of two free lists, an eager list and a max list
 */
mca_btl_base_descriptor_t* mca_btl_openib_alloc(
    struct mca_btl_base_module_t* btl,
    uint8_t order,
    size_t size)
{
    mca_btl_openib_frag_t* frag = NULL;
    mca_btl_openib_module_t* openib_btl; 
    int rc;
    openib_btl = (mca_btl_openib_module_t*) btl; 
    MCA_BTL_IB_FRAG_ALLOC_BY_SIZE(btl, frag, size, rc);
    
    if(NULL == frag)
        return NULL;
    
    /* GMS is this necessary anymore ? */
    frag->segment.seg_len = size;
    frag->base.order = order;
    frag->base.des_flags = 0;
   
    assert(frag->qp_idx <= order);
    return (mca_btl_base_descriptor_t*)frag;
}

/** 
 * Return a segment 
 * 
 * Return the segment to the appropriate 
 *  preallocated segment list 
 */ 
int mca_btl_openib_free(
                    struct mca_btl_base_module_t* btl, 
                    mca_btl_base_descriptor_t* des) 
{
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*)des; 
    
    /* is this fragment pointing at user memory? */
    if(((MCA_BTL_OPENIB_FRAG_SEND_USER == frag->type) ||
        (MCA_BTL_OPENIB_FRAG_RECV_USER == frag->type)) 
       && frag->registration != NULL) {
        btl->btl_mpool->mpool_deregister(btl->btl_mpool,
                                         (mca_mpool_base_registration_t*)
                                         frag->registration);
        frag->registration = NULL;
    }
    
    MCA_BTL_IB_FRAG_RETURN(((mca_btl_openib_module_t*) btl), frag);
        
    return OMPI_SUCCESS; 
}


/**
 * register user buffer or pack 
 * data into pre-registered buffer and return a 
 * descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 *  
 * prepare source's behavior depends on the following: 
 * Has a valid memory registration been passed to prepare_src? 
 *    if so we attempt to use the pre-registered user-buffer, if the memory registration 
 *    is too small (only a portion of the user buffer) then we must reregister the user buffer 
 * Has the user requested the memory to be left pinned? 
 *    if so we insert the memory registration into a memory tree for later lookup, we 
 *    may also remove a previous registration if a MRU (most recently used) list of 
 *    registrations is full, this prevents resources from being exhausted.
 * Is the requested size larger than the btl's max send size? 
 *    if so and we aren't asked to leave the registration pinned, then we register the memory if 
 *    the users buffer is contiguous 
 * Otherwise we choose from two free lists of pre-registered memory in which to pack the data into. 
 * 
 */
mca_btl_base_descriptor_t* mca_btl_openib_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    uint8_t order,
    size_t reserve,
    size_t* size
)
{
    mca_btl_openib_module_t *openib_btl;
    mca_btl_openib_frag_t *frag = NULL;
    mca_btl_openib_reg_t *openib_reg;
    struct iovec iov;
    uint32_t iov_count = 1;
    size_t max_data = *size;
    int rc;

    openib_btl = (mca_btl_openib_module_t*)btl;

    if(ompi_convertor_need_buffers(convertor) == false && 0 == reserve) {
        /* GMS  bloody HACK! */
        if(registration != NULL || max_data > btl->btl_max_send_size) {
            MCA_BTL_IB_FRAG_ALLOC_SEND_USER(btl, frag, rc);
            if(NULL == frag) {
                return NULL;
            }
            
            iov.iov_len = max_data;
            iov.iov_base = NULL;
            
            ompi_convertor_pack(convertor, &iov, &iov_count, &max_data);
            
            *size = max_data;

            if(NULL == registration) {
                rc = btl->btl_mpool->mpool_register(btl->btl_mpool,
                        iov.iov_base, max_data, 0, &registration);
                if(OMPI_SUCCESS != rc || NULL == registration) {
                    MCA_BTL_IB_FRAG_RETURN(openib_btl, frag);
                    return NULL;
                }
                /* keep track of the registration we did */
                frag->registration = (mca_btl_openib_reg_t*)registration;
            }
            openib_reg = (mca_btl_openib_reg_t*)registration;

            frag->base.order = order;
            frag->base.des_flags = 0;
            frag->base.des_src = &frag->segment;
            frag->base.des_src_cnt = 1;
            frag->base.des_dst = NULL;
            frag->base.des_dst_cnt = 0;
            frag->base.des_flags = 0;

            frag->sg_entry.length = max_data;
            frag->sg_entry.lkey = openib_reg->mr->lkey;
            frag->sg_entry.addr = (unsigned long)iov.iov_base;

            frag->segment.seg_len = max_data;
            frag->segment.seg_addr.pval = iov.iov_base;
            frag->segment.seg_key.key32[0] = (uint32_t)frag->sg_entry.lkey;
            
            assert(MCA_BTL_NO_ORDER == order);
            
            BTL_VERBOSE(("frag->sg_entry.lkey = %lu .addr = %llu "
                        "frag->segment.seg_key.key32[0] = %lu",
                        frag->sg_entry.lkey, frag->sg_entry.addr,
                        frag->segment.seg_key.key32[0]));

            return &frag->base;
        }
    }
    
    assert(MCA_BTL_NO_ORDER == order); 
    
    if(max_data + reserve > btl->btl_max_send_size) {
        max_data = btl->btl_max_send_size - reserve;
    }
    
    MCA_BTL_IB_FRAG_ALLOC_BY_SIZE(btl, frag, max_data + reserve, rc);
   
    if(NULL == frag)
        return NULL;

    iov.iov_len = max_data;
    iov.iov_base = (unsigned char*)frag->segment.seg_addr.pval + reserve;
    rc = ompi_convertor_pack(convertor, &iov, &iov_count, &max_data);
    
    *size  = max_data;
    frag->segment.seg_len = max_data + reserve;
    frag->segment.seg_key.key32[0] = (uint32_t)frag->sg_entry.lkey;
    /* frag->base.order = order; */
    frag->base.des_src = &frag->segment;
    frag->base.des_src_cnt = 1;
    frag->base.des_dst = NULL;
    frag->base.des_dst_cnt = 0;
    frag->base.des_flags = 0;
    frag->base.order = order;
    return &frag->base;
}

/**
 * Prepare the dst buffer
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 * prepare dest's behavior depends on the following: 
 * Has a valid memory registration been passed to prepare_src? 
 *    if so we attempt to use the pre-registered user-buffer, if the memory registration 
 *    is to small (only a portion of the user buffer) then we must reregister the user buffer 
 * Has the user requested the memory to be left pinned? 
 *    if so we insert the memory registration into a memory tree for later lookup, we 
 *    may also remove a previous registration if a MRU (most recently used) list of 
 *    registrations is full, this prevents resources from being exhausted.
 */
mca_btl_base_descriptor_t* mca_btl_openib_prepare_dst(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration,
    struct ompi_convertor_t* convertor,
    uint8_t order,
    size_t reserve,
    size_t* size)
{
    mca_btl_openib_module_t *openib_btl;
    mca_btl_openib_frag_t *frag;
    mca_btl_openib_reg_t *openib_reg;
    int rc;

    openib_btl = (mca_btl_openib_module_t*)btl;
        
    MCA_BTL_IB_FRAG_ALLOC_RECV_USER(btl, frag, rc);
    if(NULL == frag) {
        abort();
        return NULL;
    }
    
    ompi_convertor_get_current_pointer( convertor, (void**)&(frag->segment.seg_addr.pval) );

    if(NULL == registration){
        /* we didn't get a memory registration passed in, so we have to
         * register the region ourselves
         */ 
        rc = btl->btl_mpool->mpool_register(btl->btl_mpool,
                frag->segment.seg_addr.pval, *size, 0, &registration);
        if(OMPI_SUCCESS != rc || NULL == registration) {
            MCA_BTL_IB_FRAG_RETURN(openib_btl, frag);
            abort();
            return NULL;
        }
        /* keep track of the registration we did */
        frag->registration = (mca_btl_openib_reg_t*)registration;
    }
    openib_reg = (mca_btl_openib_reg_t*)registration;

    frag->sg_entry.length = *size;
    frag->sg_entry.lkey = openib_reg->mr->lkey;
    frag->sg_entry.addr = (unsigned long) frag->segment.seg_addr.pval;

    frag->segment.seg_len = *size;
    frag->segment.seg_key.key32[0] = openib_reg->mr->rkey;

    frag->base.order = order;
    frag->base.des_dst = &frag->segment;
    frag->base.des_dst_cnt = 1;
    frag->base.des_src = NULL;
    frag->base.des_src_cnt = 0;
    frag->base.des_flags = 0;

    BTL_VERBOSE(("frag->sg_entry.lkey = %lu .addr = %llu "
                "frag->segment.seg_key.key32[0] = %lu",
                frag->sg_entry.lkey, frag->sg_entry.addr,
                frag->segment.seg_key.key32[0]));

    return &frag->base;
}

static int mca_btl_finalize_hca(struct mca_btl_openib_hca_t *hca)
{
#if OMPI_HAVE_THREADS
    int hca_to_remove;
#if OMPI_ENABLE_PROGRESS_THREADS == 1
    if(hca->progress) {
        hca->progress = false;
        if (pthread_cancel(hca->thread.t_handle)) {
            BTL_ERROR(("Failed to cancel OpenIB progress thread"));
        }
        opal_thread_join(&hca->thread, NULL);
    }
    if (ibv_destroy_comp_channel(hca->ib_channel)) {
        BTL_VERBOSE(("Failed to close comp_channel"));
        return OMPI_ERROR;
    }
#endif
    /* signaling to async_tread to stop poll for this hca */
    if (mca_btl_openib_component.use_async_event_thread) {
        hca_to_remove=-(hca->ib_dev_context->async_fd);
        if (write(mca_btl_openib_component.async_pipe[1],
                    &hca_to_remove,sizeof(int))<0){
            BTL_ERROR(("Failed to write to pipe"));
            return OMPI_ERROR;
        }
    }
#endif
    if (OMPI_SUCCESS != mca_mpool_base_module_destroy(hca->mpool)) {
        BTL_VERBOSE(("Failed to release mpool"));
        return OMPI_ERROR;
    }
    if (ibv_dealloc_pd(hca->ib_pd)) {
        if (ompi_mpi_leave_pinned || ompi_mpi_leave_pinned_pipeline) {
            BTL_VERBOSE(("Warning! Failed to release PD"));
            return OMPI_SUCCESS;
        } else {
            BTL_ERROR(("Error! Failed to release PD"));
            return OMPI_ERROR;
        }
    }
    if (ibv_close_device(hca->ib_dev_context)) {
        if (ompi_mpi_leave_pinned || ompi_mpi_leave_pinned_pipeline) {
            BTL_VERBOSE(("Warning! Failed to close HCA"));
            return OMPI_SUCCESS;
        } else {
            BTL_ERROR(("Error! Failed to close HCA"));
            return OMPI_ERROR;
        }
    }
    OBJ_DESTRUCT(&hca->hca_lock); 
    free(hca);
    return OMPI_SUCCESS;
}

int mca_btl_openib_finalize(struct mca_btl_base_module_t* btl)
{
    mca_btl_openib_module_t* openib_btl; 
    mca_btl_openib_endpoint_t* endpoint;
    int ep_index, rdma_index, i;
    int qp;
    
    /* return OMPI_SUCCESS; */
    
    openib_btl = (mca_btl_openib_module_t*) btl; 

    /* Remove the btl from component list */
    if ( mca_btl_openib_component.ib_num_btls > 1 ) {
        for(i = 0; i < mca_btl_openib_component.ib_num_btls; i++){
            if (mca_btl_openib_component.openib_btls[i] == openib_btl){
                mca_btl_openib_component.openib_btls[i] =
                    mca_btl_openib_component.openib_btls[mca_btl_openib_component.ib_num_btls-1];
                break;
            }
        }
    }
    mca_btl_openib_component.ib_num_btls--;

    /* Release eager RDMAs */
    for(rdma_index=0;
            rdma_index < orte_pointer_array_get_size(openib_btl->eager_rdma_buffers);
        rdma_index++) {
        endpoint=orte_pointer_array_get_item(openib_btl->eager_rdma_buffers,rdma_index);
        if(!endpoint) {
            continue;
        }
        OBJ_RELEASE(endpoint);
    }
    /* Release all QPs */
    for(ep_index=0;
            ep_index < orte_pointer_array_get_size(openib_btl->endpoints);
            ep_index++) {
        endpoint=orte_pointer_array_get_item(openib_btl->endpoints,ep_index);
        if(!endpoint) {
            BTL_VERBOSE(("In finalize, got another null endpoint\n"));
            continue;
        }
        OBJ_RELEASE(endpoint);
    }
    /* Release SRQ resources */
    for(qp = 0; qp < mca_btl_openib_component.num_qps; qp++) { 
        if(MCA_BTL_OPENIB_SRQ_QP == mca_btl_openib_component.qp_infos[qp].type){ 
            
            MCA_BTL_OPENIB_CLEAN_PENDING_FRAGS(openib_btl,
                        &openib_btl->qps[qp].u.srq_qp.pending_frags);
            
            if (ibv_destroy_srq(openib_btl->qps[qp].u.srq_qp.srq)){
                BTL_VERBOSE(("Failed to close SRQ %d", qp));
                return OMPI_ERROR;
            }
            
            /* Destroy free lists */
            OBJ_DESTRUCT(&openib_btl->qps[qp].u.srq_qp.pending_frags);
            OBJ_DESTRUCT(&openib_btl->qps[qp].send_free);
            OBJ_DESTRUCT(&openib_btl->qps[qp].recv_free);
        } else { 
            /* Destroy free lists */
            OBJ_DESTRUCT(&openib_btl->qps[qp].send_free);
            OBJ_DESTRUCT(&openib_btl->qps[qp].recv_free);
        }
    }

    OBJ_DESTRUCT(&openib_btl->send_free_control);
    OBJ_DESTRUCT(&openib_btl->send_user_free);
    OBJ_DESTRUCT(&openib_btl->recv_user_free);

    /* Release CQs */
    if (ibv_destroy_cq(openib_btl->ib_cq[BTL_OPENIB_HP_CQ])) {
        BTL_VERBOSE(("Failed to close HP CQ"));
        return OMPI_ERROR;
    }
    if (ibv_destroy_cq(openib_btl->ib_cq[BTL_OPENIB_LP_CQ])) {
        BTL_VERBOSE(("Failed to close LP CQ"));
        return OMPI_ERROR;
    }
    
    /* Release pending lists */
    if (!(--openib_btl->hca->btls)) {
        /* All btls for the HCA were closed
         * Now we can close the HCA
         */     
        if (OMPI_SUCCESS != mca_btl_finalize_hca(openib_btl->hca)) {
            BTL_VERBOSE(("Failed to close HCA"));
            return OMPI_ERROR;
        }
    }
#if OMPI_HAVE_THREADS
    if (mca_btl_openib_component.use_async_event_thread &&
            ! mca_btl_openib_component.ib_num_btls) {
        /* signaling to async_tread to stop */
        int async_command=0;
        if (write(mca_btl_openib_component.async_pipe[1],
                    &async_command,sizeof(int))<0){
            BTL_ERROR(("Failed to write to pipe"));
            return OMPI_ERROR;
        }
        if (pthread_join(mca_btl_openib_component.async_thread, NULL)) {
            BTL_ERROR(("Failed to stop OpenIB async event thread"));
            return OMPI_ERROR;
        }
    }
#endif
    OBJ_DESTRUCT(&openib_btl->ib_lock); 
    free(openib_btl);

    BTL_VERBOSE(("Success in closing BTL resources"));

    return OMPI_SUCCESS;
}

/*
 *  Initiate a send. 
 */

int mca_btl_openib_send( 
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_btl_base_descriptor_t* descriptor, 
    mca_btl_base_tag_t tag)
   
{
    
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*)descriptor; 
    assert(frag->type == MCA_BTL_OPENIB_FRAG_SEND);
  
    frag->endpoint = endpoint; 
    frag->hdr->tag = tag;
    frag->wr_desc.sr_desc.opcode = IBV_WR_SEND;
    return mca_btl_openib_endpoint_send(endpoint, frag);
}

/*
 * RDMA WRITE local buffer to remote buffer address.
 */

int mca_btl_openib_put( mca_btl_base_module_t* btl,
                    mca_btl_base_endpoint_t* endpoint,
                    mca_btl_base_descriptor_t* descriptor)
{
    int rc = OMPI_SUCCESS;
    struct ibv_send_wr* bad_wr; 
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*) descriptor; 
/*    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl; */
    int qp = frag->base.order;

    if(MCA_BTL_NO_ORDER == qp)
        qp = mca_btl_openib_component.rdma_qp;

    /* setup for queued requests */
    frag->endpoint = endpoint;
    frag->wr_desc.sr_desc.opcode = IBV_WR_RDMA_WRITE; 
    
    /* check for a send wqe */
    if (OPAL_THREAD_ADD32(&endpoint->qps[qp].sd_wqe,-1) < 0) {
        OPAL_THREAD_ADD32(&endpoint->qps[qp].sd_wqe,1);
        OPAL_THREAD_LOCK(&endpoint->endpoint_lock);
        opal_list_append(&endpoint->pending_put_frags, (opal_list_item_t *)frag);
        OPAL_THREAD_UNLOCK(&endpoint->endpoint_lock);
        return rc;

    /* post descriptor */
    } else {
        int ib_rc;
        
        frag->wr_desc.sr_desc.send_flags = IBV_SEND_SIGNALED; 
        frag->wr_desc.sr_desc.wr.rdma.remote_addr = frag->base.des_dst->seg_addr.lval; 
        frag->wr_desc.sr_desc.wr.rdma.rkey = frag->base.des_dst->seg_key.key32[0]; 
        frag->sg_entry.addr = (unsigned long) frag->base.des_src->seg_addr.pval; 
        frag->sg_entry.length  = frag->base.des_src->seg_len; 
       
        frag->base.order = qp;
        ib_rc = ibv_post_send(endpoint->qps[qp].lcl_qp, &frag->wr_desc.sr_desc, &bad_wr); 
        if(ib_rc)
            rc = OMPI_ERROR;
    
        /* mca_btl_openib_post_srr_all(openib_btl, 1); */
/*         mca_btl_openib_endpoint_post_rr_all(endpoint, 1); */
        
    }
    return rc; 
}


/*
 * RDMA READ remote buffer to local buffer address.
 */

int mca_btl_openib_get( mca_btl_base_module_t* btl,
                    mca_btl_base_endpoint_t* endpoint,
                    mca_btl_base_descriptor_t* descriptor)
{
    int rc;
    struct ibv_send_wr* bad_wr; 
    mca_btl_openib_frag_t* frag = (mca_btl_openib_frag_t*) descriptor; 
/*    mca_btl_openib_module_t* openib_btl = (mca_btl_openib_module_t*) btl; */
    int qp = frag->base.order;
    frag->endpoint = endpoint;
    frag->wr_desc.sr_desc.opcode = IBV_WR_RDMA_READ; 

    if(MCA_BTL_NO_ORDER == qp)
        qp = mca_btl_openib_component.rdma_qp;

    /* check for a send wqe */
    if (OPAL_THREAD_ADD32(&endpoint->qps[qp].sd_wqe,-1) < 0) {

        OPAL_THREAD_ADD32(&endpoint->qps[qp].sd_wqe,1);
        OPAL_THREAD_LOCK(&endpoint->endpoint_lock);
        opal_list_append(&endpoint->pending_get_frags, (opal_list_item_t*)frag);
        OPAL_THREAD_UNLOCK(&endpoint->endpoint_lock);
        return OMPI_SUCCESS;

    /* check for a get token */
    } else if(OPAL_THREAD_ADD32(&endpoint->get_tokens,-1) < 0) {

        OPAL_THREAD_ADD32(&endpoint->qps[qp].sd_wqe,1);
        OPAL_THREAD_ADD32(&endpoint->get_tokens,1);
        OPAL_THREAD_LOCK(&endpoint->endpoint_lock);
        opal_list_append(&endpoint->pending_get_frags, (opal_list_item_t*)frag);
        OPAL_THREAD_UNLOCK(&endpoint->endpoint_lock);
        return OMPI_SUCCESS;

    } else { 
    
        frag->wr_desc.sr_desc.send_flags = IBV_SEND_SIGNALED; 
        frag->wr_desc.sr_desc.wr.rdma.remote_addr = frag->base.des_src->seg_addr.lval; 
        frag->wr_desc.sr_desc.wr.rdma.rkey = frag->base.des_src->seg_key.key32[0]; 
        frag->sg_entry.addr = (unsigned long) frag->base.des_dst->seg_addr.pval; 
        frag->sg_entry.length  = frag->base.des_dst->seg_len; 
        
        frag->base.order = qp;
        if(ibv_post_send(endpoint->qps[qp].lcl_qp, &frag->wr_desc.sr_desc, &bad_wr)){ 
            BTL_ERROR(("error posting send request errno (%d) says %s", 
                       errno, strerror(errno))); 
            rc = ORTE_ERROR;
        }  else {
            rc = ORTE_SUCCESS;
        }
#if 0 
        mca_btl_openib_post_srr_all(openib_btl, 1);
        mca_btl_openib_endpoint_post_rr_all(endpoint, 1);
#endif 
    }
    
    
    return rc; 
}

static inline struct ibv_cq *ibv_create_cq_compat(struct ibv_context *context,
        int cqe, void *cq_context, struct ibv_comp_channel *channel,
        int comp_vector)
{
#if OMPI_IBV_CREATE_CQ_ARGS == 3
    return ibv_create_cq(context, cqe, channel);
#else
    return ibv_create_cq(context, cqe, cq_context, channel, comp_vector); 
#endif
}

/* 
 * create both the high and low priority completion queues 
 * and the shared receive queue (if requested)
 */ 
int mca_btl_openib_create_cq_srq(mca_btl_openib_module_t *openib_btl)
{
    int qp;
    openib_btl->poll_cq = false; 
    /* create the SRQ's */
    for(qp = 0; qp < mca_btl_openib_component.num_qps; qp++) { 
        struct ibv_srq_init_attr attr; 
        if(MCA_BTL_OPENIB_SRQ_QP == openib_btl->qps[qp].type) { 
            attr.attr.max_wr = mca_btl_openib_component.qp_infos[qp].rd_num + 
                mca_btl_openib_component.qp_infos[qp].u.srq_qp.sd_max;
            attr.attr.max_sge = mca_btl_openib_component.ib_sg_list_size;
            openib_btl->qps[qp].u.srq_qp.rd_posted = 0; 
            openib_btl->qps[qp].u.srq_qp.srq =
                ibv_create_srq(openib_btl->hca->ib_pd, &attr); 
            if (NULL == openib_btl->qps[qp].u.srq_qp.srq) { 
                abort();
                show_init_error(__FILE__, __LINE__, "ibv_create_srq",
                                ibv_get_device_name(openib_btl->hca->ib_dev));
                return OMPI_ERROR; 
            }
        }
    }
    
    
    /* Create the CQs, one HP, one LP */
    openib_btl->ib_cq[BTL_OPENIB_LP_CQ] =
        ibv_create_cq_compat(openib_btl->hca->ib_dev_context,
                mca_btl_openib_component.ib_lp_cq_size,
#if OMPI_ENABLE_PROGRESS_THREADS == 1
                openib_btl, openib_btl->hca->ib_channel,
#else
                NULL, NULL,
#endif
                0);
    
    if (NULL == openib_btl->ib_cq[BTL_OPENIB_LP_CQ]) {
        show_init_error(__FILE__, __LINE__, "ibv_create_cq",
                        ibv_get_device_name(openib_btl->hca->ib_dev));
        return OMPI_ERROR;
    }

    openib_btl->ib_cq[BTL_OPENIB_HP_CQ] =
        ibv_create_cq_compat(openib_btl->hca->ib_dev_context,
                mca_btl_openib_component.ib_hp_cq_size,
#if OMPI_ENABLE_PROGRESS_THREADS == 1
                openib_btl, openib_btl->hca->ib_channel,
#else
                NULL, NULL,
#endif
                0);

    if(NULL == openib_btl->ib_cq[BTL_OPENIB_HP_CQ]) {
        show_init_error(__FILE__, __LINE__, "ibv_create_cq",
                        ibv_get_device_name(openib_btl->hca->ib_dev));
        return OMPI_ERROR;
    }
    openib_btl->cq_users[BTL_OPENIB_HP_CQ] = 0;
    openib_btl->cq_users[BTL_OPENIB_LP_CQ] = 0;

#if OMPI_ENABLE_PROGRESS_THREADS == 1
    if(ibv_req_notify_cq(openib_btl->ib_cq[BTL_OPENIB_LP_CQ], 0)) {
        show_init_error(__FILE__, __LINE__, "ibv_req_notify_cq",
                        ibv_get_device_name(openib_btl->hca->ib_dev));
        return OMPI_ERROR;
    }
    if(ibv_req_notify_cq(openib_btl->ib_cq[BTL_OPENIB_HP_CQ], 0)) {
        show_init_error(__FILE__, __LINE__, "ibv_req_notify_cq",
                        ibv_get_device_name(openib_btl->hca->ib_dev));
        return OMPI_ERROR;
    }
    OPAL_THREAD_LOCK(&openib_btl->hca->hca_lock);
    if (!openib_btl->hca->progress){
        int rc;
        openib_btl->hca->progress = true;
        if(OPAL_SUCCESS != (rc = opal_thread_start(&openib_btl->hca->thread))) {
            BTL_ERROR(("Unable to create progress thread, retval=%d", rc));
            return rc;
        }
    }
    OPAL_THREAD_UNLOCK(&openib_btl->hca->hca_lock);
#endif
        
    return OMPI_SUCCESS;
}

int mca_btl_openib_ft_event(int state) {
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
