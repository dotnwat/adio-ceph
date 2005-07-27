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


/*%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%%*/

#include "ompi_config.h"
#include <sched.h>
#include "include/constants.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"
#include "mca/errmgr/errmgr.h"
#include "mca/mpool/mpool.h" 
#include "pml_ob1.h"
#include "pml_ob1_hdr.h"
#include "pml_ob1_proc.h"
#include "pml_ob1_sendreq.h"
#include "pml_ob1_rdmafrag.h"
#include "pml_ob1_recvreq.h"
#include "pml_ob1_endpoint.h"


                                                                                                         
static int mca_pml_ob1_send_request_fini(struct ompi_request_t** request)
{
    MCA_PML_OB1_FINI(request);
    return OMPI_SUCCESS;
}

static int mca_pml_ob1_send_request_free(struct ompi_request_t** request)
{
    MCA_PML_OB1_FREE(request);
    return OMPI_SUCCESS;
}

static int mca_pml_ob1_send_request_cancel(struct ompi_request_t* request, int complete)
{
    /* we dont cancel send requests by now */
    return OMPI_SUCCESS;
}

static void mca_pml_ob1_send_request_construct(mca_pml_ob1_send_request_t* req)
{
    req->req_send.req_base.req_type = MCA_PML_REQUEST_SEND;
    req->req_send.req_base.req_ompi.req_fini = mca_pml_ob1_send_request_fini;
    req->req_send.req_base.req_ompi.req_free = mca_pml_ob1_send_request_free;
    req->req_send.req_base.req_ompi.req_cancel = mca_pml_ob1_send_request_cancel;
}

static void mca_pml_ob1_send_request_destruct(mca_pml_ob1_send_request_t* req)
{
}


OBJ_CLASS_INSTANCE(
    mca_pml_ob1_send_request_t,
    mca_pml_base_send_request_t,
    mca_pml_ob1_send_request_construct,
    mca_pml_ob1_send_request_destruct);

/**
 * Completion of a short message - nothing left to schedule.
 */

static void mca_pml_ob1_match_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* descriptor,
    int status)
{
    mca_pml_ob1_send_request_t* sendreq = (mca_pml_ob1_send_request_t*)descriptor->des_cbdata;
    mca_pml_ob1_endpoint_t* btl_ep = sendreq->req_endpoint;

    /* check completion status */
    if(OMPI_SUCCESS != status) {
        /* TSW - FIX */
        opal_output(0, "%s:%d FATAL", __FILE__, __LINE__);
        orte_errmgr.abort();
    }

    /* attempt to cache the descriptor */
    MCA_PML_OB1_ENDPOINT_DES_RETURN(btl_ep,descriptor);

    /* signal request completion */
    OPAL_THREAD_LOCK(&ompi_request_lock);
    sendreq->req_bytes_delivered = sendreq->req_send.req_bytes_packed;
    MCA_PML_OB1_SEND_REQUEST_COMPLETE(sendreq);
    OPAL_THREAD_UNLOCK(&ompi_request_lock);
}

/*
 *  Completion of the first fragment of a long message that 
 *  requires an acknowledgement
 */
static void mca_pml_ob1_rndv_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* descriptor,
    int status)
{
    mca_pml_ob1_send_request_t* sendreq = (mca_pml_ob1_send_request_t*)descriptor->des_cbdata;
    mca_pml_ob1_endpoint_t* btl_ep = sendreq->req_endpoint;

    /* check completion status */
    if(OMPI_SUCCESS != status) {
        /* TSW - FIX */
        opal_output(0, "%s:%d FATAL", __FILE__, __LINE__);
        orte_errmgr.abort();
    }

    /* count bytes of user data actually delivered */
    MCA_PML_OB1_SEND_REQUEST_SET_BYTES_DELIVERED(sendreq,descriptor);

#if MCA_PML_OB1_TIMESTAMPS
    if(sendreq->req_pipeline_depth == 1) {
        sendreq->t_send2 = get_profiler_timestamp();
    }
#endif

    /* return the descriptor */
    btl_ep->btl_free(btl_ep->btl, descriptor);

    /* update pipeline depth */
    OPAL_THREAD_ADD32(&sendreq->req_pipeline_depth,-1);

    /* advance the request */
    MCA_PML_OB1_SEND_REQUEST_ADVANCE(sendreq);

    /* check for pending requests */
    MCA_PML_OB1_SEND_REQUEST_PROCESS_PENDING();
}


