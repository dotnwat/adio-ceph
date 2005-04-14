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

/** @file **/

#include "orte_config.h"

#include "include/orte_constants.h"
#include "runtime/runtime.h"
#include "runtime/orte_wait.h"
#include "event/event.h"
#include "mca/rml/base/base.h"
#include "mca/ns/base/base.h"
#include "mca/gpr/base/base.h"
#include "mca/iof/base/base.h"
#include "mca/rmgr/base/base.h"
#include "util/if.h"
#include "util/malloc.h"
#include "util/output.h"
#include "util/session_dir.h"
#include "util/sys_info.h"
#include "util/proc_info.h"
#include "util/univ_info.h"

/**
 * Leave ORTE.
 *
 * @retval ORTE_SUCCESS Upon success.
 * @retval ORTE_ERROR Upon failure.
 *
 * This function performs 
 */
int orte_finalize(void)
{
    /* rmgr close depends on wait/iof */
    orte_rmgr_base_close();
    orte_wait_finalize();
    orte_iof_base_close();

    orte_ns_base_close();
    orte_gpr_base_close();
    orte_rml_base_close();

    ompi_progress_finalize();

    ompi_event_fini();

#ifndef WIN32
    orte_session_dir_finalize(orte_process_info.my_name);
#endif

    /* clean out the global structures */
    orte_sys_info_finalize();
    orte_proc_info_finalize();
    orte_univ_info_finalize();
    
    /* cleanup the if data */
    ompi_iffinalize();
    
    /* finalize the mca */
    mca_base_close();

    /* finalize the output system */
    ompi_output_finalize();
    
    /* finalize the class/object system */
    ompi_class_finalize();

    /* finalize the memory allocator */
    ompi_malloc_finalize();

    return ORTE_SUCCESS;
}

