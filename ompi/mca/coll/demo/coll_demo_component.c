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

#include "mpi.h"
#include "ompi/mca/coll/coll.h"
#include "coll-demo-version.h"
#include "coll_demo.h"

/*
 * Public string showing the coll ompi_demo component version number
 */
const char *mca_coll_demo_component_version_string =
  "OMPI/MPI demo collective MCA component version " OMPI_VERSION;

/*
 * Global variable
 */
int mca_coll_demo_priority_param = -1;
int mca_coll_demo_verbose_param = -1;
int mca_coll_demo_verbose = 0;

/*
 * Local function
 */
static int demo_open(void);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

const mca_coll_base_component_1_0_0_t mca_coll_demo_component = {

    /* First, the mca_component_t struct containing meta information
       about the component itself */

    {
        /* Indicate that we are a coll v1.0.0 component (which also
           implies a specific MCA version) */

        MCA_COLL_BASE_VERSION_1_0_0,

        /* Component name and version */

        "demo",
        OMPI_MAJOR_VERSION,
        OMPI_MINOR_VERSION,
        OMPI_RELEASE_VERSION,

        /* Component open and close functions */

        demo_open,
        NULL
    },

    /* Next the MCA v1.0.0 component meta data */

    {
        /* The component is checkpoint ready */
        MCA_BASE_METADATA_PARAM_CHECKPOINT
    },
    
    /* Initialization / querying functions */
    
    mca_coll_demo_init_query,
    mca_coll_demo_comm_query,
    NULL
};


static int demo_open(void)
{
    mca_coll_demo_priority_param = 
        mca_base_param_register_int("coll", "demo", "priority", NULL, 20);
    mca_coll_demo_verbose_param = 
        mca_base_param_register_int("coll", "demo", "verbose", NULL, 
                                    mca_coll_demo_verbose);

    return OMPI_SUCCESS;
}
