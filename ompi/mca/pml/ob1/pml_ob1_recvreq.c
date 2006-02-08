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

#include "mca/pml/pml.h"
#include "mca/bml/bml.h" 
#include "mca/btl/btl.h"
#include "mca/mpool/mpool.h" 
#include "pml_ob1_comm.h"
#include "pml_ob1_recvreq.h"
#include "pml_ob1_recvfrag.h"
#include "pml_ob1_sendreq.h"
#include "pml_ob1_rdmafrag.h"
#include "mca/bml/base/base.h" 
#include "mca/errmgr/errmgr.h"
                                                                                                               
static mca_pml_ob1_recv_frag_t* mca_pml_ob1_recv_request_match_specific_proc(
    mca_pml_ob1_recv_request_t* request, mca_pml_ob1_comm_proc_t* proc);


static int mca_pml_ob1_recv_request_fini(struct ompi_request_t** request)
{
    mca_pml_ob1_recv_request_t* recvreq = *(mca_pml_ob1_recv_request_t**)request; 
    if(recvreq->req_recv.req_base.req_persistent) {
       if(recvreq->req_recv.req_base.req_free_called) { 
           MCA_PML_OB1_RECV_REQUEST_FREE(recvreq);
       } else {
           recvreq->req_recv.req_base.req_ompi.req_state = OMPI_REQUEST_INACTIVE; 
       }
    } else {
        MCA_PML_OB1_RECV_REQUEST_FREE(recvreq);
    }
    return OMPI_SUCCESS;
}

static int mca_pml_ob1_recv_request_free(struct ompi_request_t** request)
{
    MCA_PML_OB1_RECV_REQUEST_FREE( *(mca_pml_ob1_recv_request_t**)request );
    return OMPI_SUCCESS;
} 

static int mca_pml_ob1_recv_request_cancel(struct ompi_request_t* ompi_request, int complete)
{
    mca_pml_ob1_recv_request_t* request = (mca_pml_ob1_recv_request_t*)ompi_request;
    mca_pml_ob1_comm_t* comm = request->req_recv.req_base.req_comm->c_pml_comm;

    if( true == ompi_request->req_complete ) { /* way to late to cancel this one */
       return OMPI_SUCCESS;
    }
    
    /* The rest should be protected behind the match logic lock */
    OPAL_THREAD_LOCK(&comm->matching_lock);
    if( OMPI_ANY_TAG == ompi_request->req_status.MPI_TAG ) { /* the match has not been already done */
       if( request->req_recv.req_base.req_peer == OMPI_ANY_SOURCE ) {
          opal_list_remove_item( &comm->wild_receives, (opal_list_item_t*)request );
       } else {
          mca_pml_ob1_comm_proc_t* proc = comm->procs + request->req_recv.req_base.req_peer;
          opal_list_remove_item(&proc->specific_receives, (opal_list_item_t*)request);
       }
    }
    OPAL_THREAD_UNLOCK(&comm->matching_lock);
    
    OPAL_THREAD_LOCK(&ompi_request_lock);
    ompi_request->req_status._cancelled = true;
    /* This macro will set the req_complete to true so the MPI Test/Wait* functions
     * on this request will be able to complete. As the status is marked as
     * cancelled the cancel state will be detected.
     */
    MCA_PML_BASE_REQUEST_MPI_COMPLETE(ompi_request);
    OPAL_THREAD_UNLOCK(&ompi_request_lock);
    return OMPI_SUCCESS;
}

static void mca_pml_ob1_recv_request_construct(mca_pml_ob1_recv_request_t* request)
{
    request->req_recv.req_base.req_type = MCA_PML_REQUEST_RECV;
    request->req_recv.req_base.req_ompi.req_fini = mca_pml_ob1_recv_request_fini;
    request->req_recv.req_base.req_ompi.req_free = mca_pml_ob1_recv_request_free;
    request->req_recv.req_base.req_ompi.req_cancel = mca_pml_ob1_recv_request_cancel;
}

static void mca_pml_ob1_recv_request_destruct(mca_pml_ob1_recv_request_t* request)
{
}


OBJ_CLASS_INSTANCE(
    mca_pml_ob1_recv_request_t,
    mca_pml_base_recv_request_t,
    mca_pml_ob1_recv_request_construct,
    mca_pml_ob1_recv_request_destruct);


/*
 * Release resources.
 */

static void mca_pml_ob1_ctl_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* des,
    int status)
{
    mca_bml_base_btl_t* bml_btl = (mca_bml_base_btl_t*)des->des_context;
    MCA_BML_BASE_BTL_DES_RETURN(bml_btl, des);
}

