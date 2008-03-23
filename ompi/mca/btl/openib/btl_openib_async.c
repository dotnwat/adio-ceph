/*
 * Copyright (c) 2007 Mellanox Technologies. All rights reserved.
 * Copyright (c) 2007 Cisco, Inc.  All rights reserved.
 * Copyright (c) 2006-2007 Voltaire All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */


#include "ompi_config.h"

#if OMPI_HAVE_THREADS
#include <infiniband/verbs.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <errno.h>

#include "opal/util/output.h"
#include "opal/util/show_help.h"
#include "ompi/mca/btl/btl.h"

#include "ompi/mca/btl/base/base.h"
#include "btl_openib.h"
#include "btl_openib_mca.h"
#include "btl_openib_async.h"
#include "btl_openib_proc.h"
#include "btl_openib_endpoint.h"

struct mca_btl_openib_async_poll {
    int active_poll_size;
    int poll_size;
    struct pollfd *async_pollfd;
};
typedef struct mca_btl_openib_async_poll mca_btl_openib_async_poll;

static int return_status = OMPI_ERROR;

static int btl_openib_async_poll_init(struct mca_btl_openib_async_poll *hcas_poll);
static int btl_openib_async_commandh(struct mca_btl_openib_async_poll *hcas_poll);
static int btl_openib_async_hcah(struct mca_btl_openib_async_poll *hcas_poll, int index);
static const char *openib_event_to_str (enum ibv_event_type event);

/* Function converts event to string (name)
 * Open Fabris don't have function that do this job :(
 */
static const char *openib_event_to_str (enum ibv_event_type event)
{
    switch (event) {
    case IBV_EVENT_CQ_ERR:
        return "IBV_EVENT_CQ_ERR";
    case IBV_EVENT_QP_FATAL:
        return "IBV_EVENT_QP_FATAL";
    case IBV_EVENT_QP_REQ_ERR:
        return "IBV_EVENT_QP_REQ_ERR";
    case IBV_EVENT_QP_ACCESS_ERR:
        return "IBV_EVENT_QP_ACCESS_ERR";
    case IBV_EVENT_PATH_MIG:
        return "IBV_EVENT_PATH_MIG";
    case IBV_EVENT_PATH_MIG_ERR:
        return "IBV_EVENT_PATH_MIG_ERR";
    case IBV_EVENT_DEVICE_FATAL:
        return "IBV_EVENT_DEVICE_FATAL";
    case IBV_EVENT_SRQ_ERR:
        return "IBV_EVENT_SRQ_ERR";
    case IBV_EVENT_PORT_ERR:
        return "IBV_EVENT_PORT_ERR";
    case IBV_EVENT_COMM_EST:
        return "IBV_EVENT_COMM_EST";
    case IBV_EVENT_PORT_ACTIVE:
        return "IBV_EVENT_PORT_ACTIVE";
    case IBV_EVENT_SQ_DRAINED:
        return "IBV_EVENT_SQ_DRAINED";
    case IBV_EVENT_LID_CHANGE:
        return "IBV_EVENT_LID_CHANGE";
    case IBV_EVENT_PKEY_CHANGE:
        return "IBV_EVENT_PKEY_CHANGE";
    case IBV_EVENT_SM_CHANGE:
        return "IBV_EVENT_SM_CHANGE";
    case IBV_EVENT_QP_LAST_WQE_REACHED:
        return "IBV_EVENT_QP_LAST_WQE_REACHED";
#if HAVE_DECL_IBV_EVENT_CLIENT_REREGISTER
    case IBV_EVENT_CLIENT_REREGISTER:
        return "IBV_EVENT_CLIENT_REREGISTER";
#endif
    case IBV_EVENT_SRQ_LIMIT_REACHED:
        return "IBV_EVENT_SRQ_LIMIT_REACHED";
    default:
        return "UNKNOWN";
    }
}
/* QP to endpoint */
static mca_btl_openib_endpoint_t * qp2endpoint(struct ibv_qp *qp, mca_btl_openib_hca_t *hca)
{
    mca_btl_openib_endpoint_t *ep;
    int  ep_i, qp_i;
    for(ep_i = 0; ep_i < opal_pointer_array_get_size(hca->endpoints); ep_i++) {
        ep = opal_pointer_array_get_item(hca->endpoints, ep_i);
        for(qp_i = 0; qp_i < mca_btl_openib_component.num_qps; qp_i++) {
            if (qp == ep->qps[qp_i].qp->lcl_qp)
                return ep;
        }
    }
    return NULL;
}