/**
 * Completion of additional fragments of a large message - may need
 * to schedule additional fragments.
 */

static void mca_pml_ob1_frag_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* descriptor,
    int status)
{
    mca_pml_ob1_send_request_t* sendreq = (mca_pml_ob1_send_request_t*)descriptor->des_cbdata;
    mca_pml_ob1_endpoint_t* btl_ep = sendreq->req_endpoint;
    bool schedule;

    /* check completion status */
    if(OMPI_SUCCESS != status) {
        /* TSW - FIX */
        opal_output(0, "%s:%d FATAL", __FILE__, __LINE__);
        orte_errmgr.abort();
    }

    /* count bytes of user data actually delivered */
    MCA_PML_OB1_SEND_REQUEST_SET_BYTES_DELIVERED(sendreq,descriptor);

#if MCA_PML_OB1_TIMESTAMPS
    if(sendreq->req_pipeline_depth == 1) {
        sendreq->t_send2 = get_profiler_timestamp();
    }
#endif

    /* return the descriptor */
    btl_ep->btl_free(btl_ep->btl, descriptor);

    /* check for request completion */
    OPAL_THREAD_LOCK(&ompi_request_lock);
    if (OPAL_THREAD_ADD32(&sendreq->req_pipeline_depth,-1) == 0 &&
        sendreq->req_bytes_delivered == sendreq->req_send.req_bytes_packed) {
        MCA_PML_OB1_SEND_REQUEST_COMPLETE(sendreq); 
        schedule = false;
    } else {
        schedule = true;
    }
    OPAL_THREAD_UNLOCK(&ompi_request_lock);
    if(schedule) {
        mca_pml_ob1_send_request_schedule(sendreq);
    }

    /* check for pending requests */
    MCA_PML_OB1_SEND_REQUEST_PROCESS_PENDING();
}


/**
 *  BTL requires "specially" allocated memory. Request a segment that
 *  is used for initial hdr and any eager data.
 */

