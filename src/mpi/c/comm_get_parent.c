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
#pragma weak MPI_Comm_get_parent = PMPI_Comm_get_parent
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Comm_get_parent";


int MPI_Comm_get_parent(MPI_Comm *parent) 
{

    if ( MPI_PARAM_CHECK ) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);

        if ( NULL == parent ) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, 
                                          FUNC_NAME);
        }
    }
    /*
     * ompi_mpi_comm_parent is MPI_COMM_NULL, in case this 
     * world has not been spawned by another MPI job. 
     * This is also the return value required by MPI-2.

     * check whether ompi_mpi_comm_parent has been disconnected.
       if it has not been 
         *parent = &ompi_mpi_comm_parent;
       else
       *parent  = &ompi_mpi_comm_null;
     */

    /* This function is not yet implemented */

    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_INTERN, FUNC_NAME);
}
