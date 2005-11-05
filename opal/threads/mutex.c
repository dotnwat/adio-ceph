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

#include "ompi_config.h"

#include "opal/threads/mutex.h"

/*
 * Default to a safe value
 */
bool opal_uses_threads = (bool) OMPI_HAVE_THREAD_SUPPORT;


#ifdef WIN32

#include <windows.h>

static void opal_mutex_construct(opal_mutex_t *m)
{
    InterlockedExchange(&m->m_lock, 0);
}

static void opal_mutex_destruct(opal_mutex_t *m)
{
}

#else

static void opal_mutex_construct(opal_mutex_t *m)
{
#if OMPI_HAVE_POSIX_THREADS
    pthread_mutex_init(&m->m_lock_pthread, 0);
#endif
#if OPAL_HAVE_ATOMIC_SPINLOCKS
    opal_atomic_init( &m->m_lock_atomic, OPAL_ATOMIC_UNLOCKED );
#endif
}

static void opal_mutex_destruct(opal_mutex_t *m)
{
#if OMPI_HAVE_POSIX_THREADS
    pthread_mutex_destroy(&m->m_lock_pthread);
#endif
}

#endif

OBJ_CLASS_INSTANCE(opal_mutex_t,
                   opal_object_t,
                   opal_mutex_construct,
                   opal_mutex_destruct);