int mca_pml_ob1_send_request_start(
    mca_pml_ob1_send_request_t* sendreq,
    mca_pml_ob1_endpoint_t* endpoint)
{
    mca_btl_base_descriptor_t* descriptor;
    mca_btl_base_segment_t* segment;
    mca_pml_ob1_hdr_t* hdr;
    size_t size = sendreq->req_send.req_bytes_packed;
    int rc;

    /* shortcut for zero byte */
    if(size == 0 && sendreq->req_send.req_send_mode != MCA_PML_BASE_SEND_SYNCHRONOUS) {

        /* allocate a descriptor */
        MCA_PML_OB1_ENDPOINT_DES_ALLOC(endpoint, descriptor, sizeof(mca_pml_ob1_match_hdr_t));
        if(NULL == descriptor) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        } 
        segment = descriptor->des_src;

        /* build hdr */
        hdr = (mca_pml_ob1_hdr_t*)segment->seg_addr.pval;
        hdr->hdr_common.hdr_flags = 0;
        hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_MATCH;
        hdr->hdr_match.hdr_contextid = sendreq->req_send.req_base.req_comm->c_contextid;
        hdr->hdr_match.hdr_src = sendreq->req_send.req_base.req_comm->c_my_rank;
        hdr->hdr_match.hdr_dst = sendreq->req_send.req_base.req_peer;
        hdr->hdr_match.hdr_tag = sendreq->req_send.req_base.req_tag;
        hdr->hdr_match.hdr_msg_length = 0;
        hdr->hdr_match.hdr_msg_seq = sendreq->req_send.req_base.req_sequence;

        /* short message */
        descriptor->des_cbfunc = mca_pml_ob1_match_completion;

        /* request is complete at mpi level */
        ompi_request_complete((ompi_request_t*)sendreq);
       
    } else {

        struct iovec iov;
        unsigned int iov_count;
        size_t max_data;
        bool ack = false;

        /* determine first fragment size */
        if(size > endpoint->btl_eager_limit - sizeof(mca_pml_ob1_hdr_t)) {
            size = endpoint->btl_eager_limit - sizeof(mca_pml_ob1_hdr_t);
            ack = true;
        } else if (sendreq->req_send.req_send_mode == MCA_PML_BASE_SEND_SYNCHRONOUS) {
            ack = true;
        }

        /* if an acknowledgment is not required - can get by w/ shorter hdr */
        if (ack == false) {
            int32_t free_after;

            /* allocate descriptor */
            descriptor = endpoint->btl_alloc(endpoint->btl, sizeof(mca_pml_ob1_match_hdr_t) + size);
            if(NULL == descriptor) {
                return OMPI_ERR_OUT_OF_RESOURCE;
            } 
            segment = descriptor->des_src;

            /* pack the data into the supplied buffer */
            iov.iov_base = (void*)((unsigned char*)segment->seg_addr.pval + sizeof(mca_pml_ob1_match_hdr_t));
            iov.iov_len = size;
            iov_count = 1;
            max_data = size;
            if((rc = ompi_convertor_pack(
                &sendreq->req_send.req_convertor,
                &iov,
                &iov_count,
                &max_data,
                &free_after)) < 0) {
                endpoint->btl_free(endpoint->btl, descriptor);
                return rc;
            }

            /* build match header */
            hdr = (mca_pml_ob1_hdr_t*)segment->seg_addr.pval;
            hdr->hdr_common.hdr_flags = 0;
            hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_MATCH;
            hdr->hdr_match.hdr_contextid = sendreq->req_send.req_base.req_comm->c_contextid;
            hdr->hdr_match.hdr_src = sendreq->req_send.req_base.req_comm->c_my_rank;
            hdr->hdr_match.hdr_dst = sendreq->req_send.req_base.req_peer;
            hdr->hdr_match.hdr_tag = sendreq->req_send.req_base.req_tag;
            hdr->hdr_match.hdr_msg_length = sendreq->req_send.req_bytes_packed;
            hdr->hdr_match.hdr_msg_seq = sendreq->req_send.req_base.req_sequence;

            /* update lengths */
            segment->seg_len = sizeof(mca_pml_ob1_match_hdr_t) + max_data;
            sendreq->req_send_offset = max_data;
            sendreq->req_rdma_offset = max_data;

            /* short message */
            descriptor->des_cbfunc = mca_pml_ob1_match_completion;
           
            /* request is complete at mpi level */
            ompi_request_complete((ompi_request_t*)sendreq);

        /* rendezvous header is required */
        } else {
            int32_t free_after;

            /* allocate space for hdr + first fragment */
            descriptor = endpoint->btl_alloc(endpoint->btl, sizeof(mca_pml_ob1_rendezvous_hdr_t) + size);
            if(NULL == descriptor) {
                return OMPI_ERR_OUT_OF_RESOURCE;
            }
            segment = descriptor->des_src;

            /* check to see if memory is registered */
            sendreq->req_chunk = mca_mpool_base_find(sendreq->req_send.req_addr);


            /* if the buffer is not pinned and leave pinned is false we eagerly send
               data to cover the cost of pinning the recv buffers on the peer */ 
            if(size && NULL == sendreq->req_chunk && !mca_pml_ob1.leave_pinned){ 
                /* pack the data into the supplied buffer */
                iov.iov_base = (void*)((unsigned char*)segment->seg_addr.pval + 
                                       sizeof(mca_pml_ob1_rendezvous_hdr_t));
                iov.iov_len = size;
                iov_count = 1;
                max_data = size;
                if((rc = ompi_convertor_pack(
                                             &sendreq->req_send.req_convertor,
                                             &iov,
                                             &iov_count,
                                             &max_data,
                                             &free_after)) < 0) {
                    endpoint->btl_free(endpoint->btl, descriptor);
                    return rc;
                }
                if(max_data != size) {
                    opal_output(0, "[%s:%d] max_data (%lu) != size (%lu)\n", __FILE__,__LINE__,max_data,size);
                }
            }
            /* if the buffer is pinned or leave pinned is true we do not eagerly send 
               any data */ 
            else { 
                max_data = 0; 
            }
            /* build hdr */
            hdr = (mca_pml_ob1_hdr_t*)segment->seg_addr.pval;
            hdr->hdr_common.hdr_flags = (sendreq->req_chunk != NULL ? MCA_PML_OB1_HDR_FLAGS_PIN : 0);
            hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_RNDV;
            hdr->hdr_match.hdr_contextid = sendreq->req_send.req_base.req_comm->c_contextid;
            hdr->hdr_match.hdr_src = sendreq->req_send.req_base.req_comm->c_my_rank;
            hdr->hdr_match.hdr_dst = sendreq->req_send.req_base.req_peer;
            hdr->hdr_match.hdr_tag = sendreq->req_send.req_base.req_tag;
            hdr->hdr_match.hdr_msg_length = sendreq->req_send.req_bytes_packed;
            hdr->hdr_match.hdr_msg_seq = sendreq->req_send.req_base.req_sequence;
            hdr->hdr_rndv.hdr_src_req.lval = 0; /* for VALGRIND/PURIFY - REPLACE WITH MACRO */
            hdr->hdr_rndv.hdr_src_req.pval = sendreq;
            hdr->hdr_rndv.hdr_frag_length = max_data;

            /* update lengths with number of bytes actually packed */
            segment->seg_len = sizeof(mca_pml_ob1_rendezvous_hdr_t) + max_data;
            sendreq->req_send_offset = max_data;

            /* first fragment of a long message */
            descriptor->des_cbfunc = mca_pml_ob1_rndv_completion;
        }
    }
    descriptor->des_flags |= MCA_BTL_DES_FLAGS_PRIORITY;
    descriptor->des_cbdata = sendreq;
    OPAL_THREAD_ADD32(&sendreq->req_pipeline_depth,1);

    /* send */
