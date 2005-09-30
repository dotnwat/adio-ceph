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
/**
 * @file
 */
#ifndef MCA_PTL_IB_H
#define MCA_PTL_IB_H

/* Standard system includes */
#include <sys/types.h>
#include <string.h>

/* Open MPI includes */
#include "class/ompi_free_list.h"
#include "class/ompi_bitmap.h"
#include "opal/event/event.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"
#include "opal/util/output.h"
#include "mca/mpool/mpool.h"
#include "mca/btl/base/btl_base_error.h"

#include "mca/btl/btl.h"
#include "mca/btl/base/base.h" 
#include "btl_mvapi_endpoint.h" 

#include <vapi.h>
#include <mtl_common.h>
#include <vapi_common.h>


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define MCA_BTL_IB_LEAVE_PINNED 1

/**
 * Infiniband (IB) BTL component.
 */

struct mca_btl_mvapi_component_t {
    mca_btl_base_component_1_0_0_t          super;  /**< base BTL component */ 
    
    uint32_t                                ib_num_btls;
    /**< number of hcas available to the IB component */

    struct mca_btl_mvapi_module_t            *mvapi_btls;
    /**< array of available PTLs */

    int ib_free_list_num;
    /**< initial size of free lists */

    int ib_free_list_max;
    /**< maximum size of free lists */

    int ib_free_list_inc;
    /**< number of elements to alloc when growing free lists */

    opal_list_t                             ib_procs;
    /**< list of ib proc structures */

    opal_event_t                            ib_send_event;
    /**< event structure for sends */

    opal_event_t                            ib_recv_event;
    /**< event structure for recvs */

    opal_mutex_t                            ib_lock;
    /**< lock for accessing module state */

    int                                     ib_mem_registry_hints_log_size;
    /**< log2 size of hints hash array used by memory registry */
    
    char* ib_mpool_name; 
    /**< name of ib memory pool */ 
   
    uint32_t ib_rr_buf_max; 
    /**< the maximum number of posted rr */  
   
    uint32_t ib_rr_buf_min; 
    /**< the minimum number of posted rr */ 
    
    uint32_t rd_per_peer; 
    /**< the number of recv desc posted per log(peer) in SRQ mode */ 

    size_t eager_limit; 
    size_t max_send_size; 

    uint32_t leave_pinned; 
    uint32_t reg_mru_len; 
    uint32_t use_srq; 
    
    uint32_t ib_cq_size;   /**< Max outstanding CQE on the CQ */  
    uint32_t ib_wq_size;   /**< Max outstanding WR on the WQ */ 
    uint32_t ib_sg_list_size; /**< Max scatter/gather descriptor entries on the WQ*/ 
    uint32_t ib_pkey_ix; 
    uint32_t ib_psn; 
    uint32_t ib_qp_ous_rd_atom; 
    uint32_t ib_mtu; 
    uint32_t ib_min_rnr_timer; 
    uint32_t ib_timeout; 
    uint32_t ib_retry_count; 
    uint32_t ib_rnr_retry; 
    uint32_t ib_max_rdma_dst_ops; 
    uint32_t ib_service_level; 
    uint32_t ib_static_rate; 
    uint32_t ib_src_path_bits; 
    /* number of send tokes available */ 
    uint32_t max_wr_sq_tokens; 

}; typedef struct mca_btl_mvapi_component_t mca_btl_mvapi_component_t;

extern mca_btl_mvapi_component_t mca_btl_mvapi_component;

typedef mca_btl_base_recv_reg_t mca_btl_mvapi_recv_reg_t; 
    


/**
 * IB PTL Interface
 */
struct mca_btl_mvapi_module_t {
    mca_btl_base_module_t  super;  /**< base PTL interface */
    bool btl_inited; 
    mca_btl_mvapi_recv_reg_t ib_reg[256];
    mca_btl_mvapi_port_info_t port_info;  /* contains only the subnet right now */ 
    VAPI_hca_id_t   hca_id;        /**< ID of HCA */
    IB_port_t port_id; /**< ID of the PORT */ 
    VAPI_hca_port_t port;          /**< IB port of this PTL */
    VAPI_hca_hndl_t nic;           /**< NIC handle */
    VAPI_pd_hndl_t  ptag;          /**< Protection Domain tag */
    