/*
 * Put operation has completed remotely - update request status
 */

static void mca_pml_ob1_put_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* des,
    int status)
{
    mca_bml_base_btl_t* bml_btl = (mca_bml_base_btl_t*)des->des_context;
    mca_pml_ob1_recv_request_t* recvreq = (mca_pml_ob1_recv_request_t*)des->des_cbdata;
    size_t bytes_received = 0;

    MCA_PML_OB1_COMPUTE_SEGMENT_LENGTH( des->des_dst, des->des_dst_cnt,
                                        0, bytes_received );
    OPAL_THREAD_ADD_SIZE_T(&recvreq->req_pipeline_depth,-1);
    mca_bml_base_free(bml_btl, des);

    /* check completion status */
    if( OPAL_THREAD_ADD_SIZE_T(&recvreq->req_bytes_received, bytes_received)
        >= recvreq->req_recv.req_bytes_packed ) {
        /* initialize request status */
        recvreq->req_recv.req_base.req_pml_complete = true;
        recvreq->req_recv.req_base.req_ompi.req_status._count = 
            (recvreq->req_bytes_received < recvreq->req_bytes_delivered ?
             recvreq->req_bytes_received : recvreq->req_bytes_delivered);
        OPAL_THREAD_LOCK(&ompi_request_lock);
        MCA_PML_BASE_REQUEST_MPI_COMPLETE( &(recvreq->req_recv.req_base.req_ompi) );
        OPAL_THREAD_UNLOCK(&ompi_request_lock);
    } else if (recvreq->req_rdma_offset < recvreq->req_recv.req_bytes_packed) {
        /* schedule additional rdma operations */
        mca_pml_ob1_recv_request_schedule(recvreq);
    }
}

/*
 *
 */

static void mca_pml_ob1_recv_request_ack(
    mca_pml_ob1_recv_request_t* recvreq,
    mca_pml_ob1_rendezvous_hdr_t* hdr, 
    size_t bytes_received)
{
    ompi_proc_t* proc = (ompi_proc_t*)recvreq->req_recv.req_base.req_proc;
    mca_bml_base_endpoint_t* bml_endpoint = NULL; 
    mca_btl_base_descriptor_t* des;
    mca_bml_base_btl_t* bml_btl;
    mca_pml_ob1_recv_frag_t* frag;
    mca_pml_ob1_ack_hdr_t* ack;
    int rc;

    /* if this hasn't been initialized yet - this is a synchronous send */
    if(NULL == proc) {
        ompi_proc_t *ompi_proc = ompi_comm_peer_lookup(
                recvreq->req_recv.req_base.req_comm, hdr->hdr_match.hdr_src);
        proc = recvreq->req_recv.req_base.req_proc = ompi_proc;
    }
    bml_endpoint = (mca_bml_base_endpoint_t*) proc->proc_pml; 
    bml_btl = mca_bml_base_btl_array_get_next(&bml_endpoint->btl_eager);
    
    if(hdr->hdr_msg_length > bytes_received) {
        
        /* by default copy */
        recvreq->req_rdma_offset = hdr->hdr_msg_length;

        /*
         * lookup request buffer to determine if memory is already
         * registered. 
         */

        if(ompi_convertor_need_buffers(&recvreq->req_recv.req_convertor) == 0 &&
           hdr->hdr_match.hdr_common.hdr_flags & MCA_PML_OB1_HDR_FLAGS_CONTIG) {
            recvreq->req_rdma_cnt = mca_pml_ob1_rdma_btls(
                bml_endpoint,
                recvreq->req_recv.req_base.req_addr,
                recvreq->req_recv.req_bytes_packed,
                recvreq->req_rdma);

            /* memory is already registered on both sides */
            if (hdr->hdr_match.hdr_common.hdr_flags & MCA_PML_OB1_HDR_FLAGS_PIN &&
                recvreq->req_rdma_cnt != 0) {

                /* start rdma at current fragment offset - no need to ack */
                recvreq->req_rdma_offset = recvreq->req_bytes_received;
                return;

            /* are rdma devices available for long rdma protocol */
            } else if (bml_endpoint->btl_rdma_offset < hdr->hdr_msg_length &&
                       mca_bml_base_btl_array_get_size(&bml_endpoint->btl_rdma)) {

                /* use convertor to figure out the rdma offset for this request */
                recvreq->req_rdma_offset = bml_endpoint->btl_rdma_offset;
                if(recvreq->req_rdma_offset < recvreq->req_bytes_received) {
                    recvreq->req_rdma_offset = recvreq->req_bytes_received;
                }
                ompi_convertor_set_position(
                    &recvreq->req_recv.req_convertor,
                    &recvreq->req_rdma_offset);
           }
        }
    }

    /* allocate descriptor */
    MCA_PML_OB1_DES_ALLOC(bml_btl, des, sizeof(mca_pml_ob1_ack_hdr_t));
    if(NULL == des) {
        goto retry;
    }

    /* fill out header */
    ack = (mca_pml_ob1_ack_hdr_t*)des->des_src->seg_addr.pval;
    ack->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_ACK;
    ack->hdr_common.hdr_flags = 0;
    ack->hdr_src_req = hdr->hdr_src_req;
    ack->hdr_dst_req.pval = recvreq;
    ack->hdr_rdma_offset = recvreq->req_rdma_offset;

    /* initialize descriptor */
    des->des_flags |= MCA_BTL_DES_FLAGS_PRIORITY;
    des->des_cbfunc = mca_pml_ob1_ctl_completion;

    rc = mca_bml_base_send(bml_btl, des, MCA_BTL_TAG_PML);
    if(rc != OMPI_SUCCESS) {
        mca_bml_base_free(bml_btl, des);
        goto retry;
    }
    return;

    /* queue request to retry later */
retry:
    MCA_PML_OB1_RECV_FRAG_ALLOC(frag,rc);
    frag->hdr.hdr_rndv = *hdr;
    frag->num_segments = 0;
    frag->request = recvreq;
    opal_list_append(&mca_pml_ob1.acks_pending, (opal_list_item_t*)frag);
}

                                                                                                            
/**
 * Return resources used by the RDMA
 */
                                                                                                            
