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
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdint.h>
#include <sys/types.h>
#include <sys/resource.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <sys/_time.h>
#endif
#include <sys/queue.h>
#include <sys/epoll.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <err.h>

#ifdef USE_LOG
#include "log.h"
#else
#define LOG_DBG(x)
#define log_error	warn
#endif

#include "event.h"
#include "evsignal.h"
#include "threads/mutex.h"

extern struct lam_event_list lam_eventqueue;
extern volatile sig_atomic_t lam_evsignal_caught;
extern lam_mutex_t lam_event_mutex;

/* due to limitations in the epoll interface, we need to keep track of
 * all file descriptors outself.
 */
struct evepoll {
	struct lam_event *evread;
	struct lam_event *evwrite;
};

struct epollop {
	struct evepoll *fds;
	int nfds;
	struct epoll_event *events;
	int nevents;
	int epfd;
	sigset_t evsigmask;
} epollop;

static void *epoll_init	(void);
static int epoll_add	(void *, struct lam_event *);
static int epoll_del	(void *, struct lam_event *);
static int epoll_recalc	(void *, int);
static int epoll_dispatch	(void *, struct timeval *);

struct lam_eventop lam_epollops = {
	"epoll",
	epoll_init,
	epoll_add,
	epoll_del,
	epoll_recalc,
	epoll_dispatch
};

#define NEVENT	32000

static void *
epoll_init(void)
{
	int epfd, nfiles = NEVENT;
	struct rlimit rl;

	/* Disable epollueue when this environment variable is set */
	if (getenv("EVENT_NOEPOLL"))
		return (NULL);

	memset(&epollop, 0, sizeof(epollop));

	if (getrlimit(RLIMIT_NOFILE, &rl) == 0 &&
	    rl.rlim_cur != RLIM_INFINITY)
		nfiles = rl.rlim_cur;

	/* Initalize the kernel queue */

	if ((epfd = epoll_create(nfiles)) == -1) {
		log_error("epoll_create");
		return (NULL);
	}

	epollop.epfd = epfd;

	/* Initalize fields */
	epollop.events = malloc(nfiles * sizeof(struct epoll_event));
	if (epollop.events == NULL)
		return (NULL);
	epollop.nevents = nfiles;

	epollop.fds = calloc(nfiles, sizeof(struct evepoll));
	if (epollop.fds == NULL) {
		free(epollop.events);
		return (NULL);
	}
	epollop.nfds = nfiles;

	lam_evsignal_init(&epollop.evsigmask);

	return (&epollop);
}

static int
epoll_recalc(void *arg, int max)
{
	struct epollop *epollop = arg;
	if (max > epollop->nfds) {
		struct evepoll *fds;
		int nfds;

		nfds = epollop->nfds;
		while (nfds < max)
			nfds <<= 1;

		fds = realloc(epollop->fds, nfds * sizeof(struct evepoll));
		if (fds == NULL) {
			log_error("realloc");
			return (-1);
		}
		epollop->fds = fds;
		memset(fds + epollop->nfds, 0,
		    (nfds - epollop->nfds) * sizeof(struct evepoll));
		epollop->nfds = nfds;
	}

	return (lam_evsignal_recalc(&epollop->evsigmask));
}

