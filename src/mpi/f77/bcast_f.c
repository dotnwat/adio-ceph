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

#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_BCAST = mpi_bcast_f
#pragma weak pmpi_bcast = mpi_bcast_f
#pragma weak pmpi_bcast_ = mpi_bcast_f
#pragma weak pmpi_bcast__ = mpi_bcast_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_BCAST,
                           pmpi_bcast,
                           pmpi_bcast_,
                           pmpi_bcast__,
                           pmpi_bcast_f,
                           (char *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr),
                           (buffer, count, datatype, root, comm, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_BCAST = mpi_bcast_f
#pragma weak mpi_bcast = mpi_bcast_f
#pragma weak mpi_bcast_ = mpi_bcast_f
#pragma weak mpi_bcast__ = mpi_bcast_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_BCAST,
                           mpi_bcast,
                           mpi_bcast_,
                           mpi_bcast__,
                           mpi_bcast_f,
                           (char *buffer, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr),
                           (buffer, count, datatype, root, comm, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_bcast_f(char *buffer, MPI_Fint *count, MPI_Fint *datatype, 
		 MPI_Fint *root, MPI_Fint *comm, MPI_Fint *ierr)
{
    MPI_Comm c_comm;
    MPI_Datatype c_type;

    c_comm = MPI_Comm_f2c(*comm);
    c_type = MPI_Type_f2c(*datatype);

    *ierr = OMPI_INT_2_FINT(MPI_Bcast(buffer, 
				      OMPI_FINT_2_INT(*count), 
				      c_type,
				      OMPI_FINT_2_INT(*root),
				      c_comm));
}
