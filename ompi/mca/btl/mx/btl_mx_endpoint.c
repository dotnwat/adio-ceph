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
#include <sys/time.h>
#include <time.h>
#include "include/types.h"
#include "mca/ns/base/base.h"
#include "mca/oob/base/base.h"
#include "mca/rml/rml.h"
#include "mca/errmgr/errmgr.h"
#include "dps/dps.h"
#include "btl_mx.h"
#include "btl_mx_endpoint.h" 
#include "btl_mx_proc.h"
#include "btl_mx_frag.h"


/*
 * Initialize state of the endpoint instance.
 *
 */

static void mca_btl_mx_endpoint_construct(mca_btl_base_endpoint_t* endpoint)
{
    endpoint->endpoint_btl = 0;
    endpoint->endpoint_proc = 0;
}

/*
 * Destroy a endpoint
 *
 */

static void mca_btl_mx_endpoint_destruct(mca_btl_base_endpoint_t* endpoint)
{
}


OBJ_CLASS_INSTANCE(
    mca_btl_mx_endpoint_t, 
    opal_list_item_t, 
    mca_btl_mx_endpoint_construct, 
    mca_btl_mx_endpoint_destruct);

