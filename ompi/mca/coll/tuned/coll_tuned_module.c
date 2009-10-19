/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2009 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2008      Sun Microsystems, Inc.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"
#include "coll_tuned.h"

#include <stdio.h>

#include "mpi.h"
#include "ompi/communicator/communicator.h"
#include "opal/mca/base/mca_base_param.h"
#include "ompi/mca/coll/coll.h"
#include "ompi/mca/coll/base/base.h"
#include "coll_tuned.h"
#include "coll_tuned_topo.h"
#include "coll_tuned_dynamic_rules.h"
#include "coll_tuned_dynamic_file.h"

static int tuned_module_enable(mca_coll_base_module_t *module,
			       struct ompi_communicator_t *comm);
/*
 * Initial query function that is invoked during MPI_INIT, allowing
 * this component to disqualify itself if it doesn't support the
 * required level of thread support.
 */
int ompi_coll_tuned_init_query(bool enable_progress_threads,
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
ompi_coll_tuned_comm_query(struct ompi_communicator_t *comm, int *priority)
{
    mca_coll_tuned_module_t *tuned_module;

    OPAL_OUTPUT((ompi_coll_tuned_stream, "coll:tuned:module_tuned query called"));

    /**
     * No support for inter-communicator yet.
     */
    if (OMPI_COMM_IS_INTER(comm)) {
        *priority = 0;
        return NULL;
    }

    /**
     * If it is inter-communicator and size is less than 2 we have specialized modules
     * to handle the intra collective communications.
     */
    if (OMPI_COMM_IS_INTRA(comm) && ompi_comm_size(comm) < 2) {
        *priority = 0;
        return NULL;
    }

    tuned_module = OBJ_NEW(mca_coll_tuned_module_t);
    if (NULL == tuned_module) return NULL;

    *priority = ompi_coll_tuned_priority;

    /* 
     * Choose whether to use [intra|inter] decision functions 
     * and if using fixed OR dynamic rule sets.
     * Right now you cannot mix them, maybe later on it can be changed
     * but this would probably add an extra if and funct call to the path
     */
    tuned_module->super.coll_module_enable = tuned_module_enable;
    tuned_module->super.ft_event = mca_coll_tuned_ft_event;

    /* By default stick with the fied version of the tuned collectives. Later on,
     * when the module get enabled, set the correct version based on the availability
     * of the dynamic rules.
     */
    tuned_module->super.coll_allgather  = ompi_coll_tuned_allgather_intra_dec_fixed;
    tuned_module->super.coll_allgatherv = ompi_coll_tuned_allgatherv_intra_dec_fixed;
    tuned_module->super.coll_allreduce  = ompi_coll_tuned_allreduce_intra_dec_fixed;
    tuned_module->super.coll_alltoall   = ompi_coll_tuned_alltoall_intra_dec_fixed;
    tuned_module->super.coll_alltoallv  = ompi_coll_tuned_alltoallv_intra_dec_fixed;
    tuned_module->super.coll_alltoallw  = NULL;
    tuned_module->super.coll_barrier    = ompi_coll_tuned_barrier_intra_dec_fixed;
    tuned_module->super.coll_bcast      = ompi_coll_tuned_bcast_intra_dec_fixed;
    tuned_module->super.coll_exscan     = NULL;
    tuned_module->super.coll_gather     = ompi_coll_tuned_gather_intra_dec_fixed;
    tuned_module->super.coll_gatherv    = NULL;
    tuned_module->super.coll_reduce     = ompi_coll_tuned_reduce_intra_dec_fixed;
    tuned_module->super.coll_reduce_scatter = ompi_coll_tuned_reduce_scatter_intra_dec_fixed;
    tuned_module->super.coll_scan       = NULL;
    tuned_module->super.coll_scatter    = ompi_coll_tuned_scatter_intra_dec_fixed;
    tuned_module->super.coll_scatterv   = NULL;

    return &(tuned_module->super);
}

/* We put all routines that handle the MCA user forced algorithm and parameter choices here */
/* recheck the setting of forced, called on module create (i.e. for each new comm) */
                                                                                                          
static int
ompi_coll_tuned_forced_getvalues( enum COLLTYPE type, 
                                  coll_tuned_force_algorithm_params_t *forced_values )
{
    coll_tuned_force_algorithm_mca_param_indices_t* mca_params;

    mca_params = &(ompi_coll_tuned_forced_params[type]);

    mca_base_param_lookup_int (mca_params->algorithm_param_index,    &(forced_values->algorithm));
    if( BARRIER != type ) {
        mca_base_param_lookup_int (mca_params->segsize_param_index,      &(forced_values->segsize));
        mca_base_param_lookup_int (mca_params->tree_fanout_param_index,  &(forced_values->tree_fanout));
        mca_base_param_lookup_int (mca_params->chain_fanout_param_index, &(forced_values->chain_fanout));
        mca_base_param_lookup_int (mca_params->max_requests_param_index, &(forced_values->max_requests));
    }
    return (MPI_SUCCESS);
}

/*
 * Init module on the communicator
 */
static int
tuned_module_enable( mca_coll_base_module_t *module,
                     struct ompi_communicator_t *comm )
{
    int size, i;
    mca_coll_tuned_module_t *tuned_module = (mca_coll_tuned_module_t *) module;
    mca_coll_tuned_comm_t *data = NULL;

    OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:module_init called."));

    /* This routine will become more complex and might have to be
     * broken into more sections/function calls
     *
     * Order of operations:
     * alloc memory for nb reqs (in case we fall through) 
     * add decision rules if using dynamic rules
     *     compact rules using communicator size info etc
     * build first guess cached topologies (might depend on the rules from above)
     *
     * then attach all to the communicator and return base module funct ptrs 
     */

    /* Allocate the data that hangs off the communicator */
    if (OMPI_COMM_IS_INTER(comm)) {
        size = ompi_comm_remote_size(comm);
    } else {
        size = ompi_comm_size(comm);
    }

    /**
     * we still malloc data as it is used by the TUNED modules
     * if we don't allocate it and fall back to a BASIC module routine then confuses debuggers 
     * we place any special info after the default data
     *
     * BUT on very large systems we might not be able to allocate all this memory so
     * we do check a MCA parameter to see if if we should allocate this memory
     *
     * The default is set very high  
     *
     */

    /* if we within the memory/size limit, allow preallocated data */
    if( size <= ompi_coll_tuned_preallocate_memory_comm_size_limit ) {
        data = (mca_coll_tuned_comm_t*)malloc(sizeof(struct mca_coll_tuned_comm_t) +
                                              (sizeof(ompi_request_t *) * size * 2));
        if (NULL == data) {
            return OMPI_ERROR;
        }
        data->mcct_reqs = (ompi_request_t **) (data + 1);
        data->mcct_num_reqs = size * 2;
    } else {
        data = (mca_coll_tuned_comm_t*)malloc(sizeof(struct mca_coll_tuned_comm_t)); 
        if (NULL == data) {
            return OMPI_ERROR;
        }
        data->mcct_reqs = (ompi_request_t **) NULL;
        data->mcct_num_reqs = 0;
    }

    /**
     * If using dynamic and you are MPI_COMM_WORLD and you want to use a parameter file..
     * then this effects how much storage space you need
     * (This is a basic version of what will go into V2)
     */

    /* if using dynamic rules make sure all overrides are NULL before we start override anything accidently */
    if (ompi_coll_tuned_use_dynamic_rules) {
        int has_dynamic_rules = 0;

        OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:module_init MCW & Dynamic"));

        /**
         * next dynamic state, recheck all forced rules as well
         * warning, we should check to make sure this is really an INTRA comm here...
         */
        ompi_coll_tuned_forced_getvalues( ALLGATHER,     &(data->user_forced[ALLGATHER]));
        ompi_coll_tuned_forced_getvalues( ALLGATHERV,    &(data->user_forced[ALLGATHERV]));
        ompi_coll_tuned_forced_getvalues( ALLREDUCE,     &(data->user_forced[ALLREDUCE]));
        ompi_coll_tuned_forced_getvalues( ALLTOALL,      &(data->user_forced[ALLTOALL]));
        ompi_coll_tuned_forced_getvalues( ALLTOALLV,     &(data->user_forced[ALLTOALLV]));
        ompi_coll_tuned_forced_getvalues( ALLTOALLW,     &(data->user_forced[ALLTOALLW]));
        ompi_coll_tuned_forced_getvalues( BARRIER,       &(data->user_forced[BARRIER]));
        ompi_coll_tuned_forced_getvalues( BCAST,         &(data->user_forced[BCAST]));
        ompi_coll_tuned_forced_getvalues( EXSCAN,        &(data->user_forced[EXSCAN]));
        ompi_coll_tuned_forced_getvalues( GATHER,        &(data->user_forced[GATHER]));
        ompi_coll_tuned_forced_getvalues( GATHERV,       &(data->user_forced[GATHERV]));
        ompi_coll_tuned_forced_getvalues( REDUCE,        &(data->user_forced[REDUCE]));
        ompi_coll_tuned_forced_getvalues( REDUCESCATTER, &(data->user_forced[REDUCESCATTER]));
        ompi_coll_tuned_forced_getvalues( SCAN,          &(data->user_forced[SCAN]));
        ompi_coll_tuned_forced_getvalues( SCATTER,       &(data->user_forced[SCATTER]));
        ompi_coll_tuned_forced_getvalues( SCATTERV,      &(data->user_forced[SCATTERV]));

        if( NULL != mca_coll_tuned_component.all_base_rules ) {
            /* extract our customized communicator sized rule set, for each collective */
            for( i = 0; i < COLLCOUNT; i++ ) {
                data->com_rules[i] = ompi_coll_tuned_get_com_rule_ptr( mca_coll_tuned_component.all_base_rules,
                                                                       i, size );
                if( NULL != data->com_rules[i] ) {
                    has_dynamic_rules++;
                }
            }
        }
        if( 0 == has_dynamic_rules ) {
            /* no real dynamic rules available. Switch back
             * to default.
             */
            ompi_coll_tuned_use_dynamic_rules = 0;
            OPAL_OUTPUT((ompi_coll_tuned_stream, "coll:tuned:module_enable switch back to fixed"
                         " decision by lack of dynamic rules"));
        } else {
            OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:module_enable using intra_dynamic"));
            tuned_module->super.coll_allgather  = ompi_coll_tuned_allgather_intra_dec_dynamic;
            tuned_module->super.coll_allgatherv = ompi_coll_tuned_allgatherv_intra_dec_dynamic;
            tuned_module->super.coll_allreduce  = ompi_coll_tuned_allreduce_intra_dec_dynamic;
            tuned_module->super.coll_alltoall   = ompi_coll_tuned_alltoall_intra_dec_dynamic;
            tuned_module->super.coll_alltoallv  = ompi_coll_tuned_alltoallv_intra_dec_dynamic;
            tuned_module->super.coll_alltoallw  = NULL;
            tuned_module->super.coll_barrier    = ompi_coll_tuned_barrier_intra_dec_dynamic;
            tuned_module->super.coll_bcast      = ompi_coll_tuned_bcast_intra_dec_dynamic;
            tuned_module->super.coll_exscan     = NULL;
            tuned_module->super.coll_gather     = ompi_coll_tuned_gather_intra_dec_dynamic;
            tuned_module->super.coll_gatherv    = NULL;
            tuned_module->super.coll_reduce     = ompi_coll_tuned_reduce_intra_dec_dynamic;
            tuned_module->super.coll_reduce_scatter = ompi_coll_tuned_reduce_scatter_intra_dec_dynamic;
            tuned_module->super.coll_scan       = NULL;
            tuned_module->super.coll_scatter    = ompi_coll_tuned_scatter_intra_dec_dynamic;
            tuned_module->super.coll_scatterv   = NULL;
        }
    }
    
    /* general n fan out tree */
    data->cached_ntree = NULL;
    /* binary tree */
    data->cached_bintree = NULL;
    /* binomial tree */
    data->cached_bmtree = NULL;
    /* binomial tree */
    data->cached_in_order_bmtree = NULL;
    /* chains (fanout followed by pipelines) */
    data->cached_chain = NULL;
    /* standard pipeline */
    data->cached_pipeline = NULL;
    /* in-order binary tree */
    data->cached_in_order_bintree = NULL;

    /* All done */
    tuned_module->tuned_data = data;

    OPAL_OUTPUT((ompi_coll_tuned_stream,"coll:tuned:module_init Tuned is in use"));
    return OMPI_SUCCESS;
}

int mca_coll_tuned_ft_event(int state) {
    if(OPAL_CRS_CHECKPOINT == state) {
        ;
    }
    else if(OPAL_CRS_CONTINUE == state) {
        ;
    }
    else if(OPAL_CRS_RESTART == state) {
        ;
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }

    return OMPI_SUCCESS;
}