#if MCA_PML_OB1_TIMESTAMPS
    sendreq->t_start = get_profiler_timestamp();
#endif
    rc = endpoint->btl_send(
        endpoint->btl, 
        endpoint->btl_endpoint, 
        descriptor,
        MCA_BTL_TAG_PML);
    if(OMPI_SUCCESS != rc) {
        endpoint->btl_free(endpoint->btl,descriptor);
    }
    return rc;
}


/**
 *
 */

int mca_pml_ob1_send_request_schedule(mca_pml_ob1_send_request_t* sendreq)
{ 
    /*
     * Only allow one thread in this routine for a given request.
     * However, we cannot block callers on a mutex, so simply keep track
     * of the number of times the routine has been called and run through
     * the scheduling logic once for every call.
    */
    if(OPAL_THREAD_ADD32(&sendreq->req_lock,1) == 1) {
        mca_pml_ob1_proc_t* proc = sendreq->req_proc;
        size_t num_btl_avail = mca_pml_ob1_ep_array_get_size(&proc->btl_send);
        do {
            /* allocate remaining bytes to BTLs */
            size_t bytes_remaining = sendreq->req_rdma_offset - sendreq->req_send_offset;
            while(bytes_remaining > 0 && 
                  (sendreq->req_pipeline_depth < mca_pml_ob1.send_pipeline_depth ||
                   sendreq->req_rdma_offset < sendreq->req_send.req_bytes_packed)) {
                mca_pml_ob1_endpoint_t* ep = mca_pml_ob1_ep_array_get_next(&proc->btl_send);
                mca_pml_ob1_frag_hdr_t* hdr;
                mca_btl_base_descriptor_t* des;
                int rc;

                /* if there is only one btl available or the size is less than
                 * than the min fragment size, schedule the rest via this btl
                 */
                size_t size;
                if(num_btl_avail == 1 || bytes_remaining < ep->btl_min_send_size) {
                    size = bytes_remaining;

                /* otherwise attempt to give the BTL a percentage of the message
                 * based on a weighting factor. for simplicity calculate this as
                 * a percentage of the overall message length (regardless of amount
                 * previously assigned)
                 */
                } else {
                    size = (ep->btl_weight * bytes_remaining) / 100;
                } 

                /* makes sure that we don't exceed BTL max send size */
                if (ep->btl_max_send_size != 0 && 
                    size > ep->btl_max_send_size - sizeof(mca_pml_ob1_frag_hdr_t)) {
                    size = ep->btl_max_send_size - sizeof(mca_pml_ob1_frag_hdr_t);
                }
                                                                                                                  
                /* pack into a descriptor */
                ompi_convertor_set_position(&sendreq->req_send.req_convertor, 
                    &sendreq->req_send_offset);
                des = ep->btl_prepare_src(
                    ep->btl,
                    ep->btl_endpoint,
                    NULL,
                    &sendreq->req_send.req_convertor,
                    sizeof(mca_pml_ob1_frag_hdr_t),
                    &size);
                if(des == NULL) {
                    OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
                    opal_list_append(&mca_pml_ob1.send_pending, (opal_list_item_t*)sendreq);
                    OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
                    break;
                }
                des->des_cbfunc = mca_pml_ob1_frag_completion;
                des->des_cbdata = sendreq;

                /* setup header */
                hdr = (mca_pml_ob1_frag_hdr_t*)des->des_src->seg_addr.pval;
                hdr->hdr_common.hdr_flags = 0;
                hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_FRAG;
                hdr->hdr_frag_length = size;
                hdr->hdr_frag_offset = sendreq->req_send_offset;
                hdr->hdr_src_req.pval = sendreq;
                hdr->hdr_dst_req = sendreq->req_recv;

                /* update state */
                sendreq->req_send_offset += size;
                OPAL_THREAD_ADD32(&sendreq->req_pipeline_depth,1);

                /* initiate send - note that this may complete before the call returns */
                rc = ep->btl_send(ep->btl, ep->btl_endpoint, des, MCA_BTL_TAG_PML);
                if(rc == OMPI_SUCCESS) {
                    bytes_remaining -= size;
                } else {
                    sendreq->req_send_offset -= size;
                    OPAL_THREAD_ADD32(&sendreq->req_pipeline_depth,-1);
                    ep->btl_free(ep->btl,des);
                    OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
                    opal_list_append(&mca_pml_ob1.send_pending, (opal_list_item_t*)sendreq);
                    OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
                    break;
                }
#if MCA_PML_OB1_TIMESTAMPS
                if(bytes_remaining == 0)
                    sendreq->t_scheduled = get_profiler_timestamp();
#endif
            }
        } while (OPAL_THREAD_ADD32(&sendreq->req_lock,-1) > 0);
    }
    return OMPI_SUCCESS;
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
    mca_pml_ob1_endpoint_t* endpoint = frag->rdma_ep;
    MCA_PML_OB1_RDMA_FRAG_RETURN(frag);
    MCA_PML_OB1_ENDPOINT_DES_RETURN(endpoint, des);
}

