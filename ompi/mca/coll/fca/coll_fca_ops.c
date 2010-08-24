/**
  Copyright (c) 2010 Voltaire, Inc. All rights reserved.
  $COPYRIGHT$

  Additional copyrights may follow

  $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"
#include "coll_fca.h"


/**
 * Returns the index of the rank 'ran' in the local ranks group, or -1 if not exists.
 */
static inline int __find_local_rank(mca_coll_fca_module_t *fca_module, int rank)
{
    int i;

    for (i = 0; i < fca_module->num_local_procs; ++i) {
        if (rank == fca_module->local_ranks[i])
            return i;
    }
    return -1;
}

static mca_coll_fca_dtype_info_t* mca_coll_fca_get_dtype(ompi_datatype_t *dtype)
{
    mca_coll_fca_dtype_info_t *dtype_info;
    ptrdiff_t lb, extent;
    int id = dtype->id;
    int fca_dtype;

    if (id < 0 || id >= FCA_DT_MAX_PREDEFINED)
        return NULL;

    dtype_info = &mca_coll_fca_component.fca_dtypes[id];
    if (dtype_info->mpi_dtype == dtype)
        return dtype_info;

    /* assert we don't overwrite another datatype */
    assert(dtype_info->mpi_dtype == MPI_DATATYPE_NULL);
    fca_dtype = mca_coll_fca_component.fca_ops.translate_mpi_dtype(dtype->name);
    if (fca_dtype < 0)
        return NULL;

    FCA_DT_GET_TRUE_EXTENT(dtype, &lb, &extent);
    dtype_info->mpi_dtype = dtype;
    dtype_info->mpi_dtype_extent = extent;
    dtype_info->fca_dtype = fca_dtype;
    dtype_info->fca_dtype_extent = mca_coll_fca_component.fca_ops.get_dtype_size(fca_dtype);
    FCA_VERBOSE(2, "Added new dtype[%d]: %s fca id: %d, mpi size: %d, fca size: %d",
                id, dtype->name, dtype_info->fca_dtype, dtype_info->mpi_dtype_extent,
                dtype_info->fca_dtype_extent);
    return dtype_info;
}

static mca_coll_fca_op_info_t *mca_coll_fca_get_op(ompi_op_t *op)
{
    mca_coll_fca_op_info_t *op_info;
    int i, fca_op;
    //char opname[MPI_MAX_OBJECT_NAME + 1];

    /*
     * Find 'op' in the array by exhaustive search. We assume all valid ops are
     * in the beginning. If we stumble on a MPI_OP_NULL, we try to resolve the FCA
     * operation code and store it in the array.
     */
    op_info = mca_coll_fca_component.fca_reduce_ops;
    for (i = 0; i < FCA_MAX_OPS; ++i, ++op_info) {
        if (op_info->mpi_op == op) {
            return op_info;
        } else if (op_info->mpi_op == MPI_OP_NULL) {
            //mca_coll_fca_get_op_name(op, opname, MPI_MAX_OBJECT_NAME);
            fca_op = mca_coll_fca_component.fca_ops.translate_mpi_op(op->o_name);
            if (fca_op < 0)
                return NULL;
            op_info->mpi_op = op;
            op_info->fca_op = fca_op;
            FCA_VERBOSE(2, "Added new op[%d]: %s fca id: %d", i, op->o_name, fca_op);
            return op_info;
        }
    }
    /* assert the array does not overflow */
    //assert(mca_coll_fca_component.fca_reduce_ops[FCA_MAX_OPS - 1] == MPI_OP_NULL);
    return NULL;
}

static int mca_coll_fca_get_buf_size(ompi_datatype_t *dtype, int count)
{
    ptrdiff_t true_lb, true_extent;

    FCA_DT_GET_TRUE_EXTENT(dtype, &true_lb, &true_extent);

    /* If the datatype is the same packed as it is unpacked, we
      can save a memory copy and just do the reduction operation
      directly.  However, if the representation is not the same, then we need to get a
      receive convertor and a temporary buffer to receive into. */
   if (!FCA_DT_IS_CONTIGUOUS_MEMORY_LAYOUT(dtype, count)) {
       FCA_VERBOSE(5, "Unsupported datatype layout, only contiguous is supported now");
       return OMPI_ERROR;
   }

   // TODO add support for non-contiguous layout
   return true_extent * count;
}

static int mca_coll_fca_fill_reduce_spec(int count, ompi_datatype_t *dtype,
                                         ompi_op_t *op, fca_reduce_spec_t *spec,
                                         int max_fca_payload)
{
    mca_coll_fca_dtype_info_t *dtype_info;
    mca_coll_fca_op_info_t *op_info;

    /* Check dtype */
    dtype_info = mca_coll_fca_get_dtype(dtype);
    if (!dtype_info) {
        FCA_VERBOSE(10, "Unsupported dtype: %s", dtype->name);
        return OMPI_ERROR;
    }

    /* Check FCA size */
    if (dtype_info->fca_dtype_extent * count > max_fca_payload) {
        FCA_VERBOSE(10, "Unsupported buffer size: %d", dtype_info->fca_dtype_extent * count);
        return OMPI_ERROR;
    }

    /* Check operation */
    op_info = mca_coll_fca_get_op(op);
    if (!op_info) {
        FCA_VERBOSE(10, "Unsupported op: %s", op->o_name);
        return OMPI_ERROR;
    }

    /* Fill spec */
    spec->dtype = dtype_info->fca_dtype;
    spec->op = op_info->fca_op;
    spec->length = count;
    spec->buf_size = dtype_info->mpi_dtype_extent * count;
    if (MPI_IN_PLACE == spec->sbuf) {
        FCA_VERBOSE(10, "Using MPI_IN_PLACE for sbuf");
        spec->sbuf = spec->rbuf;
    } else if (MPI_IN_PLACE == spec->rbuf) {
        FCA_VERBOSE(10, "Using MPI_IN_PLACE for rbuf");
        spec->rbuf = spec->sbuf;
    }
    return OMPI_SUCCESS;
}

