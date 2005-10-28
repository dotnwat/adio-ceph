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
/** @file:
 *
 * The Open MPI general purpose registry - implementation.
 *
 */

/*
 * includes
 */

#include "orte_config.h"

#include "opal/util/trace.h"

#include "orte/include/orte_constants.h"
#include "orte/include/orte_types.h"
#include "orte/dps/dps.h"

#include "orte/mca/errmgr/errmgr.h"

#include "orte/mca/gpr/base/base.h"

int orte_gpr_base_pack_put(orte_buffer_t *cmd,
                size_t cnt, orte_gpr_value_t **values)
{
    orte_gpr_cmd_flag_t command;
    int rc;

    OPAL_TRACE(3);
    
    command = ORTE_GPR_PUT_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, values, cnt, ORTE_GPR_VALUE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    return ORTE_SUCCESS;
}


int orte_gpr_base_pack_get(orte_buffer_t *cmd,
			  orte_gpr_addr_mode_t mode,
			  char *segment, char **tokens, char **keys)
{
    orte_gpr_cmd_flag_t command;
    char **ptr;
    int rc;
    size_t n;

    OPAL_TRACE(3);
    
    command = ORTE_GPR_GET_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_ADDR_MODE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &segment, 1, ORTE_STRING))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* compute number of tokens */
    n = 0;
    if (NULL != tokens) {
        ptr = tokens;
        while (NULL != ptr[n]) {
           n++;
        }
    }

    /* pack number of tokens */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &n, 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (n > 0) {
         if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, tokens, n, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            return rc;
         }
    }

    /* compute number of keys */
    n = 0;
    if (NULL != keys) {
        ptr = keys;
        while (NULL != ptr[n]) {
           n++;
        }
    }

    /* pack number of keys */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &n, 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (n > 0) {
        if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, keys, n, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    return ORTE_SUCCESS;
}

int orte_gpr_base_pack_get_conditional(orte_buffer_t *cmd,
			  orte_gpr_addr_mode_t mode,
			  char *segment, char **tokens, char **keys,
			  size_t num_conditions, orte_gpr_keyval_t **conditions)
{
    orte_gpr_cmd_flag_t command;
    char **ptr;
    int rc;
    size_t n;

    OPAL_TRACE(3);
    
    command = ORTE_GPR_GET_CMD;

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &command, 1, ORTE_GPR_CMD))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &mode, 1, ORTE_GPR_ADDR_MODE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &segment, 1, ORTE_STRING))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }

    /* compute number of tokens */
    n = 0;
    if (NULL != tokens) {
        ptr = tokens;
        while (NULL != ptr[n]) {
           n++;
        }
    }

    /* pack number of tokens */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &n, 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (n > 0) {
         if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, tokens, n, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            return rc;
         }
    }

    /* compute number of keys */
    n = 0;
    if (NULL != keys) {
        ptr = keys;
        while (NULL != ptr[n]) {
           n++;
        }
    }

    /* pack number of keys */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &n, 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    if (n > 0) {
        if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, keys, n, ORTE_STRING))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
    }

    /* pack number of conditions */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, &num_conditions, 1, ORTE_SIZE))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    /* pack conditions */
    if (ORTE_SUCCESS != (rc = orte_dps.pack(cmd, conditions, num_conditions, ORTE_KEYVAL))) {
        ORTE_ERROR_LOG(rc);
        return rc;
    }
    
    return ORTE_SUCCESS;
}
