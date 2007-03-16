/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/request/request.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Ssend_init = PMPI_Ssend_init
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Ssend_init";


int MPI_Ssend_init(void *buf, int count, MPI_Datatype type,
                   int dest, int tag, MPI_Comm comm,
                   MPI_Request *request) 
{
    int rc;

    OPAL_CR_TEST_CHECKPOINT_READY();

    if ( MPI_PARAM_CHECK ) {
        rc = MPI_SUCCESS;
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if (ompi_comm_invalid(comm)) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COMM, FUNC_NAME);
        } else if (count < 0) {
            rc = MPI_ERR_COUNT;
        } else if (type == MPI_DATATYPE_NULL) {
            rc = MPI_ERR_TYPE;
        } else if (tag < 0 || tag > mca_pml.pml_max_tag) {
            rc = MPI_ERR_TAG;
        } else if (ompi_comm_peer_invalid(comm, dest) &&
                   (MPI_PROC_NULL != dest)) {
            rc = MPI_ERR_RANK;
        } else if (request == NULL) {
            rc = MPI_ERR_REQUEST;
        }
        OMPI_ERRHANDLER_CHECK(rc, comm, rc, FUNC_NAME);
    }

    if (MPI_PROC_NULL == dest) {
        *request = OBJ_NEW(ompi_request_t);
        /* Other fields were initialized by the constructor for
           ompi_request_t */
        (*request)->req_type = OMPI_REQUEST_NOOP;
        (*request)->req_status = ompi_request_empty.req_status;
        (*request)->req_complete = true;
        (*request)->req_state = OMPI_REQUEST_INACTIVE;
        (*request)->req_persistent = true;
        (*request)->req_free = ompi_request_persistent_proc_null_free;
        return MPI_SUCCESS;
    }

    rc = MCA_PML_CALL(isend_init(buf,count,type,dest,tag,
                                 MCA_PML_BASE_SEND_SYNCHRONOUS,comm,request));
    OMPI_ERRHANDLER_RETURN(rc, comm, rc, FUNC_NAME);
}