/* XRC recive QP to endpoint */
static mca_btl_openib_endpoint_t * xrc_qp2endpoint(uint32_t qp_num, mca_btl_openib_hca_t *hca)
{
    mca_btl_openib_endpoint_t *ep;
    int  ep_i;
    for(ep_i = 0; ep_i < opal_pointer_array_get_size(hca->endpoints); ep_i++) {
        ep = opal_pointer_array_get_item(hca->endpoints, ep_i);
        if (qp_num == ep->xrc_recv_qp_num)
            return ep;
    }
    return NULL;
}

/* Function inits mca_btl_openib_async_poll */
static int btl_openib_async_poll_init(struct mca_btl_openib_async_poll *hcas_poll)
{
    hcas_poll->active_poll_size = 1;
    hcas_poll->poll_size = 4;
    hcas_poll->async_pollfd = malloc(sizeof(struct pollfd) * hcas_poll->poll_size);
    if (NULL == hcas_poll->async_pollfd) {
        BTL_ERROR(("Failed malloc: %s:%d"
                    , __FILE__, __LINE__));
        return OMPI_ERROR;
    }
    /* Creating comunication channel with the main thread */
    hcas_poll->async_pollfd[0].fd = mca_btl_openib_component.async_pipe[0];
    hcas_poll->async_pollfd[0].events = POLLIN;
    hcas_poll->async_pollfd[0].revents = 0;
    return OMPI_SUCCESS;
}

/* Function handle async thread commands */
static int btl_openib_async_commandh(struct mca_btl_openib_async_poll *hcas_poll)
{
    struct pollfd *async_pollfd_tmp;
    int fd,flags,j;
    /* Got command from main thread */
    if (read(hcas_poll->async_pollfd[0].fd, &fd, sizeof(int)) < 0) {
        BTL_ERROR(("Read failed [%d]",errno));
        return OMPI_ERROR;
    }
    BTL_VERBOSE(("GOT event from -> %d",fd));
    if (fd > 0) {
        BTL_VERBOSE(("Adding HCA [%d] to async event poll[%d]"
                    ,fd,hcas_poll->active_poll_size));
        flags = fcntl(fd, F_GETFL);
        if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) < 0) {
            BTL_ERROR(("Failed to change file descriptor of async event"));
            return OMPI_ERROR;
        }
        if ((hcas_poll->active_poll_size + 1) > hcas_poll->poll_size) {
            hcas_poll->poll_size+=hcas_poll->poll_size;
            async_pollfd_tmp = malloc(sizeof(struct pollfd) * hcas_poll->poll_size);
            if (NULL == async_pollfd_tmp) {
                BTL_ERROR(("Failed malloc: %s:%d"
                            "Fatal error, stoping asyn event thread"
                            , __FILE__, __LINE__));
                return OMPI_ERROR;
            }
            memcpy (async_pollfd_tmp,hcas_poll->async_pollfd,
                    sizeof(struct pollfd) * (hcas_poll->active_poll_size));
            free(hcas_poll->async_pollfd);
            hcas_poll->async_pollfd = async_pollfd_tmp;
        }
        hcas_poll->async_pollfd[hcas_poll->active_poll_size].fd = fd;
        hcas_poll->async_pollfd[hcas_poll->active_poll_size].events = POLLIN;
        hcas_poll->async_pollfd[hcas_poll->active_poll_size].revents = 0;
        hcas_poll->active_poll_size++;
    } else if (fd < 0) {
        bool fd_found = false;
        /* Removing HCA from poll */
        fd = -(fd);
        BTL_VERBOSE(("Removing HCA [%d] from async event poll [%d]"
                    ,fd,hcas_poll->active_poll_size));
        if (hcas_poll->active_poll_size > 1) {
            for (j=0; (j < hcas_poll->active_poll_size || !fd_found); j++) {
                if (hcas_poll->async_pollfd[j].fd == fd) {
                    hcas_poll->async_pollfd[j].fd =
                        hcas_poll->async_pollfd[hcas_poll->active_poll_size-1].fd;
                    hcas_poll->async_pollfd[j].events =
                        hcas_poll->async_pollfd[hcas_poll->active_poll_size-1].events;
                    hcas_poll->async_pollfd[j].revents =
                        hcas_poll->async_pollfd[hcas_poll->active_poll_size-1].revents;
                    fd_found = true;
                }
            }
            if (!fd_found) {
                BTL_ERROR(("Requested FD[%d] was not found in poll array\n",fd));
                return OMPI_ERROR;
            }
        }
        hcas_poll->active_poll_size--;
    } else {
        /* Got 0 - command to close the thread */
        BTL_VERBOSE(("Async event thread exit"));
        free(hcas_poll->async_pollfd);
        return_status = OMPI_SUCCESS;
        pthread_exit(&return_status);
    }
    return OMPI_SUCCESS;
}