int
epoll_dispatch(void *arg, struct timeval *tv)
{
	struct epollop *epollop = arg;
	struct epoll_event *events = epollop->events;
	struct evepoll *evep;
	int i, res, timeout;

	if (lam_evsignal_deliver(&epollop->evsigmask) == -1)
		return (-1);

	timeout = tv->tv_sec * 1000 + tv->tv_usec / 1000;
        lam_mutex_unlock(&lam_event_lock);
	res = epoll_wait(epollop->epfd, events, epollop->nevents, timeout);
        lam_mutex_lock(&lam_event_lock);

	if (lam_evsignal_recalc(&epollop->evsigmask) == -1)
		return (-1);

	if (res == -1) {
		if (errno != EINTR) {
			log_error("epoll_wait");
			return (-1);
		}

		lam_evsignal_process();
		return (0);
	} else if (lam_evsignal_caught)
		lam_evsignal_process();

	LOG_DBG((LOG_MISC, 80, "%s: epoll_wait reports %d", __func__, res));

	for (i = 0; i < res; i++) {
		int which = 0;
		int what = events[i].events;
		struct lam_event *evread = NULL, *evwrite = NULL;

		evep = (struct evepoll *)events[i].data.ptr;
   
                if (what & EPOLLHUP)
                        what |= EPOLLIN | EPOLLOUT;
                else if (what & EPOLLERR)
                        what |= EPOLLIN | EPOLLOUT;

		if (what & EPOLLIN) {
			evread = evep->evread;
			which |= EV_READ;
		}

		if (what & EPOLLOUT) {
			evwrite = evep->evwrite;
			which |= EV_WRITE;
		}

		if (!which)
			continue;

		if (evread != NULL && !(evread->ev_events & EV_PERSIST))
			lam_event_del(evread);
		if (evwrite != NULL && evwrite != evread &&
		    !(evwrite->ev_events & EV_PERSIST))
			lam_event_del(evwrite);

		if (evread != NULL)
			lam_event_active(evread, EV_READ, 1);
		if (evwrite != NULL)
			lam_event_active(evwrite, EV_WRITE, 1);
	}

	return (0);
}


static int
epoll_add(void *arg, struct lam_event *ev)
{
	struct epollop *epollop = arg;
	struct epoll_event epev;
	struct evepoll *evep;
	int fd, op, events;

	if (ev->ev_events & EV_SIGNAL)
		return (lam_evsignal_add(&epollop->evsigmask, ev));

	fd = ev->ev_fd;
	if (fd >= epollop->nfds) {
		/* Extent the file descriptor array as necessary */
		if (epoll_recalc(epollop, fd) == -1)
			return (-1);
	}
	evep = &epollop->fds[fd];
	op = EPOLL_CTL_ADD;
	events = 0;
	if (evep->evread != NULL) {
		events |= EPOLLIN;
		op = EPOLL_CTL_MOD;
	}
	if (evep->evwrite != NULL) {
		events |= EPOLLOUT;
		op = EPOLL_CTL_MOD;
	}

	if (ev->ev_events & EV_READ)
		events |= EPOLLIN;
	if (ev->ev_events & EV_WRITE)
		events |= EPOLLOUT;

	epev.data.ptr = evep;
	epev.events = events;
	if (epoll_ctl(epollop->epfd, op, ev->ev_fd, &epev) == -1)
			return (-1);

	/* Update events responsible */
	if (ev->ev_events & EV_READ)
		evep->evread = ev;
	if (ev->ev_events & EV_WRITE)
		evep->evwrite = ev;

	return (0);
}

static int
epoll_del(void *arg, struct lam_event *ev)
{
	struct epollop *epollop = arg;
	struct epoll_event epev;
	struct evepoll *evep;
	int fd, events, op;
	int needwritedelete = 1, needreaddelete = 1;

	if (ev->ev_events & EV_SIGNAL)
		return (lam_evsignal_del(&epollop->evsigmask, ev));

	fd = ev->ev_fd;
	if (fd >= epollop->nfds)
		return (0);
	evep = &epollop->fds[fd];

	op = EPOLL_CTL_DEL;
	events = 0;

	if (ev->ev_events & EV_READ)
		events |= EPOLLIN;
	if (ev->ev_events & EV_WRITE)
		events |= EPOLLOUT;

	if ((events & (EPOLLIN|EPOLLOUT)) != (EPOLLIN|EPOLLOUT)) {
		if ((events & EPOLLIN) && evep->evwrite != NULL) {
			needwritedelete = 0;
			events = EPOLLOUT;
			op = EPOLL_CTL_MOD;
		} else if ((events & EPOLLOUT) && evep->evread != NULL) {
			needreaddelete = 0;
			events = EPOLLIN;
			op = EPOLL_CTL_MOD;
		}
	}

	epev.events = events;
	epev.data.ptr = evep;

	if (epoll_ctl(epollop->epfd, op, fd, &epev) == -1)
		return (-1);

	if (needreaddelete)
		evep->evread = NULL;
	if (needwritedelete)
		evep->evwrite = NULL;

	return (0);
}
