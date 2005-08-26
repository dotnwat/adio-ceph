/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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

#include "mpi.h"
#include "opal/mca/base/base.h"
#include "opal/mca/paffinity/base/base.h"
#include "opal/mca/maffinity/base/base.h"
#include "opal/runtime/opal_progress.h"
#include "opal/util/sys_info.h"
#include "opal/threads/threads.h"
#include "opal/util/show_help.h"
#include "opal/util/stacktrace.h"
#include "opal/runtime/opal.h"
#include "opal/event/event.h"

#include "orte/util/proc_info.h"
#include "orte/util/session_dir.h"
#include "orte/runtime/runtime.h"
#include "orte/mca/oob/oob.h"
#include "orte/mca/oob/base/base.h"
#include "orte/mca/ns/ns.h"
#include "orte/mca/ns/base/base.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/schema/schema.h"
#include "orte/mca/soh/soh.h"
#include "orte/mca/soh/base/base.h"
#include "orte/mca/errmgr/errmgr.h"

#include "ompi/include/constants.h"
#include "ompi/runtime/mpiruntime.h"
#include "ompi/runtime/params.h"
#include "ompi/communicator/communicator.h"
#include "ompi/group/group.h"
#include "ompi/info/info.h"
#include "ompi/errhandler/errcode.h"
#include "ompi/errhandler/errclass.h"
#include "ompi/request/request.h"
#include "ompi/op/op.h"
#include "ompi/file/file.h"
#include "ompi/attribute/attribute.h"
#include "ompi/mca/allocator/base/base.h"
#include "ompi/mca/allocator/allocator.h"
#include "ompi/mca/mpool/base/base.h"
#include "ompi/mca/mpool/mpool.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/pml/base/pml_base_module_exchange.h"
#include "ompi/mca/pml/base/base.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "ompi/mca/io/io.h"
#include "ompi/mca/io/base/base.h"

/*
 * Global variables and symbols for the MPI layer
 */

bool ompi_mpi_initialized = false;
bool ompi_mpi_finalized = false;

bool ompi_mpi_thread_multiple = false;
int ompi_mpi_thread_requested = MPI_THREAD_SINGLE;
int ompi_mpi_thread_provided = MPI_THREAD_SINGLE;

opal_thread_t *ompi_mpi_main_thread = NULL;

bool ompi_mpi_maffinity_setup = false;