/* Function handle async hca events */
static int btl_openib_async_hcah(struct mca_btl_openib_async_poll *hcas_poll, int index)
{
    int j;
    mca_btl_openib_hca_t *hca = NULL;
    struct ibv_async_event event;
    bool xrc_event = false;
    int event_type;

    /* We need to find correct hca and process this event */
    for (j=0; j < mca_btl_openib_component.ib_num_btls; j++) {
        if (mca_btl_openib_component.openib_btls[j]->hca->ib_dev_context->async_fd ==
                hcas_poll->async_pollfd[index].fd ) {
            hca = mca_btl_openib_component.openib_btls[j]->hca;
            break;
        }
    }
    if (NULL != hca) {
        if (ibv_get_async_event((struct ibv_context *)hca->ib_dev_context,&event) < 0) {
            if (EWOULDBLOCK == errno) {
                /* No event found ?
                 * It was handled by somebody other */
                return OMPI_SUCCESS;
            } else {
                BTL_ERROR(("Failed to get async event"));
                return OMPI_ERROR;
            }
        }

        event_type = event.event_type;
#if HAVE_XRC
        /* is it XRC event ?*/
        if (IBV_XRC_QP_EVENT_FLAG & event.event_type) {
            xrc_event = true;
            /* Clean the bitnd handel as usual */
            event_type ^= IBV_XRC_QP_EVENT_FLAG;
        }
#endif
        switch(event_type) {
            case IBV_EVENT_PATH_MIG:
                BTL_ERROR(("Alternative path migration event reported"));
                if (APM_ENABLED) {
                    BTL_ERROR(("Trying to find additional path..."));
                    if (!xrc_event) 
                        mca_btl_openib_load_apm(event.element.qp,
                                qp2endpoint(event.element.qp, hca));
#if HAVE_XRC
                    else
                        mca_btl_openib_load_apm_xrc_rcv(event.element.xrc_qp_num,
                                xrc_qp2endpoint(event.element.xrc_qp_num, hca));
#endif
                }
                break;
            case IBV_EVENT_DEVICE_FATAL:
                /* Set the flag to fatal */
                hca->got_fatal_event = true;
                /* It is not critical to protect the counter */
                OPAL_THREAD_ADD32(&mca_btl_openib_component.fatal_counter, 1);
            case IBV_EVENT_CQ_ERR:
            case IBV_EVENT_QP_FATAL:
            case IBV_EVENT_QP_REQ_ERR:
            case IBV_EVENT_QP_ACCESS_ERR:
            case IBV_EVENT_PATH_MIG_ERR:
            case IBV_EVENT_SRQ_ERR:
            case IBV_EVENT_PORT_ERR:
                opal_show_help("help-mpi-btl-openib.txt", "of error event",
                    true,orte_process_info.nodename, orte_process_info.pid,
                    event.event_type, openib_event_to_str(event.event_type),
                    xrc_event ? "true" : "false");
                break;
            case IBV_EVENT_COMM_EST:
            case IBV_EVENT_PORT_ACTIVE:
            case IBV_EVENT_SQ_DRAINED:
            case IBV_EVENT_LID_CHANGE:
            case IBV_EVENT_PKEY_CHANGE:
            case IBV_EVENT_SM_CHANGE:
            case IBV_EVENT_QP_LAST_WQE_REACHED:
#if HAVE_DECL_IBV_EVENT_CLIENT_REREGISTER
            case IBV_EVENT_CLIENT_REREGISTER:
#endif
            case IBV_EVENT_SRQ_LIMIT_REACHED:
                break;
            default:
                opal_show_help("help-mpi-btl-openib.txt", "of unknown event",
                        true,orte_process_info.nodename, orte_process_info.pid,
                        event.event_type, xrc_event ? "true" : "false");
        }
        ibv_ack_async_event(&event);
    } else {
        /* the hca == NULL , we failed to locate the HCA
         * this failure should not never happed */
        BTL_ERROR(("Failed to find HCA with FD %d."
                    "Fatal error, stoping asyn event thread"
                    ,hcas_poll->async_pollfd[index].fd));
        return OMPI_ERROR;
    }
    return OMPI_SUCCESS;
}

/* This Async event thread is handling all async event of
 * all btls/hcas in openib component
 */
void* btl_openib_async_thread(void * async)
{
    int rc;
    int i;
    struct mca_btl_openib_async_poll hcas_poll;

    if (OMPI_SUCCESS != btl_openib_async_poll_init(&hcas_poll)) {
        BTL_ERROR(("Fatal error, stoping asyn event thread"));
        pthread_exit(&return_status);
    }

    while(1) {
        rc = poll(hcas_poll.async_pollfd, hcas_poll.active_poll_size, -1);
        if (rc < 0) {
            if (errno != EINTR) {
                BTL_ERROR(("Poll failed.Fatal error, stoping asyn event thread"));
                pthread_exit(&return_status);
            } else {
                /* EINTR - we got interupt */
                continue;
            }
        }
        for(i = 0; i < hcas_poll.active_poll_size; i++) {
            switch (hcas_poll.async_pollfd[i].revents) {
                case 0:
                    /* no events */
                    break;
                case POLLIN:
                    /* Processing our event */
                    if (0 == i) {
                        /* 0 poll we use for comunication with main thread */
                        if (OMPI_SUCCESS != btl_openib_async_commandh(&hcas_poll)) {
                            free(hcas_poll.async_pollfd);
                            BTL_ERROR(("Failed to process async thread process."
                                        "Fatal error, stoping asyn event thread"));
                            pthread_exit(&return_status);
                        }
                    } else {
                        /* We get hca event */
                        if (btl_openib_async_hcah(&hcas_poll, i)) {
                            free(hcas_poll.async_pollfd);
                            BTL_ERROR(("Failed to process async thread process."
                                        "Fatal error, stoping asyn event thread"));
                            pthread_exit(&return_status);
                        }
                    }
                    break;
                default:
                    /* Get event other than POLLIN
                     * this case should not never happend */
                    BTL_ERROR(("Got unexpected event %d."
                                "Fatal error, stoping asyn event thread"
                                ,hcas_poll.async_pollfd[i].revents));
                    free(hcas_poll.async_pollfd);
                    pthread_exit(&return_status);
            }
        }
    }
    return PTHREAD_CANCELED;
}

static void apm_update_attr(struct ibv_qp_attr *attr, enum ibv_qp_attr_mask *mask)
{
    *mask = IBV_QP_ALT_PATH|IBV_QP_PATH_MIG_STATE;
    attr->alt_ah_attr.dlid = attr->ah_attr.dlid + 1;
    attr->alt_ah_attr.src_path_bits = attr->ah_attr.src_path_bits + 1;
    attr->alt_ah_attr.static_rate = attr->ah_attr.static_rate;
    attr->alt_ah_attr.sl = attr->ah_attr.sl;
    attr->alt_pkey_index = attr->pkey_index;
    attr->alt_port_num = attr->port_num;
    attr->alt_timeout = attr->timeout;
    attr->path_mig_state = IBV_MIG_REARM;
    BTL_VERBOSE(("New APM LMC loaded: alt_src_port:%d, dlid: %d, src_bits %d, old_src_bits: %d, old_dlid %d",
                attr->alt_port_num, attr->alt_ah_attr.dlid,
                attr->alt_ah_attr.src_path_bits, attr->ah_attr.src_path_bits, attr->ah_attr.dlid));
}

static int apm_update_port(mca_btl_openib_endpoint_t *ep,
        struct ibv_qp_attr *attr, enum ibv_qp_attr_mask *mask)
{
    size_t port_i;
    uint16_t apm_lid = 0;

    if (attr->port_num == ep->endpoint_btl->apm_port) {
        /* all ports were used */
        BTL_ERROR(("APM: already all ports were used port_num %d apm_port %d",
                    attr->port_num, ep->endpoint_btl->apm_port));
        return OMPI_ERROR;
    }
    /* looking for alternatve lid on remote site */
    for(port_i = 0; port_i < ep->endpoint_proc->proc_port_count; port_i++) {
        if (ep->endpoint_proc->proc_ports[port_i].lid == attr->ah_attr.dlid - mca_btl_openib_component.apm_lmc) {
            apm_lid = ep->endpoint_proc->proc_ports[port_i].apm_lid;
        }
    }
    if (0 == apm_lid) {
        /* APM was disabled on one of site ? */
        BTL_VERBOSE(("APM: Was disabled ? dlid %d %d %d", attr->ah_attr.dlid, attr->ah_attr.src_path_bits, ep->endpoint_btl->src_path_bits));
        return OMPI_ERROR;
    }
    /* We guess cthat the LMC is the same on all ports */
    attr->alt_ah_attr.static_rate = attr->ah_attr.static_rate;
    attr->alt_ah_attr.sl = attr->ah_attr.sl;
    attr->alt_pkey_index = attr->pkey_index;
    attr->alt_timeout = attr->timeout;
    attr->path_mig_state = IBV_MIG_REARM;
    *mask = IBV_QP_ALT_PATH|IBV_QP_PATH_MIG_STATE;