    VAPI_cq_hndl_t cq_hndl_high;    /**< High Priority Completion Queue handle */ 
    VAPI_cq_hndl_t  cq_hndl_low;       /**< Low Priority Completion Queue handle */
    
    EVAPI_async_handler_hndl_t async_handler;
    /**< Async event handler used to detect weird/unknown events */
    
    ompi_free_list_t send_free_eager;    /**< free list of eager buffer descriptors */
    ompi_free_list_t send_free_max; /**< free list of max buffer descriptors */ 
    ompi_free_list_t send_free_frag; /**< free list of frags only... used for pining memory */ 
    
    ompi_free_list_t recv_free_eager;    /**< High priority free list of buffer descriptors */
    ompi_free_list_t recv_free_max;      /**< Low priority free list of buffer descriptors */ 

    opal_list_t reg_mru_list;   /**< a most recently used list of mca_mpool_mvapi_registration_t 
                                       entries, this allows us to keep a working set of memory pinned */ 
    
    opal_list_t repost;            /**< list of buffers to repost */
    opal_mutex_t ib_lock;          /**< module level lock */ 
    
    VAPI_rr_desc_t*                          rr_desc_post;
    VAPI_srq_hndl_t srq_hndl_high; /**< A high priority shared receive queue 
                                         runtime optional, can also use a receive queue 
                                         per queue pair.. */ 
    VAPI_srq_hndl_t srq_hndl_low; /**< A low priority shared receive queue */ 
        
    uint32_t srr_posted_high;  /**< number of high priority shared rr posted to the nic*/ 
    uint32_t srr_posted_low;  /**< number of low priority shared rr posted to the nic*/ 

    /**< an array to allow posting of rr in one swoop */ 
    size_t ib_inline_max; /**< max size of inline send*/ 
    
    uint32_t num_peers; 
    uint32_t rd_buf_max; 

    
}; typedef struct mca_btl_mvapi_module_t mca_btl_mvapi_module_t;
    


#define MCA_BTL_MVAPI_POST_SRR_HIGH(post_srr_high_mvapi_btl, \
                                    post_srr_high_additional) \
{ \
        OPAL_THREAD_LOCK(&post_srr_high_mvapi_btl->ib_lock); \
        if(post_srr_high_mvapi_btl->srr_posted_high <= mca_btl_mvapi_component.ib_rr_buf_min+post_srr_high_additional && \
           post_srr_high_mvapi_btl->srr_posted_high < mca_btl_mvapi_component.ib_rr_buf_max){ \
           MCA_BTL_MVAPI_POST_SRR_SUB(mca_btl_mvapi_component.ib_rr_buf_max -  \
                                      post_srr_high_mvapi_btl->srr_posted_high, \
                                      post_srr_high_mvapi_btl, \
                                      &post_srr_high_mvapi_btl->recv_free_eager, \
                                      &post_srr_high_mvapi_btl->srr_posted_high, \
                                      post_srr_high_mvapi_btl->nic, \
                                      post_srr_high_mvapi_btl->srq_hndl_high); \
        } \
        OPAL_THREAD_UNLOCK(&post_srr_high_mvapi_btl->ib_lock); \
}

#define MCA_BTL_MVAPI_POST_SRR_LOW(post_srr_low_mvapi_btl, \
                                             post_srr_low_additional) \
{ \
    OPAL_THREAD_LOCK(&post_srr_low_mvapi_btl->ib_lock); \
    if(post_srr_low_mvapi_btl->srr_posted_low <= mca_btl_mvapi_component.ib_rr_buf_min+post_srr_low_additional && \
       post_srr_low_mvapi_btl->srr_posted_low < mca_btl_mvapi_component.ib_rr_buf_max){ \
        MCA_BTL_MVAPI_POST_SRR_SUB(mca_btl_mvapi_component.ib_rr_buf_max -  \
                                            post_srr_low_mvapi_btl->srr_posted_low, \
                                            post_srr_low_mvapi_btl, \
                                            &post_srr_low_mvapi_btl->recv_free_max, \
                                            &post_srr_low_mvapi_btl->srr_posted_low, \
                                            post_srr_low_mvapi_btl->nic, \
                                            post_srr_low_mvapi_btl->srq_hndl_low); \
    } \
    OPAL_THREAD_UNLOCK(&post_srr_low_mvapi_btl->ib_lock); \
}


