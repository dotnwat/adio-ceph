/*
 * $HEADER$
 */
/*
 * P2P Transport Layer (PTL)
 */
#ifndef MCA_PTL_H
#define MCA_PTL_H

#include "mca/mca.h"
#include "lam/lam.h"
#include "lam/lfc/lam_list.h"
#include "mpi/proc/proc.h"
#include "mca/mpi/pml/pml.h"


/*
 * PTL types
 */

struct mca_ptl_t;
struct mca_ptl_base_peer_t;
struct mca_ptl_base_fragment_t;
struct mca_ptl_base_recv_request_t;
struct mca_ptl_base_send_request_t;
struct mca_ptl_base_recv_frag_t;
struct mca_ptl_base_send_frag_t;
struct mca_ptl_base_match_header_t;

typedef uint64_t mca_ptl_base_sequence_t;
typedef uint64_t mca_ptl_base_tstamp_t;
typedef lam_list_t mca_ptl_base_queue_t;
                                                                                                            
/*
 *  PTL module interface functions and datatype.
 */

/**
 *  MCA->PTL Intializes the PTL module and creates specific PTL instance(s).
 *
 *  @param num_ptls (OUT) Returns the number of ptl instances created.
 *
 * @param allow_multi_user_threads (OUT) Whether this module can run
 * at MPI_THREAD_MULTIPLE or not.
 *
 * @param have_hidden_threads (OUT) Whether this module may use
 * hidden threads (e.g., progress threads) or not.
 *
 *  @return Array of pointers to PTL instances.
 */
typedef struct mca_ptl_t** (*mca_ptl_base_module_init_fn_t)(
    int *num_ptls, 
    bool *allow_multi_user_threads,
    bool *have_hidden_threads
);

                                                                                                            
/**
 *  PML->PTL Progresses outstanding requests in each PTL module.
 *
 *  @param timstamp (IN)     The current time - used for retransmisstion timers.
 */
typedef void (*mca_ptl_base_module_progress_fn_t)(
    mca_ptl_base_tstamp_t timestamp
);

/**
 *  PTL module version and interface functions.
 */

struct mca_ptl_base_module_1_0_0_t {
  mca_base_module_t ptlm_version;
  mca_base_module_data_1_0_0_t ptlm_data;
  mca_ptl_base_module_init_fn_t ptlm_init;
  mca_ptl_base_module_progress_fn_t ptlm_progress;
};
typedef struct mca_ptl_base_module_1_0_0_t mca_ptl_base_module_1_0_0_t;
typedef struct mca_ptl_base_module_1_0_0_t mca_ptl_base_module_t;


/*
 *  PTL instance interface functions and datatype.
 */

/**
 *  PML->PTL notification of change in the process list.
 *
 *  @param ptl (IN)     
 *  @param procs (IN)   
 *  @param nprocs (IN)  
 *  @return            
 */
typedef int (*mca_ptl_base_add_proc_fn_t)(
    struct mca_ptl_t* ptl, 
    struct lam_proc_t* proc, 
    struct mca_ptl_base_peer_t**
);

/**
 *  PML->PTL notification of change in the process list.
 *
 *  @param ptl (IN)     
 *  @param procs (IN)   
 *  @param nprocs (IN)  
 *  @return            
 */
typedef int (*mca_ptl_base_del_proc_fn_t)(
    struct mca_ptl_t* ptl, 
    struct lam_proc_t* proc, 
    struct mca_ptl_base_peer_t*
);

/**
 *  MCA->PTL Clean up any resources held by PTL instance before the module is unloaded.
 *  
 *  @param ptl (IN)   The PTL module instance that is being unloaded.
 */
typedef int (*mca_ptl_base_finalize_fn_t)(
    struct mca_ptl_t* ptl
);
                                                                                                         
typedef int (*mca_ptl_base_request_alloc_fn_t)(
    struct mca_ptl_t* ptl, 
    struct mca_ptl_base_send_request_t** request
);

typedef void (*mca_ptl_base_request_return_fn_t)(
    struct mca_ptl_t* ptl, 
    struct mca_ptl_base_send_request_t* request
);

typedef void (*mca_ptl_base_frag_return_fn_t)(
    struct mca_ptl_t* ptl, 
    struct mca_ptl_base_recv_frag_t* frag
);

typedef int (*mca_ptl_base_send_fn_t)(
    struct mca_ptl_t* ptl, 
    struct mca_ptl_base_peer_t* ptl_base_peer, 
    struct mca_ptl_base_send_request_t* send_request,
    size_t size,
    int flags
);

typedef void (*mca_ptl_base_recv_fn_t)(
    struct mca_ptl_t* ptl, 
    struct mca_ptl_base_recv_frag_t* recv_frag,
    struct lam_status_public_t* recv_status
);

typedef int (*mca_ptl_base_match_fn_t)(
    struct mca_ptl_base_recv_frag_t* frag,
    struct mca_ptl_base_match_header_t* header
);

typedef void (*mca_ptl_base_recv_progress_fn_t)(
    struct mca_ptl_base_recv_request_t* recv_request,
    struct mca_ptl_base_recv_frag_t* recv_frag
);

typedef void (*mca_ptl_base_send_progress_fn_t)(
    struct mca_ptl_base_send_request_t* send_request,
    struct mca_ptl_base_send_frag_t* send_frag
);

/**
 * PTL instance interface functions and common state.
 */

struct mca_ptl_t {

    /* PTL common attributes */
    mca_ptl_base_module_t* ptl_module;
    size_t      ptl_first_frag_size;   /**< maximum size of first fragment */
    size_t      ptl_min_frag_size;     /**< threshold below which the PTL will not fragment */
    size_t      ptl_max_frag_size;     /**< maximum fragment size supported by the PTL */
    uint32_t    ptl_exclusivity;       /**< indicates this PTL should be used exclusively */
    uint32_t    ptl_latency;           /**< relative ranking of latency used to prioritize ptls */
    uint32_t    ptl_bandwidth;         /**< bandwidth (Mbytes/sec) supported by each endpoint */

    /* PML->PTL function table */
    mca_ptl_base_add_proc_fn_t         ptl_add_proc;
    mca_ptl_base_del_proc_fn_t         ptl_del_proc;
    mca_ptl_base_finalize_fn_t         ptl_finalize;
    mca_ptl_base_send_fn_t             ptl_send;
    mca_ptl_base_recv_fn_t             ptl_recv;
    mca_ptl_base_request_alloc_fn_t    ptl_request_alloc;
    mca_ptl_base_request_return_fn_t   ptl_request_return;
    mca_ptl_base_frag_return_fn_t      ptl_frag_return;

    /* PTL->PML function table - filled in by PML at init */
    mca_ptl_base_match_fn_t            ptl_match;
    mca_ptl_base_send_progress_fn_t    ptl_send_progress;
    mca_ptl_base_recv_progress_fn_t    ptl_recv_progress;
};
typedef struct mca_ptl_t mca_ptl_t;

/*
 * Macro for use in modules that are of type ptl v1.0.0
 */
#define MCA_PTL_BASE_VERSION_1_0_0 \
  /* coll v1.0 is chained to MCA v1.0 */ \
  MCA_BASE_VERSION_1_0_0, \
  /* ptl v1.0 */ \
  "ptl", 1, 0, 0

#endif /* LAM_MCA_PTL_H */