int ompi_mpi_init(int argc, char **argv, int requested, int *provided)
{
    int ret;
    ompi_proc_t** procs;
    size_t nprocs;
    char *error = NULL;
    bool compound_cmd = false;
    
    /* Join the run-time environment - do the things that don't hit
       the registry */

    if (ORTE_SUCCESS != (ret = opal_init())) {
        error = "ompi_mpi_init: opal_init failed";
        goto error;
    }

    /* Setup ORTE stage 1 */
    
    if (ORTE_SUCCESS != (ret = orte_init_stage1())) {
        error = "ompi_mpi_init: orte_init_stage1 failed";
        goto error;
    }

    /* If we are not the seed nor a singleton, AND we have not set the
       orte_debug flag, then start recording the compound command that
       starts us up.  if we are the seed or a singleton, then don't do
       this - the registry is local, so we'll just drive it
       directly */

    if (orte_process_info.seed ||
        NULL == orte_process_info.ns_replica ||
        orte_debug_flag) {
        compound_cmd = false;
    } else {
        if (ORTE_SUCCESS != (ret = orte_gpr.begin_compound_cmd())) {
            ORTE_ERROR_LOG(ret);
            error = "ompi_mpi_init: orte_gpr.begin_compound_cmd failed";
            goto error;
        }
        compound_cmd = true;
    }

    /* Now do the things that hit the registry */

    if (ORTE_SUCCESS != (ret = orte_init_stage2())) {
        ORTE_ERROR_LOG(ret);
        error = "ompi_mpi_init: orte_init_stage2 failed";
        goto error;
    }

    /* Once we've joined the RTE, see if any MCA parameters were
       passed to the MPI level */

    if (OMPI_SUCCESS != (ret = ompi_mpi_register_params())) {
        error = "mca_mpi_register_params() failed";
        goto error;
    }

    /* Setup process affinity */

    if (ompi_mpi_paffinity_alone) {
        int param, value;
        bool set = false;
        param = mca_base_param_find("mpi", NULL, "paffinity_processor");
        if (param >= 0) {
            if (OMPI_SUCCESS == mca_base_param_lookup_int(param, &value)) {
                if (value >= 0) {
                    if (OPAL_SUCCESS == opal_paffinity_base_set(value)) {
                        set = true;
                    }
                }
            }
            if (!set) {
                char *vpid;
                orte_ns_base_get_vpid_string(&vpid, orte_process_info.my_name);
                opal_show_help("help-mpi-runtime",
                               "mpi_init:startup:paffinity-unavailable", 
                               true, vpid);
                free(vpid);
            }

            /* If we were able to set processor affinity, try setting
               up memory affinity */

            else {
                if (OPAL_SUCCESS == opal_maffinity_base_open() &&
                    OPAL_SUCCESS == opal_maffinity_base_select()) {
                    ompi_mpi_maffinity_setup = true;
                }
            }
        }
    }

#ifndef WIN32
    if (OMPI_SUCCESS != (ret = opal_util_register_stackhandlers ())) {
        error = "util_register_stackhandlers() failed";
        goto error;
    }
#endif

    /* initialize datatypes. This step should be done early as it will
     * create the local convertor and local arch used in the proc
     * init.
     */
    if (OMPI_SUCCESS != (ret = ompi_ddt_init())) {
        error = "ompi_ddt_init() failed";
        goto error;
    }

    /* Initialize OMPI procs */
    if (OMPI_SUCCESS != (ret = ompi_proc_init())) {
        error = "mca_proc_init() failed";
        goto error;
    }

    /* initialize the progress engine for MPI functionality */
    if (OMPI_SUCCESS != opal_progress_mpi_init()) {
        error = "opal_progress_mpi_init() failed";
        goto error;
    }

    /* Open up MPI-related MCA components */

    if (OMPI_SUCCESS != (ret = mca_allocator_base_open())) {
        error = "mca_allocator_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_mpool_base_open())) {
        error = "mca_mpool_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_pml_base_open())) {
        error = "mca_pml_base_open() failed";
        goto error;
    }
    if (OMPI_SUCCESS != (ret = mca_coll_base_open())) {
        error = "mca_coll_base_open() failed";
        goto error;
    }

    /* In order to reduce the common case for MPI apps (where they
       don't use MPI-2 IO or MPI-1 topology functions), the io and
       topo frameworks are initialized lazily, at the first use of
       relevant functions (e.g., MPI_FILE_*, MPI_CART_*, MPI_GRAPH_*),
       so they are not opened here. */

    /* Initialize module exchange */

    if (OMPI_SUCCESS != (ret = mca_pml_base_modex_init())) {
        error = "mca_pml_base_modex_init() failed";
        goto error;
    }

    /* Select which MPI components to use */

    if (OMPI_SUCCESS != 
        (ret = mca_mpool_base_init(OMPI_ENABLE_PROGRESS_THREADS,
                                   OMPI_ENABLE_MPI_THREADS))) {
        error = "mca_mpool_base_init() failed";
        goto error;
    }

    if (OMPI_SUCCESS != 
        (ret = mca_pml_base_select(OMPI_ENABLE_PROGRESS_THREADS,
                                   OMPI_ENABLE_MPI_THREADS))) {
        error = "mca_pml_base_select() failed";
        goto error;
    }

    if (OMPI_SUCCESS != 
        (ret = mca_coll_base_find_available(OMPI_ENABLE_PROGRESS_THREADS,
                                            OMPI_ENABLE_MPI_THREADS))) {
        error = "mca_coll_base_find_available() failed";
        goto error;
    }

    /* io and topo components are not selected here -- see comment
       above about the io and topo frameworks being loaded lazily */

    /* Initialize each MPI handle subsystem */
    /* initialize requests */
    if (OMPI_SUCCESS != (ret = ompi_request_init())) {
        error = "ompi_request_init() failed";
        goto error;
    }

    /* initialize info */
    if (OMPI_SUCCESS != (ret = ompi_info_init())) {
        error = "ompi_info_init() failed";
        goto error;
    }

    /* initialize error handlers */
    if (OMPI_SUCCESS != (ret = ompi_errhandler_init())) {
        error = "ompi_errhandler_init() failed";
        goto error;
    }

    /* initialize error codes */
    if (OMPI_SUCCESS != (ret = ompi_mpi_errcode_init())) {
        error = "ompi_mpi_errcode_init() failed";
        goto error;
    }

    /* initialize error classes */
    if (OMPI_SUCCESS != (ret = ompi_errclass_init())) {
        error = "ompi_errclass_init() failed";
        goto error;
    }
    
    /* initialize internal error codes */
    if (OMPI_SUCCESS != (ret = ompi_errcode_intern_init())) {
        error = "ompi_errcode_intern_init() failed";
        goto error;
    }
     
    /* initialize groups  */
    if (OMPI_SUCCESS != (ret = ompi_group_init())) {
        error = "ompi_group_init() failed";
        goto error;
    }

    /* initialize communicators */
    if (OMPI_SUCCESS != (ret = ompi_comm_init())) {
        error = "ompi_comm_init() failed";
        goto error;
    }

    /* initialize ops */
    if (OMPI_SUCCESS != (ret = ompi_op_init())) {
        error = "ompi_op_init() failed";
        goto error;
    }

    /* initialize file handles */
    if (OMPI_SUCCESS != (ret = ompi_file_init())) {
        error = "ompi_file_init() failed";
        goto error;
    }

    /* initialize attribute meta-data structure for comm/win/dtype */
    if (OMPI_SUCCESS != (ret = ompi_attr_init())) {
        error = "ompi_attr_init() failed";
        goto error;
    }
    /* do module exchange */
    if (OMPI_SUCCESS != (ret = mca_pml_base_modex_exchange())) {
        error = "mca_pml_base_modex_exchange() failed";
        goto error;
    }

    /* store our process info on registry */
    if (ORTE_SUCCESS != (ret = orte_schema.store_my_info())) {
        ORTE_ERROR_LOG(ret);
        error = "could not store my info on registry";
        goto error;
    }
    
    /* Let system know we are at STG1 Barrier */
    if (ORTE_SUCCESS != (ret = orte_soh.set_proc_soh(orte_process_info.my_name,
                                ORTE_PROC_STATE_AT_STG1, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "set process state failed";
        goto error;
    }

    /* if the compound command is operative, execute it */

    if (compound_cmd) {
        if (OMPI_SUCCESS != (ret = orte_gpr.exec_compound_cmd())) {
            ORTE_ERROR_LOG(ret);
            error = "ompi_rte_init: orte_gpr.exec_compound_cmd failed";
            goto error;
        }
    }

     /* FIRST BARRIER - WAIT FOR MSG FROM RMGR_PROC_STAGE_GATE_MGR TO ARRIVE */
    if (ORTE_SUCCESS != (ret = orte_rml.xcast(NULL, NULL, 0, NULL, NULL))) {
        ORTE_ERROR_LOG(ret);
        error = "ompi_mpi_init: failed to see all procs register\n";
        goto error;
    }

    /* start PTL's */
    ret = MCA_PML_CALL(enable(true));
    if( OMPI_SUCCESS != ret ) {
        error = "PML control failed";
        goto error;
    }

    /* add all ompi_proc_t's to PML */
    if (NULL == (procs = ompi_proc_world(&nprocs))) {
        error = "ompi_proc_world() failed";
        goto error;
    }
    ret = MCA_PML_CALL(add_procs(procs, nprocs));
    free(procs);
    if( OMPI_SUCCESS != ret ) {
        error = "PML add procs failed";
        goto error;
    }

    MCA_PML_CALL(add_comm(&ompi_mpi_comm_world));
    MCA_PML_CALL(add_comm(&ompi_mpi_comm_self));

    /* Figure out the final MPI thread levels.  If we were not
       compiled for support for MPI threads, then don't allow
       MPI_THREAD_MULTIPLE. */

    ompi_mpi_thread_requested = requested;
    if (OMPI_HAVE_THREAD_SUPPORT == 0) {
        ompi_mpi_thread_provided = *provided = MPI_THREAD_SINGLE;
        ompi_mpi_main_thread = NULL;
    } else if (OMPI_ENABLE_MPI_THREADS == 1) {
        ompi_mpi_thread_provided = *provided = requested;
        ompi_mpi_main_thread = opal_thread_get_self();
    } else {
        if (MPI_THREAD_MULTIPLE == requested) {
            ompi_mpi_thread_provided = *provided = MPI_THREAD_SERIALIZED;
        } else {
            ompi_mpi_thread_provided = *provided = requested;
        }
        ompi_mpi_main_thread = opal_thread_get_self();
    }

    ompi_mpi_thread_multiple = (ompi_mpi_thread_provided == 
                                MPI_THREAD_MULTIPLE);
    if (OMPI_ENABLE_PROGRESS_THREADS == 1 ||
        OMPI_ENABLE_MPI_THREADS == 1) {
        opal_set_using_threads(true);
    }

    /* Init coll for the comms */

    if (OMPI_SUCCESS != 
        (ret = mca_coll_base_comm_select(MPI_COMM_SELF, NULL))) {
        error = "mca_coll_base_comm_select(MPI_COMM_SELF) failed";
        goto error;
    }

    if (OMPI_SUCCESS !=
        (ret = mca_coll_base_comm_select(MPI_COMM_WORLD, NULL))) {
        error = "mca_coll_base_comm_select(MPI_COMM_WORLD) failed";
        goto error;
    }

#if OMPI_ENABLE_PROGRESS_THREADS && 0
    /* BWB - XXX - FIXME - is this actually correct? */
    /* setup I/O forwarding */
    if (orte_process_info.seed == false) {
        if (OMPI_SUCCESS != (ret = ompi_mpi_init_io())) {
            error = "ompi_rte_init_io failed";
            goto error;
        }
    }
#endif

    /*
     * Dump all MCA parameters if requested
     */
    if (ompi_mpi_show_mca_params) {
       ompi_show_all_mca_params(ompi_mpi_comm_world.c_my_rank, 
                                nprocs, 
                                orte_system_info.nodename);
    }

    /* Let system know we are at STG2 Barrier */

    if (ORTE_SUCCESS != (ret = orte_soh.set_proc_soh(orte_process_info.my_name,
                                ORTE_PROC_STATE_AT_STG2, 0))) {
        ORTE_ERROR_LOG(ret);
        error = "set process state failed";
        goto error;
    }

     /* BWB - is this still needed? */
#if OMPI_ENABLE_PROGRESS_THREADS == 0
    opal_progress_events(OPAL_EVLOOP_NONBLOCK);
#endif

    /* Second barrier -- wait for message from
       RMGR_PROC_STAGE_GATE_MGR to arrive */

    if (ORTE_SUCCESS != (ret = orte_rml.xcast(NULL, NULL, 0, NULL, NULL))) {
        ORTE_ERROR_LOG(ret);
        error = "ompi_mpi_init: failed to see all procs register\n";
        goto error;
    }

    /* new very last step: check whether we have been spawned or not.
       We introduce that at the very end, since we need collectives,
       datatypes, ptls etc. up and running here.... */

    if (OMPI_SUCCESS != (ret = ompi_comm_dyn_init())) {
        error = "ompi_comm_dyn_init() failed";
        goto error;
    }

 error:
    if (ret != OMPI_SUCCESS) {
        opal_show_help("help-mpi-runtime",
                       "mpi_init:startup:internal-failure", true,
                       "MPI_INIT", "MPI_INIT", error, ret);
        return ret;
    }

    /* put the event library in "high performance MPI mode" */
    if (OMPI_SUCCESS != opal_progress_mpi_enable()) {
        error = "opal_progress_mpi_enable() failed";
        goto error;
    }

    /* All done.  Wasn't that simple? */

    ompi_mpi_initialized = true;

    if (orte_debug_flag) {
        opal_output(0, "[%lu,%lu,%lu] ompi_mpi_init completed",
                    ORTE_NAME_ARGS(orte_process_info.my_name));
    }

    return MPI_SUCCESS;
}