/**
 *  An RDMA put operation has completed:
 *  (1) Update request status and if required set completed
 *  (2) Send FIN control message to the destination 
 */

static void mca_pml_ob1_put_completion(
    mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* ep,
    struct mca_btl_base_descriptor_t* des,
    int status)
{
    mca_pml_ob1_rdma_frag_t* frag = (mca_pml_ob1_rdma_frag_t*)des->des_cbdata;
    mca_pml_ob1_send_request_t* sendreq = frag->rdma_req;
    mca_btl_base_descriptor_t* fin;
    mca_pml_ob1_fin_hdr_t* hdr;
    int rc;

    /* check completion status */
    if(OMPI_SUCCESS != status) {
        /* TSW - FIX */
        ORTE_ERROR_LOG(status);
        orte_errmgr.abort();
    }

#if MCA_PML_OB1_TIMESTAMPS
    /* update statistics */
    sendreq->t_fin[sendreq->t_fin_index++] = get_profiler_timestamp();
    if(sendreq->t_fin_index >= MCA_PML_OB1_NUM_TSTAMPS)
        sendreq->t_fin_index = 0;
#endif

    /* check for request completion */
    OPAL_THREAD_LOCK(&ompi_request_lock);
    sendreq->req_bytes_delivered += frag->rdma_length;
    if(sendreq->req_bytes_delivered >= sendreq->req_send.req_bytes_packed) {
        MCA_PML_OB1_SEND_REQUEST_COMPLETE(sendreq);
    }
    OPAL_THREAD_UNLOCK(&ompi_request_lock);

    /* allocate descriptor for fin control message - note that
     * the rdma descriptor cannot be reused as it points directly
     * at the user buffer
     */
    frag->rdma_state = MCA_PML_OB1_RDMA_FIN;

    MCA_PML_OB1_ENDPOINT_DES_ALLOC(frag->rdma_ep, fin, sizeof(mca_pml_ob1_fin_hdr_t));
    if(NULL == fin) {
        OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
        opal_list_append(&mca_pml_ob1.rdma_pending, (opal_list_item_t*)frag);
        OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
        goto cleanup;
    }
    fin->des_flags |= MCA_BTL_DES_FLAGS_PRIORITY;
    fin->des_cbfunc = mca_pml_ob1_fin_completion;
    fin->des_cbdata = frag;

    /* fill in header */
    hdr = (mca_pml_ob1_fin_hdr_t*)fin->des_src->seg_addr.pval;
    hdr->hdr_common.hdr_flags = 0;
    hdr->hdr_common.hdr_type = MCA_PML_OB1_HDR_TYPE_FIN;
    hdr->hdr_src = frag->rdma_hdr.hdr_rdma.hdr_src;
    hdr->hdr_dst = frag->rdma_hdr.hdr_rdma.hdr_dst;
    hdr->hdr_rdma_offset = frag->rdma_hdr.hdr_rdma.hdr_rdma_offset;
    hdr->hdr_rdma_length = frag->rdma_length;

    /* queue request */
    rc = btl->btl_send(
        btl,
        ep,
        fin,
        MCA_BTL_TAG_PML);
    if(OMPI_SUCCESS != rc) {
        btl->btl_free(btl, fin);
        if(rc == OMPI_ERR_OUT_OF_RESOURCE) {
            OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
            opal_list_append(&mca_pml_ob1.rdma_pending, (opal_list_item_t*)frag);
            OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
        } else {
            /* TSW - FIX */
            ORTE_ERROR_LOG(rc);
            orte_errmgr.abort();
        }
    }

cleanup:
    /* return rdma descriptor - do this after queuing the fin message - as 
     * release rdma resources (unpin memory) can take some time.
     */
    des->des_dst = NULL; 
    des->des_dst_cnt = 0; 
    btl->btl_free(btl, des);

}


