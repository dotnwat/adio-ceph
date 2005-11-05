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

#ifndef OMPI_GEN_REQUEST_H
#define OMPI_GEN_REQUEST_H

#include "ompi_config.h"
#include "request/request.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_grequest_t);

/*
 * Fortran types for the generalized request functions.
 */
typedef void (MPI_F_Grequest_query_function)(void* extra_state, MPI_Status* status, int* ierr );
typedef void (MPI_F_Grequest_free_function)( void* extra_state, int* err );
typedef void (MPI_F_Grequest_cancel_function)( void* extra_state, int* complete, int* ierr );

typedef union {
    MPI_Grequest_query_function*   c_query;
    MPI_F_Grequest_query_function* f_query;
} MPI_Grequest_query_fct_t;

typedef union {
    MPI_Grequest_free_function*   c_free;
    MPI_F_Grequest_free_function* f_free;
} MPI_Grequest_free_fct_t;

typedef union {
    MPI_Grequest_cancel_function*   c_cancel;
    MPI_F_Grequest_cancel_function* f_cancel;
} MPI_Grequest_cancel_fct_t;

struct ompi_grequest_t {
    ompi_request_t greq_base;
    MPI_Grequest_query_fct_t greq_query;
    MPI_Grequest_free_fct_t greq_free;
    MPI_Grequest_cancel_fct_t greq_cancel;
    void *greq_state;
};
typedef struct ompi_grequest_t ompi_grequest_t;


/*
 * Start a generalized request.
 */

OMPI_DECLSPEC int ompi_grequest_start(
    MPI_Grequest_query_function *gquery,
    MPI_Grequest_free_function *gfree,
    MPI_Grequest_cancel_function *gcancel,
    void* gstate,
    ompi_request_t** request);

/*
 * Mark a generalized request as complete.
 */
OMPI_DECLSPEC int ompi_grequest_complete(ompi_grequest_t*);
#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
