/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
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

#ifndef OMPI_SYS_ARCH_ATOMIC_H
#define OMPI_SYS_ARCH_ATOMIC_H 1


#if OMPI_WANT_SMP_LOCKS
#define MB() __asm__  __volatile__ ("" : : : "memory")
#else
#define MB()
#endif

#ifdef OMPI_GENERATE_ASM_FILE
struct ompi_lock_t {
    union {
        volatile int lock;         /**< The lock address (an integer) */
        volatile unsigned char sparc_lock; /**< The lock address on sparc */
        char padding[sizeof(int)]; /**< Array for optional padding */
    } u;
};
typedef struct ompi_lock_t ompi_lock_t;
#endif

/**********************************************************************
 *
 * Define constants for UltraSparc 64
 *
 *********************************************************************/
#define OMPI_HAVE_ATOMIC_MEM_BARRIER 1

#define OMPI_HAVE_ATOMIC_CMPSET_32 0
#define OMPI_HAVE_ATOMIC_CMPSET_64 0

#define OMPI_HAVE_ATOMIC_MATH_32 1
#define OMPI_HAVE_ATOMIC_SUB_32 1
#define OMPI_HAVE_ATOMIC_ADD_32 1

#define OMPI_HAVE_ATOMIC_SPINLOCKS 1

/**********************************************************************
 *
 * Memory Barriers
 *
 *********************************************************************/
#if OMPI_GCC_INLINE_ASSEMBLY

static inline void ompi_atomic_mb(void)
{
    MB();
}


static inline void ompi_atomic_rmb(void)
{
    MB();
}


static inline void ompi_atomic_wmb(void)
{
    MB();
}

#endif /* OMPI_GCC_INLINE_ASSEMBLY */


/**********************************************************************
 *
 * Atomic spinlocks
 *
 *********************************************************************/
#if OMPI_GCC_INLINE_ASSEMBLY

/* for these, the lock is held whenever lock.sparc_lock != 0.  We
   attempt to leave it as OMPI_LOCKED whenever possible */


static inline void ompi_atomic_init(ompi_lock_t* lock, int value)
{
    lock->u.sparc_lock = (unsigned char) value;
}


static inline int ompi_atomic_trylock(ompi_lock_t *lock)
{
    unsigned char result;

    /* try to load the lock byte (atomically making the memory byte
       contain all 1s).  If the byte used to be 0, we now have the
       lock.  Otherwise, someone else has the lock.  Either way, the
       lock is now held. */
    __asm__ __volatile__ ("\t"
                          "ldstub [%1], %0"
                          : "=r"(result)
                          : "r"(&(lock->u.sparc_lock))
                          : "memory");
    return (result == 0);
}


static inline void ompi_atomic_lock(ompi_lock_t *lock)
{
    /* From page 264 of The SPARC Architecture Manual, Version 8 */
    __asm__ __volatile__ (
                          "retry:               \n\t"
                          "ldstub [%0], %%l0    \n\t"
                          "tst    %%l0          \n\t"
                          "be     out           \n\t"
                          "nop                  \n"
                          "loop:                \n\t"
                          "ldub   [%0], %%l0    \n\t"
                          "tst    %%l0          \n\t"
                          "bne    loop          \n\t"
                          "nop                  \n\t"
                          "ba,a   retry         \n"
                          "out:                 \n\t"
                          "nop"
                          :
                          : "r"(&(lock->u.sparc_lock))
                          : "%l0", "memory");
}


static inline void ompi_atomic_unlock(ompi_lock_t *lock)
{
    /* 0 out that byte in memory */
    __asm__ __volatile__ ("\t"
                          "stbar             \n\t"
                          "stb %%g0, [%0]    \n\t"
                          :
                          : "r"(&(lock->u.sparc_lock))
                          : "memory");
}

#endif /* OMPI_GCC_INLINE_ASSEMBLY */


#endif /* ! OMPI_SYS_ARCH_ATOMIC_H */