/**
 *  Receiver has scheduled an RDMA operation:
 *  (1) Allocate an RDMA fragment to maintain the state of the operation
 *  (2) Call BTL prepare_src to pin/prepare source buffers
 *  (3) Queue the RDMA put 
 */

void mca_pml_ob1_send_request_put(
    mca_pml_ob1_send_request_t* sendreq,
    mca_btl_base_module_t* btl,
    mca_pml_ob1_rdma_hdr_t* hdr)
{ 
    mca_pml_ob1_proc_t* proc = sendreq->req_proc;
    mca_pml_ob1_endpoint_t* ep = mca_pml_ob1_ep_array_find(&proc->btl_rdma,btl);
    mca_mpool_base_registration_t* reg = NULL;
    mca_btl_base_descriptor_t* des;
    mca_pml_ob1_rdma_frag_t* frag;
    size_t offset = hdr->hdr_rdma_offset;
    size_t i, size = 0;
    int rc;

    MCA_PML_OB1_RDMA_FRAG_ALLOC(frag, rc); 
    if(NULL == frag) {
        /* TSW - FIX */
        ORTE_ERROR_LOG(rc);
        orte_errmgr.abort();
    }

    /* setup fragment */
    for(i=0; i<hdr->hdr_seg_cnt; i++) {
        size += hdr->hdr_segs[i].seg_len;
        frag->rdma_segs[i] = hdr->hdr_segs[i];
    }
    frag->rdma_hdr.hdr_rdma = *hdr;
    frag->rdma_req = sendreq; 
    frag->rdma_ep = ep;
    frag->rdma_state = MCA_PML_OB1_RDMA_PREPARE;

    /* look for a prior registration on this interface */
    if(NULL != sendreq->req_chunk) {
        mca_mpool_base_reg_mpool_t* mpool = sendreq->req_chunk->mpools;
        while(mpool->mpool != NULL) {
            if(mpool->user_data == (void*) btl) { 
                reg = mpool->mpool_registration; 
                break;
            }
            mpool++;
        }
    }

#if MCA_PML_OB1_TIMESTAMPS
    sendreq->t_pin[sendreq->t_pin_index++] = get_profiler_timestamp();
    if(sendreq->t_pin_index >= MCA_PML_OB1_NUM_TSTAMPS)
        sendreq->t_pin_index = 0;
#endif

    /* setup descriptor */
    ompi_convertor_set_position(&sendreq->req_send.req_convertor, &offset);
    des = btl->btl_prepare_src(
        btl, 
        ep->btl_endpoint,
        reg,
        &sendreq->req_send.req_convertor, 
        0,
        &size);
    if(NULL == des) { 
        OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
        opal_list_append(&mca_pml_ob1.rdma_pending, (opal_list_item_t*)frag);
        OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
    }
    frag->rdma_state = MCA_PML_OB1_RDMA_PUT;
    frag->rdma_length = size;

    des->des_dst = frag->rdma_segs;
    des->des_dst_cnt = hdr->hdr_seg_cnt;
    des->des_cbfunc = mca_pml_ob1_put_completion;
    des->des_cbdata = frag;

#if MCA_PML_OB1_TIMESTAMPS
    /* queue put */
    sendreq->t_put[sendreq->t_put_index++] = get_profiler_timestamp();
    if(sendreq->t_put_index >= MCA_PML_OB1_NUM_TSTAMPS)
        sendreq->t_put_index = 0;
#endif

    if(OMPI_SUCCESS != (rc = btl->btl_put(btl, ep->btl_endpoint, des))) {
        if(rc == OMPI_ERR_OUT_OF_RESOURCE) {
            OPAL_THREAD_LOCK(&mca_pml_ob1.lock);
            opal_list_append(&mca_pml_ob1.rdma_pending, (opal_list_item_t*)frag);
            OPAL_THREAD_UNLOCK(&mca_pml_ob1.lock);
        } else {
            /* TSW - FIX */
            ORTE_ERROR_LOG(rc);
            orte_errmgr.abort();
        }
    }
#ifdef HAVE_SCHED_YIELD
    sched_yield();
#endif
}