    attr->alt_port_num = ep->endpoint_btl->apm_port;
    attr->alt_ah_attr.src_path_bits = ep->endpoint_btl->src_path_bits;
    attr->alt_ah_attr.dlid = apm_lid;

    BTL_VERBOSE(("New APM port loaded: alt_src_port:%d, dlid: %d, src_bits: %d:%d, old_dlid %d",
                attr->alt_port_num, attr->alt_ah_attr.dlid,
                attr->ah_attr.src_path_bits, attr->alt_ah_attr.src_path_bits,
                attr->ah_attr.dlid));
    return OMPI_SUCCESS;
}

/* Load new dlid to the QP */
void mca_btl_openib_load_apm(struct ibv_qp *qp, mca_btl_openib_endpoint_t *ep)
{
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_qp_attr attr;
    enum ibv_qp_attr_mask mask = 0;
    struct mca_btl_openib_module_t *btl;

    BTL_VERBOSE(("APM: Loading alternative path"));
    assert (NULL != ep);
    btl = ep->endpoint_btl;

    if (ibv_query_qp(qp, &attr, mask, &qp_init_attr))
        BTL_ERROR(("Failed to ibv_query_qp, qp num: %d", qp->qp_num));

    if (mca_btl_openib_component.apm_lmc &&
            attr.ah_attr.src_path_bits - btl->src_path_bits < mca_btl_openib_component.apm_lmc) {
        BTL_VERBOSE(("APM LMC: src: %d btl_src: %d lmc_max: %d",
                    attr.ah_attr.src_path_bits,
                    btl->src_path_bits,
                    mca_btl_openib_component.apm_lmc));
        apm_update_attr(&attr, &mask);
    } else {
        if (mca_btl_openib_component.apm_ports) {
            /* Try to migrate to next port */
            if (OMPI_SUCCESS != apm_update_port(ep, &attr, &mask))
                return;
        } else {
            BTL_ERROR(("Failed to load alternative path, all %d were used",
                        attr.ah_attr.src_path_bits - btl->src_path_bits));
        }
    }

    if (ibv_modify_qp(qp, &attr, mask))
        BTL_ERROR(("Failed to ibv_query_qp, qp num: %p, errno says: %s (%d)"
                    ,qp->qp_num ,strerror(errno), errno));
}

void mca_btl_openib_load_apm_xrc_rcv(uint32_t qp_num, mca_btl_openib_endpoint_t *ep)
{
#if HAVE_XRC
    struct ibv_qp_init_attr qp_init_attr;
    struct ibv_qp_attr attr;
    enum ibv_qp_attr_mask mask = 0;
    struct mca_btl_openib_module_t *btl;

    BTL_VERBOSE(("APM XRC: Loading alternative path"));
    assert (NULL != ep);
    btl = ep->endpoint_btl;

    if (ibv_query_xrc_rcv_qp(btl->hca->xrc_domain, qp_num, &attr, mask, &qp_init_attr))
        BTL_ERROR(("Failed to ibv_query_qp, qp num: %d", qp_num));

    if (mca_btl_openib_component.apm_lmc &&
            attr.ah_attr.src_path_bits - btl->src_path_bits < mca_btl_openib_component.apm_lmc) {
        apm_update_attr(&attr, &mask);
    } else {
        if (mca_btl_openib_component.apm_ports) {
            /* Try to migrate to next port */
            if (OMPI_SUCCESS != apm_update_port(ep, &attr, &mask))
                return;
        } else {
            BTL_ERROR(("Failed to load alternative path, all %d were used",
                        attr.ah_attr.src_path_bits - btl->src_path_bits));
        }
    }

    ibv_modify_xrc_rcv_qp(btl->hca->xrc_domain, qp_num, &attr, mask);
    /* Maybe the qp already was modified by other process - ignoring error */
#endif
}

#endif
