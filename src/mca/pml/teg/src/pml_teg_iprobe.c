#include "pml_teg_recvreq.h"


int mca_pml_teg_iprobe(
    int src,
    int tag,
    struct ompi_communicator_t* comm,
    int* matched,
    ompi_status_public_t* status)
{
    int rc;

    mca_ptl_base_recv_request_t recvreq;
    OBJ_CONSTRUCT(&recvreq, mca_ptl_base_recv_request_t);
    recvreq.super.req_type = MCA_PML_REQUEST_IPROBE;
    MCA_PTL_BASE_RECV_REQUEST_INIT(
        &recvreq,
        NULL,
        0,
        NULL,
        src,
        tag,
        comm,
        true);

    if((rc = mca_pml_teg_recv_request_start(&recvreq)) != OMPI_SUCCESS) {
        OBJ_DESTRUCT(&recvreq);
        return rc;
    }
    if((*matched = recvreq.super.req_mpi_done) == true && (NULL != status)) {
        *status = recvreq.super.req_status;
    }

    OBJ_DESTRUCT(&recvreq);
    return OMPI_SUCCESS;
}


int mca_pml_teg_probe(
    int src,
    int tag,
    struct ompi_communicator_t* comm,
    ompi_status_public_t* status)
{
    int rc;
    mca_ptl_base_recv_request_t recvreq;
    OBJ_CONSTRUCT(&recvreq, mca_ptl_base_recv_request_t);
    recvreq.super.req_type = MCA_PML_REQUEST_PROBE;
    MCA_PTL_BASE_RECV_REQUEST_INIT(
        &recvreq,
        NULL,
        0,
        NULL,
        src,
        tag,
        comm,
        true);

    if((rc = mca_pml_teg_recv_request_start(&recvreq)) != OMPI_SUCCESS) {
        OBJ_DESTRUCT(&recvreq);
        return rc;
    }

    if(recvreq.super.req_mpi_done == false) {
        /* give up and sleep until completion */
        if(ompi_using_threads()) {
            ompi_mutex_lock(&mca_pml_teg.teg_request_lock);
            mca_pml_teg.teg_request_waiting++;
            while(recvreq.super.req_mpi_done == false)
                ompi_condition_wait(&mca_pml_teg.teg_request_cond, &mca_pml_teg.teg_request_lock);
            mca_pml_teg.teg_request_waiting--;
            ompi_mutex_unlock(&mca_pml_teg.teg_request_lock);
        } else {
            mca_pml_teg.teg_request_waiting++;
            while(recvreq.super.req_mpi_done == false)
                ompi_condition_wait(&mca_pml_teg.teg_request_cond, &mca_pml_teg.teg_request_lock);
            mca_pml_teg.teg_request_waiting--;
        }
    }

    if(NULL != status) {
        *status = recvreq.super.req_status;
    }
    OBJ_DESTRUCT(&recvreq);
    return OMPI_SUCCESS;
}

