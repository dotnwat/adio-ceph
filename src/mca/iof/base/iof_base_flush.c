#include "ompi_config.h"
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif
#include <netinet/in.h>
#include "util/output.h"
#include "mca/oob/base/base.h"
#include "mca/iof/base/base.h"
#include "iof_base_endpoint.h"
#include "iof_base_fragment.h"

/**
 * timer callback out of the event loop
 */

static void mca_iof_base_timer_cb(int fd, short flags, void *cbdata)
{
    int *flushed = (int*)cbdata;
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    *flushed = 1;
    ompi_condition_signal(&mca_iof_base.iof_condition);
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
}


/*
 * flush output streams and block until there is no pending I/O
 * on any of the current endpoints.
 */

int mca_iof_base_flush(void)
{
    ompi_list_item_t* item;
    ompi_event_t ev;
    struct timeval tv = { 0, 0 };
    int flushed = 0;
    size_t pending;
                                                                                                                              
    /* flush any pending output */
    fflush(NULL);
                                                                                                                              
    /* force all file descriptors to be progressed at least once,
     * wait on a timer callback to be called out of the event loop
    */
    OMPI_THREAD_LOCK(&mca_iof_base.iof_lock);
    mca_iof_base.iof_waiting++;

    ompi_evtimer_set(&ev, mca_iof_base_timer_cb, &flushed);
    ompi_event_add(&ev, &tv);
    while(flushed == 0)
        ompi_condition_wait(&mca_iof_base.iof_condition, &mca_iof_base.iof_lock);
                                                                                                                              
    /* wait for all of the endpoints to reach an idle state */
    pending = ompi_list_get_size(&mca_iof_base.iof_endpoints);
    while(pending == ompi_list_get_size(&mca_iof_base.iof_endpoints)) {
        pending = 0;
        for(item = ompi_list_get_first(&mca_iof_base.iof_endpoints);
            item != ompi_list_get_end(&mca_iof_base.iof_endpoints);
            item =  ompi_list_get_next(item)) {
            mca_iof_base_endpoint_t* endpoint = (mca_iof_base_endpoint_t*)item;
            if(mca_iof_base_endpoint_pending(endpoint)) {
                pending++;
            }
        }
        if(pending != 0) {
            ompi_condition_wait(&mca_iof_base.iof_condition, &mca_iof_base.iof_lock);
        }
    }
    mca_iof_base.iof_waiting--;
    OMPI_THREAD_UNLOCK(&mca_iof_base.iof_lock);
    return OMPI_SUCCESS;
}

