/*
 * $HEADERS$
 */
#include "lam_config.h"
#include <stdio.h>

#include "mpi.h"
#include "mpi/interface/c/bindings.h"

#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILING_DEFINES
#pragma weak MPI_Request_f2c = PMPI_Request_f2c
#endif

MPI_Request MPI_Request_f2c(MPI_Fint request) {
    return (MPI_Request)0;
}
