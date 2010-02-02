/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2009, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#include "config.h"

#include <stdlib.h>
#include <string.h>

#include "vt_defs.h"
#include "vt_error.h"
#include "vt_thrd.h"

/* data structure which hold the mutex's raw monitor */
struct VTThrdMutex_struct
{
  jrawMonitorID m;
};

static jrawMonitorID threadCountMutex = NULL;
static jrawMonitorID mutexInitMutex = NULL;

static jvmtiEnv* jvmti       = NULL;
static uint32_t  threadCount = 1;

void VTThrd_initJava()
{
  static uint8_t initflag = 1;

  if (initflag)
  {
    jvmtiError error;
    char tname[VT_MAX_THREAD_NAME_LEN];
    uint32_t* tid;

    initflag = 0;

    /* store pointer to JVMTI's environment */
    jvmti = vt_jvmti_agent->jvmti;

    /* create ID for first thread (=0) */
    tid = (uint32_t*)malloc(sizeof(uint32_t));
    if (tid == NULL)
      vt_error();
    *tid = 0;

    /* put thread-ID to thread-specific data */
    error = (*jvmti)->SetThreadLocalStorage(jvmti, NULL, (void*)tid);
    vt_java_check_error(jvmti, error, "SetThreadLocalStorage");

    /* create raw monitor for thread count */
    error = (*jvmti)->CreateRawMonitor(jvmti, "thread count",
                                       &threadCountMutex);
    vt_java_check_error(jvmti, error, "CreateRawMonitor[thread count]");

    /* create raw monitor for mutex init */
    error = (*jvmti)->CreateRawMonitor(jvmti, "mutex init",
                                       &mutexInitMutex);
    vt_java_check_error(jvmti, error, "CreateRawMonitor[mutex init]");

#if defined(VT_METR)
/*    if (vt_metric_num() > 0)
      vt_metric_thread_init((long (*)(void))(pthread_self));*/
#endif /* VT_METR */

    /* get name of current thread */
    vt_java_get_thread_name(NULL, NULL, tname, sizeof(tname));

    /* create thread object for master thread */
    VTThrdv[0] = VTThrd_create(0, 0, tname);
    VTThrd_open(VTThrdv[0], 0);
  }
}

void VTThrd_registerThread(jthread thread, const char* tname)
{
  jvmtiError error;
  uint32_t *tid;

  /* check whether an ID is already created for this thread */
  error = (*jvmti)->GetThreadLocalStorage(jvmti, thread, (void**)&tid);
  vt_java_check_error(jvmti, error, "GetThreadLocalStorage");
  if (tid == NULL)
  {
    /* create new thread-ID */
    tid = (uint32_t*)malloc(sizeof(uint32_t));
    if (tid == NULL) vt_error();

    /* lock thread count */
    error = (*jvmti)->RawMonitorEnter(jvmti, threadCountMutex);
    vt_java_check_error(jvmti, error, "RawMonitorEnter");

    /* increment number of threads */
    *tid = threadCount++;

    /* unlock thread count */
    error = (*jvmti)->RawMonitorExit(jvmti, threadCountMutex);
    vt_java_check_error(jvmti, error, "RawMonitorEnter");

    /* put new ID to thread-specific data */
    error = (*jvmti)->SetThreadLocalStorage(jvmti, thread, (void*)tid);
    vt_java_check_error(jvmti, error, "SetThreadLocalStorage");

    /* create new thread object */
    vt_cntl_msg(2, "Dynamic thread creation. Thread #%d (%s)",
                *tid, tname ? tname : "unnamed");
    VTThrdv[*tid] = VTThrd_create(*tid, 0, tname);
    VTThrd_open(VTThrdv[*tid], *tid);
  }
}

uint32_t VTThrd_getThreadId()
{
  jvmtiError error;
  uint32_t *tid;

  /* get thread-ID from thread-specific data */
  error = (*jvmti)->GetThreadLocalStorage(jvmti, NULL, (void**)&tid);
  vt_java_check_error(jvmti, error, "GetThreadLocalStorage");

  if (tid == NULL && vt_jvmti_agent->vm_is_dead)
    return 0;
  else
    vt_assert(tid != NULL);

  return *tid;
}

void VTThrd_createMutex(VTThrdMutex** mutex)
{
  jvmtiError error;

  vt_assert(mutexInitMutex != NULL);

  error = (*jvmti)->RawMonitorEnter(jvmti, mutexInitMutex);
  vt_java_check_error(jvmti, error, "RawMonitorEnter");
  if (*mutex == NULL)
  {
    static uint8_t rawmon_id = 0;
    char rawmon_name[10];

    *mutex = (VTThrdMutex*)malloc(sizeof(VTThrdMutex));
    if (*mutex == NULL)
      vt_error();

    snprintf(rawmon_name, sizeof(rawmon_name) - 1, "rawmon%d", rawmon_id++);

    error = (*jvmti)->CreateRawMonitor(jvmti, rawmon_name,
                                       &((*mutex)->m));
    vt_java_check_error(jvmti, error, "CreateRawMonitor");
  }
  error = (*jvmti)->RawMonitorExit(jvmti, mutexInitMutex);
  vt_java_check_error(jvmti, error, "RawMonitorExit");
}

void VTThrd_deleteMutex(VTThrdMutex** mutex)
{
  jvmtiError error;

  if (*mutex == NULL) return;

  error = (*jvmti)->RawMonitorEnter(jvmti, mutexInitMutex);
  vt_java_check_error(jvmti, error, "RawMonitorEnter");
  if (*mutex != NULL )
  {
    error = (*jvmti)->DestroyRawMonitor(jvmti, (*mutex)->m);
    vt_java_check_error(jvmti, error, "DestroyRawMonitor");
    free(*mutex);
    *mutex = NULL;
  }
  error = (*jvmti)->RawMonitorExit(jvmti, mutexInitMutex);
  vt_java_check_error(jvmti, error, "RawMonitorExit");
}

void VTThrd_lock(VTThrdMutex** mutex)
{
  jvmtiError error;

  if (*mutex == NULL)
    VTThrd_createMutex(mutex);

  error = (*jvmti)->RawMonitorEnter(jvmti, (*mutex)->m);
  vt_java_check_error(jvmti, error, "RawMonitorEnter");
}

void VTThrd_unlock(VTThrdMutex** mutex)
{
  jvmtiError error;

  vt_assert(*mutex != NULL);

  error = (*jvmti)->RawMonitorExit(jvmti, (*mutex)->m);
  vt_java_check_error(jvmti, error, "RawMonitorExit");
}
