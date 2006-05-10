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
#include "btl_sm_frag.h"

/**
 * Internal alignment for the SM BTL. Don't know yet if it's interesting to export it
 * via the MCA parameters. It might make sense on some NUMA machines to have the
 * fragments aligned on 64 bits instead ...
 * What we really need here is to have the header on the same cache line to
 * speed-up the access to the header information.
 */
#define BTL_SM_ALIGNMENT_TO   32

static inline void mca_btl_sm_frag_constructor(mca_btl_sm_frag_t* frag)
{
    size_t alignment;

    frag->segment.seg_addr.pval = frag+1;
    alignment = (size_t)((unsigned long)frag->segment.seg_addr.pval &
                         (unsigned long)(BTL_SM_ALIGNMENT_TO - 1));
    if( 0 != alignment ) {
        unsigned long ptr = (unsigned long)frag->segment.seg_addr.pval;
        alignment = BTL_SM_ALIGNMENT_TO - alignment;
        ptr += alignment;
        frag->segment.seg_addr.pval = (void*)ptr;
    }
    frag->segment.seg_len = frag->size - alignment;
    frag->base.des_src = &frag->segment;
    frag->base.des_src_cnt = 1;
    frag->base.des_dst = &frag->segment;
    frag->base.des_dst_cnt = 1;
    frag->base.des_flags = 0;
}

static void mca_btl_sm_frag1_constructor(mca_btl_sm_frag_t* frag)
{
    frag->size = mca_btl_sm_component.eager_limit;
    mca_btl_sm_frag_constructor(frag);
}

static void mca_btl_sm_frag2_constructor(mca_btl_sm_frag_t* frag)
{
    frag->size = mca_btl_sm_component.max_frag_size;
    mca_btl_sm_frag_constructor(frag);
}

static void mca_btl_sm_frag_destructor(mca_btl_sm_frag_t* frag)
{
}


OBJ_CLASS_INSTANCE(
    mca_btl_sm_frag_t,
    mca_btl_base_descriptor_t,
    mca_btl_sm_frag_constructor,
    mca_btl_sm_frag_destructor);

OBJ_CLASS_INSTANCE(
    mca_btl_sm_frag1_t,
    mca_btl_base_descriptor_t,
    mca_btl_sm_frag1_constructor,
    mca_btl_sm_frag_destructor);

OBJ_CLASS_INSTANCE(
    mca_btl_sm_frag2_t,
    mca_btl_base_descriptor_t,
    mca_btl_sm_frag2_constructor,
    mca_btl_sm_frag_destructor);

