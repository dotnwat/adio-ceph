
/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2006 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006-2007 Voltaire. All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */
/**
 * @file
 */
#ifndef MCA_BTL_SM_H
#define MCA_BTL_SM_H

#include <stdlib.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif  /* HAVE_SYS_TYPES_H */
#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif  /* HAVE_SYS_SOCKET_H */
#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif  /* HAVE_NETINET_IN_H */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */
#include "opal/class/opal_free_list.h"
#include "opal/class/opal_bitmap.h"
#include "ompi/class/ompi_free_list.h"
#include "opal/event/event.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/btl/btl.h"
#include "ompi/mca/btl/base/base.h"

#include "ompi/mca/mpool/mpool.h"
#include "ompi/mca/common/sm/common_sm_mmap.h"

#include "opal/mca/maffinity/base/base.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/*
 * Shared Memory FIFOs
 */

struct sm_fifo_t {
    /* This queue pointer is used only by the heads. */
    volatile void **queue;           char pad0[CACHE_LINE_SIZE - sizeof(void **)           ];
    /* This lock is used by the heads. */
    opal_atomic_lock_t head_lock;    char pad1[CACHE_LINE_SIZE - sizeof(opal_atomic_lock_t)];
    /* This index is used by the head holding the head lock. */
    volatile int head;               char pad2[CACHE_LINE_SIZE - sizeof(int)               ];
    /* This mask is used "read only" by all processes. */
    unsigned int mask;               char pad3[CACHE_LINE_SIZE - sizeof(int)               ];
    /* The following are used only by the tail. */
    volatile void **queue_recv;
    opal_atomic_lock_t tail_lock;
    volatile int tail;
    int num_to_clear;
    int lazy_free;                   char pad4[CACHE_LINE_SIZE - sizeof(void **)
                                                               - sizeof(opal_atomic_lock_t)
                                                               - sizeof(int) * 3           ];
};
typedef struct sm_fifo_t sm_fifo_t;

/*
 * Shared Memory resource managment
 */

#if OMPI_ENABLE_PROGRESS_THREADS == 1
#define DATA (char)0
#define DONE (char)1
#endif

typedef struct mca_btl_sm_mem_node_t {
    mca_mpool_base_module_t* sm_mpool; /**< shared memory pool */
} mca_btl_sm_mem_node_t;

/**
 * Shared Memory (SM) BTL module.
 */
struct mca_btl_sm_component_t {
    mca_btl_base_component_2_0_0_t super;  /**< base BTL component */
    int sm_free_list_num;              /**< initial size of free lists */
    int sm_free_list_max;              /**< maximum size of free lists */
    int sm_free_list_inc;              /**< number of elements to alloc when growing free lists */
    int32_t sm_max_procs;              /**< upper limit on the number of processes using the shared memory pool */
    int sm_extra_procs;                /**< number of extra procs to allow */
    char* sm_mpool_name;               /**< name of shared memory pool module */
    mca_mpool_base_module_t **sm_mpools; /**< shared memory pools (one for each memory node */
    mca_mpool_base_module_t *sm_mpool; /**< mpool on local node */
    void* sm_mpool_base;               /**< base address of shared memory pool */
    size_t eager_limit;                /**< first fragment size */
    size_t max_frag_size;              /**< maximum (second and beyone) fragment size */
    opal_mutex_t sm_lock;
    mca_common_sm_mmap_t *mmap_file;   /**< description of mmap'ed file */
    mca_common_sm_file_header_t *sm_ctl_header;  /* control header in
                                                    shared memory */
    sm_fifo_t **shm_fifo;              /**< pointer to fifo 2D array in shared memory */
    char **shm_bases;                  /**< pointer to base pointers in shared memory */
    uint16_t *shm_mem_nodes;           /**< pointer to mem noded in shared memory */
    sm_fifo_t **fifo;                  /**< cached copy of the pointer to the 2D
                                          fifo array.  The address in the shared
                                          memory segment sm_ctl_header is a relative,
                                          but this one, in process private memory, is
                                          a real virtual address */
    uint16_t *mem_nodes;               /**< cached copy of mem nodes of each local rank */
    size_t fifo_size;                  /**< number of FIFO queue entries */
    size_t fifo_lazy_free;             /**< number of reads before lazy fifo free is triggered */
    int nfifos;                        /**< number of FIFOs per receiver */
    ptrdiff_t *sm_offset;              /**< offset to be applied to shared memory
                                          addresses, per local process value */
    int32_t num_smp_procs;             /**< current number of smp procs on this host */
    int32_t my_smp_rank;               /**< My SMP process rank.  Used for accessing
                                        *   SMP specfic data structures. */
    ompi_free_list_t sm_frags_eager;   /**< free list of sm first */
    ompi_free_list_t sm_frags_max;     /**< free list of sm second */
    ompi_free_list_t sm_first_frags_to_progress;  /**< list of first
                                                    fragments that are
                                                    awaiting resources */
    struct mca_btl_base_endpoint_t **sm_peers;

