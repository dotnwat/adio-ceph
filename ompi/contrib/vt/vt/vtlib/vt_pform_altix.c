/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2008, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#include "config.h"

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include "vt_pform.h"
#include "vt_error.h"

#ifndef TIMER_PAPI_REAL_CYC
#  define TIMER_PAPI_REAL_CYC 10
#endif
#ifndef TIMER_PAPI_REAL_USEC
#  define TIMER_PAPI_REAL_USEC 11
#endif

#if TIMER != TIMER_MMTIMER && \
    TIMER != TIMER_CLOCK_GETTIME && \
    TIMER != TIMER_PAPI_REAL_CYC && \
    TIMER != TIMER_PAPI_REAL_USEC
# error Unknown timer specified! Check the timer configuration in 'config.h'.
#endif

#if TIMER == TIMER_MMTIMER
# include <errno.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <fcntl.h>
# include <sys/ioctl.h>
# include <sys/mman.h>

# if defined(HAVE_SN_MMTIMER_H) && HAVE_SN_MMTIMER_H
#   include <sn/mmtimer.h>
# elif defined(HAVE_LINUX_MMTIMER_H) && HAVE_LINUX_MMTIMER_H
#   include <linux/mmtimer.h>
# else
#   include <mmtimer.h>
# endif

# ifndef MMTIMER_FULLNAME
#   define MMTIMER_FULLNAME "/dev/mmtimer"
# endif

  static volatile unsigned long *mmdev_timer_addr;
  static uint64_t mmdev_ticks_per_sec;
#elif TIMER == TIMER_CLOCK_GETTIME
# include <time.h>
  static uint64_t vt_time_base = 0;
#elif TIMER == TIMER_PAPI_REAL_CYC
# include <vt_metric.h>
#elif TIMER == TIMER_PAPI_REAL_USEC
# include <vt_metric.h>
  static uint64_t vt_time_base = 0;
#endif

/* platform specific initialization */
void vt_pform_init() {
#if TIMER == TIMER_MMTIMER
  int fd;
  unsigned long femtosecs_per_tick = 0;
  int offset;

  if((fd = open(MMTIMER_FULLNAME, O_RDONLY)) == -1) {
    vt_error_msg("Failed to open " MMTIMER_FULLNAME);
  }

  if ((offset = ioctl(fd, MMTIMER_GETOFFSET, 0)) == -ENOSYS) {
    vt_error_msg("Cannot get mmtimer offset");
  }

  if ((mmdev_timer_addr = mmap(0, getpagesize(), PROT_READ, MAP_SHARED, fd, 0))
       == MAP_FAILED) {
    vt_error_msg("Cannot mmap mmtimer");
  }
  mmdev_timer_addr += offset;

  ioctl(fd, MMTIMER_GETRES, &femtosecs_per_tick);
  mmdev_ticks_per_sec = (uint64_t)(1.0 / (1e-15 * femtosecs_per_tick));

  close(fd);
#elif TIMER == TIMER_CLOCK_GETTIME
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  vt_time_base = tp.tv_sec - (tp.tv_sec & 0xFF);
#elif TIMER == TIMER_PAPI_REAL_USEC
  vt_time_base = vt_metric_real_usec();
#endif
}

/* directory of global file system  */
char* vt_pform_gdir() {
  return ".";
}

/* directory of local file system  */
char* vt_pform_ldir() {
  #ifdef PFORM_LDIR
    return PFORM_LDIR;
  #else
    return "/tmp";
  #endif
}

/* clock resolution */
uint64_t vt_pform_clockres() {
#if TIMER == TIMER_MMTIMER
  return mmdev_ticks_per_sec;
#elif TIMER == TIMER_CLOCK_GETTIME
  return 1e9;
#elif TIMER == TIMER_PAPI_REAL_CYC
  return vt_metric_clckrt();
#elif TIMER == TIMER_PAPI_REAL_USEC
  return 1e6;
#endif
}

/* local or global wall-clock time */
uint64_t vt_pform_wtime() {
#if TIMER == TIMER_MMTIMER
  return *mmdev_timer_addr;
#elif TIMER == TIMER_CLOCK_GETTIME
  struct timespec tp;
  clock_gettime(CLOCK_REALTIME, &tp);
  return ((tp.tv_sec - vt_time_base) * 1e9) + tp.tv_nsec;
#elif TIMER == TIMER_PAPI_REAL_CYC
  return vt_metric_real_cyc();
#elif TIMER == TIMER_PAPI_REAL_USEC
  return vt_metric_real_usec() - vt_time_base;
#endif
}

/* unique numeric SMP-node identifier */
long vt_pform_node_id() {
  return gethostid();
}

/* unique string SMP-node identifier */
char* vt_pform_node_name() {
  static char node[64];
  char *dlmt;
  gethostname(node, 64);
  if ( (dlmt = strchr(node, '.')) != NULL ) *dlmt = '\0';
    return node;
}

/* number of CPUs */
int vt_pform_num_cpus() {
  return sysconf(_SC_NPROCESSORS_CONF);
}
