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

#include "ompi/mca/rcache/rcache.h"
#include "rcache_rb.h"
#include "rcache_rb_tree.h"
#include "rcache_rb_mru.h"
#include "opal/util/output.h"
#include "ompi/mca/mpool/base/base.h" 

extern unsigned int mca_mpool_base_page_size; 
extern unsigned int mca_mpool_base_page_size_log;

/**
 * Initialize the rcache 
 */ 

void mca_rcache_rb_module_init( mca_rcache_rb_module_t* rcache ) { 

    rcache->base.rcache_find = mca_rcache_rb_find; 
    rcache->base.rcache_insert = mca_rcache_rb_insert; 
    rcache->base.rcache_delete = mca_rcache_rb_delete; 
    rcache->base.rcache_finalize = mca_rcache_rb_finalize; 
    OBJ_CONSTRUCT(&rcache->base.lock, opal_mutex_t);
    mca_rcache_rb_tree_init(rcache);
    mca_rcache_rb_mru_init(rcache);
}

int mca_rcache_rb_find (
                        struct mca_rcache_base_module_t* rcache, 
                        void* addr, 
                        size_t size, 
                        ompi_pointer_array_t* regs, 
                        uint32_t *cnt
                        ){ 
    
    int rc = OMPI_SUCCESS; 
    mca_rcache_rb_tree_item_t* tree_item = NULL; 
    void* base_addr; 
    void* bound_addr; 
    if(size == 0) { 
        return OMPI_ERROR; 
    }
    OPAL_THREAD_LOCK(&rcache->lock);
    *cnt = 0;
    
    base_addr = down_align_addr(addr, mca_mpool_base_page_size_log);
    bound_addr = up_align_addr((void*) ((unsigned long) addr + size - 1), mca_mpool_base_page_size_log);
        
    
    while(base_addr <= bound_addr) { 
        tree_item = mca_rcache_rb_tree_find( (mca_rcache_rb_module_t*) rcache, base_addr ); 
        if(NULL != tree_item) { 
            ompi_pointer_array_add(regs, (void*) tree_item->reg); 
            if( tree_item->reg->flags & MCA_MPOOL_FLAGS_CACHE ) { 
                rc = mca_rcache_rb_mru_touch((mca_rcache_rb_module_t*)rcache, 
                                             tree_item->reg); 
                if(OMPI_SUCCESS != rc) { 
                    OPAL_THREAD_UNLOCK(&rcache->lock);
                    return OMPI_ERROR;
                }
            }
            OPAL_THREAD_ADD32((int32_t*) &tree_item->reg->ref_count, 1); 
            (*cnt)++; 
            assert(tree_item->reg->bound - tree_item->reg->base >= 0); 
            assert(((void*) tree_item->reg->bound) >= addr);
            base_addr = tree_item->reg->bound + 1;
        }
        else { 
            base_addr =(void*) ((unsigned long) base_addr + mca_mpool_base_page_size);
        }
    }
    
    
    OPAL_THREAD_UNLOCK(&rcache->lock);
    return OMPI_SUCCESS;
}

int mca_rcache_rb_insert ( 
                          struct mca_rcache_base_module_t* rcache, 
                          mca_mpool_base_registration_t* reg, 
                          uint32_t flags
                          ) { 
    int rc = OMPI_SUCCESS;
    OPAL_THREAD_LOCK(&rcache->lock); 
    reg->flags = flags;
    if(flags & MCA_MPOOL_FLAGS_CACHE) { 
        rc = mca_rcache_rb_mru_insert( (mca_rcache_rb_module_t*) rcache, reg); 
        if(OMPI_SUCCESS != rc) { 
            OPAL_THREAD_UNLOCK(&rcache->lock);
            if(OMPI_ERR_TEMP_OUT_OF_RESOURCE == rc) {
                /* if the registration is too big for the rcache, 
                   don't cahce it and reset the flags so the upper level 
                   handles things appropriatly */ 
                reg->flags = 0;
                return OMPI_SUCCESS;
            }
            return rc; 
        }
        OPAL_THREAD_ADD32((int32_t*)&reg->ref_count, 1); 
    }
    rc = mca_rcache_rb_tree_insert((mca_rcache_rb_module_t*)rcache, reg);
    OPAL_THREAD_ADD32((int32_t*) &reg->ref_count, 1); 
    OPAL_THREAD_UNLOCK(&rcache->lock); 
    return rc;
}

int mca_rcache_rb_delete (
                          struct mca_rcache_base_module_t* rcache, 
                          mca_mpool_base_registration_t* reg, 
                          uint32_t flags
                          ) { 
    int rc = OMPI_SUCCESS; 
    assert(reg->ref_count >= 1); 
    OPAL_THREAD_LOCK(&rcache->lock); 
    if(flags & MCA_MPOOL_FLAGS_CACHE) { 
        assert(reg->ref_count >= 2);
        OPAL_THREAD_ADD32((int32_t*)&reg->ref_count, -1);
        rc = mca_rcache_rb_mru_delete( (mca_rcache_rb_module_t*) rcache, reg); 
    }
    if(OMPI_SUCCESS != rc) { 
        OPAL_THREAD_UNLOCK(&rcache->lock);
        return rc; 
    }
    reg->flags = 0; 
    OPAL_THREAD_ADD32((int32_t*)&reg->ref_count, -1);
    rc =  mca_rcache_rb_tree_delete((mca_rcache_rb_module_t*)rcache, reg );
    OPAL_THREAD_UNLOCK(&rcache->lock);
    return rc; 
}



/**
  * finalize
  */
void mca_rcache_rb_finalize(
                            struct mca_rcache_base_module_t* rcache
                            ) { 

}





