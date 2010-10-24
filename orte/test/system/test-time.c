/*
 * Compile with:
 * cc -I/usr/local/include -o time-test time-test.c -L/usr/local/lib -levent
 */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#ifndef WIN32
#include <unistd.h>
#include <sys/time.h>
#endif
#include <errno.h>

#include "opal/runtime/opal.h"
#include "opal/mca/event/event.h"

int called = 0;

#define NEVENT	20000

struct opal_event_t *ev[NEVENT];

static int
rand_int(int n)
{
#ifdef WIN32
	return (int)(rand() % n);
#else
	return (int)(random() % n);
#endif
}

static void
time_cb(evutil_socket_t fd, short event, void *arg)
{
	struct timeval tv;
	int i, j;

	called++;

	if (called < 10*NEVENT) {
		for (i = 0; i < 10; i++) {
			j = rand_int(NEVENT);
			tv.tv_sec = 0;
			tv.tv_usec = rand_int(50000);
			if (tv.tv_usec % 2)
				opal_evtimer_add(ev[j], &tv);
			else
				opal_evtimer_del(ev[j]);
		}
	}
}

int
main(int argc, char **argv)
{
	struct timeval tv;
	int i;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int	err;

	wVersionRequested = MAKEWORD(2, 2);

	err = WSAStartup(wVersionRequested, &wsaData);
#endif

	/* Initalize the event library */
	opal_init(argc, argv);

	for (i = 0; i < NEVENT; i++) {
		/* Initalize one event */
		ev[i] = opal_evtimer_new(time_cb, ev[i]);
		tv.tv_sec = 0;
		tv.tv_usec = rand_int(50000);
		opal_evtimer_add(ev[i], &tv);
	}

	opal_event.dispatch();

        opal_finalize();
	return (called < NEVENT);
}

