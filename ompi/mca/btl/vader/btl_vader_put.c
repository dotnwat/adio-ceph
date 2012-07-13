/* -*- Mode: C; c-basic-offset:4 ; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2010-2011 Los Alamos National Security, LLC.  
 *                         All rights reserved. 
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"

#include "btl_vader.h"
#include "btl_vader_frag.h"
#include "btl_vader_endpoint.h"

/**
 * Initiate an synchronous put.
 *
 * @param btl (IN)         BTL module
 * @param endpoint (IN)    BTL addressing information
 * @param descriptor (IN)  Description of the data to be transferred
 */
int mca_btl_vader_put (struct mca_btl_base_module_t *btl,
                       struct mca_btl_base_endpoint_t *endpoint,
                       struct mca_btl_base_descriptor_t *des)
{
    mca_btl_vader_frag_t *frag = (mca_btl_vader_frag_t *) des;
    mca_btl_base_segment_t *src = des->des_src;
    mca_btl_base_segment_t *dst = des->des_dst;
    size_t size = min(dst->seg_len, src->seg_len);
    mca_mpool_base_registration_t *reg;
    void *rem_ptr;

    reg = vader_get_registation (endpoint->peer_smp_rank,
                                 (void *)(uintptr_t) dst->seg_addr.lval,
                                 dst->seg_len, 0);
    if (OPAL_UNLIKELY(NULL == reg)) {
        return OMPI_ERROR;
    }

    rem_ptr = vader_reg_to_ptr (reg, (void *)(uintptr_t) dst->seg_addr.lval);

    vader_memmove (rem_ptr, (void *)(uintptr_t) src->seg_addr.lval, size);

    vader_return_registration (reg, endpoint->peer_smp_rank);

    /* always call the callback function */
    frag->base.des_flags |= MCA_BTL_DES_SEND_ALWAYS_CALLBACK;

    mca_btl_vader_frag_complete (frag);

    return OMPI_SUCCESS;
}