    opal_free_list_t pending_send_fl;
    int mem_node;
    int num_mem_nodes;
    
#if OMPI_ENABLE_PROGRESS_THREADS == 1
    char sm_fifo_path[PATH_MAX];   /**< path to fifo used to signal this process */
    int  sm_fifo_fd;               /**< file descriptor corresponding to opened fifo */
    opal_thread_t sm_fifo_thread;
#endif
};
typedef struct mca_btl_sm_component_t mca_btl_sm_component_t;
OMPI_MODULE_DECLSPEC extern mca_btl_sm_component_t mca_btl_sm_component;

struct btl_sm_pending_send_item_t
{
    opal_free_list_item_t super;
    void *data;
};
typedef struct btl_sm_pending_send_item_t btl_sm_pending_send_item_t;

/***
 * FIFO support for sm BTL.
 */

/***
 * One or more FIFO components may be a pointer that must be
 * accessed by multiple processes.  Since the shared region may
 * be mmapped differently into each process's address space,
 * these pointers will be relative to some base address.  Here,
 * we define macros to translate between relative addresses and
 * virtual addresses.
 */
#define VIRTUAL2RELATIVE(VADDR ) ((long)(VADDR)  - (long)mca_btl_sm_component.shm_bases[mca_btl_sm_component.my_smp_rank])
#define RELATIVE2VIRTUAL(OFFSET) ((long)(OFFSET) + (long)mca_btl_sm_component.shm_bases[mca_btl_sm_component.my_smp_rank])

/* ================================================== */
/* ================================================== */
/* ================================================== */

#define SM_FIFO_FREE  (void *) (-2)

static inline int sm_fifo_init(int fifo_size, mca_mpool_base_module_t *mpool,
                               sm_fifo_t *fifo, int lazy_free)
{
    int i, qsize;

    /* figure out the queue size (a power of two that is at least 1) */
    qsize = 1;
    while ( qsize < fifo_size )
        qsize <<= 1;

    /* allocate the queue in the receiver's address space */
    fifo->queue_recv = (volatile void **)mpool->mpool_alloc(
            mpool, sizeof(void *) * qsize, CACHE_LINE_SIZE, 0, NULL);
    if(NULL == fifo->queue_recv) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }

    /* initialize the queue */
    for ( i = 0; i < qsize; i++ )
        fifo->queue_recv[i] = SM_FIFO_FREE;

    /* shift queue address to be relative */
    fifo->queue = (volatile void **) VIRTUAL2RELATIVE(fifo->queue_recv);

    /* initialize the locks */
    opal_atomic_init(&(fifo->head_lock), OPAL_ATOMIC_UNLOCKED);
    opal_atomic_init(&(fifo->tail_lock), OPAL_ATOMIC_UNLOCKED);
    opal_atomic_unlock(&(fifo->head_lock));  /* should be unnecessary */
    opal_atomic_unlock(&(fifo->tail_lock));  /* should be unnecessary */

    /* other initializations */
    fifo->head = 0;
    fifo->mask = qsize - 1;
    fifo->tail = 0;
    fifo->num_to_clear = 0;
    fifo->lazy_free = lazy_free;

    return OMPI_SUCCESS;
}


static inline int sm_fifo_write(void *value, sm_fifo_t *fifo)
{
    volatile void **q = (volatile void **) RELATIVE2VIRTUAL(fifo->queue);

    /* if there is no free slot to write, report exhausted resource */
    if ( SM_FIFO_FREE != q[fifo->head] )
        return OMPI_ERR_OUT_OF_RESOURCE;

    /* otherwise, write to the slot and advance the head index */
    opal_atomic_rmb();
    q[fifo->head] = value;
    fifo->head = (fifo->head + 1) & fifo->mask;
    opal_atomic_wmb(); 
    return OMPI_SUCCESS;
}


static inline void *sm_fifo_read(sm_fifo_t *fifo)
{
    void *value;

    /* read the next queue entry */
    value = (void *) fifo->queue_recv[fifo->tail];

    opal_atomic_rmb();

    /* if you read a non-empty slot, advance the tail pointer */
    if ( SM_FIFO_FREE != value ) {

        fifo->tail = ( fifo->tail + 1 ) & fifo->mask;
        fifo->num_to_clear += 1;

        /* check if it's time to free slots, which we do lazily */
        if ( fifo->num_to_clear >= fifo->lazy_free ) {
            int i = (fifo->tail - fifo->num_to_clear ) & fifo->mask;

            while ( fifo->num_to_clear > 0 ) {
                fifo->queue_recv[i] = SM_FIFO_FREE;
                i = (i+1) & fifo->mask;
                fifo->num_to_clear -= 1;
            }
            opal_atomic_wmb();
        }
    }

    return value;
}

/**
 * Register shared memory module parameters with the MCA framework
 */
extern int mca_btl_sm_component_open(void);

/**
 * Any final cleanup before being unloaded.
 */
extern int mca_btl_sm_component_close(void);

