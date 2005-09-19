/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "orte_config.h"
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <string.h>

#include "opal/util/trace.h"

#include "orte/include/orte_constants.h"
#include "orte/dps/dps.h"
#include "orte/mca/rmgr/base/base.h"
#include "orte/mca/errmgr/errmgr.h"


/*
 * 
 */

int orte_rmgr_base_pack_cmd(orte_buffer_t* buffer, orte_rmgr_cmd_t cmd, orte_jobid_t jobid)
{
     int rc;
     
     OPAL_TRACE(4);
    
     rc = orte_dps.pack(buffer, &cmd, 1, ORTE_RMGR_CMD);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }

     rc = orte_dps.pack(buffer, &jobid, 1, ORTE_JOBID);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }
     return ORTE_SUCCESS;
}

/*
 *
 */

int orte_rmgr_base_pack_create_cmd(
    orte_buffer_t* buffer,
    orte_app_context_t** context,
    size_t num_context)
{
     int rc;
     
     orte_rmgr_cmd_t cmd = ORTE_RMGR_CMD_CREATE;

     OPAL_TRACE(4);
    
     rc = orte_dps.pack(buffer, &cmd, 1, ORTE_RMGR_CMD);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }

     rc = orte_dps.pack(buffer, &num_context, 1, ORTE_SIZE);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }
     rc = orte_dps.pack(buffer, context, num_context, ORTE_APP_CONTEXT);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }
     return ORTE_SUCCESS;
}

                                                                                                                           
int orte_rmgr_base_pack_terminate_proc_cmd(
    orte_buffer_t* buffer,
    const orte_process_name_t* name)
{
     int rc;
     
     orte_rmgr_cmd_t cmd = ORTE_RMGR_CMD_CREATE;

     OPAL_TRACE(4);
    
     rc = orte_dps.pack(buffer, &cmd, 1, ORTE_RMGR_CMD);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }

     rc = orte_dps.pack(buffer, (void*)name, 1, ORTE_NAME);
     if(ORTE_SUCCESS != rc) {
         ORTE_ERROR_LOG(rc);
         return rc;
     }
     return ORTE_SUCCESS;
}

                                                                                                                           
int orte_rmgr_base_unpack_rsp(
    orte_buffer_t* buffer)
{
    int32_t rc;
    size_t cnt = 1;

    OPAL_TRACE(4);
    
    if(ORTE_SUCCESS != (rc = orte_dps.unpack(buffer,&rc,&cnt,ORTE_INT32))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    return rc;
}
                                                                                                                           
int orte_rmgr_base_unpack_create_rsp(
    orte_buffer_t* buffer,
    orte_jobid_t* jobid)
{
    int32_t rc;
    size_t cnt;

    OPAL_TRACE(4);
    
    cnt = 1;
    if(ORTE_SUCCESS != (rc = orte_dps.unpack(buffer,jobid,&cnt,ORTE_JOBID))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    cnt = 1;
    if(ORTE_SUCCESS != (rc = orte_dps.unpack(buffer,&rc,&cnt,ORTE_INT32))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    return rc;
}


