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
 *
 * Utility functions for Open MPI object manipulation by the One-sided code
 *
 * Utility functions for creating / finding handles for Open MPI
 * objects, usually based on indexes sent from remote peers.
 */

#include "ompi/datatype/datatype.h"
#include "ompi/datatype/datatype_internal.h"
#include "ompi/proc/proc.h"
#include "ompi/op/op.h"

BEGIN_C_DECLS

/**
 * Create datatype based on packed payload
 *
 * Create a useable MPI datatype based on it's packed description.
 * The datatype is owned by the calling process and must be
 * OBJ_RELEASEd when no longer in use.
 *
 * @param remote_proc The ompi_proc_t pointer for the remote process
 * @param payload     A pointer to the pointer to the payload.  The
 *                    pointer to the payload will be moved past the
 *                    datatype information upon successful return.
 *
 * @retval NULL       A failure occrred
 * @retval non-NULL   A fully operational datatype
 */
static inline
struct ompi_datatype_t*
ompi_osc_base_datatype_create(ompi_proc_t *remote_proc,  void **payload)
{
    struct ompi_datatype_t *datatype =
        ompi_ddt_create_from_packed_description(payload, remote_proc);
    if (NULL == datatype) return NULL;
    if (ompi_ddt_is_predefined(datatype)) OBJ_RETAIN(datatype);
    return datatype;
}


/**
 * Create datatype based on Fortran Index
 *
 * Create a useable MPI datatype based on it's Fortran index, which is
 * globally the same for predefined operations.  The op handle is
 * owned by the calling process and must be OBJ_RELEASEd when no
 * longer in use.
 *
 * @param op_id       The fortran index for the operaton
 *
 * @retval NULL       A failure occrred
 * @retval non-NULL   An op handle
 */
static inline
ompi_op_t *
ompi_osc_base_op_create(int op_id)
{
    ompi_op_t *op = MPI_Op_f2c(op_id);
    OBJ_RETAIN(op);
    return op;
}


/**
 * Get the primitive datatype information for a legal one-sided accumulate datatype
 *
 * Get the primitive datatype information for a legal one-sided
 * accumulate datatype.  This includes the primitive datatype used to
 * build the datatype (there can be only one) and the number of
 * instances of that primitive datatype in the datatype (there can be
 * many).
 *
 * @param datatype      legal one-sided datatype
 * @param prim_datatype The primitive datatype used to build datatype
 * @param prim_count    Number of instances of prim_datattpe in datatype
 *
 * @retval OMPI_SUCCESS Success
 */
static inline int
ompi_osc_base_get_primitive_type_info(ompi_datatype_t *datatype,
                                      ompi_datatype_t **prim_datatype, 
                                      uint32_t *prim_count)
{
    struct ompi_datatype_t *primitive_datatype = NULL;
    uint32_t primitive_count;

    /* get underlying type... */
    if (ompi_ddt_is_predefined(datatype)) {
        primitive_datatype = datatype;
        primitive_count = 1;
    } else {
        int i, found_index = -1;
        uint64_t mask = 1;
        for (i = 0 ; i < DT_MAX_PREDEFINED ; ++i) {
            if (datatype->bdt_used & mask) {
                found_index = i;
                break;
            }
            mask *= 2;
        }
        primitive_datatype = (ompi_datatype_t*)
            ompi_ddt_basicDatatypes[found_index];
        primitive_count = datatype->nbElems;
    }

    *prim_datatype = primitive_datatype;
    *prim_count = primitive_count;

    return OMPI_SUCCESS;
}


/**
 * Apply the operation specified from inbuf to outbut
 *
 * Apply the specified reduction operation from inbuf to outbuf.
 * inbuf must contain count instances of datatype, in the local
 * process's binary mode.
 *
 * @retval OMPI_SUCCESS           Success
 * @retval OMPI_ERR_NOT_SUPPORTED Called with op == ompi_mpi_op_replace
 */
int ompi_osc_base_process_op(void *outbuf,
                             void *inbuf,
                             size_t inbuflen,
                             struct ompi_datatype_t *datatype,
                             int count,
                             ompi_op_t *op);

END_C_DECLS
