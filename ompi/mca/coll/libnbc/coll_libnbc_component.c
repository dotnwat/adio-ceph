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
 * Copyright (c) 2008      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include "coll_libnbc.h"

#include "mpi.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/communicator/communicator.h"

/*
 * Public string showing the coll ompi_libnbc component version number
 */
const char *mca_coll_libnbc_component_version_string =
    "Open MPI libnbc collective MCA component version " OMPI_VERSION;


static int libnbc_priority = 10;


static int libnbc_open(void);
static int libnbc_close(void);
static int libnbc_register(void);
static int libnbc_init_query(bool, bool);
static mca_coll_base_module_t *libnbc_comm_query(struct ompi_communicator_t *, int *);
static int libnbc_module_enable(mca_coll_base_module_t *, struct ompi_communicator_t *);
static int libnbc_progress(void);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

ompi_coll_libnbc_component_t mca_coll_libnbc_component = {
    {
        /* First, the mca_component_t struct containing meta information
         * about the component itself */
        {
            MCA_COLL_BASE_VERSION_2_0_0,

            /* Component name and version */
            "libnbc",
            OMPI_MAJOR_VERSION,
            OMPI_MINOR_VERSION,
            OMPI_RELEASE_VERSION,

            /* Component open and close functions */
            libnbc_open,
            libnbc_close,
            NULL,
            libnbc_register
        },
        {
            /* The component is checkpoint ready */
            MCA_BASE_METADATA_PARAM_CHECKPOINT
        },

        /* Initialization / querying functions */
        libnbc_init_query,
        libnbc_comm_query
    }
};


static int
libnbc_open(void)
{
    int ret;

    OBJ_CONSTRUCT(&mca_coll_libnbc_component.requests, ompi_free_list_t);
    ret = ompi_free_list_init(&mca_coll_libnbc_component.requests,
                              sizeof(ompi_coll_libnbc_request_t),
                              OBJ_CLASS(ompi_coll_libnbc_request_t),
                              0,
                              -1,
                              8,
                              NULL);
    if (OMPI_SUCCESS != ret) return ret;

    OBJ_CONSTRUCT(&mca_coll_libnbc_component.active_requests, opal_list_t);
    mca_coll_libnbc_component.active_comms = 0;

    opal_atomic_init(&mca_coll_libnbc_component.progress_lock, OPAL_ATOMIC_UNLOCKED);

    return OMPI_SUCCESS;
}

static int
libnbc_close(void)
{
    if (0 != mca_coll_libnbc_component.active_comms) {
        opal_progress_unregister(libnbc_progress);
    }

    OBJ_DESTRUCT(&mca_coll_libnbc_component.requests);
    OBJ_DESTRUCT(&mca_coll_libnbc_component.active_requests);

    return OMPI_SUCCESS;
}


static int
libnbc_register(void)
{
    /* Use a low priority, but allow other components to be lower */

    mca_base_param_reg_int(&mca_coll_libnbc_component.super.collm_version,
                           "priority",
                           "Priority of the libnbc coll component",
                           false, false, libnbc_priority,
                           &libnbc_priority);

    return OMPI_SUCCESS;
}



/*
 * Initial query function that is invoked during MPI_INIT, allowing
 * this component to disqualify itself if it doesn't support the
 * required level of thread support.
 */
static int
libnbc_init_query(bool enable_progress_threads,
                  bool enable_mpi_threads)
{
    /* Nothing to do */
    return OMPI_SUCCESS;
}


/*
 * Invoked when there's a new communicator that has been created.
 * Look at the communicator and decide which set of functions and
 * priority we want to return.
 */
