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
#ifndef _PERUSE_INTERNAL_H_
#define _PERUSE_INTERNAL_H_

#include "ompi/peruse/peruse.h"
#include "opal/class/opal_list.h"
#include "ompi/communicator/communicator.h"
#include "ompi/file/file.h"
#include "ompi/win/win.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif


typedef int (ompi_peruse_callback_f)(peruse_event_h event_h,
              MPI_Aint unique_id, void * spec, void * param);

OMPI_DECLSPEC OBJ_CLASS_DECLARATION(ompi_peruse_t);

struct ompi_peruse_handle_t {
    opal_list_item_t super;         /**< Allow handle to be placed on a list */
    opal_mutex_t lock;              /**< Lock protecting the entry  XXX needed?*/
    int active;                     /**< Whether this handle has been activated */
    int event;                      /**< Event being watched */
    int type;                       /**< Object-type this event is registered on */
    ompi_communicator_t* comm;      /**< Corresponding communicicator */
    ompi_file_t* file;              /**< Corresponding file */
    ompi_win_t* win;                /**< Corresponding window, in case we have support */
    ompi_peruse_callback_f* fn;     /**< Callback function specified by user */
    void * param;                   /**< Parameters being passed to callback */
};

typedef struct ompi_peruse_handle_t ompi_peruse_handle_t;
OMPI_DECLSPEC extern opal_class_t ompi_peruse_handle_t_class;

enum {
    PERUSE_TYPE_INVALID=-1,
    PERUSE_TYPE_COMM,
    PERUSE_TYPE_FILE,
    PERUSE_TYPE_WIN
};


/*
 * Module internal function declarations
 */
int ompi_peruse_init (void);
int ompi_peruse_finalize (void);


/*
 * Global macros
 */

#define PERUSE_ENABLED 1          /* XXX */

#ifdef OMPI_WANT_PERUSE
#define PERUSE_TRACE_COMM_EVENT(event, base_req, op)                                   \
do {                                                                                   \
    if( NULL != (base_req)->req_comm->c_peruse_handles ) {                             \
        ompi_peruse_handle_t * _ptr = (base_req)->req_comm->c_peruse_handles[(event)]; \
        if (NULL != _ptr && _ptr->active) {                                            \
            peruse_comm_spec_t _comm_spec;                                             \
            _comm_spec.comm      = (base_req)->req_comm;                               \
            _comm_spec.buf       = (base_req)->req_addr;                               \
            _comm_spec.count     = (base_req)->req_count;                              \
            _comm_spec.datatype  = (base_req)->req_datatype;                           \
            _comm_spec.peer      = (base_req)->req_peer;                               \
            _comm_spec.tag       = (base_req)->req_tag;                                \
            _comm_spec.operation = (op);                                               \
            _ptr->fn(_ptr, (MPI_Aint)(base_req), &_comm_spec, _ptr->param);            \
        }                                                                              \
    }                                                                                  \
} while(0)

#define PERUSE_TRACE_MSG_EVENT(event, comm_ptr, hdr_peer, hdr_tag, op)                 \
do {                                                                                   \
    if( NULL != (comm_ptr)->c_peruse_handles ) {                                       \
        ompi_peruse_handle_t * _ptr = (comm_ptr)->c_peruse_handles[(event)];           \
        if (NULL != _ptr && _ptr->active) {                                            \
            peruse_comm_spec_t _comm_spec;                                             \
            _comm_spec.comm      = (ompi_communicator_t*) (comm_ptr);                  \
            _comm_spec.buf       = NULL;                                               \
            _comm_spec.count     = 0;                                                  \
            _comm_spec.datatype  = MPI_DATATYPE_NULL;                                  \
            _comm_spec.peer      = (hdr_peer);                                         \
            _comm_spec.tag       = (hdr_tag);                                          \
            _comm_spec.operation = (op);                                               \
            _ptr->fn (_ptr, (MPI_Aint)/* unique_id */ 0, &_comm_spec, _ptr->param);    \
        }                                                                              \
    }                                                                                  \
} while(0)

#else
#define PERUSE_TRACE_COMM_EVENT(event, base_req, op)
#define PERUSE_TRACE_MSG_EVENT(event, comm_ptr, hdr_peer, hdr_tag, op)
#endif


#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* _PERUSE_INTERNAL_H_ */
