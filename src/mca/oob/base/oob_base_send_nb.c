/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */


#include "ompi_config.h"
#include "include/constants.h"
#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"
#include <string.h>

#ifdef HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif
    

/*
 * internal struct for non-blocking packed sends
 */

struct mca_oob_send_cbdata {
    ompi_buffer_t cbbuf;
    struct iovec cbiov;
    mca_oob_callback_packed_fn_t cbfunc;
    void* cbdata;
};
typedef struct mca_oob_send_cbdata mca_oob_send_cbdata_t;

static void mca_oob_send_callback(
    int status,
    ompi_process_name_t* peer,
    struct iovec* msg,
    int count,
    int tag,
    void* cbdata);

/*
 * Non-blocking version of mca_oob_send().
 *
 * @param peer (IN)    Opaque name of peer process.
 * @param msg (IN)     Array of iovecs describing user buffers and lengths.
 * @param count (IN)   Number of elements in iovec array.
 * @param flags (IN)   Currently unused.
 * @param cbfunc (IN)  Callback function on send completion.
 * @param cbdata (IN)  User data that is passed to callback function.
 * @return             OMPI error code (<0) on error number of bytes actually sent.
 *
 */

int mca_oob_send_nb(ompi_process_name_t* peer, struct iovec* msg, int count, int tag,
                    int flags, mca_oob_callback_fn_t cbfunc, void* cbdata)
{
    return(mca_oob.oob_send_nb(peer, msg, count, tag, flags, cbfunc, cbdata));
}


/**
*  Non-blocking version of mca_oob_send_packed().
*
*  @param peer (IN)    Opaque name of peer process.
*  @param buffer (IN)  Opaque buffer handle.
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param flags (IN)   Currently unused.
*  @param cbfunc (IN)  Callback function on send completion.
*  @param cbdata (IN)  User data that is passed to callback function.
*  @return             OMPI error code (<0) on error number of bytes actually sent.
*
*  The user supplied callback function is called when the send completes. Note that
*  the callback may occur before the call to mca_oob_send returns to the caller,
*  if the send completes during the call.
*
*/

int mca_oob_send_packed_nb(
    ompi_process_name_t* peer,
    ompi_buffer_t buffer,
    int tag,
    int flags,
    mca_oob_callback_packed_fn_t cbfunc,
    void* cbdata)
{
    mca_oob_send_cbdata_t *oob_cbdata;
    void *dataptr;
    size_t datalen;
    int rc;

    /* first build iovec from buffer information */
    rc = ompi_buffer_size (buffer, &datalen);
    if (OMPI_ERROR==rc) { 
        return (rc); 
    }

    rc = ompi_buffer_get_ptrs (buffer, NULL, NULL, &dataptr);
    if (OMPI_ERROR==rc) { 
        return (rc); 
    }

    /* allocate a struct to pass into callback */
    if(NULL == (oob_cbdata = malloc(sizeof(mca_oob_send_cbdata_t)))) {
        return OMPI_ERR_OUT_OF_RESOURCE;
    }
    oob_cbdata->cbfunc = cbfunc;
    oob_cbdata->cbdata = cbdata;
    oob_cbdata->cbiov.iov_base = dataptr;
    oob_cbdata->cbiov.iov_len = datalen;

    /* queue up the request */
    rc = mca_oob.oob_send_nb(peer, &oob_cbdata->cbiov, 1, tag, flags, mca_oob_send_callback, oob_cbdata);
    if(rc != OMPI_SUCCESS) {
        free(oob_cbdata);
    }
    return rc;
}

/**
*  Callback function on send completion for buffer PACKED message only.
*  i.e. only mca_oob_send_packed_nb and mca_oob_recv_packed_nb USE this.
*
*  @param status (IN)  Completion status - equivalent to the return value from blocking send/recv.
*  @param peer (IN)    Opaque name of peer process.
*  @param buffer (IN)  For sends, this is a pointer to a prepacked buffer
                       For recvs, OOB creates and returns a buffer
*  @param tag (IN)     User defined tag for matching send/recv.
*  @param cbdata (IN)  User data.
*/
                                                                                                                              
static void mca_oob_send_callback(
    int status,
    ompi_process_name_t* peer,
    struct iovec* msg,
    int count,
    int tag,
    void* cbdata)
{
    /* validate status */
    mca_oob_send_cbdata_t *oob_cbdata = cbdata;
    if(status < 0) {
        oob_cbdata->cbfunc(status, peer, NULL, tag, oob_cbdata->cbdata);
        free(oob_cbdata);
        return;
    }
                                                                                                                                 oob_cbdata->cbfunc(status, peer, oob_cbdata->cbbuf, tag, oob_cbdata->cbdata);
    free(oob_cbdata);
}