static void mca_pml_ob1_fin_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* des,
    int status)
{
                                                                                                            
    mca_pml_ob1_rdma_frag_t* frag = (mca_pml_ob1_rdma_frag_t*)des->des_cbdata;
    mca_bml_base_btl_t* bml_btl = (mca_bml_base_btl_t*) des->des_context;
    MCA_PML_OB1_RDMA_FRAG_RETURN(frag);
    MCA_BML_BASE_BTL_DES_RETURN(bml_btl, des);
}


/*
 *
 */

static void mca_pml_ob1_rget_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* des,
    int status)
{
    mca_bml_base_btl_t* bml_btl = (mca_bml_base_btl_t*)des->des_context;
    mca_pml_ob1_rdma_frag_t* frag = (mca_pml_ob1_rdma_frag_t*)des->des_cbdata;
    mca_pml_ob1_recv_request_t* recvreq = frag->rdma_req;
    mca_pml_ob1_fin_hdr_t* hdr;
    mca_btl_base_descriptor_t *fin;
    int rc;
   
    /* is receive request complete */
    if( OPAL_THREAD_ADD_SIZE_T(&recvreq->req_bytes_received, frag->rdma_length)
        == recvreq->req_recv.req_bytes_packed ) {
        recvreq->req_recv.req_base.req_ompi.req_status._count = 
            (recvreq->req_bytes_received < recvreq->req_bytes_delivered ?
             recvreq->req_bytes_received : recvreq->req_bytes_delivered);
        recvreq->req_recv.req_base.req_pml_complete = true;
        OPAL_THREAD_LOCK(&ompi_request_lock);
        MCA_PML_BASE_REQUEST_MPI_COMPLETE( &(recvreq->req_recv.req_base.req_ompi) );
        OPAL_THREAD_UNLOCK(&ompi_request_lock);
    }

    /* return descriptor */
    mca_bml_base_free(bml_btl, des); 

    /* queue up a fin control message to source */
    MCA_PML_OB1_DES_ALLOC(bml_btl, fin, sizeof(mca_pml_ob1_fin_hdr_t));
    if(NULL == fin) {
        opal_output(0, "[%s:%d] unable to allocate descriptor", __FILE__,__LINE__);
        orte_errmgr.abort();
    }
    fin->des_flags |= MCA_BTL_DES_FLAGS_PRIORITY;
    fin->des_cbfunc = mca_pml_ob1_fin_completion;
    fin->des_cbdata = frag;
                                                                                                            
    /* fill in header */
    hdr = (mca_pml_ob1_fin_hdr_t*)fin->des_src->seg_addr.pval;
    hdr->hdr_common.hdr_flags = 0;
    hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_FIN;
    hdr->hdr_des = frag->rdma_hdr.hdr_rget.hdr_des;
                                                                                                            
    /* queue request */
    rc = mca_bml_base_send(
        bml_btl,
        fin,
        MCA_BTL_TAG_PML
        );
    if(OMPI_SUCCESS != rc) {
        opal_output(0, "[%s:%d] unable to queue fin", __FILE__,__LINE__);
        orte_errmgr.abort();
    }
}


