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

#include "ompi/include/constants.h"
#include "ompi/communicator/communicator.h"
#include "ompi/mca/coll/coll.h"
#include "opal/util/show_help.h"
#include "coll_sm.h"


/*
 * Public string showing the coll ompi_sm component version number
 */
const char *mca_coll_sm_component_version_string =
    "Open MPI sm collective MCA component version " OMPI_VERSION;


/*
 * Local functions
 */

static int sm_open(void);
static int sm_close(void);


/*
 * Instantiate the public struct with all of our public information
 * and pointers to our public functions in it
 */

mca_coll_sm_component_t mca_coll_sm_component = {

    /* First, fill in the super (mca_coll_base_component_1_0_0_t) */

    {
        /* First, the mca_component_t struct containing meta
           information about the component itself */
        
        {
            /* Indicate that we are a coll v1.0.0 component (which
               also implies a specific MCA version) */

            MCA_COLL_BASE_VERSION_1_0_0,

            /* Component name and version */

            "sm",
            OMPI_MAJOR_VERSION,
            OMPI_MINOR_VERSION,
            OMPI_RELEASE_VERSION,

            /* Component open and close functions */

            sm_open,
            sm_close,
        },
        
        /* Next the MCA v1.0.0 component meta data */

        {
            /* Whether the component is checkpointable or not */
            
            true
        },

        /* Initialization / querying functions */
        
        mca_coll_sm_init_query,
        mca_coll_sm_comm_query,
        mca_coll_sm_comm_unquery,
    },

    /* sm-component specifc information */

    /* (default) priority */
    75,

    /* (default) control unit size (bytes) */
    64,

    /* (default) bootstrap filename */
    "coll-sm-bootstrap",

    /* (default) number of segments in bootstrap file */
    8,

    /* (default) mpool name to use */
    "sm",

    /* (default) number of segments for each communicator in the mpool
       area */
    2,

    /* (default) fragment size */
    8192,

    /* (default) degree of tree for tree-based operations (must be <=
       control unit size) */
    4,

    /* default values for non-MCA parameters */
    0, /* bootstrap size -- filled in below */
    0, /* mpool data size -- filled in below */
    NULL, /* data mpool pointer */
    false, /* whether this process created the data mpool */
    NULL /* pointer to meta data about bootstrap area */
};


/*
 * Open the component
 */
static int sm_open(void)
{
    size_t size1, size2;
    mca_base_component_t *c = &mca_coll_sm_component.super.collm_version;
    mca_coll_sm_component_t *cs = &mca_coll_sm_component;

    /* If we want to be selected (i.e., all procs on one node), then
       we should have a high priority */

    mca_base_param_reg_int(c, "priority", 
                           "Priority of the sm coll component",
                           false, false, 
                           cs->sm_priority,
                           &cs->sm_priority);

    mca_base_param_reg_int(c, "control_size",
                           "Length of the control data -- should usually be either a cache line on most SMPs, or a page on machine where pages that support direct memory affinity placement (in bytes)",
                           false, false,
                           cs->sm_control_size,
                           &cs->sm_control_size);

    mca_base_param_reg_string(c, "bootstrap_filename", 
                              "Filename (in the Open MPI session directory) of the coll sm component bootstrap rendezvous mmap file",
                              false, false,
                              cs->sm_bootstrap_filename,
                              &cs->sm_bootstrap_filename);

    mca_base_param_reg_int(c, "bootstrap_num_segments",
                           "Number of segments in the bootstrap file",
                           false, false,
                           cs->sm_bootstrap_num_segments,
                           &cs->sm_bootstrap_num_segments);

    mca_base_param_reg_int(c, "fragment_size",
                           "Fragment size (in bytes) used for passing data through shared memory (will be rounded up to the nearest control_size size)",
                           false, false,
                           cs->sm_fragment_size,
                           &cs->sm_fragment_size);
    if (0 != (cs->sm_fragment_size % cs->sm_control_size)) {
        cs->sm_fragment_size += cs->sm_control_size - 
            (cs->sm_fragment_size % cs->sm_control_size);
    }

    mca_base_param_reg_string(c, "mpool", 
                              "Name of the mpool component to use",
                              false, false,
                              cs->sm_mpool_name,
                              &cs->sm_mpool_name);
    
    mca_base_param_reg_int(c, "communicator_num_segments",
                           "Number of shared memory collective segments on each communicator (must be >= 2)",
                           false, false,
                           cs->sm_communicator_num_segments,
                           &cs->sm_communicator_num_segments);
    if (cs->sm_communicator_num_segments < 2) {
        cs->sm_communicator_num_segments = 2;
    }

    mca_base_param_reg_int(c, "tree_degree",
                           "Degree of the tree for tree-based operations (must be <= control size and <= 255)",
                           false, false,
                           cs->sm_tree_degree,
                           &cs->sm_tree_degree);
    if (cs->sm_tree_degree > cs->sm_control_size) {
        opal_show_help("help-coll-sm.txt", 
                       "tree-degree-larger-than-control", true,
                       cs->sm_tree_degree, cs->sm_control_size);
        cs->sm_tree_degree = cs->sm_control_size;
    }
    if (cs->sm_tree_degree > 255) {
        opal_show_help("help-coll-sm.txt", 
                       "tree-degree-larger-than-255", true,
                       cs->sm_tree_degree);
        cs->sm_tree_degree = 255;
    }

    /* Size of the bootstrap shared memory area. */

    size1 = 
        sizeof(mca_coll_sm_bootstrap_header_extension_t) +
        (mca_coll_sm_component.sm_bootstrap_num_segments *
         sizeof(mca_coll_sm_bootstrap_comm_setup_t)) +
        (sizeof(uint32_t) * mca_coll_sm_component.sm_bootstrap_num_segments);
    mca_base_param_reg_int(c, "shared_mem_used_bootstrap",
                           "Amount of shared memory used in the shared memory bootstrap area (in bytes)",
                           false, true,
                           size1, NULL);

    /* Calculate how much space we need in the data mpool.  There are
       several values to add (one of these for each segment):

       - size of the control data:
           - fan-in data (num_procs * control_size size)
           - fan-out data (num_procs * control_size size)
       - size of message data
           - fan-in data (num_procs * (frag_size rounded up to
             control_size size))
           - fan-out data (num_procs * (frag_size rounded up
             to control_size size))

       So it's:

           num_segs * ((num_procs * control_size * 2) + (num_procs * frag * 2))

       Which reduces to:

           num_segs * num_procs * 2 * (control_size + frag)

       For this example, assume num_procs = 1.
    */

    size2 = cs->sm_communicator_num_segments * 2 *
        (cs->sm_control_size + cs->sm_fragment_size);
    mca_base_param_reg_int(c, "shared_mem_used_data",
                           "Amount of shared memory used in the shared memory data area for one process (in bytes)",
                           false, true,
                           size2, NULL);

    return OMPI_SUCCESS;
}


/*
 * Close the component
 */
static int sm_close(void)
{
    if (NULL != mca_coll_sm_component.sm_mpool_name) {
        free(mca_coll_sm_component.sm_mpool_name);
        mca_coll_sm_component.sm_mpool_name = NULL;
    }

    mca_coll_sm_bootstrap_finalize();

    return OMPI_SUCCESS;
}
