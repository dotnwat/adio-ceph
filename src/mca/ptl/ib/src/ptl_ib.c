/*
 * $HEADER$
 */

#include <string.h>
#include "util/output.h"
#include "util/if.h"
#include "mca/pml/pml.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/ptl_base_header.h"
#include "mca/pml/base/pml_base_sendreq.h"
#include "mca/ptl/base/ptl_base_sendfrag.h"
#include "mca/pml/base/pml_base_recvreq.h"
#include "mca/ptl/base/ptl_base_recvfrag.h"
#include "mca/base/mca_base_module_exchange.h"
#include "ptl_ib.h"
//#include "ptl_ib_sendfrag.h"

mca_ptl_ib_module_t mca_ptl_ib_module = {
    {
        &mca_ptl_ib_component.super,
        1, /* max size of request cache */
        sizeof(mca_ptl_ib_send_frag_t), /* bytes required by ptl for a request */
        0, /* max size of first fragment */
        0, /* min fragment size */
        0, /* max fragment size */
        0, /* exclusivity */
        0, /* latency */
        0, /* bandwidth */
        MCA_PTL_PUT,  /* ptl flags */
        mca_ptl_ib_add_procs,
        mca_ptl_ib_del_procs,
        mca_ptl_ib_finalize,
        mca_ptl_ib_send,
        mca_ptl_ib_send,
        NULL,
        mca_ptl_ib_matched,
        mca_ptl_ib_request_init,
        mca_ptl_ib_request_fini
    }
};


int mca_ptl_ib_add_procs(struct mca_ptl_base_module_t* base_module, 
        size_t nprocs, struct ompi_proc_t **ompi_procs, 
        struct mca_ptl_base_peer_t** peers, ompi_bitmap_t* reachable)
{
    int i, rc;
    struct ompi_proc_t* ompi_proc;
    mca_ptl_ib_proc_t* module_proc;
    mca_ptl_base_peer_t* module_peer;

    for(i = 0; i < nprocs; i++) {

        ompi_proc = ompi_procs[i];
        module_proc = mca_ptl_ib_proc_create(ompi_proc);

        if(NULL == module_proc) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        /*
         * Check to make sure that the peer has at least as many interface 
         * addresses exported as we are trying to use. If not, then 
         * don't bind this PTL instance to the proc.
         */

        OMPI_THREAD_LOCK(&module_proc->proc_lock);
        if(module_proc->proc_addr_count == module_proc->proc_peer_count) {
            OMPI_THREAD_UNLOCK(&module_proc->proc_lock);
            return OMPI_ERR_UNREACH;
        }

        /* The ptl_proc datastructure is shared by all IB PTL
         * instances that are trying to reach this destination. 
         * Cache the peer instance on the ptl_proc.
         */
        module_peer = OBJ_NEW(mca_ptl_ib_peer_t);

        if(NULL == module_peer) {
            OMPI_THREAD_UNLOCK(&module_proc->proc_lock);
            return OMPI_ERR_OUT_OF_RESOURCE;
        }

        module_peer->peer_module = (mca_ptl_ib_module_t*)base_module;

        rc = mca_ptl_ib_proc_insert(module_proc, module_peer);
        if(rc != OMPI_SUCCESS) {
            OBJ_RELEASE(module_peer);
            OMPI_THREAD_UNLOCK(&module_proc->proc_lock);
            return rc;
        }

        ompi_bitmap_set_bit(reachable, i);
        OMPI_THREAD_UNLOCK(&module_proc->proc_lock);
        peers[i] = module_peer;
    }

    return OMPI_SUCCESS;
}

int mca_ptl_ib_del_procs(struct mca_ptl_base_module_t* ptl, 
        size_t nprocs, 
        struct ompi_proc_t **procs, 
        struct mca_ptl_base_peer_t ** peers)
{
    /* Stub */
    D_PRINT("Stub\n");
    return OMPI_SUCCESS;
}

int mca_ptl_ib_finalize(struct mca_ptl_base_module_t* ptl)
{
    /* Stub */
    D_PRINT("Stub\n");
    return OMPI_SUCCESS;
}

int mca_ptl_ib_request_init( struct mca_ptl_base_module_t* ptl,
        struct mca_pml_base_send_request_t* request)
{
    mca_ptl_ib_send_frag_t *ib_send_frag;

    ib_send_frag = mca_ptl_ib_alloc_send_frag(ptl,
            request);

