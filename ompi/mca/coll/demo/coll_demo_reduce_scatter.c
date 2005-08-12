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

#include "ompi_config.h"

#include "mpi.h"
#include "ompi/include/constants.h"
#include "opal/util/output.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/base.h"
#include "coll_demo.h"


/*
 *	reduce_scatter
 *
 *	Function:	- reduce then scatter
 *	Accepts:	- same as MPI_Reduce_scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_reduce_scatter_intra(void *sbuf, void *rbuf, int *rcounts,
                                       struct ompi_datatype_t *dtype,
                                       struct ompi_op_t *op,
                                       struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo scatter_intra");
    return comm->c_coll_basic_module->coll_reduce_scatter(sbuf, rbuf, rcounts,
                                                          dtype, op, comm);
}


/*
 *	reduce_scatter_inter
 *
 *	Function:	- reduce/scatter operation
 *	Accepts:	- same arguments as MPI_Reduce_scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_reduce_scatter_inter(void *sbuf, void *rbuf, int *rcounts,
                                       struct ompi_datatype_t *dtype,
                                       struct ompi_op_t *op,
                                       struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo scatter_inter");
    return comm->c_coll_basic_module->coll_reduce_scatter(sbuf, rbuf, rcounts,
                                                          dtype, op, comm);
}
