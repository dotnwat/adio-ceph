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

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <numa.h>

#include "opal/include/constants.h"
#include "opal/mca/maffinity/maffinity.h"
#include "opal/mca/maffinity/base/base.h"
#include "maffinity_libnuma.h"


/*
 * Local functions
 */
static int libnuma_module_init(void);
static int libnuma_module_set(opal_maffinity_base_segment_t *segments,
                                size_t num_segments, bool am_allocator);

/*
 * Libnuma maffinity module
 */
static const opal_maffinity_base_module_1_0_0_t module = {

    /* Initialization function */

    libnuma_module_init,

    /* Module function pointers */

    libnuma_module_set
};


const opal_maffinity_base_module_1_0_0_t *
opal_maffinity_libnuma_component_query(int *query)
{
    int param;

    if (-1 == numa_available()) {
        return NULL;
    }
    param = mca_base_param_find("maffinity", "libnuma", "priority");
    mca_base_param_lookup_int(param, query);

    return &module;
}


static int libnuma_module_init(void)
{
    /* Tell libnuma that we want all memory affinity to be local. */

    numa_set_localalloc();

    return OPAL_SUCCESS;
}


static int libnuma_module_set(opal_maffinity_base_segment_t *segments,
                              size_t num_segments, bool am_allocator)
{
    size_t i;

    /* Only the allocator does anything */

    if (!am_allocator) {
        return OPAL_SUCCESS;
    }

    for (i = 0; i < num_segments; ++i) {
        /* JMS do something */
    }

    return OPAL_SUCCESS;
}
