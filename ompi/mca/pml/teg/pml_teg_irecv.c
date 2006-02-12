/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include "ompi/request/request.h"
#include "pml_teg_recvreq.h"


int mca_pml_teg_irecv_init(void *addr,
                           size_t count,
                           struct ompi_datatype_t * datatype,
                           int src,
                           int tag,
                           struct ompi_communicator_t *comm,
                           struct ompi_request_t **request)
{
    int rc;
    mca_ptl_base_recv_request_t *recvreq;
    MCA_PML_TEG_RECV_REQUEST_ALLOC(recvreq, rc);
    if (NULL == recvreq)
        return rc;

    MCA_PML_TEG_RECV_REQUEST_INIT(recvreq,
                                   addr,
                                   count, datatype, src, tag, comm, true);

    *request = (ompi_request_t *) recvreq;
    return OMPI_SUCCESS;
}

int mca_pml_teg_irecv(void *addr,
                      size_t count,
                      struct ompi_datatype_t * datatype,
                      int src,
                      int tag,
                      struct ompi_communicator_t *comm,
                      struct ompi_request_t **request)
{
    int rc;

    mca_ptl_base_recv_request_t *recvreq;
    MCA_PML_TEG_RECV_REQUEST_ALLOC(recvreq, rc);
    if (NULL == recvreq)
        return rc;

    MCA_PML_TEG_RECV_REQUEST_INIT(recvreq,
                                   addr,
                                   count, datatype, src, tag, comm, false);

    if ((rc = mca_pml_teg_recv_request_start(recvreq)) != OMPI_SUCCESS) {
        MCA_PML_TEG_RECV_REQUEST_RETURN(recvreq);
        return rc;
    }
    *request = (ompi_request_t *) recvreq;
    return OMPI_SUCCESS;
}


int mca_pml_teg_recv(void *addr,
                     size_t count,
                     struct ompi_datatype_t * datatype,
                     int src,
                     int tag,
                     struct ompi_communicator_t *comm,
                     ompi_status_public_t * status)
{
    int rc;
    mca_ptl_base_recv_request_t *recvreq;
    MCA_PML_TEG_RECV_REQUEST_ALLOC(recvreq, rc);
    if (NULL == recvreq)
        return rc;

    MCA_PML_TEG_RECV_REQUEST_INIT(recvreq,
                                   addr,
                                   count, datatype, src, tag, comm, false);

    if ((rc = mca_pml_teg_recv_request_start(recvreq)) != OMPI_SUCCESS) {
        goto recv_finish;
    }

    if (recvreq->req_recv.req_base.req_ompi.req_complete == false) {
        /* give up and sleep until completion */
        if (opal_using_threads()) {
            opal_mutex_lock(&ompi_request_lock);
            ompi_request_waiting++;
            while (recvreq->req_recv.req_base.req_ompi.req_complete == false)
                opal_condition_wait(&ompi_request_cond, &ompi_request_lock);
            ompi_request_waiting--;
            opal_mutex_unlock(&ompi_request_lock);
        } else {
            ompi_request_waiting++;
            while (recvreq->req_recv.req_base.req_ompi.req_complete == false)
                opal_condition_wait(&ompi_request_cond, &ompi_request_lock);
            ompi_request_waiting--;
        }
    }
 recv_finish:
    if (NULL != status) {  /* return status */
        *status = recvreq->req_recv.req_base.req_ompi.req_status;
    }

    MCA_PML_TEG_RECV_REQUEST_RETURN(recvreq);
    return recvreq->req_recv.req_base.req_ompi.req_status.MPI_ERROR;
}
