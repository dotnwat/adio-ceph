/*
 * Compile with:
 * cc -I/usr/local/include -o time-test time-test.c -L/usr/local/lib -levent
 */

#include <sys/types.h>
#include <sys/stat.h>
#ifndef WIN32
#include <sys/queue.h>
#include <unistd.h>
#else
#include <time.h>
#endif
#include <sys/time.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <event.h>

int lasttime;

void
timeout_cb(int fd, short event, void *arg)
{
	struct timeval tv;
	struct lam_event *timeout = arg;
	int newtime = time(NULL);

	printf("%s: called at %d: %d\n", __func__, newtime,
	    newtime - lasttime);
	lasttime = newtime;

	timerclear(&tv);
	tv.tv_sec = 2;
	lam_event_add(timeout, &tv);
}

int
main (int argc, char **argv)
{
	struct lam_event timeout;
	struct timeval tv;
 
	/* Initalize the event library */
	lam_event_init();

	/* Initalize one event */
	lam_evtimer_set(&timeout, timeout_cb, &timeout);

	timerclear(&tv);
	tv.tv_sec = 2;
	lam_event_add(&timeout, &tv);

	lasttime = time(NULL);
	
	lam_event_dispatch();

	return (0);
}

