/*
 * $HEADERS$
 */
#include "lam_config.h"
#include <stdio.h>

#include "mpi.h"
#include "runtime/runtime.h"
#include "mpi/c/bindings.h"
#include "mca/pml/pml.h"


#if LAM_HAVE_WEAK_SYMBOLS && LAM_PROFILING_DEFINES
#pragma weak MPI_Sendrecv = PMPI_Sendrecv
#endif

#if LAM_PROFILING_DEFINES
#include "mpi/c/profile/defines.h"
#endif

int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype recvtype,
                 int dest, int sendtag, void *recvbuf, int recvcount,
                 MPI_Datatype sendtype, int source, int recvtag,
                 MPI_Comm comm,  MPI_Status *status) 
{
    lam_request_t* req;
    int rc;

    if ( MPI_PARAM_CHECK ) {
        rc = MPI_SUCCESS;
        if ( LAM_MPI_INVALID_STATE ) {
            rc = MPI_ERR_INTERN;
        } else if (lam_comm_invalid(comm)) {
            rc = MPI_ERR_COMM;
        } else if (sendcount < 0) {
            rc = MPI_ERR_COUNT;
        } else if (sendtype == MPI_DATATYPE_NULL) {
            rc = MPI_ERR_TYPE;
        } else if (lam_comm_peer_invalid(comm, dest)) {
            rc = MPI_ERR_RANK;
        } else if (sendtag < 0 || sendtag > MPI_TAG_UB_VALUE) {
            rc = MPI_ERR_TAG;
        } else if (recvcount < 0) {
            rc = MPI_ERR_COUNT;
        } else if (recvtype == MPI_DATATYPE_NULL) {
            rc = MPI_ERR_TYPE;
        } else if (source != MPI_PROC_NULL && source != MPI_ANY_SOURCE && lam_comm_peer_invalid(comm, source)) {
            rc = MPI_ERR_RANK;
        } else if (recvtag < MPI_ANY_TAG || recvtag > MPI_TAG_UB_VALUE) {
            rc = MPI_ERR_TAG;
        }
        LAM_ERRHANDLER_CHECK(rc, comm, rc, "MPI_Sendrecv");
    }

    if (source != MPI_PROC_NULL) { /* post recv */
        rc = mca_pml.pml_irecv(recvbuf, recvcount, recvtype,
                           source, recvtag, comm, &req);
        LAM_ERRHANDLER_CHECK(rc, comm, rc, "MPI_Sendrecv");
    }

    if (dest != MPI_PROC_NULL) { /* send */
        rc = mca_pml.pml_send(sendbuf, sendcount, sendtype, dest,
                           sendtag, MCA_PML_BASE_SEND_STANDARD, comm);
        LAM_ERRHANDLER_CHECK(rc, comm, rc, "MPI_Sendrecv");
    }

    if (source != MPI_PROC_NULL) { /* wait for recv */
 
        rc = mca_pml.pml_wait(1, &req, NULL, status);

    } else {

        status->MPI_ERROR = MPI_SUCCESS;
        status->MPI_SOURCE = MPI_PROC_NULL;
        status->MPI_TAG = MPI_ANY_TAG;
        status->_count = 0;
        rc = MPI_SUCCESS;
    }
    LAM_ERRHANDLER_RETURN(rc, comm, rc, "MPI_Sendrecv");
}
