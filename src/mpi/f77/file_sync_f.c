/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_SYNC = mpi_file_sync_f
#pragma weak pmpi_file_sync = mpi_file_sync_f
#pragma weak pmpi_file_sync_ = mpi_file_sync_f
#pragma weak pmpi_file_sync__ = mpi_file_sync_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_SYNC,
                           pmpi_file_sync,
                           pmpi_file_sync_,
                           pmpi_file_sync__,
                           pmpi_file_sync_f,
                           (MPI_Fint *fh, MPI_Fint *ierr),
                           (fh, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_SYNC = mpi_file_sync_f
#pragma weak mpi_file_sync = mpi_file_sync_f
#pragma weak mpi_file_sync_ = mpi_file_sync_f
#pragma weak mpi_file_sync__ = mpi_file_sync_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_SYNC,
                           mpi_file_sync,
                           mpi_file_sync_,
                           mpi_file_sync__,
                           mpi_file_sync_f,
                           (MPI_Fint *fh, MPI_Fint *ierr),
                           (fh, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_file_sync_f(MPI_Fint *fh, MPI_Fint *ierr)
{
  /* This function not yet implemented */
}
