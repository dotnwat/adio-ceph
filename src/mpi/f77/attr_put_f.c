/*
 * $HEADER$
 */

#include "lam_config.h"

#include <stdio.h>

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILE_LAYER
#pragma weak PMPI_ATTR_PUT = mpi_attr_put_f
#pragma weak pmpi_attr_put = mpi_attr_put_f
#pragma weak pmpi_attr_put_ = mpi_attr_put_f
#pragma weak pmpi_attr_put__ = mpi_attr_put_f
#elif LAM_PROFILE_LAYER
LAM_GENERATE_F77_BINDINGS (PMPI_ATTR_PUT,
                           pmpi_attr_put,
                           pmpi_attr_put_,
                           pmpi_attr_put__,
                           pmpi_attr_put_f,
                           (MPI_Fint *comm, MPI_Fint *keyval, char *attribute_val, MPI_Fint *ierr),
                           (comm, keyval, attribute_val, ierr) )
#endif

#if LAM_HAVE_WEAK_SYMBOLS
#pragma weak MPI_ATTR_PUT = mpi_attr_put_f
#pragma weak mpi_attr_put = mpi_attr_put_f
#pragma weak mpi_attr_put_ = mpi_attr_put_f
#pragma weak mpi_attr_put__ = mpi_attr_put_f
#endif

#if ! LAM_HAVE_WEAK_SYMBOLS && ! LAM_PROFILE_LAYER
LAM_GENERATE_F77_BINDINGS (MPI_ATTR_PUT,
                           mpi_attr_put,
                           mpi_attr_put_,
                           mpi_attr_put__,
                           mpi_attr_put_f,
                           (MPI_Fint *comm, MPI_Fint *keyval, char *attribute_val, MPI_Fint *ierr),
                           (comm, keyval, attribute_val, ierr) )
#endif


#if LAM_PROFILE_LAYER && ! LAM_HAVE_WEAK_SYMBOLS
#include "mpi/c/profile/defines.h"
#endif

void mpi_attr_put_f(MPI_Fint *comm, MPI_Fint *keyval, char *attribute_val, MPI_Fint *ierr)
{

}
