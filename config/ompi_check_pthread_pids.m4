dnl
dnl Copyright (c) 2004-2005 The Trustees of Indiana University.
dnl                         All rights reserved.
dnl Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
dnl                         All rights reserved.
dnl Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
dnl                         University of Stuttgart.  All rights reserved.
dnl Copyright (c) 2004-2005 The Regents of the University of California.
dnl                         All rights reserved.
dnl $COPYRIGHT$
dnl 
dnl Additional copyrights may follow
dnl 
dnl $HEADER$
dnl

define(OMPI_CHECK_PTHREAD_PIDS,[
#
# Arguments: none
#
# Dependencies: None
#
# Sets:
#  OMPI_THREADS_HAVE_DIFFERENT_PIDS (variable)
#
# Test for Linux-like threads in the system. We will need to handle things like
# getpid() differently in the case of a Linux-like threads model.
#

AH_TEMPLATE([OMPI_THREADS_HAVE_DIFFERENT_PIDS],
    [Do threads have different pids (pthreads on linux)])

AC_MSG_CHECKING([if threads have different pids (pthreads on linux)])
CPPFLAGS_save="$CPPFLAGS"
CPPFLAGS="$CPPFLAGS $THREAD_CPPFLAGS"
LDFLAGS_save="$LDFLAGS"
LDFLAGS="$LDFLAGS $THREAD_LDFLAGS"
LIBS_save="$LIBS"
LIBS="$LIBS $THREAD_LIBS"
AC_TRY_RUN([#include <pthread.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
void *checkpid(void *arg);
int main(int argc, char* argv[]) {
  pthread_t thr;
  int pid, retval;
  pid = getpid();
  pthread_create(&thr, NULL, checkpid, &pid);
  pthread_join(thr, (void **) &retval);
  exit(retval);
}
void *checkpid(void *arg) {
   int ret;
   int ppid = *((int *) arg);
   if (ppid == getpid())
     ret = 0;
   else
     ret = 1;
     pthread_exit((void *) ret);
}], 
[MSG=no OMPI_THREADS_HAVE_DIFFERENT_PIDS=0], 
[MSG=yes OMPI_THREADS_HAVE_DIFFERENT_PIDS=1])

CPPFLAGS="$CPPFLAGS_save"
LDFLAGS="$LDFLAGS_save"
LIBS="$LIBS_save"

AC_MSG_RESULT([$MSG])
AC_DEFINE_UNQUOTED(OMPI_THREADS_HAVE_DIFFERENT_PIDS, $OMPI_THREADS_HAVE_DIFFERENT_PIDS)

#
# if pthreads is not available, then the system does not have an insane threads
# model
#
unset MSG])dnl
