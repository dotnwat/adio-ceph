/*
 * $HEADER$
 */

#include "lam_config.h"

#include "mpi.h"
#include "mpi/c/bindings.h"

#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILING_DEFINES
#pragma weak MPI_Keyval_free = PMPI_Keyval_free
#endif

int MPI_Keyval_free(int *keyval) {
    return MPI_SUCCESS;
}
