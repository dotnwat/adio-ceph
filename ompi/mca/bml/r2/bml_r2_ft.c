/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2006 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007      Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdlib.h>
#include <string.h>

#include "opal/util/show_help.h"
#include "orte/mca/ns/ns.h"
#include "ompi/runtime/ompi_cr.h"
#include "ompi/class/ompi_bitmap.h"
#include "ompi/mca/bml/bml.h"
#include "ompi/mca/bml/base/base.h"
#include "ompi/mca/btl/btl.h"
#include "ompi/mca/btl/base/base.h"
#include "ompi/mca/bml/base/bml_base_endpoint.h" 
#include "ompi/mca/bml/base/bml_base_btl.h" 
#include "ompi/mca/pml/pml.h"
#include "ompi/mca/pml/base/base.h"
#include "ompi/mca/pml/base/pml_base_module_exchange.h"
#include "orte/mca/smr/smr.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/gpr/gpr.h"
#include "orte/class/orte_proc_table.h" 
#include "ompi/proc/proc.h"

#include "bml_r2.h"
#include "bml_r2_ft.h"

int mca_bml_r2_ft_event(int state) {
    ompi_proc_t** procs;
    size_t num_procs;
    size_t btl_idx;
    int ret, p;
    
    if(OPAL_CRS_CHECKPOINT == state) {
        /* Do nothing for now */
    }
    else if(OPAL_CRS_CONTINUE == state) {
        /* Since nothing in Checkpoint, we are fine here */
    }
    else if(OPAL_CRS_RESTART == state) {
        procs = ompi_proc_all(&num_procs);
        if(NULL == procs) {
            return OMPI_ERR_OUT_OF_RESOURCE;
        }
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }
    
    /*
     * Call ft_event in:
     * - BTL modules
     * - MPool modules
     *
     * These should be cleaning out stale state, and memory references in 
     * preparation for being shut down.
     */
    for(btl_idx = 0; btl_idx < mca_bml_r2.num_btl_modules; btl_idx++) {
        /*
         * Notify BTL
         */
        if( NULL != (mca_bml_r2.btl_modules[btl_idx])->btl_ft_event) {
            opal_output_verbose(10, ompi_cr_output,
                                "bml:r2: ft_event: Notify the %s BTL.\n",
                                (mca_bml_r2.btl_modules[btl_idx])->btl_component->btl_version.mca_component_name);
            if(OMPI_SUCCESS != (ret = (mca_bml_r2.btl_modules[btl_idx])->btl_ft_event(state) ) ) {
                continue;
            }
        }
        
        /*
         * Notify Mpool
         */
        if( NULL != (mca_bml_r2.btl_modules[btl_idx])->btl_mpool) {
            opal_output_verbose(10, ompi_cr_output,
                                "bml:r2: ft_event: Notify the %s MPool.\n",
                                (mca_bml_r2.btl_modules[btl_idx])->btl_mpool->mpool_component->mpool_version.mca_component_name);
            if(OMPI_SUCCESS != (ret = (mca_bml_r2.btl_modules[btl_idx])->btl_mpool->mpool_ft_event(state) ) ) {
                continue;
            }
        }
    }
    
    if(OPAL_CRS_CHECKPOINT == state) {
        ;
    }
    else if(OPAL_CRS_CONTINUE == state) {
        ;
    }
    else if(OPAL_CRS_RESTART == state) {

        mca_bml_r2.num_btl_modules  = 0;
        mca_bml_r2.num_btl_progress = 0;

        if( NULL != mca_bml_r2.btl_modules) {
            free( mca_bml_r2.btl_modules);
        }
        if( NULL != mca_bml_r2.btl_progress ) {
            free( mca_bml_r2.btl_progress);
        }

        opal_output_verbose(10, ompi_cr_output,
                            "bml:r2: ft_event(reboot): Reselect BTLs\n");

        /*
         * Close the BTLs
         *
         * Need to do this because we may have BTL components that were
         * unloaded in the first selection that may be available now.
         * Conversely we may have BTL components loaded now that
         * are not available now.
         */
        if( OMPI_SUCCESS != (ret = mca_btl_base_close())) {
            opal_output(0, "bml:r2: ft_event(Restart): Failed to close BTL framework\n");
            return ret;
        }

        /*
         * Re-open the BTL framework to get the full list of components.
         */
        if( OMPI_SUCCESS != (ret = mca_btl_base_open()) ) {
            opal_output(0, "bml:r2: ft_event(Restart): Failed to open BTL framework\n");
            return ret;
        }
        
        /*
         * Re-select the BTL components/modules
         * This will cause the BTL components to discover the available
         * network options on this machine, and post proper modex informaiton.
         */
        if( OMPI_SUCCESS != (ret = mca_btl_base_select(OMPI_ENABLE_PROGRESS_THREADS,
                                                       OMPI_ENABLE_MPI_THREADS) ) ) {
            opal_output(0, "bml:r2: ft_event(Restart): Failed to select in BTL framework\n");
            return ret;
        }

        /*
         * Clear some structures so we can properly repopulate them
         */
        mca_bml_r2.btls_added = false;

        for(p = 0; p < (int)num_procs; ++p) {
            OBJ_RELEASE(procs[p]->proc_bml);
            procs[p]->proc_bml = NULL;

            OBJ_RELEASE(procs[p]);
        }

        if( NULL != procs ) {
            free(procs);
            procs = NULL;
        }
    }
    else if(OPAL_CRS_TERM == state ) {
        ;
    }
    else {
        ;
    }
    
    return OMPI_SUCCESS;
}
