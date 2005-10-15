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
 *
 * These symbols are in a file by themselves to provide nice linker
 * semantics.  Since linkers generally pull in symbols by object
 * files, keeping these symbols as the only symbols in this file
 * prevents utility programs such as "ompi_info" from having to import
 * entire components just to query their version and parameters.
 */

#include "ompi_config.h"
#include "coll_hierarch.h"

#include "mpi.h"
#include "mca/coll/coll.h"

/*
 * Public string showing the coll ompi_hierarch component version number
 */
const char *mca_coll_hierarch_component_version_string =
  "OMPI/MPI hierarch collective MCA component version " OMPI_VERSION;

/*
 * Global variable
 */
int mca_coll_hierarch_priority_param = -1;
int mca_coll_hierarch_verbose_param = -1;
int mca_coll_hierarch_use_rdma_param=-1;   
int mca_coll_hierarch_ignore_sm_param=-1;   


/*
 * Local function
 */
static int hierarch_open(void);

/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

const mca_coll_base_component_1_0_0_t mca_coll_hierarch_component = {

  /* First, the mca_component_t struct containing meta information
     about the component itself */

  {
    /* Indicate that we are a coll v1.0.0 component (which also implies a
       specific MCA version) */

    MCA_COLL_BASE_VERSION_1_0_0,

    /* Component name and version */

    "hierarch",
    OMPI_MAJOR_VERSION,
    OMPI_MINOR_VERSION,
    OMPI_RELEASE_VERSION,

    /* Component open and close functions */

    hierarch_open,
    NULL
  },

  /* Next the MCA v1.0.0 component meta data */

  {
   /* Whether the component is checkpointable or not */
   true
  },

  /* Initialization / querying functions */
  mca_coll_hierarch_init_query,
  mca_coll_hierarch_comm_query,
  mca_coll_hierarch_comm_unquery
};


static int hierarch_open(void)
{
    /* Use a high priority, but allow other components to be higher */
    
    mca_coll_hierarch_priority_param = 
      mca_base_param_register_int("coll", "hierarch", "priority", NULL, 50);
    mca_coll_hierarch_verbose_param = 
      mca_base_param_register_int("coll", "hierarch", "verbose", NULL, 0);
    mca_coll_hierarch_use_rdma_param = 
        mca_base_param_register_int("coll", "hierarch", "use_rdma", NULL, 0);
    mca_coll_hierarch_ignore_sm_param = 
        mca_base_param_register_int("coll", "hierarch", "ignore_sm", NULL, 0);

    return OMPI_SUCCESS;
}