/**
 * SM module initialization.
 *
 * @param num_btls (OUT)                  Number of BTLs returned in BTL array.
 * @param enable_progress_threads (IN)    Flag indicating whether BTL is allowed to have progress threads
 * @param enable_mpi_threads (IN)         Flag indicating whether BTL must support multilple simultaneous invocations from different threads
 *
 */
extern mca_btl_base_module_t** mca_btl_sm_component_init(
    int *num_btls,
    bool enable_progress_threads,
    bool enable_mpi_threads
);

/**
 * shared memory component progress.
 */
extern int mca_btl_sm_component_progress(void);

/**
 * SM BTL Interface
 */
struct mca_btl_sm_t {
    mca_btl_base_module_t  super;       /**< base BTL interface */
    bool btl_inited;  /**< flag indicating if btl has been inited */
    mca_btl_base_module_error_cb_fn_t error_cb;
};
typedef struct mca_btl_sm_t mca_btl_sm_t;

extern mca_btl_sm_t mca_btl_sm;

/**
 * Register a callback function that is called on error..
 *
 * @param btl (IN)     BTL module
 * @return             Status indicating if cleanup was successful
 */

int mca_btl_sm_register_error_cb(
    struct mca_btl_base_module_t* btl,
    mca_btl_base_module_error_cb_fn_t cbfunc
);

/**
 * Cleanup any resources held by the BTL.
 *
 * @param btl  BTL instance.
 * @return     OMPI_SUCCESS or error status on failure.
 */

extern int mca_btl_sm_finalize(
    struct mca_btl_base_module_t* btl
);


/**
 * PML->BTL notification of change in the process list.
 * PML->BTL Notification that a receive fragment has been matched.
 * Called for message that is send from process with the virtual
 * address of the shared memory segment being different than that of
 * the receiver.
 *
 * @param btl (IN)
 * @param proc (IN)
 * @param peer (OUT)
 * @return     OMPI_SUCCESS or error status on failure.
 *
 */

extern int mca_btl_sm_add_procs(
    struct mca_btl_base_module_t* btl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_btl_base_endpoint_t** peers,
    struct opal_bitmap_t* reachability
);


/**
 * PML->BTL notification of change in the process list.
 *
 * @param btl (IN)     BTL instance
 * @param proc (IN)    Peer process
 * @param peer (IN)    Peer addressing information.
 * @return             Status indicating if cleanup was successful
 *
 */
extern int mca_btl_sm_del_procs(
    struct mca_btl_base_module_t* btl,
    size_t nprocs,
    struct ompi_proc_t **procs,
    struct mca_btl_base_endpoint_t **peers
);


/**
 * Allocate a segment.
 *
 * @param btl (IN)      BTL module
 * @param size (IN)     Request segment size.
 */
extern mca_btl_base_descriptor_t* mca_btl_sm_alloc(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    uint8_t order,
    size_t size,
    uint32_t flags
);

/**
 * Return a segment allocated by this BTL.
 *
 * @param btl (IN)      BTL module
 * @param segment (IN)  Allocated segment.
 */
extern int mca_btl_sm_free(
    struct mca_btl_base_module_t* btl,
    mca_btl_base_descriptor_t* segment
);


/**
 * Pack data
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
struct mca_btl_base_descriptor_t* mca_btl_sm_prepare_src(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    mca_mpool_base_registration_t* registration,
    struct ompi_convertor_t* convertor,
    uint8_t order,
    size_t reserve,
    size_t* size,
    uint32_t flags
);


/**
 * Initiate an inlined send to the peer or return a descriptor.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
extern int mca_btl_sm_sendi( struct mca_btl_base_module_t* btl,
                             struct mca_btl_base_endpoint_t* endpoint,
                             struct ompi_convertor_t* convertor,
                             void* header,
                             size_t header_size,
                             size_t payload_size,
                             uint8_t order,
                             uint32_t flags,
                             mca_btl_base_tag_t tag,
                             mca_btl_base_descriptor_t** descriptor );

/**
 * Initiate a send to the peer.
 *
 * @param btl (IN)      BTL module
 * @param peer (IN)     BTL peer addressing
 */
extern int mca_btl_sm_send(
    struct mca_btl_base_module_t* btl,
    struct mca_btl_base_endpoint_t* endpoint,
    struct mca_btl_base_descriptor_t* descriptor,
    mca_btl_base_tag_t tag
);

/**
 * Fault Tolerance Event Notification Function
 * @param state Checkpoint Stae
 * @return OMPI_SUCCESS or failure status
 */
int mca_btl_sm_ft_event(int state);

#if OMPI_ENABLE_PROGRESS_THREADS == 1
void mca_btl_sm_component_event_thread(opal_object_t*);
#endif

#if OMPI_ENABLE_PROGRESS_THREADS == 1
#define MCA_BTL_SM_SIGNAL_PEER(peer) \
{ \
    unsigned char cmd = DATA; \
    if(write(peer->fifo_fd, &cmd, sizeof(cmd)) != sizeof(cmd)) { \
        opal_output(0, "mca_btl_sm_send: write fifo failed: errno=%d\n", errno); \
    } \
}
#else
#define MCA_BTL_SM_SIGNAL_PEER(peer)
#endif

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif

