/*
 * $HEADERS$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "runtime/runtime.h"
#include "communicator/communicator.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Comm_free = PMPI_Comm_free
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

int MPI_Comm_free(MPI_Comm *comm) {
    
    if ( MPI_PARAM_CHECK ) {
        OMPI_ERR_INIT_FINALIZE; 
        
        if ( NULL == *comm  || MPI_COMM_WORLD == *comm ||
             MPI_COMM_SELF == *comm  || ompi_comm_invalid (*comm))
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM, 
                                         "MPI_Comm_free");
    }
    
    ompi_comm_free ( comm ); 
    
    *comm = MPI_COMM_NULL;
    return MPI_SUCCESS;
}