#define MCA_BTL_MVAPI_POST_SRR_SUB(cnt, \
                                   mvapi_btl, \
                                   frag_list, \
                                   srr_posted, \
                                   nic, \
                                   srq_hndl) \
{\
    do { \
    uint32_t i; \
    VAPI_ret_t ret; \
    uint32_t rwqe_posted = 0; \
    int rc; \
    opal_list_item_t* item = NULL; \
    mca_btl_mvapi_frag_t* frag = NULL; \
    VAPI_rr_desc_t* desc_post = mvapi_btl->rr_desc_post; \
    for(i = 0; i < cnt; i++) { \
        OMPI_FREE_LIST_WAIT(frag_list, item, rc); \
        frag = (mca_btl_mvapi_frag_t*) item; \
        frag->sg_entry.len = frag->size + \
            ((unsigned char*) frag->segment.seg_addr.pval-  \
             (unsigned char*) frag->hdr);  \
       desc_post[i] = frag->rr_desc; \
    }\
    ret = VAPI_post_srq( nic, \
                               srq_hndl, \
                               cnt, \
                               desc_post, \
                               &rwqe_posted); \
   if(VAPI_OK != ret) { \
        BTL_ERROR(("error posting receive descriptors to shared receive queue: %s",\
                   VAPI_strerror(ret))); \
   } else if(rwqe_posted < 1) { \
       BTL_ERROR(("error posting receive descriptors to shared receive queue, number of entries posted is %d", rwqe_posted)); \
   } else {\
        OPAL_THREAD_ADD32(srr_posted, cnt); \
   }\
   } while(0);\
}
struct mca_btl_mvapi_frag_t; 
extern mca_btl_mvapi_module_t mca_btl_mvapi_module;

/**
 * Register IB component parameters with the MCA framework
 */
extern int mca_btl_mvapi_component_open(void);

/**
 * Any final cleanup before being unloaded.
 */
extern int mca_btl_mvapi_component_close(void);

/**
 * IB component initialization.
 * 
 * @param num_btl_modules (OUT)                  Number of BTLs returned in BTL array.
 * @param allow_multi_user_threads (OUT)  Flag indicating wether BTL supports user threads (TRUE)
 * @param have_hidden_threads (OUT)       Flag indicating wether BTL uses threads (TRUE)
 *
 *  (1) read interface list from kernel and compare against component parameters
 *      then create a BTL instance for selected interfaces
 *  (2) setup IB listen socket for incoming connection attempts
 *  (3) publish BTL addressing info 
 *
 */
extern mca_btl_base_module_t** mca_btl_mvapi_component_init(
    int *num_btl_modules, 
    bool allow_multi_user_threads,
    bool have_hidden_threads
);


/**
 * IB component progress.
 */
extern int mca_btl_mvapi_component_progress(
                                         void
);


/**
 * Register a callback function that is called on receipt
 * of a fragment.
 *
 * @param btl (IN)     BTL module
 * @return             Status indicating if cleanup was successful
 *
 * When the process list changes, the PML notifies the BTL of the
 * change, to provide the opportunity to cleanup or release any
 * resources associated with the peer.
 */

int mca_btl_mvapi_register(
    struct mca_btl_base_module_t* btl,
    mca_btl_base_tag_t tag,
    mca_btl_base_module_recv_cb_fn_t cbfunc,
    void* cbdata
);
                                                                                                                     

/**
 * Cleanup any resources held by the BTL.
 * 
 * @param btl  BTL instance.
 * @return     OMPI_SUCCESS or error status on failure.
 */

extern int mca_btl_mvapi_finalize(
    struct mca_btl_base_module_t* btl
);


/**
 * PML->BTL notification of change in the process list.
 * 
 * @param btl (IN)
 * @param nprocs (IN)     Number of processes
 * @param procs (IN)      Set of processes
 * @param peers (OUT)     Set of (optional) peer addressing info.
 * @param peers (IN/OUT)  Set of processes that are reachable via this BTL.
 * @return     OMPI_SUCCESS or error status on failure.
 * 
 */

