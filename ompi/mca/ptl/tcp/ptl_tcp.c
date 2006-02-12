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
#include <string.h>
#include "ompi/class/ompi_bitmap.h"
#include "opal/util/output.h"
#include "opal/util/if.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/ptl/ptl.h"
#include "ompi/mca/ptl/base/ptl_base_header.h"
#include "ompi/mca/ptl/base/ptl_base_sendreq.h"
#include "ompi/mca/ptl/base/ptl_base_sendfrag.h"
#include "ompi/mca/ptl/base/ptl_base_recvreq.h"
#include "ompi/mca/ptl/base/ptl_base_recvfrag.h"
#include "ptl_tcp.h"
#include "ptl_tcp_addr.h"
#include "ptl_tcp_peer.h"
#include "ptl_tcp_proc.h"
#include "ptl_tcp_sendreq.h"
#include "ptl_tcp_recvfrag.h"


mca_ptl_tcp_module_t mca_ptl_tcp_module = {
    {
    &mca_ptl_tcp_component.super,
    16, /* max size of request cache */
    sizeof(mca_ptl_tcp_send_request_t) - sizeof(mca_ptl_base_send_request_t), /* bytes required by ptl for a request */
    0, /* max size of first fragment */
    0, /* min fragment size */
    0, /* max fragment size */
    0, /* exclusivity */
    0, /* latency */
    0, /* bandwidth */
    MCA_PTL_PUT,  /* ptl flags */
    mca_ptl_tcp_add_procs,
    mca_ptl_tcp_del_procs,
    mca_ptl_tcp_finalize,
    mca_ptl_tcp_send,
    mca_ptl_tcp_send,
    NULL,
    mca_ptl_tcp_matched,
    mca_ptl_tcp_request_init,
    mca_ptl_tcp_request_fini,
    NULL,
    NULL,
    NULL
    }
};

/*
 * For each peer process:
 * (1) Lookup/create a parallel structure that represents the TCP state of the peer process.
 * (2) Use the mca_pml_base_modex_recv function determine the endpoints exported by the peer.
 * (3) Create a data structure to represent the state of the connection to the peer.
 * (4) Select an address exported by the peer to use for this connection.
 */