    if(NULL == ib_send_frag) {
        D_PRINT("Unable to allocate ib_send_frag");
        return OMPI_ERR_OUT_OF_RESOURCE;
    } else {
        ((mca_ptl_ib_send_request_t *)request)->req_frag = 
            ib_send_frag;
    }

    D_PRINT("sendfrag = %p, lkey = %d", 
            ib_send_frag,
            ib_send_frag->ib_buf.hndl.lkey);

    return OMPI_SUCCESS;
}

void mca_ptl_ib_request_fini( struct mca_ptl_base_module_t* ptl,
        struct mca_pml_base_send_request_t* request)
{
    D_PRINT("");
    OBJ_DESTRUCT(request+1);
}

/*
 *  Initiate a send. If this is the first fragment, use the fragment
 *  descriptor allocated with the send requests, otherwise obtain
 *  one from the free list. Initialize the fragment and foward
 *  on to the peer.
 */

int mca_ptl_ib_send( struct mca_ptl_base_module_t* ptl,
    struct mca_ptl_base_peer_t* ptl_peer,
    struct mca_pml_base_send_request_t* sendreq,
    size_t offset,
    size_t size,
    int flags)
{
    mca_ptl_ib_send_frag_t* sendfrag;
    int rc = OMPI_SUCCESS;

    if (0 == offset) {
        sendfrag = (mca_ptl_ib_send_frag_t *)
            &((mca_ptl_ib_send_request_t*)sendreq)->req_frag;
    } else {

        /* TODO: Implementation for messages > frag size */
        ompi_list_item_t* item;
        OMPI_FREE_LIST_GET(&mca_ptl_ib_component.ib_send_frags, item, rc);

        if(NULL == (sendfrag = (mca_ptl_ib_send_frag_t*)item)) {
            return rc;
        }
    }

    D_PRINT("Sendfrag = %p, Lkey = %d", 
            sendfrag,
            sendfrag->ib_buf.hndl.lkey);

    rc = mca_ptl_ib_send_frag_init(sendfrag, ptl_peer,
            sendreq, offset, &size, flags);
    if(rc != OMPI_SUCCESS) {
        return rc;
    }

    /* Update the offset after actual fragment size is determined,
     * and before attempting to send the fragment */
    sendreq->req_offset += size;

    rc = mca_ptl_ib_peer_send(ptl_peer, sendfrag);

    return rc;
}


/*
 *  A posted receive has been matched - if required send an
 *  ack back to the peer and process the fragment. Copy the
 *  data to user buffer
 */

void mca_ptl_ib_matched(mca_ptl_base_module_t* module,
    mca_ptl_base_recv_frag_t* frag)
{
    mca_pml_base_recv_request_t *request;
    mca_ptl_base_header_t *header;
    mca_ptl_ib_recv_frag_t *recv_frag;

    header  = &frag->frag_base.frag_header;
    request = frag->frag_request;
    recv_frag = (mca_ptl_ib_recv_frag_t*) frag;

    D_PRINT("Matched frag\n");

    if (header->hdr_common.hdr_flags & MCA_PTL_FLAGS_ACK_MATCHED) {
        D_PRINT("Doh, I cannot send an ack!\n");
    }

    /* Process the fragment */

    /* IN TCP case, IO_VEC is first allocated.
     * then recv the data, and copy if needed,
     * But in ELAN cases, we save the data into an
     * unex buffer if the recv descriptor is not posted
     * (for too long) (TODO).
     * We then need to copy from
     * unex_buffer to application buffer */

    if (header->hdr_frag.hdr_frag_length > 0) {

        struct iovec iov;
        ompi_proc_t *proc;

        iov.iov_base = frag->frag_base.frag_addr;
        iov.iov_len  = frag->frag_base.frag_size;

        proc = ompi_comm_peer_lookup(request->req_base.req_comm,
                request->req_base.req_peer);

        ompi_convertor_copy(proc->proc_convertor,
                &frag->frag_base.frag_convertor);

        ompi_convertor_init_for_recv(
                &frag->frag_base.frag_convertor,
                0,
                request->req_base.req_datatype,
                request->req_base.req_count,
                request->req_base.req_addr,
                header->hdr_frag.hdr_frag_offset);
        ompi_convertor_unpack(&frag->frag_base.frag_convertor, &iov, 1);
    }
    mca_ptl_ib_recv_frag_done(header, frag, request);
}
