/* -*- Mode: C; c-basic-offset:4 ; -*- */

/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004 The Ohio State University.
 *                    All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"
#include "include/types.h"
#include "datatype/datatype.h"
#include "ptl_gm.h"
#include "ptl_gm_sendfrag.h"
#include "ptl_gm_priv.h"

static void mca_ptl_gm_send_frag_construct (mca_ptl_gm_send_frag_t* frag);
static void mca_ptl_gm_send_frag_destruct (mca_ptl_gm_send_frag_t* frag);

static void mca_ptl_gm_recv_frag_construct (mca_ptl_gm_recv_frag_t* frag);
static void mca_ptl_gm_recv_frag_destruct (mca_ptl_gm_recv_frag_t* frag);

/*
 * send fragment constructor/destructors.
 */

static void
mca_ptl_gm_send_frag_construct (mca_ptl_gm_send_frag_t* frag)
{
}

static void
mca_ptl_gm_send_frag_destruct (mca_ptl_gm_send_frag_t* frag)
{
}

ompi_class_t mca_ptl_gm_send_frag_t_class = {
    "mca_ptl_gm_send_frag_t",
    OBJ_CLASS (mca_ptl_base_send_frag_t),
    (ompi_construct_t) mca_ptl_gm_send_frag_construct,
    (ompi_destruct_t) mca_ptl_gm_send_frag_destruct
};

/* It's not yet clear for me what's the best solution here. Block until we
 * get a free request or allocate a new one. The fist case allow us to never
 * take care of the gm allocated DMA buffer as all send fragments already have
 * one attached, but it can stop the application progression. The second case
 * require special cases: we should set the data in the header inside the fragment
 * and later when we get some free fragments with DMA memory attached we should
 * put the header back there, and send it.
 *
 * I will implement the first case and add the second one in my TODO list.
 */
mca_ptl_gm_send_frag_t*
mca_ptl_gm_alloc_send_frag( struct mca_ptl_gm_module_t* ptl,
			    struct mca_pml_base_send_request_t* sendreq )
{
    ompi_list_item_t* item;
    mca_ptl_gm_send_frag_t* sendfrag;
    int32_t rc;

    /* first get a gm_send_frag */
    OMPI_FREE_LIST_GET( &(ptl->gm_send_frags), item, rc );
    sendfrag = (mca_ptl_gm_send_frag_t*)item;
    /* And then get some DMA memory to put the data */
    OMPI_FREE_LIST_WAIT( &(ptl->gm_send_dma_frags), item, rc );
    ompi_atomic_sub( &(ptl->num_send_tokens), 1 );
    assert( ptl->num_send_tokens >= 0 );
    sendfrag->send_buf = (void*)item;

    sendfrag->frag_send.frag_request         = sendreq;
    sendfrag->frag_send.frag_base.frag_owner = (struct mca_ptl_base_module_t*)ptl;
    sendfrag->frag_send.frag_base.frag_addr  = sendreq->req_base.req_addr;
    sendfrag->frag_bytes_processed           = 0;
    sendfrag->frag_bytes_validated           = 0;
    sendfrag->status                         = -1;
    sendfrag->type                           = PUT;
    
    return sendfrag;
}

int mca_ptl_gm_send_frag_done( mca_ptl_gm_send_frag_t* frag,
                               mca_pml_base_send_request_t* req )
{
    return OMPI_SUCCESS;
}

int mca_ptl_gm_put_frag_init( struct mca_ptl_gm_send_frag_t** putfrag,
			      struct mca_ptl_gm_peer_t* ptl_peer,
			      struct mca_ptl_gm_module_t* gm_ptl,
			      struct mca_pml_base_send_request_t* sendreq,
			      size_t offset, size_t* size, int flags )
{
    ompi_convertor_t* convertor;
    mca_ptl_gm_send_frag_t* frag;

    frag = mca_ptl_gm_alloc_send_frag( gm_ptl, sendreq ); /*alloc_put_frag */

    frag->frag_send.frag_base.frag_peer  = (struct mca_ptl_base_peer_t*)ptl_peer;
    frag->frag_send.frag_base.frag_size  = *size;
    frag->frag_offset                    = offset;

    if( (*size) > 0 ) {
        convertor = &(frag->frag_send.frag_base.frag_convertor);
        ompi_convertor_copy( &(sendreq->req_convertor), convertor );
        ompi_convertor_init_for_send( convertor, 0,
                                      sendreq->req_base.req_datatype,
                                      sendreq->req_base.req_count,
                                      sendreq->req_base.req_addr,
                                      offset, NULL );
    }
    *putfrag = frag;

    return OMPI_SUCCESS; 
}

/*
 * recv fragment constructor/destructors.
 */

static void
mca_ptl_gm_recv_frag_construct (mca_ptl_gm_recv_frag_t* frag)
{
}

static void
mca_ptl_gm_recv_frag_destruct (mca_ptl_gm_recv_frag_t* frag)
{
}

ompi_class_t mca_ptl_gm_recv_frag_t_class = {
    "mca_ptl_gm_recv_frag_t",
    OBJ_CLASS (mca_ptl_base_recv_frag_t),
    (ompi_construct_t) mca_ptl_gm_recv_frag_construct,
    (ompi_construct_t) mca_ptl_gm_recv_frag_destruct
};