/*
 *
 */

static void mca_pml_ob1_recv_request_rget(
    mca_pml_ob1_recv_request_t* recvreq,
    mca_btl_base_module_t* btl,
    mca_pml_ob1_rget_hdr_t* hdr)
{
    mca_bml_base_endpoint_t* bml_endpoint = NULL;
    mca_bml_base_btl_t* bml_btl;
    mca_pml_ob1_rdma_frag_t* frag;
    mca_btl_base_descriptor_t* descriptor;
    mca_mpool_base_registration_t* reg;
    size_t i, size = 0;
    int rc;

    /* lookup bml datastructures */
    bml_endpoint = (mca_bml_base_endpoint_t*)recvreq->req_recv.req_base.req_proc->proc_pml; 
    bml_btl = mca_bml_base_btl_array_find(&bml_endpoint->btl_eager, btl);
    if(NULL == bml_btl) {
        opal_output(0, "[%s:%d] invalid bml for rdma get", __FILE__, __LINE__);
        orte_errmgr.abort();
    }

    /* allocate/initialize a fragment */
    MCA_PML_OB1_RDMA_FRAG_ALLOC(frag,rc);
    for(i=0; i<hdr->hdr_seg_cnt; i++) {
        size += hdr->hdr_segs[i].seg_len;
        frag->rdma_segs[i] = hdr->hdr_segs[i];
    }
    frag->rdma_hdr.hdr_rget = *hdr;
    frag->rdma_req = recvreq;
    frag->rdma_ep = bml_endpoint;
    frag->rdma_state = MCA_PML_OB1_RDMA_PREPARE;

    /* is there an existing registration for this btl */
    reg = mca_pml_ob1_rdma_registration(
        bml_btl, 
        recvreq->req_recv.req_base.req_addr,
        recvreq->req_recv.req_bytes_packed);
    if(NULL != reg) {
         recvreq->req_rdma[0].bml_btl = bml_btl; 
         recvreq->req_rdma[0].btl_reg = reg;
         recvreq->req_rdma_cnt = 1;
    }
    
    /* prepare descriptor */
    mca_bml_base_prepare_dst(
        bml_btl, 
        reg,
        &recvreq->req_recv.req_convertor,
        0,
        &size, 
        &descriptor);
    if(NULL == descriptor) {
        opal_output(0, "[%s:%d] unable to allocate descriptor for rdma get", __FILE__, __LINE__);
        orte_errmgr.abort();
    }

    frag->rdma_length = size;
    descriptor->des_src = frag->rdma_segs;
    descriptor->des_src_cnt = hdr->hdr_seg_cnt;
    descriptor->des_cbdata = frag;
    descriptor->des_cbfunc = mca_pml_ob1_rget_completion;

    /* queue up get request */
    rc = mca_bml_base_get(bml_btl,descriptor);
    if(rc != OMPI_SUCCESS) {
        opal_output(0, "[%s:%d] rdma get failed with error %d", __FILE__, __LINE__, rc);
        orte_errmgr.abort();
    }
}


/*
 * Update the recv request status to reflect the number of bytes
 * received and actually delivered to the application. 
 */