mca_coll_base_module_t *
libnbc_comm_query(struct ompi_communicator_t *comm, 
                  int *priority)
{
    ompi_coll_libnbc_module_t *module;

    module = OBJ_NEW(ompi_coll_libnbc_module_t);
    if (NULL == module) return NULL;

    *priority = libnbc_priority;

    module->super.coll_module_enable = libnbc_module_enable;
    if (OMPI_COMM_IS_INTER(comm)) {
        module->super.coll_iallgather = ompi_coll_libnbc_iallgather_inter;
        module->super.coll_iallgatherv = ompi_coll_libnbc_iallgatherv_inter;
        module->super.coll_iallreduce = ompi_coll_libnbc_iallreduce_inter;
        module->super.coll_ialltoall = ompi_coll_libnbc_ialltoall_inter;
        module->super.coll_ialltoallv = ompi_coll_libnbc_ialltoallv_inter;
        module->super.coll_ialltoallw = ompi_coll_libnbc_ialltoallw_inter;
        module->super.coll_ibarrier = ompi_coll_libnbc_ibarrier_inter;
        module->super.coll_ibcast = ompi_coll_libnbc_ibcast_inter;
        module->super.coll_iexscan = NULL;
        module->super.coll_igather = ompi_coll_libnbc_igather_inter;
        module->super.coll_igatherv = ompi_coll_libnbc_igatherv_inter;
        module->super.coll_ireduce = ompi_coll_libnbc_ireduce_inter;
        module->super.coll_ireduce_scatter = ompi_coll_libnbc_ireduce_scatter_inter;
        module->super.coll_ireduce_scatter_block = ompi_coll_libnbc_ireduce_scatter_block_inter;
        module->super.coll_iscan = NULL;
        module->super.coll_iscatter = ompi_coll_libnbc_iscatter_inter;
        module->super.coll_iscatterv = ompi_coll_libnbc_iscatterv_inter;
    } else {
        module->super.coll_iallgather = ompi_coll_libnbc_iallgather;
        module->super.coll_iallgatherv = ompi_coll_libnbc_iallgatherv;
        module->super.coll_iallreduce = ompi_coll_libnbc_iallreduce;
        module->super.coll_ialltoall = ompi_coll_libnbc_ialltoall;
        module->super.coll_ialltoallv = ompi_coll_libnbc_ialltoallv;
        module->super.coll_ialltoallw = ompi_coll_libnbc_ialltoallw;
        module->super.coll_ibarrier = ompi_coll_libnbc_ibarrier;
        module->super.coll_ibcast = ompi_coll_libnbc_ibcast;
        module->super.coll_iexscan = ompi_coll_libnbc_iexscan;
        module->super.coll_igather = ompi_coll_libnbc_igather;
        module->super.coll_igatherv = ompi_coll_libnbc_igatherv;
        module->super.coll_ireduce = ompi_coll_libnbc_ireduce;
        module->super.coll_ireduce_scatter = ompi_coll_libnbc_ireduce_scatter;
        module->super.coll_ireduce_scatter_block = ompi_coll_libnbc_ireduce_scatter_block;
        module->super.coll_iscan = ompi_coll_libnbc_iscan;
        module->super.coll_iscatter = ompi_coll_libnbc_iscatter;
        module->super.coll_iscatterv = ompi_coll_libnbc_iscatterv;
    }
    module->super.ft_event = NULL;

    if (OMPI_SUCCESS != NBC_Init_comm(comm, module)) {
        OBJ_RELEASE(module);
        return NULL;
    }

    return &(module->super);
}


/*
 * Init module on the communicator
 */
static int
libnbc_module_enable(mca_coll_base_module_t *module,
                     struct ompi_communicator_t *comm)
{
    /* All done */
    if (0 == mca_coll_libnbc_component.active_comms++) {
        opal_progress_register(libnbc_progress);
    }
    return OMPI_SUCCESS;
}


static int
libnbc_progress(void)
{
    opal_list_item_t *item;

    if (opal_atomic_trylock(&mca_coll_libnbc_component.progress_lock)) return 0;

    for (item = opal_list_get_first(&mca_coll_libnbc_component.active_requests) ;
         item != opal_list_get_end(&mca_coll_libnbc_component.active_requests) ;
         item = opal_list_get_next(item)) {
        ompi_coll_libnbc_request_t* request = (ompi_coll_libnbc_request_t*) item;
        if (NBC_OK == NBC_Progress(request)) {
            /* done, remove and complete */
            item = opal_list_remove_item(&mca_coll_libnbc_component.active_requests,
                                         &request->super.super.super);

            request->super.req_status.MPI_ERROR = OMPI_SUCCESS;
            OPAL_THREAD_LOCK(&ompi_request_lock);
            ompi_request_complete(&request->super, true);
            OPAL_THREAD_UNLOCK(&ompi_request_lock);
        }
        item = opal_list_get_next(item);
    }

    opal_atomic_unlock(&mca_coll_libnbc_component.progress_lock);

    return 0;
}


static void
libnbc_module_construct(ompi_coll_libnbc_module_t *module)
{
}


static void
libnbc_module_destruct(ompi_coll_libnbc_module_t *module)
{
    if (0 == --mca_coll_libnbc_component.active_comms) {
        opal_progress_unregister(libnbc_progress);
    }
}


OBJ_CLASS_INSTANCE(ompi_coll_libnbc_module_t,
                   mca_coll_base_module_t,
                   libnbc_module_construct,
                   libnbc_module_destruct);


static int
request_cancel(struct ompi_request_t *request, int complete)
{
    return MPI_ERR_REQUEST;
}


static int
request_free(struct ompi_request_t **ompi_req)
{
    ompi_coll_libnbc_request_t *request = 
        (ompi_coll_libnbc_request_t*) *ompi_req;

    if (true != request->super.req_complete) {
        return MPI_ERR_REQUEST;
    }

    OMPI_COLL_LIBNBC_REQUEST_RETURN(request);

    *ompi_req = MPI_REQUEST_NULL;

    return OMPI_SUCCESS;
}


static void
request_construct(ompi_coll_libnbc_request_t *request)
{
    request->super.req_type = OMPI_REQUEST_COLL;
    request->super.req_status._cancelled = 0;
    request->super.req_free = request_free;
    request->super.req_cancel = request_cancel;
}


OBJ_CLASS_INSTANCE(ompi_coll_libnbc_request_t, 
                   ompi_request_t,
                   request_construct,
                   NULL);
