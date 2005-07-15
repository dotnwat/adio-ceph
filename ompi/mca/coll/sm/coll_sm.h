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

#ifndef MCA_COLL_SM_EXPORT_H
#define MCA_COLL_SM_EXPORT_H

#include "ompi_config.h"

#include "mpi.h"
#include "opal/mca/mca.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/mpool/mpool.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

#define PUB(foo) mca_coll_sm##foo

    /** 
     * Structure to hold the sm coll component.  First it holds the
     * base coll component, and then holds a bunch of
     * sm-coll-component-specific stuff (e.g., current MCA param
     * values). 
     */
    typedef struct mca_coll_sm_component_t {
        /** Base coll component */
        mca_coll_base_component_1_0_0_t super;

        /** Priority of this component */
        int sm_priority;
        /** Name of the mpool that this component will look for */
        char *sm_mpool_name;
        /** Mpool that will be used */
        mca_mpool_base_module_t *sm_mpool;
    } mca_coll_sm_component_t;

    /** 
     * Structure for sm collective module, per communicator. The
     * structure mainly stores memory pointers to the specific
     * poritions in the shared memory area. Each shared memory area is
     * reserved for special functions.  The shared memory is split
     * between two types of areas. One is control section that stores
     * shared flags used during synchronization, while other section
     * is purely used to pass messages from one process to other. 
     */
    typedef struct mca_coll_base_module_comm_t {

        /* JMS fill in here */
        int foo;

    } mca_coll_base_module_comm_t;

    
    /**
     * Global component instance
     */
    extern mca_coll_sm_component_t mca_coll_sm_component;


    /*
     * coll module functions
     */

    int mca_coll_sm_init_query(bool enable_progress_threads,
                               bool enable_mpi_threads);

    const struct mca_coll_base_module_1_0_0_t *
    mca_coll_sm_comm_query(struct ompi_communicator_t *comm, int *priority,
                           struct mca_coll_base_comm_t **data);

    int mca_coll_sm_comm_unquery(struct ompi_communicator_t *comm,
                                 struct mca_coll_base_comm_t *data);

    const struct mca_coll_base_module_1_0_0_t *
    mca_coll_sm_module_init(struct ompi_communicator_t *comm);

    int mca_coll_sm_module_finalize(struct ompi_communicator_t *comm);

    int mca_coll_sm_allgather_intra(void *sbuf, int scount, 
                                    struct ompi_datatype_t *sdtype, 
                                    void *rbuf, int rcount, 
                                    struct ompi_datatype_t *rdtype, 
                                    struct ompi_communicator_t *comm);
    int mca_coll_sm_allgatherv_intra(void *sbuf, int scount, 
                                     struct ompi_datatype_t *sdtype, 
                                     void * rbuf, int *rcounts, int *disps, 
                                     struct ompi_datatype_t *rdtype, 
                                     struct ompi_communicator_t *comm);
    int mca_coll_sm_allreduce_intra(void *sbuf, void *rbuf, int count, 
                                    struct ompi_datatype_t *dtype, 
                                    struct ompi_op_t *op, 
                                    struct ompi_communicator_t *comm);
    int mca_coll_sm_alltoall_intra(void *sbuf, int scount, 
                                   struct ompi_datatype_t *sdtype, 
                                   void* rbuf, int rcount, 
                                   struct ompi_datatype_t *rdtype, 
                                   struct ompi_communicator_t *comm);
    int mca_coll_sm_alltoallv_intra(void *sbuf, int *scounts, int *sdisps, 
                                    struct ompi_datatype_t *sdtype, 
                                    void *rbuf, int *rcounts, int *rdisps, 
                                    struct ompi_datatype_t *rdtype, 
                                    struct ompi_communicator_t *comm);
    int mca_coll_sm_alltoallw_intra(void *sbuf, int *scounts, int *sdisps, 
                                    struct ompi_datatype_t **sdtypes, 
                                    void *rbuf, int *rcounts, int *rdisps, 
                                    struct ompi_datatype_t **rdtypes, 
                                    struct ompi_communicator_t *comm);
    int mca_coll_sm_barrier_intra(struct ompi_communicator_t *comm);
    int mca_coll_sm_bcast_intra(void *buff, int count, 
                                struct ompi_datatype_t *datatype,
                                int root, 
                                struct ompi_communicator_t *comm);
    int mca_coll_sm_bcast_log_intra(void *buff, int count, 
                                    struct ompi_datatype_t *datatype, 
                                    int root, 
                                    struct ompi_communicator_t *comm);
    int mca_coll_sm_exscan_intra(void *sbuf, void *rbuf, int count, 
                                 struct ompi_datatype_t *dtype, 
                                 struct ompi_op_t *op, 
                                 struct ompi_communicator_t *comm);
    int mca_coll_sm_gather_intra(void *sbuf, int scount, 
                                 struct ompi_datatype_t *sdtype, void *rbuf, 
                                 int rcount, struct ompi_datatype_t *rdtype, 
                                 int root, struct ompi_communicator_t *comm);
    int mca_coll_sm_gatherv_intra(void *sbuf, int scount, 
                                  struct ompi_datatype_t *sdtype, void *rbuf, 
                                  int *rcounts, int *disps, 
                                  struct ompi_datatype_t *rdtype, int root, 
                                  struct ompi_communicator_t *comm);
    int mca_coll_sm_reduce_intra(void *sbuf, void* rbuf, int count, 
                                 struct ompi_datatype_t *dtype, 
                                 struct ompi_op_t *op, 
                                 int root,
                                 struct ompi_communicator_t *comm);
    int mca_coll_sm_reduce_log_intra(void *sbuf, void* rbuf, int count, 
                                     struct ompi_datatype_t *dtype, 
                                     struct ompi_op_t *op, 
                                     int root, 
                                     struct ompi_communicator_t *comm);
    int mca_coll_sm_reduce_scatter_intra(void *sbuf, void *rbuf, 
                                         int *rcounts, 
                                         struct ompi_datatype_t *dtype, 
                                         struct ompi_op_t *op, 
                                         struct ompi_communicator_t *comm);
    int mca_coll_sm_scan_intra(void *sbuf, void *rbuf, int count, 
                               struct ompi_datatype_t *dtype, 
                               struct ompi_op_t *op, 
                               struct ompi_communicator_t *comm);
    int mca_coll_sm_scatter_intra(void *sbuf, int scount, 
                                  struct ompi_datatype_t *sdtype, void *rbuf, 
                                  int rcount, struct ompi_datatype_t *rdtype, 
                                  int root, struct ompi_communicator_t *comm);
    int mca_coll_sm_scatterv_intra(void *sbuf, int *scounts, int *disps, 
                                   struct ompi_datatype_t *sdtype, 
                                   void* rbuf, int rcount, 
                                   struct ompi_datatype_t *rdtype, int root, 
                                   struct ompi_communicator_t *comm);
    
#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* MCA_COLL_SM_EXPORT_H */
