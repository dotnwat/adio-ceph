/*
 * $HEADER$
 */
/**
 * @file
 */
#ifndef MCA_PTL_TCP_H
#define MCA_PTL_TCP_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "class/ompi_free_list.h"
#include "event/event.h"
#include "mca/pml/pml.h"
#include "mca/ptl/ptl.h"

#define MCA_PTL_TCP_STATISTICS 0

/**
 * TCP PTL module.
 */
struct mca_ptl_tcp_module_1_0_0_t {
    mca_ptl_base_module_1_0_0_t super;    /**< base PTL module */
    struct mca_ptl_tcp_t** tcp_ptls;      /**< array of available PTLs */
    size_t tcp_num_ptls;                  /**< number of ptls actually used */
    size_t tcp_max_ptls;                  /**< maximum number of ptls - available kernel ifs */
    int tcp_listen_sd;                    /**< listen socket for incoming connection requests */
    unsigned short tcp_listen_port;       /**< listen port */
    char* tcp_if_include;                 /**< comma seperated list of interface to include */
    char* tcp_if_exclude;                 /**< comma seperated list of interface to exclude */
    int   tcp_free_list_num;              /**< initial size of free lists */
    int   tcp_free_list_max;              /**< maximum size of free lists */
    int   tcp_free_list_inc;              /**< number of elements to alloc when growing free lists */
    int   tcp_sndbuf;                     /**< socket sndbuf size */
    int   tcp_rcvbuf;                     /**< socket rcvbuf size */
    ompi_free_list_t tcp_send_requests;    /**< free list of tcp send requests -- sendreq + sendfrag */
    ompi_free_list_t tcp_send_frags;       /**< free list of tcp send fragments */
    ompi_free_list_t tcp_recv_frags;       /**< free list of tcp recv fragments */
    ompi_list_t tcp_procs;                 /**< list of tcp proc structures */
    ompi_list_t tcp_pending_acks;          /**< list of pending acks - retry as sends complete */
    struct mca_ptl_tcp_proc_t* tcp_local; /**< the tcp proc instance corresponding to the local process */
    ompi_event_t tcp_send_event;           /**< event structure for sends */
    ompi_event_t tcp_recv_event;           /**< event structure for recvs */
    ompi_mutex_t tcp_lock;                 /**< lock for accessing module state */
};
typedef struct mca_ptl_tcp_module_1_0_0_t mca_ptl_tcp_module_1_0_0_t;
typedef struct mca_ptl_tcp_module_1_0_0_t mca_ptl_tcp_module_t;
struct mca_ptl_tcp_recv_frag_t;
struct mca_ptl_tcp_send_frag_t;

extern mca_ptl_tcp_module_1_0_0_t mca_ptl_tcp_module;

/**
 * Register TCP module parameters with the MCA framework
 */
extern int mca_ptl_tcp_module_open(void);

/**
 * Any final cleanup before being unloaded.
 */
extern int mca_ptl_tcp_module_close(void);

/**
 * TCP module initialization.
 * 
 * @param num_ptls (OUT)                  Number of PTLs returned in PTL array.
 * @param allow_multi_user_threads (OUT)  Flag indicating wether PTL supports user threads (TRUE)
 * @param have_hidden_threads (OUT)       Flag indicating wether PTL uses threads (TRUE)
 *
 *  (1) read interface list from kernel and compare against module parameters
 *      then create a PTL instance for selected interfaces
 *  (2) setup TCP listen socket for incoming connection attempts
 *  (3) publish PTL addressing info 
 *
 */
extern mca_ptl_t** mca_ptl_tcp_module_init(
    int *num_ptls, 
    bool *allow_multi_user_threads,
    bool *have_hidden_threads
);

/**
 * TCP module control.
 */
extern int mca_ptl_tcp_module_control(
    int param,
    void* value,
    size_t size
);

/**
 * TCP module progress.
 */
extern int mca_ptl_tcp_module_progress(
   mca_ptl_tstamp_t tstamp
);

/**
 * TCP PTL Interface
 */
struct mca_ptl_tcp_t {
    mca_ptl_t          super;       /**< base PTL interface */
    int                ptl_ifindex; /**< PTL interface index */
    struct sockaddr_in ptl_ifaddr;  /**< PTL interface address */
    struct sockaddr_in ptl_ifmask;  /**< PTL interface netmask */
#if MCA_PTL_TCP_STATISTICS
    size_t ptl_bytes_sent;
    size_t ptl_bytes_recv;
    size_t ptl_send_handler;
#endif
};
typedef struct mca_ptl_tcp_t mca_ptl_tcp_t;

extern mca_ptl_tcp_t mca_ptl_tcp;


/**
 * Cleanup any resources held by the PTL.
 * 
 * @param ptl  PTL instance.
 * @return     OMPI_SUCCESS or error status on failure.
 */

extern int mca_ptl_tcp_finalize(
    struct mca_ptl_t* ptl
);


/**
 * PML->PTL notification of change in the process list.
 * 
 * @param ptl (IN)
 * @param nprocs (IN)     Number of processes
 * @param procs (IN)      Set of processes
 * @param peers (OUT)     Set of (optional) peer addressing info.
 * @param peers (IN/OUT)  Set of processes that are reachable via this PTL.
 * @return     OMPI_SUCCESS or error status on failure.
 * 
 */

extern int mca_ptl_tcp_add_procs(
    struct mca_ptl_t* ptl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_ptl_base_peer_t** peers,
    ompi_bitmap_t* reachable
);

                                                                                                               
/**
 * PML->PTL notification of change in the process list.
 *
 * @param ptl (IN)     PTL instance
 * @param nproc (IN)   Number of processes.
 * @param procs (IN)   Set of processes.
 * @param peers (IN)   Set of peer data structures.
 * @return             Status indicating if cleanup was successful
 *
 */
extern int mca_ptl_tcp_del_procs(
    struct mca_ptl_t* ptl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_ptl_base_peer_t** peers
);

/**
 * PML->PTL Allocate a send request from the PTL modules free list.
 *
 * @param ptl (IN)       PTL instance
 * @param request (OUT)  Pointer to allocated request.
 * @return               Status indicating if allocation was successful.
 *
 */
extern int mca_ptl_tcp_request_alloc(
    struct mca_ptl_t* ptl,
    struct mca_pml_base_send_request_t**
);

/**
 * PML->PTL Return a send request to the PTL modules free list.
 *
 * @param ptl (IN)       PTL instance
 * @param request (IN)   Pointer to allocated request.
 *
 */
extern void mca_ptl_tcp_request_return(
    struct mca_ptl_t* ptl,
    struct mca_pml_base_send_request_t*
);

/**
 * PML->PTL Notification that a receive fragment has been matched.
 *
 * @param ptl (IN)          PTL instance
 * @param recv_frag (IN)    Receive fragment
 *
 */
extern void mca_ptl_tcp_matched(
    struct mca_ptl_t* ptl,
    struct mca_ptl_base_recv_frag_t* frag
);
                                                                                                 
/**
 * PML->PTL Initiate a send of the specified size.
 *
 * @param ptl (IN)               PTL instance
 * @param ptl_base_peer (IN)     PTL peer addressing
 * @param send_request (IN/OUT)  Send request (allocated by PML via mca_ptl_base_request_alloc_fn_t)
 * @param size (IN)              Number of bytes PML is requesting PTL to deliver
 * @param flags (IN)             Flags that should be passed to the peer via the message header.
 * @param request (OUT)          OMPI_SUCCESS if the PTL was able to queue one or more fragments
 */
extern int mca_ptl_tcp_send(
    struct mca_ptl_t* ptl,
    struct mca_ptl_base_peer_t* ptl_peer,
    struct mca_pml_base_send_request_t*,
    size_t offset,
    size_t size,
    int flags
);

/**
 * Return a recv fragment to the modules free list.
 *
 * @param ptl (IN)   PTL instance
 * @param frag (IN)  TCP receive fragment
 *
 */
extern void mca_ptl_tcp_recv_frag_return(
    struct mca_ptl_t* ptl,
    struct mca_ptl_tcp_recv_frag_t* frag
);



/**
 * Return a send fragment to the modules free list.
 *
 * @param ptl (IN)   PTL instance
 * @param frag (IN)  TCP send fragment
 *
 */
extern void mca_ptl_tcp_send_frag_return(
    struct mca_ptl_t* ptl,
    struct mca_ptl_tcp_send_frag_t*
);

#endif