void mca_pml_ob1_recv_request_progress(
    mca_pml_ob1_recv_request_t* recvreq,
    mca_btl_base_module_t* btl,
    mca_btl_base_segment_t* segments,
    size_t num_segments)
{
    size_t bytes_received = 0;
    size_t bytes_delivered = 0;
    size_t data_offset = 0;
    mca_pml_ob1_hdr_t* hdr = (mca_pml_ob1_hdr_t*)segments->seg_addr.pval;

    MCA_PML_OB1_COMPUTE_SEGMENT_LENGTH( segments, num_segments,
                                        0, bytes_received );
    switch(hdr->hdr_common.hdr_type) {
        case MCA_PML_OB1_HDR_TYPE_MATCH:

            bytes_received -= sizeof(mca_pml_ob1_match_hdr_t);
            recvreq->req_recv.req_bytes_packed = bytes_received;
            recvreq->req_bytes_delivered = bytes_received;
            MCA_PML_OB1_RECV_REQUEST_MATCHED(recvreq,&hdr->hdr_match);
            MCA_PML_OB1_RECV_REQUEST_UNPACK(
                recvreq,
                segments,
                num_segments,
                sizeof(mca_pml_ob1_match_hdr_t),
                data_offset,
                bytes_received,
                bytes_delivered);
            break;

        case MCA_PML_OB1_HDR_TYPE_RNDV:

            bytes_received -= sizeof(mca_pml_ob1_rendezvous_hdr_t);
            recvreq->req_recv.req_bytes_packed = hdr->hdr_rndv.hdr_msg_length;
            recvreq->req_bytes_delivered = hdr->hdr_rndv.hdr_msg_length;
            recvreq->req_send = hdr->hdr_rndv.hdr_src_req;
            MCA_PML_OB1_RECV_REQUEST_MATCHED(recvreq,&hdr->hdr_match);
            mca_pml_ob1_recv_request_ack(recvreq, &hdr->hdr_rndv, bytes_received);
            MCA_PML_OB1_RECV_REQUEST_UNPACK(
                recvreq,
                segments,
                num_segments,
                sizeof(mca_pml_ob1_rendezvous_hdr_t),
                data_offset,
                bytes_received,
                bytes_delivered);
            break;

        case MCA_PML_OB1_HDR_TYPE_RGET:

            recvreq->req_recv.req_bytes_packed = hdr->hdr_rndv.hdr_msg_length;
            recvreq->req_bytes_delivered = hdr->hdr_rndv.hdr_msg_length;
            MCA_PML_OB1_RECV_REQUEST_MATCHED(recvreq,&hdr->hdr_match);
            mca_pml_ob1_recv_request_rget(recvreq, btl, &hdr->hdr_rget);
            return;

        case MCA_PML_OB1_HDR_TYPE_FRAG:

            bytes_received -= sizeof(mca_pml_ob1_frag_hdr_t);
            data_offset = hdr->hdr_frag.hdr_frag_offset;
            MCA_PML_OB1_RECV_REQUEST_UNPACK(
                recvreq,
                segments,
                num_segments,
                sizeof(mca_pml_ob1_frag_hdr_t),
                data_offset,
                bytes_received,
                bytes_delivered);
            break;

        default:
            break;
    }

    /* check completion status */
    if( OPAL_THREAD_ADD_SIZE_T(&recvreq->req_bytes_received, bytes_received)
        >= recvreq->req_recv.req_bytes_packed ) {
        /* initialize request status */
        recvreq->req_recv.req_base.req_ompi.req_status._count =
            (recvreq->req_bytes_received < recvreq->req_bytes_delivered ?
             recvreq->req_bytes_received : recvreq->req_bytes_delivered);
        recvreq->req_recv.req_base.req_pml_complete = true;
        OPAL_THREAD_LOCK(&ompi_request_lock);
        MCA_PML_BASE_REQUEST_MPI_COMPLETE( &(recvreq->req_recv.req_base.req_ompi) );
        OPAL_THREAD_UNLOCK(&ompi_request_lock);
    } else if (recvreq->req_rdma_offset < recvreq->req_recv.req_bytes_packed) {
        /* schedule additional rdma operations */
        mca_pml_ob1_recv_request_schedule(recvreq);
    }
}


/**
 * Handle completion of a probe request
 */

void mca_pml_ob1_recv_request_matched_probe(
    mca_pml_ob1_recv_request_t* recvreq,
    mca_btl_base_module_t* btl,
    mca_btl_base_segment_t* segments,
    size_t num_segments)
{
    size_t bytes_packed = 0;
    mca_pml_ob1_hdr_t* hdr = (mca_pml_ob1_hdr_t*)segments->seg_addr.pval;

    switch(hdr->hdr_common.hdr_type) {
        case MCA_PML_OB1_HDR_TYPE_MATCH:

            MCA_PML_OB1_COMPUTE_SEGMENT_LENGTH( segments, num_segments,
                                                sizeof(mca_pml_ob1_match_hdr_t),
                                                bytes_packed );
            break;

        case MCA_PML_OB1_HDR_TYPE_RNDV:
        case MCA_PML_OB1_HDR_TYPE_RGET:

            bytes_packed = hdr->hdr_rndv.hdr_msg_length;
            break;
    }

    /* set completion status */
    recvreq->req_recv.req_base.req_ompi.req_status.MPI_TAG = hdr->hdr_match.hdr_tag;
    recvreq->req_recv.req_base.req_ompi.req_status.MPI_SOURCE = hdr->hdr_match.hdr_src;
    recvreq->req_recv.req_base.req_ompi.req_status._count = bytes_packed;
    OPAL_THREAD_LOCK(&ompi_request_lock);
    recvreq->req_recv.req_base.req_pml_complete = true; 
    MCA_PML_BASE_REQUEST_MPI_COMPLETE( &(recvreq->req_recv.req_base.req_ompi) );
    OPAL_THREAD_UNLOCK(&ompi_request_lock);
}


