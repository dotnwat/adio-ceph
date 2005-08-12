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
 *	exscan_intra
 *
 *	Function:	- exscan
 *	Accepts:	- same arguments as MPI_Exscan()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_exscan_intra(void *sbuf, void *rbuf, int count,
                               struct ompi_datatype_t *dtype, 
                               struct ompi_op_t *op, 
                               struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo exscan_intra");
    return comm->c_coll_basic_module->coll_exscan(sbuf, rbuf, count, dtype,
                                                  op, comm);
}


/*
 *	exscan_inter
 *
 *	Function:	- exscan
 *	Accepts:	- same arguments as MPI_Exscan()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_exscan_inter(void *sbuf, void *rbuf, int count,
                               struct ompi_datatype_t *dtype, 
                               struct ompi_op_t *op, 
                               struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo exscan_inter");
    return comm->c_coll_basic_module->coll_exscan(sbuf, rbuf, count, dtype,
                                                  op, comm);
}
