/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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
#include "ompi/constants.h"
#include "opal/util/output.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "coll_demo.h"


/*
 *	scatter_intra
 *
 *	Function:	- scatter operation
 *	Accepts:	- same arguments as MPI_Scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_scatter_intra(void *sbuf, int scount, 
                                struct ompi_datatype_t *sdtype,
                                void *rbuf, int rcount, 
                                struct ompi_datatype_t *rdtype,
                                int root, 
                                struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo scatter_intra");
    return comm->c_coll_basic_module->coll_scatter(sbuf, scount, sdtype,
                                                   rbuf, rcount, rdtype,
                                                   root, comm);
}


/*
 *	scatter_inter
 *
 *	Function:	- scatter operation
 *	Accepts:	- same arguments as MPI_Scatter()
 *	Returns:	- MPI_SUCCESS or error code
 */
int mca_coll_demo_scatter_inter(void *sbuf, int scount, 
                                struct ompi_datatype_t *sdtype,
                                void *rbuf, int rcount, 
                                struct ompi_datatype_t *rdtype,
                                int root, 
                                struct ompi_communicator_t *comm)
{
    opal_output_verbose(10, mca_coll_base_output, "In demo scatter_inter");
    return comm->c_coll_basic_module->coll_scatter(sbuf, scount, sdtype,
                                                   rbuf, rcount, rdtype,
                                                   root, comm);
}