/*
 * Schedule RDMA protocol.
 *
*/

void mca_pml_ob1_recv_request_schedule(mca_pml_ob1_recv_request_t* recvreq)
{
    if(OPAL_THREAD_ADD32(&recvreq->req_lock,1) == 1) {
        ompi_proc_t* proc = (ompi_proc_t*)recvreq->req_recv.req_base.req_proc;
        mca_bml_base_endpoint_t* bml_endpoint = (mca_bml_base_endpoint_t*) proc->proc_pml; 
        mca_bml_base_btl_t* bml_btl; 
        do {
            size_t bytes_remaining = recvreq->req_recv.req_bytes_packed - recvreq->req_rdma_offset;
            while(bytes_remaining > 0 && recvreq->req_pipeline_depth < mca_pml_ob1.recv_pipeline_depth) {
                size_t hdr_size;
                size_t size;
                mca_pml_ob1_rdma_hdr_t* hdr;
                mca_btl_base_descriptor_t* dst;
                mca_btl_base_descriptor_t* ctl;
                mca_mpool_base_registration_t * reg = NULL;
                size_t num_btl_avail;
                int rc;

                if(recvreq->req_rdma_cnt) {
 
                    /*
                     * Select the next btl out of the list w/ preregistered
                     * memory.
                    */
                    bml_btl = recvreq->req_rdma[recvreq->req_rdma_idx].bml_btl;
                    num_btl_avail = recvreq->req_rdma_cnt - recvreq->req_rdma_idx;
                    reg = recvreq->req_rdma[recvreq->req_rdma_idx].btl_reg;
                    if(++recvreq->req_rdma_idx >= recvreq->req_rdma_cnt)
                        recvreq->req_rdma_idx = 0;

                    /*
                     *  If more than one NIC is available - try to use both for anything
                     *  larger than the eager limit
                     */
                    if(num_btl_avail == 1 || bytes_remaining < bml_btl->btl_eager_limit) {
                        size = bytes_remaining;

                    /* otherwise attempt to give the BTL a percentage of the message
                     * based on a weighting factor. for simplicity calculate this as
                     * a percentage of the overall message length (regardless of amount
                     * previously assigned)
                     */
                    } else {
                        size = (size_t)(bml_btl->btl_weight * bytes_remaining);
                    }
                     
                } else {

                    /*
                     * Otherwise, schedule round-robin across the
                     * available RDMA nics dynamically registering/deregister
                     * as required.
                    */
                    num_btl_avail = mca_bml_base_btl_array_get_size(&bml_endpoint->btl_rdma);
                    bml_btl = mca_bml_base_btl_array_get_next(&bml_endpoint->btl_rdma);
                    
                    /*
                     *  If more than one NIC is available - try to use both for anything
                     *  larger than the eager limit
                     */
                    if(num_btl_avail == 1 || bytes_remaining < bml_btl->btl_eager_limit) {
                        size = bytes_remaining;

                    /* otherwise attempt to give the BTL a percentage of the message
                     * based on a weighting factor. for simplicity calculate this as
                     * a percentage of the overall message length (regardless of amount
                     * previously assigned)
                     */
                    } else {
                        size = (size_t)(bml_btl->btl_weight * bytes_remaining);
                    }
    
                    /* makes sure that we don't exceed BTL max rdma size */
                    if (bml_btl->btl_max_rdma_size != 0 && size > bml_btl->btl_max_rdma_size) {
                        size = bml_btl->btl_max_rdma_size;
                    }
                }

                /* prepare a descriptor for RDMA */
                ompi_convertor_set_position(&recvreq->req_recv.req_convertor, &recvreq->req_rdma_offset);
                mca_bml_base_prepare_dst(
                                         bml_btl, 
                                         reg,
                                         &recvreq->req_recv.req_convertor,
                                         0,
                                         &size, 
                                         &dst);
                if(dst == NULL) {
                    OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
                    opal_list_append(&mca_pml_ob1.recv_pending, (opal_list_item_t*)recvreq);
                    OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
                    break;
                }
                dst->des_cbfunc = mca_pml_ob1_put_completion;
                dst->des_cbdata = recvreq;

                /* prepare a descriptor for rdma control message */
                hdr_size = sizeof(mca_pml_ob1_rdma_hdr_t);
                if(dst->des_dst_cnt > 1) {
                    hdr_size += (sizeof(mca_btl_base_segment_t) * (dst->des_dst_cnt-1));
                }

                MCA_PML_OB1_DES_ALLOC(bml_btl, ctl, hdr_size);
                if(ctl == NULL) {
                    mca_bml_base_free(bml_btl,dst);
                    OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
                    opal_list_append(&mca_pml_ob1.recv_pending, (opal_list_item_t*)recvreq);
                    OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
                    break;
                }
                ctl->des_flags |= MCA_BTL_DES_FLAGS_PRIORITY;
                ctl->des_cbfunc = mca_pml_ob1_ctl_completion;
                
                /* fill in rdma header */
                hdr = (mca_pml_ob1_rdma_hdr_t*)ctl->des_src->seg_addr.pval;
                hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_PUT;
                hdr->hdr_common.hdr_flags = 0;
                hdr->hdr_req = recvreq->req_send;
                hdr->hdr_des.pval = dst;
                hdr->hdr_rdma_offset = recvreq->req_rdma_offset;
                hdr->hdr_seg_cnt = dst->des_dst_cnt;
                memcpy(hdr->hdr_segs, dst->des_dst, dst->des_dst_cnt * sizeof(mca_btl_base_segment_t));

                /* update request state */
                recvreq->req_rdma_offset += size;
                OPAL_THREAD_ADD_SIZE_T(&recvreq->req_pipeline_depth,1);

                /* send rdma request to peer */
                rc = mca_bml_base_send(bml_btl, ctl, MCA_BTL_TAG_PML);
                if(rc == OMPI_SUCCESS) {
                    bytes_remaining -= size;
                } else {
                    mca_bml_base_free(bml_btl,ctl);
                    mca_bml_base_free(bml_btl,dst);
                    recvreq->req_rdma_offset -= size;
                    OPAL_THREAD_ADD_SIZE_T(&recvreq->req_pipeline_depth,-1);
                    OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
                    opal_list_append(&mca_pml_ob1.recv_pending, (opal_list_item_t*)recvreq);
                    OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
                    break;
                }

                /* run progress as the prepare (pinning) can take some time */
                /* mca_pml_ob1_progress(); */
            }
        } while(OPAL_THREAD_ADD32(&recvreq->req_lock,-1) > 0);
    }
}