int mca_ptl_tcp_add_procs(
    struct mca_ptl_base_module_t* ptl, 
    size_t nprocs, 
    struct ompi_proc_t **ompi_procs, 
    struct mca_ptl_base_peer_t** peers, 
    ompi_bitmap_t* reachable)
{
    size_t i;
    mca_ptl_tcp_module_t *ptl_tcp = (mca_ptl_tcp_module_t*)ptl;
    struct ompi_proc_t * proc_self = ompi_proc_local();

    for(i=0; i<nprocs; i++) {
        struct ompi_proc_t *ompi_proc = ompi_procs[i];
        mca_ptl_tcp_proc_t* ptl_proc;
        mca_ptl_base_peer_t* ptl_peer;
        int rc;

        /* Dont let me register tcp for self */
        if( proc_self == ompi_proc ) continue;

        ptl_proc = mca_ptl_tcp_proc_create(ompi_proc);
        if(NULL == ptl_proc)
            return OMPI_ERR_OUT_OF_RESOURCE;

        /* 
         * Check to make sure that the peer has at least as many interface addresses
         * exported as we are trying to use. If not, then don't bind this PTL instance
         * to the proc, as we have already associated a PTL instance w/ each of the
         * endpoints published by the peer.
        */
        OPAL_THREAD_LOCK(&ptl_proc->proc_lock);
        if(ptl_proc->proc_addr_count == ptl_proc->proc_peer_count) {
	    OPAL_THREAD_UNLOCK(&ptl_proc->proc_lock);
	    continue;
	}

        /* The ptl_proc datastructure is shared by all TCP PTL instances that are trying 
         * to reach this destination. Cache the peer instance on the ptl_proc.
         */
        ptl_peer = OBJ_NEW(mca_ptl_tcp_peer_t);
        if(NULL == ptl_peer) {
            OPAL_THREAD_UNLOCK(&ptl_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
        ptl_peer->peer_ptl = (mca_ptl_tcp_module_t*)ptl;
        rc = mca_ptl_tcp_proc_insert(ptl_proc, ptl_peer);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(ptl_peer);
            OPAL_THREAD_UNLOCK(&ptl_proc->proc_lock);
            continue;  /* UNREACHABLE it's not a problem, others PTL can be used to send the data */
        }
        /* do we need to convert to/from network byte order */
        if(ompi_proc->proc_arch != proc_self->proc_arch)
            ptl_peer->peer_nbo = true;

        ompi_bitmap_set_bit(reachable, i);
        OPAL_THREAD_UNLOCK(&ptl_proc->proc_lock);
        peers[i] = ptl_peer;
        opal_list_append(&ptl_tcp->ptl_peers, (opal_list_item_t*)ptl_peer);
        /* we increase the count of MPI users of the event library
           once per peer, so that we are used until we aren't
           connected to a peer */
        opal_progress_event_increment();
    }
    return OMPI_SUCCESS;
}

/*
 * Cleanup the peer datastructure(s) - and remove the cooresponding 
 * tcp process data structure(s).
 */

int mca_ptl_tcp_del_procs(struct mca_ptl_base_module_t* ptl, size_t nprocs, struct ompi_proc_t **procs, struct mca_ptl_base_peer_t ** peers)
{
    size_t i;
    mca_ptl_tcp_module_t *ptl_tcp = (mca_ptl_tcp_module_t*)ptl;

    for(i=0; i<nprocs; i++) {
        opal_list_remove_item(&ptl_tcp->ptl_peers, (opal_list_item_t*)peers[i]);
        OBJ_RELEASE(peers[i]);
        opal_progress_event_decrement();
    }
    return OMPI_SUCCESS;
}

/*
 * Cleanup all peer data structures associated w/ the ptl.
 */

int mca_ptl_tcp_finalize(struct mca_ptl_base_module_t* ptl)
{
    opal_list_item_t* item;
    mca_ptl_tcp_module_t *ptl_tcp = (mca_ptl_tcp_module_t*)ptl;
    for( item = opal_list_remove_first(&ptl_tcp->ptl_peers);
         item != NULL;
         item = opal_list_remove_first(&ptl_tcp->ptl_peers)) {
        mca_ptl_tcp_peer_t *peer = (mca_ptl_tcp_peer_t*)item;
        OBJ_RELEASE(peer);
        opal_progress_event_decrement();
    }
    free(ptl);
    return OMPI_SUCCESS;
}

/*
 * Initialize a request for use by the ptl. Use the extra memory allocated
 * along w/ the ptl to cache the first fragment control information.
 */

int mca_ptl_tcp_request_init(struct mca_ptl_base_module_t* ptl, struct mca_ptl_base_send_request_t* request)
{
    OBJ_CONSTRUCT(request+1, mca_ptl_tcp_send_frag_t);
    return OMPI_SUCCESS;
}


/*
 * Cleanup any resources cached along w/ the request.
 */

void mca_ptl_tcp_request_fini(struct mca_ptl_base_module_t* ptl, struct mca_ptl_base_send_request_t* request)
{
    OBJ_DESTRUCT(request+1);
}


void mca_ptl_tcp_recv_frag_return(struct mca_ptl_base_module_t* ptl, struct mca_ptl_tcp_recv_frag_t* frag)
{
    if(frag->frag_recv.frag_is_buffered) {
        free(frag->frag_recv.frag_base.frag_addr);
        frag->frag_recv.frag_is_buffered = false;
        frag->frag_recv.frag_base.frag_addr = NULL;
    }
    OMPI_FREE_LIST_RETURN(&mca_ptl_tcp_component.tcp_recv_frags, (opal_list_item_t*)frag);
}


void mca_ptl_tcp_send_frag_return(struct mca_ptl_base_module_t* ptl, struct mca_ptl_tcp_send_frag_t* frag)
{
    if(opal_list_get_size(&mca_ptl_tcp_component.tcp_pending_acks)) {
        mca_ptl_tcp_recv_frag_t* pending;
        OPAL_THREAD_LOCK(&mca_ptl_tcp_component.tcp_lock);
        pending = (mca_ptl_tcp_recv_frag_t*)opal_list_remove_first(&mca_ptl_tcp_component.tcp_pending_acks);
        if(NULL == pending) {
            OPAL_THREAD_UNLOCK(&mca_ptl_tcp_component.tcp_lock);
            OMPI_FREE_LIST_RETURN(&mca_ptl_tcp_component.tcp_send_frags, (opal_list_item_t*)frag);
            return;
        }
        OPAL_THREAD_UNLOCK(&mca_ptl_tcp_component.tcp_lock);
        mca_ptl_tcp_send_frag_init_ack(frag, ptl, pending->frag_recv.frag_base.frag_peer, pending);
        if(frag->frag_send.frag_base.frag_peer->peer_nbo) {
            MCA_PTL_BASE_ACK_HDR_HTON(frag->frag_send.frag_base.frag_header.hdr_ack);
        }
        mca_ptl_tcp_peer_send(pending->frag_recv.frag_base.frag_peer, frag, 0);
        mca_ptl_tcp_recv_frag_return(ptl, pending);
    } else {
        OMPI_FREE_LIST_RETURN(&mca_ptl_tcp_component.tcp_send_frags, (opal_list_item_t*)frag);
    }
}

/*
 *  Initiate a send. If this is the first fragment, use the fragment
 *  descriptor allocated with the send requests, otherwise obtain
 *  one from the free list. Initialize the fragment and foward
 *  on to the peer.
 */

int mca_ptl_tcp_send(
    struct mca_ptl_base_module_t* ptl,
    struct mca_ptl_base_peer_t* ptl_peer,
    struct mca_ptl_base_send_request_t* sendreq,
    size_t offset,
    size_t size,
    int flags)
{
    mca_ptl_tcp_send_frag_t* sendfrag;
    int rc;
    if (offset == 0 && sendreq->req_cached) {
        sendfrag = &((mca_ptl_tcp_send_request_t*)sendreq)->req_frag;
    } else {
        opal_list_item_t* item;
        OMPI_FREE_LIST_GET(&mca_ptl_tcp_component.tcp_send_frags, item, rc);
        if(NULL == (sendfrag = (mca_ptl_tcp_send_frag_t*)item))
            return rc;
    }
    rc = mca_ptl_tcp_send_frag_init(sendfrag, ptl_peer, sendreq, offset, &size, flags);
    if(rc != OMPI_SUCCESS)
        return rc;
    /* must update the offset after actual fragment size is determined -- and very important --
     * before attempting to send the fragment 
     */
    mca_ptl_base_send_request_offset(sendreq, size);
    return mca_ptl_tcp_peer_send(ptl_peer, sendfrag, offset);
}


/*
 *  A posted receive has been matched - if required send an
 *  ack back to the peer and process the fragment.
 */

void mca_ptl_tcp_matched(
    mca_ptl_base_module_t* ptl,
    mca_ptl_base_recv_frag_t* frag)
{
    /* send ack back to peer? */
    mca_ptl_base_header_t* header = &frag->frag_base.frag_header;
    if(header->hdr_common.hdr_flags & MCA_PTL_FLAGS_ACK) {
        int rc;
        mca_ptl_tcp_send_frag_t* ack;
        mca_ptl_tcp_recv_frag_t* recv_frag = (mca_ptl_tcp_recv_frag_t*)frag;
        opal_list_item_t* item;
        MCA_PTL_TCP_SEND_FRAG_ALLOC(item, rc);
        ack = (mca_ptl_tcp_send_frag_t*)item;

        if(NULL == ack) {
            OPAL_THREAD_LOCK(&mca_ptl_tcp_component.tcp_lock);
            recv_frag->frag_ack_pending = true;
            opal_list_append(&mca_ptl_tcp_component.tcp_pending_acks, (opal_list_item_t*)frag);
            OPAL_THREAD_UNLOCK(&mca_ptl_tcp_component.tcp_lock);
        } else {
            mca_ptl_tcp_send_frag_init_ack(ack, ptl, recv_frag->frag_recv.frag_base.frag_peer, recv_frag);
            if(ack->frag_send.frag_base.frag_peer->peer_nbo) {
                MCA_PTL_BASE_ACK_HDR_HTON(ack->frag_send.frag_base.frag_header.hdr_ack);
            }
            mca_ptl_tcp_peer_send(ack->frag_send.frag_base.frag_peer, ack, 0);
        }
    }

    /* process fragment if complete */
    mca_ptl_tcp_recv_frag_progress((mca_ptl_tcp_recv_frag_t*)frag);
}


