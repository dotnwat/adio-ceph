/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_ALLREDUCE = mpi_allreduce_f
#pragma weak pmpi_allreduce = mpi_allreduce_f
#pragma weak pmpi_allreduce_ = mpi_allreduce_f
#pragma weak pmpi_allreduce__ = mpi_allreduce_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_ALLREDUCE,
                           pmpi_allreduce,
                           pmpi_allreduce_,
                           pmpi_allreduce__,
                           pmpi_allreduce_f,
                           (char *sendbuf, char *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, recvbuf, count, datatype, op, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_ALLREDUCE = mpi_allreduce_f
#pragma weak mpi_allreduce = mpi_allreduce_f
#pragma weak mpi_allreduce_ = mpi_allreduce_f
#pragma weak mpi_allreduce__ = mpi_allreduce_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_ALLREDUCE,
                           mpi_allreduce,
                           mpi_allreduce_,
                           mpi_allreduce__,
                           mpi_allreduce_f,
                           (char *sendbuf, char *recvbuf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm, MPI_Fint *ierr),
                           (sendbuf, recvbuf, count, datatype, op, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_allreduce_f(char *sendbuf, char *recvbuf, MPI_Fint *count,
		     MPI_Fint *datatype, MPI_Fint *op, MPI_Fint *comm,
		     MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    MPI_Datatype c_type;
    MPI_Op c_op;

    c_comm = MPI_Comm_f2c(*comm);
    c_type = MPI_Type_f2c(*datatype);
    c_op = MPI_Op_f2c(*op);

    *ierr = OMPI_INT_2_FINT(MPI_Allreduce(sendbuf, recvbuf,
					  OMPI_FINT_2_INT(*count),
					  c_type, c_op, c_comm));
}
