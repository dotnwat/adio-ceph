/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_ALLGATHERV = mpi_allgatherv_f
#pragma weak pmpi_allgatherv = mpi_allgatherv_f
#pragma weak pmpi_allgatherv_ = mpi_allgatherv_f
#pragma weak pmpi_allgatherv__ = mpi_allgatherv_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_ALLGATHERV,
                           pmpi_allgatherv,
                           pmpi_allgatherv_,
                           pmpi_allgatherv__,
                           pmpi_allgatherv_f,
                           (char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, char *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_ALLGATHERV = mpi_allgatherv_f
#pragma weak mpi_allgatherv = mpi_allgatherv_f
#pragma weak mpi_allgatherv_ = mpi_allgatherv_f
#pragma weak mpi_allgatherv__ = mpi_allgatherv_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_ALLGATHERV,
                           mpi_allgatherv,
                           mpi_allgatherv_,
                           mpi_allgatherv__,
                           mpi_allgatherv_f,
                           (char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype, char *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs, MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, sendcount, sendtype, recvbuf, recvcounts, displs, recvtype, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_allgatherv_f(char *sendbuf, MPI_Fint *sendcount, MPI_Fint *sendtype,
		      char *recvbuf, MPI_Fint *recvcounts, MPI_Fint *displs,
		      MPI_Fint *recvtype, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    MPI_Datatype c_sendtype, c_recvtype;
    int size;
    OMPI_ARRAY_NAME_DECL(recvcounts);
    OMPI_ARRAY_NAME_DECL(displs);

    c_comm = MPI_Comm_f2c(*comm);
    c_sendtype = MPI_Type_f2c(*sendtype);
    c_recvtype = MPI_Type_f2c(*recvtype);

    MPI_Comm_size(c_comm, &size);
    OMPI_ARRAY_FINT_2_INT(recvcounts, size);
    OMPI_ARRAY_FINT_2_INT(displs, size);

    *ierr = OMPI_INT_2_FINT(MPI_Allgatherv(sendbuf, 
					   OMPI_FINT_2_INT(*sendcount),
					   c_sendtype, 
					   recvbuf, 
					   OMPI_ARRAY_NAME_CONVERT(recvcounts),
					   OMPI_ARRAY_NAME_CONVERT(displs),
					   c_recvtype, c_comm));

    OMPI_ARRAY_FINT_2_INT_CLEANUP(recvcounts);
    OMPI_ARRAY_FINT_2_INT_CLEANUP(displs);
}
