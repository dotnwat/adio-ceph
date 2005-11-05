/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2005 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
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

#ifndef ORTE_SOH_TYPES_H
#define ORTE_SOH_TYPES_H

#include "orte_config.h"

/*
 * Process exit codes
 */

typedef int orte_exit_code_t;

/*
 * Process state codes
 */

typedef int8_t orte_proc_state_t;

#define ORTE_PROC_STATE_INIT           0x01  /* process entry has been created by rmaps */
#define ORTE_PROC_STATE_LAUNCHED       0x02  /* process has been launched by pls */
#define ORTE_PROC_STATE_AT_STG1        0x03  /* process is at Stage Gate 1 barrier in orte_init */
#define ORTE_PROC_STATE_AT_STG2        0x04  /* process is at Stage Gate 2 barrier in orte_init */
#define ORTE_PROC_STATE_RUNNING        0x06  /* process has exited orte_init and is running */
#define ORTE_PROC_STATE_AT_STG3        0x07  /* process is at Stage Gate 3 barrier in orte_finalize */
#define ORTE_PROC_STATE_FINALIZED      0x08  /* process has completed orte_finalize and is running */
#define ORTE_PROC_STATE_TERMINATED     0x09  /* process has terminated and is no longer running */
#define ORTE_PROC_STATE_ABORTED        0x0A  /* process aborted */

/*
 * Job state codes
 */

typedef int8_t orte_job_state_t;

#define ORTE_JOB_STATE_INIT           0x01  /* job entry has been created by rmaps */
#define ORTE_JOB_STATE_LAUNCHED       0x02  /* job has been launched by pls */
#define ORTE_JOB_STATE_AT_STG1        0x03  /* all processes are at Stage Gate 1 barrier in orte_init */
#define ORTE_JOB_STATE_AT_STG2        0x04  /* all processes are at Stage Gate 2 barrier in orte_init */
#define ORTE_JOB_STATE_RUNNING        0x06  /* all processes have exited orte_init and is running */
#define ORTE_JOB_STATE_AT_STG3        0x07  /* all processes are at Stage Gate 3 barrier in orte_finalize */
#define ORTE_JOB_STATE_FINALIZED      0x08  /* all processes have completed orte_finalize and is running */
#define ORTE_JOB_STATE_TERMINATED     0x09  /* all processes have terminated and is no longer running */
#define ORTE_JOB_STATE_ABORTED        0x0A  /* at least one process aborted, causing job to abort */

/**
 * Node State, corresponding to the ORTE_NODE_STATE_* #defines,
 * below.  These are #defines instead of an enum because the thought
 * is that we may have lots and lots of entries of these in the
 * registry and by making this an int8_t, it's only 1 byte, whereas an
 * enum defaults to an int (probably 4 bytes).  So it's a bit of a
 * space savings.
 */
typedef int8_t orte_node_state_t;

/** Node is in an unknown state (see orte_node_state_t) */
#define ORTE_NODE_STATE_UNKNOWN  0x00
/** Node is down (see orte_node_state_t) */
#define ORTE_NODE_STATE_DOWN     0x01
/** Node is up / available for use (see orte_node_state_t) */
#define ORTE_NODE_STATE_UP       0x02
/** Node is rebooting (only some systems will support this; see
    orte_node_state_t) */
#define ORTE_NODE_STATE_REBOOT   0x03
#endif
