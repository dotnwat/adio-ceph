/*
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "mpi/c/bindings.h"
#include "communicator/communicator.h"
#include "errhandler/errhandler.h"
#include "file/file.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_File_sync = PMPI_File_sync
#endif

#if OMPI_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_File_sync";


int MPI_File_sync(MPI_File fh)
{
    int rc;

    if (MPI_PARAM_CHECK) {
        rc = MPI_SUCCESS;
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_file_invalid(fh)) {
            rc = MPI_ERR_FILE;
            fh = MPI_FILE_NULL;
        }
        OMPI_ERRHANDLER_CHECK(rc, fh, rc, FUNC_NAME);
    }

    /* Call the back-end io component function */

    switch (fh->f_io_version) {
    case MCA_IO_BASE_V_1_0_0:
        rc = fh->f_io_selected_module.v1_0_0.
            io_module_file_sync(fh);
        break;

    default:
        rc = MPI_ERR_INTERN;
        break;
    }

    /* All done */
    
    OMPI_ERRHANDLER_RETURN(rc, fh, rc, FUNC_NAME);
}
