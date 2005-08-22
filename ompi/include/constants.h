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

#ifndef OMPI_CONSTANTS_H
#define OMPI_CONSTANTS_H

#include "orte/include/orte_constants.h"

#define OMPI_ERR_BASE   ORTE_ERR_MAX

/* error codes */
enum {
    /* Error codes inherited from ORTE/OPAL.  Still enum values so
       that we might get nice debugger help */
    OMPI_SUCCESS                  = ORTE_SUCCESS,

    OMPI_ERROR                    = ORTE_ERROR,
    OMPI_ERR_OUT_OF_RESOURCE      = ORTE_ERR_OUT_OF_RESOURCE,
    OMPI_ERR_TEMP_OUT_OF_RESOURCE = ORTE_ERR_TEMP_OUT_OF_RESOURCE,
    OMPI_ERR_RESOURCE_BUSY        = ORTE_ERR_RESOURCE_BUSY,
    OMPI_ERR_BAD_PARAM            = ORTE_ERR_BAD_PARAM,
    OMPI_ERR_FATAL                = ORTE_ERR_FATAL,
    OMPI_ERR_NOT_IMPLEMENTED      = ORTE_ERR_NOT_IMPLEMENTED,
    OMPI_ERR_NOT_SUPPORTED        = ORTE_ERR_NOT_SUPPORTED,
    OMPI_ERR_INTERUPTED           = ORTE_ERR_INTERUPTED,
    OMPI_ERR_WOULD_BLOCK          = ORTE_ERR_WOULD_BLOCK,
    OMPI_ERR_IN_ERRNO             = ORTE_ERR_IN_ERRNO,
    OMPI_ERR_UNREACH              = ORTE_ERR_UNREACH,
    OMPI_ERR_NOT_FOUND            = ORTE_ERR_NOT_FOUND,
    OMPI_EXISTS                   = ORTE_EXISTS, /* indicates that the specified object already exists */
    OMPI_ERR_TIMEOUT              = ORTE_ERR_TIMEOUT,
    OMPI_ERR_NOT_AVAILABLE        = ORTE_ERR_NOT_AVAILABLE,
    OMPI_ERR_PERM                 = ORTE_ERR_PERM,
    OMPI_ERR_VALUE_OUT_OF_BOUNDS  = ORTE_ERR_VALUE_OUT_OF_BOUNDS,
    OMPI_ERR_FILE_READ_FAILURE    = ORTE_ERR_FILE_READ_FAILURE,
    OMPI_ERR_FILE_WRITE_FAILURE   = ORTE_ERR_FILE_WRITE_FAILURE,
    OMPI_ERR_FILE_OPEN_FAILURE    = ORTE_ERR_FILE_OPEN_FAILURE,

    OMPI_ERR_RECV_LESS_THAN_POSTED      = ORTE_ERR_RECV_LESS_THAN_POSTED,
    OMPI_ERR_RECV_MORE_THAN_POSTED      = ORTE_ERR_RECV_MORE_THAN_POSTED,
    OMPI_ERR_NO_MATCH_YET               = ORTE_ERR_NO_MATCH_YET,
    OMPI_ERR_BUFFER                     = ORTE_ERR_BUFFER,
    OMPI_ERR_REQUEST                    = ORTE_ERR_REQUEST,
    OMPI_ERR_NO_CONNECTION_ALLOWED      = ORTE_ERR_NO_CONNECTION_ALLOWED,
    OMPI_ERR_CONNECTION_REFUSED         = ORTE_ERR_CONNECTION_REFUSED   ,
    OMPI_ERR_CONNECTION_FAILED          = ORTE_ERR_CONNECTION_FAILED,
    OMPI_STARTUP_DETECTED               = ORTE_STARTUP_DETECTED,
    OMPI_SHUTDOWN_DETECTED              = ORTE_SHUTDOWN_DETECTED,
    OMPI_PROC_STARTING                  = ORTE_PROC_STARTING,
    OMPI_PROC_STOPPED                   = ORTE_PROC_STOPPED,
    OMPI_PROC_TERMINATING               = ORTE_PROC_TERMINATING,
    OMPI_PROC_ALIVE                     = ORTE_PROC_ALIVE,
    OMPI_PROC_RUNNING                   = ORTE_PROC_RUNNING,
    OMPI_PROC_KILLED                    = ORTE_PROC_KILLED,
    OMPI_PROC_EXITED                    = ORTE_PROC_EXITED,
    OMPI_NODE_UP                        = ORTE_NODE_UP,
    OMPI_NODE_DOWN                      = ORTE_NODE_DOWN,
    OMPI_NODE_BOOTING                   = ORTE_NODE_BOOTING,
    OMPI_NODE_ERROR                     = ORTE_NODE_ERROR,
    OMPI_PACK_MISMATCH                  = ORTE_PACK_MISMATCH,
    OMPI_ERR_PACK_FAILURE               = ORTE_ERR_PACK_FAILURE,
    OMPI_ERR_UNPACK_FAILURE             = ORTE_ERR_UNPACK_FAILURE,
    OMPI_ERR_COMM_FAILURE               = ORTE_ERR_COMM_FAILURE,
    OMPI_UNPACK_INADEQUATE_SPACE        = ORTE_UNPACK_INADEQUATE_SPACE,
    OMPI_UNPACK_READ_PAST_END_OF_BUFFER = ORTE_UNPACK_READ_PAST_END_OF_BUFFER,
    OMPI_ERR_GPR_DATA_CORRUPT           = ORTE_ERR_GPR_DATA_CORRUPT,
    OMPI_ERR_TYPE_MISMATCH              = ORTE_ERR_TYPE_MISMATCH             
};

#define OMPI_ERR_MAX                    (OMPI_ERR_BASE - 1)

#define OMPI_NAMESPACE_SEGMENT        "ompi-namespace"

/*
 * OMPI-specific names for triggers and subscriptions used across processes
 */
#define OMPI_ATTRIBUTE_SUBSCRIPTION     "ompi-attribute-sub"
#define OMPI_PROC_SUBSCRIPTION          "ompi-proc-sub"
#define OMPI_OOB_SUBSCRIPTION           "ompi-oob-sub"
#define OMPI_MODEX_SUBSCRIPTION         "ompi-modex-sub"


#endif /* OMPI_CONSTANTS_H */

