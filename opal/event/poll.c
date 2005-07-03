/*	$OpenBSD: poll.c,v 1.2 2002/06/25 15:50:15 mickey Exp $	*/

/*
 * Copyright 2000-2003 Niels Provos <provos@citi.umich.edu>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "ompi_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#ifdef HAVE_POLL_H
#include <poll.h>
#endif
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <errno.h>
#include <err.h>

#ifdef USE_LOG
#include "log.h"
#else
#define LOG_DBG(x)
#define log_error(x)	perror(x)
#endif

#include "event.h"
#include "opal/util/output.h"
#if OPAL_EVENT_USE_SIGNALS
#include "evsignal.h"
#endif
#include "opal/threads/mutex.h"


extern struct opal_event_list opal_eventqueue;
extern volatile sig_atomic_t opal_evsignal_caught;
extern opal_mutex_t opal_event_lock;

/* Open MPI: make this struct instance be static */
static struct pollop {
	int event_count;		/* Highest number alloc */
	struct pollfd *event_set;
	struct opal_event **event_back;
#if OPAL_EVENT_USE_SIGNALS
	sigset_t evsigmask;
#endif
} pollop;

static void *poll_init	(void);
static int poll_add		(void *, struct opal_event *);
static int poll_del		(void *, struct opal_event *);
#if 0
/* Open MPI: JMS As far as I can tell, this function is not used
   anywhere */
static int poll_recalc	(void *, int);
#endif
static int poll_dispatch	(void *, struct timeval *);

const struct opal_eventop opal_pollops = {
	"poll",
	poll_init,
	poll_add,
	poll_del,
	NULL,
	poll_dispatch
};

static void *
poll_init(void)
{
	/* Disable poll when this environment variable is set */
	if (getenv("EVENT_NOPOLL"))
		return (NULL);

	memset(&pollop, 0, sizeof(pollop));
#if OPAL_EVENT_USE_SIGNALS
	opal_evsignal_init(&pollop.evsigmask);
#endif
	return (&pollop);
}

/*
 * Called with the highest fd that we know about.  If it is 0, completely
 * recalculate everything.
 */

#if 0
/* Open MPI: JMS As far as I can tell, this function is not used
   anywhere. */
static int
poll_recalc(void *arg, int max)
{
#if OPAL_EVENT_USE_SIGNALS
	struct pollop *pop = arg;
	return (opal_evsignal_recalc(&pop->evsigmask));
#else
	return (0);
#endif
}
#endif

static int
poll_dispatch(void *arg, struct timeval *tv)
{
	int res, i, count, sec, nfds;
	struct opal_event *ev;
	struct pollop *pop = arg;

	count = pop->event_count;
	nfds = 0;
	TAILQ_FOREACH(ev, &opal_eventqueue, ev_next) {
		if (nfds + 1 >= count) {
			if (count < 256)
				count = 256;
			else
				count <<= 1;

			/* We need more file descriptors */
			pop->event_set = realloc(pop->event_set,
			    count * sizeof(struct pollfd));
			if (pop->event_set == NULL) {
				log_error("realloc");
				return (-1);
			}
			pop->event_back = realloc(pop->event_back,
			    count * sizeof(struct opal_event *));
			if (pop->event_back == NULL) {
				log_error("realloc");
				return (-1);
			}
			pop->event_count = count;
		}
		if (ev->ev_events & OPAL_EV_WRITE) {
			struct pollfd *pfd = &pop->event_set[nfds];
			pfd->fd = ev->ev_fd;
			pfd->events = POLLOUT;
			pfd->revents = 0;

			pop->event_back[nfds] = ev;

			nfds++;
		}
		if (ev->ev_events & OPAL_EV_READ) {
			struct pollfd *pfd = &pop->event_set[nfds];

			pfd->fd = ev->ev_fd;
			pfd->events = POLLIN;
			pfd->revents = 0;

			pop->event_back[nfds] = ev;

			nfds++;
		}
	}

#if OPAL_EVENT_USE_SIGNALS
	if (opal_evsignal_deliver(&pop->evsigmask) == -1)
		return (-1);
#endif

	sec = tv->tv_sec * 1000 + tv->tv_usec / 1000;
        opal_mutex_unlock(&opal_event_lock);
	    res = poll(pop->event_set, nfds, sec);
        opal_mutex_lock(&opal_event_lock);

#if OPAL_EVENT_USE_SIGNALS
	if (opal_evsignal_recalc(&pop->evsigmask) == -1)
		return (-1);
#endif

	if (res == -1) {
		if (errno != EINTR) {
			opal_output(0, "poll failed with errno=%d\n", errno);
			return (-1);
		}

#if OPAL_EVENT_USE_SIGNALS
		opal_evsignal_process();
#endif
		return (0);
	} 

#if OPAL_EVENT_USE_SIGNALS
	else if (opal_evsignal_caught)
		opal_evsignal_process();
#endif

	LOG_DBG((LOG_MISC, 80, "%s: poll reports %d", __func__, res));

	if (res == 0)
		return (0);

	for (i = 0; i < nfds; i++) {
                int what = pop->event_set[i].revents;
		
		res = 0;

		/* If the file gets closed notify or any badness happend */
		if (what & (POLLHUP | POLLERR | POLLNVAL))
			what |= POLLIN|POLLOUT;
		if (what & POLLIN)
			res |= OPAL_EV_READ;
		if (what & POLLOUT)
			res |= OPAL_EV_WRITE;
		if (res == 0)
			continue;

		ev = pop->event_back[i];
		res &= ev->ev_events;

		if (res) {
			if (!(ev->ev_events & OPAL_EV_PERSIST))
				opal_event_del_i(ev);
			opal_event_active_i(ev, res, 1);
		}	
	}

	return (0);
}

static int
poll_add(void *arg, struct opal_event *ev)
{
#if OPAL_EVENT_USE_SIGNALS
	struct pollop *pop = arg;
	if (ev->ev_events & OPAL_EV_SIGNAL)
		return (opal_evsignal_add(&pop->evsigmask, ev));
#endif
	return (0);
}

/*
 * Nothing to be done here.
 */

static int
poll_del(void *arg, struct opal_event *ev)
{
#if OPAL_EVENT_USE_SIGNALS
	struct pollop *pop = arg;
#endif
	if (!(ev->ev_events & OPAL_EV_SIGNAL))
		return (0);
#if OPAL_EVENT_USE_SIGNALS
	return (opal_evsignal_del(&pop->evsigmask, ev));
#else
	return (0);
#endif
}

