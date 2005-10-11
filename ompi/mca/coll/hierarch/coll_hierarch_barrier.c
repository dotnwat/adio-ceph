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
#include "coll_hierarch.h"

#include "mpi.h"
#include "ompi/include/constants.h"
#include "opal/util/output.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/base.h"
#include "coll_hierarch.h"


/*
 *	barrier_intra
 *
 *	Function:	- barrier using O(N) algorithm
 *	Accepts:	- same as MPI_Barrier()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_hierarch_barrier_intra(struct ompi_communicator_t *comm)
{
  opal_output_verbose(10, mca_coll_base_output, "In hierarch barrier_intra");
  return MPI_SUCCESS;
}


