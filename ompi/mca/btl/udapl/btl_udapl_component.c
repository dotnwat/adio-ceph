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
#include "ompi/include/constants.h"
#include "opal/event/event.h"
#include "opal/util/if.h"
#include "opal/util/argv.h"
#include "opal/util/output.h"
#include "mca/pml/pml.h"
#include "mca/btl/btl.h"

#include "mca/base/mca_base_param.h"
#include "mca/pml/base/pml_base_module_exchange.h"
#include "mca/errmgr/errmgr.h"
#include "mca/mpool/base/base.h" 
#include "btl_udapl.h"
#include "btl_udapl_frag.h"
#include "btl_udapl_endpoint.h" 
#include "mca/btl/base/base.h" 
#include "datatype/convertor.h" 

mca_btl_udapl_component_t mca_btl_udapl_component = {
    {
        /* First, the mca_base_component_t struct containing meta information
           about the component itself */

        {
            /* Indicate that we are a pml v1.0.0 component (which also implies a
               specific MCA version) */

            MCA_BTL_BASE_VERSION_1_0_0,

            "udapl", /* MCA component name */
            OMPI_MAJOR_VERSION,  /* MCA component major version */
            OMPI_MINOR_VERSION,  /* MCA component minor version */
            OMPI_RELEASE_VERSION,  /* MCA component release version */
            mca_btl_udapl_component_open,  /* component open */
            mca_btl_udapl_component_close  /* component close */
        },

        /* Next the MCA v1.0.0 component meta data */

        {
            /* Whether the component is checkpointable or not */

            false
        },

        mca_btl_udapl_component_init,  
        mca_btl_udapl_component_progress,
    }
};


/*
 * utility routines for parameter registration
 */

static inline char* mca_btl_udapl_param_register_string(
                                                     const char* param_name, 
                                                     const char* default_value)
{
    char *param_value;
    int id = mca_base_param_register_string("btl","ib",param_name,NULL,default_value);
    mca_base_param_lookup_string(id, &param_value);
    return param_value;
}

static inline int mca_btl_udapl_param_register_int(
        const char* param_name, 
        int default_value)
{
    int id = mca_base_param_register_int("btl","ib",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}

/*
 *  Called by MCA framework to open the component, registers
 *  component parameters.
 */

int mca_btl_udapl_component_open(void)
{

    
    /* initialize state */
    mca_btl_udapl_component.udapl_num_btls=0;
    mca_btl_udapl_component.udapl_btls=NULL;
    
    /* initialize objects */ 
    OBJ_CONSTRUCT(&mca_btl_udapl_component.udapl_procs, opal_list_t);

    /* register uDAPL component parameters */
    mca_btl_udapl_component.udapl_free_list_num =
        mca_btl_udapl_param_register_int ("free_list_num", 8);
    mca_btl_udapl_component.udapl_free_list_max =
        mca_btl_udapl_param_register_int ("free_list_max", 1024);
    mca_btl_udapl_component.udapl_free_list_inc =
        mca_btl_udapl_param_register_int ("free_list_inc", 32);
    mca_btl_udapl_component.udapl_mpool_name = 
        mca_btl_udapl_param_register_string("mpool", "ib"); 
    mca_btl_udapl_module.super.btl_exclusivity =
        mca_btl_udapl_param_register_int ("exclusivity", 0);
    mca_btl_udapl_module.super.btl_eager_limit = 
        mca_btl_udapl_param_register_int ("first_frag_size", 64*1024) - sizeof(mca_btl_base_header_t);
    mca_btl_udapl_module.super.btl_min_send_size =
        mca_btl_udapl_param_register_int ("min_send_size", 64*1024) - sizeof(mca_btl_base_header_t);
    mca_btl_udapl_module.super.btl_max_send_size =
        mca_btl_udapl_param_register_int ("max_send_size", 128*1024) - sizeof(mca_btl_base_header_t);
    mca_btl_udapl_module.super.btl_min_rdma_size = 
        mca_btl_udapl_param_register_int("min_rdma_size", 1024*1024); 
    mca_btl_udapl_module.super.btl_max_rdma_size = 
        mca_btl_udapl_param_register_int("max_rdma_size", 1024*1024); 
    mca_btl_udapl_module.super.btl_flags  = 
        mca_btl_udapl_param_register_int("flags", MCA_BTL_FLAGS_PUT); 
    return OMPI_SUCCESS;
}

/*
 * component cleanup - sanity checking of queue lengths
 */

int mca_btl_udapl_component_close(void)
{
    return OMPI_SUCCESS;
}

/*
 *  uDAPL component initialization:
 *  (1) read interface list from kernel and compare against component parameters
 *      then create a BTL instance for selected interfaces
 *  (2) setup uDAPL listen socket for incoming connection attempts
 *  (3) register BTL parameters with the MCA
 */

mca_btl_base_module_t** mca_btl_udapl_component_init(int *num_btl_modules, 
                                                  bool enable_progress_threads,
                                                  bool enable_mpi_threads)
{
    return NULL;
}

/*
 *  uDAPL component progress.
 */


int mca_btl_udapl_component_progress()
{
    return 0;
}

