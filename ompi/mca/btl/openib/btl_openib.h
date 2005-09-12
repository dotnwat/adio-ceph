
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

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define MCA_BTL_IB_LEAVE_PINNED 1

/**
 * Infiniband (IB) BTL component.
 */

struct mca_btl_openib_component_t {
    mca_btl_base_component_1_0_0_t          super;  /**< base BTL component */ 
    
    uint32_t                                ib_num_btls;
    /**< number of hcas available to the IB component */

    struct mca_btl_openib_module_t             *openib_btls;
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
    
    size_t eager_limit; 
    size_t max_send_size; 
    uint32_t leave_pinned; 
    uint32_t reg_mru_len; 

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

    
}; typedef struct mca_btl_openib_component_t mca_btl_openib_component_t;

extern mca_btl_openib_component_t mca_btl_openib_component;

typedef mca_btl_base_recv_reg_t mca_btl_openib_recv_reg_t; 
    


/**
 * IB PTL Interface
 */
struct mca_btl_openib_module_t {
    mca_btl_base_module_t  super;  /**< base PTL interface */
    bool btl_inited; 
    mca_btl_openib_recv_reg_t ib_reg[256]; 
        
    uint8_t port_num;           /**< ID of the PORT */ 
    struct ibv_device *ib_dev;  /* the ib device */ 
    struct ibv_context *ib_dev_context; 
    struct ibv_pd *ib_pd; 
    struct ibv_cq *ib_cq_high;
    struct ibv_cq *ib_cq_low; 
    struct ibv_port_attr* ib_port_attr; 
    struct ibv_recv_wr* rr_desc_post;

    
    ompi_free_list_t send_free_eager;    /**< free list of eager buffer descriptors */
    ompi_free_list_t send_free_max; /**< free list of max buffer descriptors */ 
    ompi_free_list_t send_free_frag; /**< free list of frags only... used for pining memory */ 
    
    ompi_free_list_t recv_free_eager;    /**< High priority free list of buffer descriptors */
    ompi_free_list_t recv_free_max;      /**< Low priority free list of buffer descriptors */ 

    opal_list_t reg_mru_list;   /**< a most recently used list of mca_mpool_openib_registration_t 
                                       entries, this allows us to keep a working set of memory pinned */ 
    
    opal_list_t repost;            /**< list of buffers to repost */
    opal_mutex_t ib_lock;          /**< module level lock */ 
    
    
    /**< an array to allow posting of rr in one swoop */ 
    size_t ib_inline_max; /**< max size of inline send*/ 
    bool poll_cq; 
    
    
    
}; typedef struct mca_btl_openib_module_t mca_btl_openib_module_t;
    

struct mca_btl_openib_frag_t; 
extern mca_btl_openib_module_t mca_btl_openib_module;

/**
 * Register IB component parameters with the MCA framework
 */
extern int mca_btl_openib_component_open(void);

/**
 * Any final cleanup before being unloaded.
 */
extern int mca_btl_openib_component_close(void);

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
extern mca_btl_base_module_t** mca_btl_openib_component_init(
    int *num_btl_modules, 
    bool allow_multi_user_threads,
    bool have_hidden_threads
);


/**
 * IB component progress.
 */
extern int mca_btl_openib_component_progress(
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

int mca_btl_openib_register(
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

extern int mca_btl_openib_finalize(
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

extern int mca_btl_openib_add_procs(
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
extern int mca_btl_openib_del_procs(
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
extern int mca_btl_openib_send(
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
extern int mca_btl_openib_put(
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
extern int mca_btl_openib_get(
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
extern mca_btl_base_descriptor_t* mca_btl_openib_alloc(
                                                       struct mca_btl_base_module_t* btl, 
                                                       size_t size); 
    

/**
 * Return a segment allocated by this BTL.
 *
 * @param btl (IN)         BTL module
 * @param descriptor (IN)  Allocated descriptor.
 */
extern int mca_btl_openib_free(
                               struct mca_btl_base_module_t* btl, 
                               mca_btl_base_descriptor_t* des); 
    

/**
 * Pack data and return a descriptor that can be
 * used for send/put.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
mca_btl_base_descriptor_t* mca_btl_openib_prepare_src(
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
extern mca_btl_base_descriptor_t* mca_btl_openib_prepare_dst( 
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
extern void mca_btl_openib_send_frag_return(
                                            struct mca_btl_base_module_t* btl,
                                            struct mca_btl_openib_frag_t*
                                            );


int mca_btl_openib_module_init(mca_btl_openib_module_t* openib_btl); 


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif
