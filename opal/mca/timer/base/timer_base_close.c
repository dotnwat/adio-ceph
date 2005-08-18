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

#include "opal/include/constants.h"
#include "opal/mca/mca.h"
#include "opal/mca/base/base.h"
#include "opal/mca/timer/timer.h"
#include "opal/mca/timer/base/base.h"


int opal_timer_base_close(void)
{
    /* Close all components that are still open (this should only
       happen during laminfo). */
    mca_base_components_close(0, &opal_timer_base_components_opened, NULL);
    OBJ_DESTRUCT(&opal_timer_base_components_opened);

    /* All done */
    return OPAL_SUCCESS;
}