extern int mca_btl_mvapi_add_procs(
    struct mca_btl_base_module_t* btl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_btl_base_endpoint_t** peers,
    ompi_bitmap_t* reachable
);

/**
 * PML->BTL notification of change in the process list.
 *
 * @param btl (IN)     BTL instance
 * @param nproc (IN)   Number of processes.
 * @param procs (IN)   Set of processes.
 * @param peers (IN)   Set of peer data structures.
 * @return             Status indicating if cleanup was successful
 *
 */
extern int mca_btl_mvapi_del_procs(
    struct mca_btl_base_module_t* btl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_btl_base_endpoint_t** peers
);


/**
 * PML->BTL Initiate a send of the specified size.
 *
 * @param btl (IN)               BTL instance
 * @param btl_base_peer (IN)     BTL peer addressing
 * @param send_request (IN/OUT)  Send request (allocated by PML via mca_btl_base_request_alloc_fn_t)
 * @param size (IN)              Number of bytes PML is requesting BTL to deliver
 * @param flags (IN)             Flags that should be passed to the peer via the message header.
 * @param request (OUT)          OMPI_SUCCESS if the BTL was able to queue one or more fragments
 */
extern int mca_btl_mvapi_send(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* btl_peer,
    struct mca_btl_base_descriptor_t* descriptor, 
    mca_btl_base_tag_t tag
);

/**
 * PML->BTL Initiate a put of the specified size.
 *
 * @param btl (IN)               BTL instance
 * @param btl_base_peer (IN)     BTL peer addressing
 * @param send_request (IN/OUT)  Send request (allocated by PML via mca_btl_base_request_alloc_fn_t)
 * @param size (IN)              Number of bytes PML is requesting BTL to deliver
 * @param flags (IN)             Flags that should be passed to the peer via the message header.
 * @param request (OUT)          OMPI_SUCCESS if the BTL was able to queue one or more fragments
 */
extern int mca_btl_mvapi_put(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* btl_peer,
    struct mca_btl_base_descriptor_t* decriptor
);


/**
 * PML->BTL Initiate a get of the specified size.
 *
 * @param btl (IN)               BTL instance
 * @param btl_base_peer (IN)     BTL peer addressing
 * @param send_request (IN/OUT)  Send request (allocated by PML via mca_btl_base_request_alloc_fn_t)
 * @param size (IN)              Number of bytes PML is requesting BTL to deliver
 * @param flags (IN)             Flags that should be passed to the peer via the message header.
 * @param request (OUT)          OMPI_SUCCESS if the BTL was able to queue one or more fragments
 */
extern int mca_btl_mvapi_get(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* btl_peer,
    struct mca_btl_base_descriptor_t* decriptor
);


/**
 * Allocate a descriptor.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Requested descriptor size.
 */
extern mca_btl_base_descriptor_t* mca_btl_mvapi_alloc(
    struct mca_btl_base_module_t* btl, 
    size_t size); 


/**
 * Return a segment allocated by this BTL.
 *
 * @param btl (IN)         BTL module
 * @param descriptor (IN)  Allocated descriptor.
 */
extern int mca_btl_mvapi_free(
    struct mca_btl_base_module_t* btl, 
    mca_btl_base_descriptor_t* des); 
    

/**
 * Pack data and return a descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
mca_btl_base_descriptor_t* mca_btl_mvapi_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* peer,
    mca_mpool_base_registration_t* registration, 
    struct ompi_convertor_t* convertor,
    size_t reserve,
    size_t* size
);

/**
 * Allocate a descriptor initialized for RDMA write.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
extern mca_btl_base_descriptor_t* mca_btl_mvapi_prepare_dst( 
                                                         struct mca_btl_base_module_t* btl, 
                                                         struct mca_btl_base_endpoint_t* peer,
                                                         mca_mpool_base_registration_t* registration, 
                                                         struct ompi_convertor_t* convertor,
                                                         size_t reserve,
                                                         size_t* size); 
/**
 * Return a send fragment to the modules free list.
 *
 * @param btl (IN)   BTL instance
 * @param frag (IN)  IB send fragment
 *
 */
extern void mca_btl_mvapi_send_frag_return(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_mvapi_frag_t*
);


int mca_btl_mvapi_module_init(mca_btl_mvapi_module_t* mvapi_btl); 


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif
