/*
 * $HEADER$
 */

#include "lam_config.h"

#include <string.h>

#include "lam/util/strncpy.h"
#include "lam/totalview.h"
#include "mpi.h"
#include "mpi/interface/c/bindings.h"
#include "mpi/communicator/communicator.h"

#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILING_DEFINES 
#pragma weak MPI_Comm_set_name = PMPI_Comm_set_name
#endif

int
MPI_Comm_set_name(MPI_Comm comm, char *name)
{
  if (comm == MPI_COMM_NULL) {
    /* -- Invoke error function -- */
  }
  
  if (name == NULL) {
    /* -- Invoke error function -- */
  }

  /* -- Thread safety entrance -- */

  /* Copy in the name */
  
  strncpy(comm->c_name, name, MPI_MAX_OBJECT_NAME);
  comm->c_name[MPI_MAX_OBJECT_NAME - 1] = 0;

  /* -- Tracing information for new communicator name -- */

#if 0  
  /* Force TotalView DLL to take note of this name setting */

  ++lam_tv_comm_sequence_number;
#endif

  /* -- Thread safety exit -- */
  
  return MPI_SUCCESS;
}