/*
 * This routine is used to match a posted receive when the source process 
 * is specified.
*/

void mca_pml_ob1_recv_request_match_specific(mca_pml_ob1_recv_request_t* request)
{
    mca_pml_ob1_comm_t* comm = request->req_recv.req_base.req_comm->c_pml_comm;
    mca_pml_ob1_comm_proc_t* proc = comm->procs + request->req_recv.req_base.req_peer;
    mca_pml_ob1_recv_frag_t* frag;
   
    /* check for a specific match */
    OPAL_THREAD_LOCK(&comm->matching_lock);

    /* assign sequence number */
    request->req_recv.req_base.req_sequence = comm->recv_sequence++;

    if (opal_list_get_size(&proc->unexpected_frags) > 0 &&
        (frag = mca_pml_ob1_recv_request_match_specific_proc(request, proc)) != NULL) {
        OPAL_THREAD_UNLOCK(&comm->matching_lock);
        
        if( !((MCA_PML_REQUEST_IPROBE == request->req_recv.req_base.req_type) ||
              (MCA_PML_REQUEST_PROBE == request->req_recv.req_base.req_type)) ) {
            mca_pml_ob1_recv_request_progress(request,frag->btl,frag->segments,frag->num_segments);
            MCA_PML_OB1_RECV_FRAG_RETURN(frag);
        } else {
            mca_pml_ob1_recv_request_matched_probe(request,frag->btl,frag->segments,frag->num_segments);
        }
        return; /* match found */
    }

    /* We didn't find any matches.  Record this irecv so we can match 
     * it when the message comes in.
    */
    if(request->req_recv.req_base.req_type != MCA_PML_REQUEST_IPROBE) { 
        opal_list_append(&proc->specific_receives, (opal_list_item_t*)request);
    }
    OPAL_THREAD_UNLOCK(&comm->matching_lock);
}


/*
 * this routine is used to try and match a wild posted receive - where
 * wild is determined by the value assigned to the source process
*/

void mca_pml_ob1_recv_request_match_wild(mca_pml_ob1_recv_request_t* request)
{
    mca_pml_ob1_comm_t* comm = request->req_recv.req_base.req_comm->c_pml_comm;
    mca_pml_ob1_comm_proc_t* proc = comm->procs;
    size_t proc_count = comm->num_procs;
    size_t i;

    /*
     * Loop over all the outstanding messages to find one that matches.
     * There is an outer loop over lists of messages from each
     * process, then an inner loop over the messages from the
     * process.
    */
    OPAL_THREAD_LOCK(&comm->matching_lock);

    /* assign sequence number */
    request->req_recv.req_base.req_sequence = comm->recv_sequence++;

    for (i = 0; i < proc_count; i++) {
        mca_pml_ob1_recv_frag_t* frag;

        /* continue if no frags to match */
        if (opal_list_get_size(&proc->unexpected_frags) == 0) {
            proc++;
            continue;
        }

        /* loop over messages from the current proc */
        if ((frag = mca_pml_ob1_recv_request_match_specific_proc(request, proc)) != NULL) {
            OPAL_THREAD_UNLOCK(&comm->matching_lock);

            if( !((MCA_PML_REQUEST_IPROBE == request->req_recv.req_base.req_type) ||
                  (MCA_PML_REQUEST_PROBE == request->req_recv.req_base.req_type)) ) {
                mca_pml_ob1_recv_request_progress(request,frag->btl,frag->segments,frag->num_segments);
                MCA_PML_OB1_RECV_FRAG_RETURN(frag);
            } else {
                mca_pml_ob1_recv_request_matched_probe(request,frag->btl,frag->segments,frag->num_segments);
            }
            return; /* match found */
        }
        proc++;
    } 

    /* We didn't find any matches.  Record this irecv so we can match to
     * it when the message comes in.
    */
 
    if(request->req_recv.req_base.req_type != MCA_PML_REQUEST_IPROBE)
        opal_list_append(&comm->wild_receives, (opal_list_item_t*)request);
    OPAL_THREAD_UNLOCK(&comm->matching_lock);
}


/*
 *  this routine tries to match a posted receive.  If a match is found,
 *  it places the request in the appropriate matched receive list. This
 *  function has to be called with the communicator matching lock held.
*/

static mca_pml_ob1_recv_frag_t* mca_pml_ob1_recv_request_match_specific_proc(
    mca_pml_ob1_recv_request_t* request, 
    mca_pml_ob1_comm_proc_t* proc)
{
    opal_list_t* unexpected_frags = &proc->unexpected_frags;
    mca_pml_ob1_recv_frag_t* frag;
    mca_pml_ob1_match_hdr_t* hdr;
    int tag = request->req_recv.req_base.req_tag;

    if( OMPI_ANY_TAG == tag ) {
        for (frag =  (mca_pml_ob1_recv_frag_t*)opal_list_get_first(unexpected_frags);
             frag != (mca_pml_ob1_recv_frag_t*)opal_list_get_end(unexpected_frags);
             frag =  (mca_pml_ob1_recv_frag_t*)opal_list_get_next(frag)) {
            hdr = &(frag->hdr.hdr_match);
            
            /* check first frag - we assume that process matching has been done already */
            if( hdr->hdr_tag >= 0 ) {
                goto find_fragment;
            } 
        }
    } else {
        for (frag =  (mca_pml_ob1_recv_frag_t*)opal_list_get_first(unexpected_frags);
             frag != (mca_pml_ob1_recv_frag_t*)opal_list_get_end(unexpected_frags);
             frag =  (mca_pml_ob1_recv_frag_t*)opal_list_get_next(frag)) {
            hdr = &(frag->hdr.hdr_match);
            
            /* check first frag - we assume that process matching has been done already */
            if ( tag == hdr->hdr_tag ) {
                /* we assume that the tag is correct from MPI point of view (ie. >= 0 ) */
                goto find_fragment;
            } 
        }
    }
    return NULL;
 find_fragment:
    if( !((MCA_PML_REQUEST_IPROBE == request->req_recv.req_base.req_type) ||
          (MCA_PML_REQUEST_PROBE == request->req_recv.req_base.req_type)) ) {
        opal_list_remove_item(unexpected_frags, (opal_list_item_t*)frag);
        frag->request = request;
    } 
    return frag;
}

