/* -*- C -*-
 *
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
/** @file:
 *
 * The Open MPI General Purpose Registry - Replica component
 *
 */

/*
 * includes
 */
#include "orte_config.h"

#include "include/orte_types.h"
#include "util/output.h"
#include "dps/dps.h"
#include "mca/errmgr/errmgr.h"
#include "mca/ns/ns_types.h"
#include "mca/rml/rml.h"

#include "gpr_replica_comm.h"

int orte_gpr_replica_remote_notify(orte_process_name_t *recipient,
                orte_gpr_notify_message_t *message)
{
    orte_buffer_t buffer;
    orte_gpr_cmd_flag_t command;
    int rc;

    if (orte_gpr_replica_globals.debug) {
        ompi_output(0, "sending trigger message");
    }

    command = ORTE_GPR_NOTIFY_CMD;

    OBJ_CONSTRUCT(&buffer, orte_buffer_t);

    if (ORTE_SUCCESS != (rc = orte_dps.pack(&buffer, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(&buffer, &(message->cnt), 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if(message->cnt > 0) {
        if (ORTE_SUCCESS != (rc = orte_dps.pack(&buffer, message->data,
                                    message->cnt, ORTE_GPR_NOTIFY_DATA))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }
    OBJ_RELEASE(message);
    
    OMPI_THREAD_UNLOCK(&orte_gpr_replica_globals.mutex);
    
    if (0 > orte_rml.send_buffer(recipient, &buffer, ORTE_RML_TAG_GPR_NOTIFY, 0)) {
        ORTE_ERROR_LOG(ORTE_ERR_COMM_FAILURE);
        OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);
        return ORTE_ERR_COMM_FAILURE;
    }
    OMPI_THREAD_LOCK(&orte_gpr_replica_globals.mutex);

    return ORTE_SUCCESS;
}