/*
 *  Function:   - barrier
 *  Returns:    - MPI_SUCCESS or error code
 */
int mca_coll_fca_barrier(struct ompi_communicator_t *comm,
                         mca_coll_base_module_t *module)
{
    mca_coll_fca_module_t *fca_module = (mca_coll_fca_module_t*)module;
    int ret;

    FCA_VERBOSE(5,"Using FCA Barrier");
    ret = mca_coll_fca_component.fca_ops.do_barrier(fca_module->fca_comm);
    if (ret < 0) {
        FCA_ERROR("Barrier failed: %s", mca_coll_fca_component.fca_ops.strerror(ret));
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}

/*
 *  Function:   - broadcast
 *  Accepts:    - same arguments as MPI_Bcast()
 *  Returns:    - MPI_SUCCESS or error code
 */
int mca_coll_fca_bcast(void *buff, int count, struct ompi_datatype_t *datatype,
                       int root, struct ompi_communicator_t *comm,
                       mca_coll_base_module_t *module)
{
    mca_coll_fca_module_t *fca_module = (mca_coll_fca_module_t*)module;
    fca_bcast_spec_t spec;
    int ret;

    FCA_VERBOSE(5,"[%d] Calling mca_coll_fca_bcast, root=%d, count=%d",
                ompi_comm_rank(comm), root, count);

    spec.size = mca_coll_fca_get_buf_size(datatype, count);
    if (spec.size < 0 || spec.size > fca_module->fca_comm_caps.max_payload) {
        FCA_VERBOSE(5, "Unsupported bcast operation, dtype=%s[%d] using fallback\n",
                    datatype->name, count);
        return fca_module->previous_bcast(buff, count, datatype, root, comm,
                                          fca_module->previous_bcast_module);
    }

    FCA_VERBOSE(5,"Using FCA Bcast");
    spec.buf = buff;
    spec.root_indx = __find_local_rank(fca_module, root);
    ret = mca_coll_fca_component.fca_ops.do_bcast(fca_module->fca_comm, &spec);
    if (ret < 0) {
        FCA_ERROR("Bcast failed: %s", mca_coll_fca_component.fca_ops.strerror(ret));
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}

/*
 *  Reduce
 *
 *  Function:   - reduce
 *  Accepts:    - same as MPI_Reduce()
 *  Returns:    - MPI_SUCCESS or error code
 */
int mca_coll_fca_reduce(void *sbuf, void *rbuf, int count,
                        struct ompi_datatype_t *dtype, struct ompi_op_t *op,
                        int root, struct ompi_communicator_t *comm,
                        mca_coll_base_module_t *module)
{

    mca_coll_fca_module_t *fca_module = (mca_coll_fca_module_t*)module;
    fca_reduce_spec_t spec;
    int ret;

    spec.is_root   = fca_module->rank == root;
    spec.sbuf      = sbuf;
    spec.rbuf      = rbuf;
    if (mca_coll_fca_fill_reduce_spec(count, dtype, op, &spec, fca_module->fca_comm_caps.max_payload)
            != OMPI_SUCCESS) {
        FCA_VERBOSE(5, "Unsupported reduce operation %s, using fallback\n", op->o_name);
        return fca_module->previous_reduce(sbuf, rbuf, count, dtype, op, root,
                                           comm, fca_module->previous_reduce_module);
    }

    FCA_VERBOSE(5,"Using FCA Reduce");
    ret = mca_coll_fca_component.fca_ops.do_reduce(fca_module->fca_comm, &spec);
    if (ret < 0) {
        FCA_ERROR("Reduce failed: %s", mca_coll_fca_component.fca_ops.strerror(ret));
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}

/*
 *  Allreduce
 *
 *  Function:   - allreduce
 *  Accepts:    - same as MPI_Allreduce()
 *  Returns:    - MPI_SUCCESS or error code
 */
int mca_coll_fca_allreduce(void *sbuf, void *rbuf, int count,
                           struct ompi_datatype_t *dtype, struct ompi_op_t *op,
                           struct ompi_communicator_t *comm,
                           mca_coll_base_module_t *module)
{
    mca_coll_fca_module_t *fca_module = (mca_coll_fca_module_t*)module;
    fca_reduce_spec_t spec;
    int ret;

    spec.sbuf      = sbuf;
    spec.rbuf      = rbuf;
    if (mca_coll_fca_fill_reduce_spec(count, dtype, op, &spec, fca_module->fca_comm_caps.max_payload)
            != OMPI_SUCCESS) {
        FCA_VERBOSE(5, "Unsupported allreduce operation %s, using fallback\n", op->o_name);
        return fca_module->previous_allreduce(sbuf, rbuf, count, dtype, op,
                                           comm, fca_module->previous_allreduce_module);
    }

    FCA_VERBOSE(5,"Using FCA Allreduce");
    ret = mca_coll_fca_component.fca_ops.do_all_reduce(fca_module->fca_comm, &spec);
    if (ret < 0) {
        FCA_ERROR("Allreduce failed: %s", mca_coll_fca_component.fca_ops.strerror(ret));
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}

