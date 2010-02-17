/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2010, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif

#include "config.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <stdint.h>
#include <sys/resource.h>
#include <stdarg.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <dlfcn.h>

#include "vt_iowrap.h"
#include "vt_iowrap_helper.h"
#include "vt_defs.h"
#include "vt_env.h"
#include "vt_error.h"
#include "vt_memhook.h"
#include "vt_pform.h"
#include "vt_trc.h"
#include "vt_thrd.h"


#if defined(_FILE_OFFSET_BITS) && (_FILE_OFFSET_BITS > SIZEOF_LONG)
#define OFF_T_STRARG "%lli"
#else
#define OFF_T_STRARG "%li"
#endif

#define DISABLE_IORECORD 1

/** 
 * Boolean macro for checking if we shall trace right now 
 * Tracing can be disabled globally via vt_io_tracing_enabled = 0 or
 * for each function separately via iofunctions[IDX].traceme = 0
 */
#define DO_TRACE() \
	( vt_is_alive && \
	  VTTHRD_MY_VTTHRD && \
	  VTTHRD_IO_TRACING_ENABLED(VTTHRD_MY_VTTHRD) && \
	  iofunctions[FUNC_IDX(VT_IOWRAP_THISFUNCNAME)].traceme )

struct iofunctions iofunctions[NUMFUNCTIONS];
static void *iolib_handle = NULL;

/** \brief	VT file id for I/O, set in init macro */
static uint32_t	vt_fid = VT_NO_ID;

/** \brief      VT file id to be used when user supplied fd == -1 */
static uint32_t invalid_fd_fid = 0;

/*********************************************/
/*                                           */ 
/*     helper functions                      */
/*                                           */ 
/*********************************************/

/* exits vt if we can not locate a symbol via dlsym
 */
static void symload_fail(const char *str, const char *errstr)
{
  printf("VampirTrace: FATAL: dlsym() error for symbol %s: %s\n",
	 str, errstr);
  exit(EXIT_FAILURE);
}

static void get_iolib_handle(void)
{
  if( !iolib_handle ) {
    char *iolib_pathname = vt_env_iolibpathname();

    if( iolib_pathname == NULL ) {
#ifndef DEFAULT_LIBC_PATHNAME
#error VampirTrace is not properly configured, DEFAULT_LIBC_PATHNAME is not set! Please report this incident to vampirsupport@zih.tu-dresden.de
#endif
      iolib_pathname = DEFAULT_LIBC_PATHNAME;
    }
    (void)dlerror();
    iolib_handle = dlopen( iolib_pathname, RTLD_LAZY | RTLD_LOCAL );
    if( !iolib_handle ) {
      printf("VampirTrace: FATAL: dlopen(\"%s\") error: %s\n", iolib_pathname, dlerror());
      exit(EXIT_FAILURE);
    }
  }
}

/*********************************************/
/*                                           */ 
/*     init & finalize                       */
/*                                           */ 
/*********************************************/

int (*libc_fprintf)(FILE *, const char *, ...) = NULL;
void vt_iowrap_externals_init()
{
	static int need_init = 1;
	if( need_init ) {
		union {
			void *p;
			void (*f)(void);
		} lib_func;
		get_iolib_handle();
		(void)dlerror();
		lib_func.p = dlsym( iolib_handle, "fprintf" );
		libc_fprintf = (int (*)(FILE *, const char *, ...))lib_func.f;
		if( !libc_fprintf )
			symload_fail( "fprintf", dlerror() );
		need_init = 0;
	}
}

void vt_iowrap_init()
{
	static int lib_inited = 0;

	vt_debug_msg(DBG_INIT, "iowrap_init: init check");
	if (lib_inited)
		return;
	lib_inited = 1;

        max_open_files = get_max_open_files();
        fd_to_vampirid=(vampir_file_t *) malloc(max_open_files*sizeof(vampir_file_t));
        if( fd_to_vampirid==NULL )
                vt_error_msg("iowrap_init: unable to allocate memory for file descriptor mapping");
        memset( fd_to_vampirid, 0, max_open_files*sizeof(vampir_file_t));

        file_group_id_stdio = vt_def_file_group("stdio");
        file_group_id_rest  = vt_def_file_group("fileio");

        vt_iofile_open( "<STDIN>", 0 );
        vt_iofile_open( "<STDOUT>", 1 );
        vt_iofile_open( "<STDERR>", 2 );
        invalid_fd_fid = vt_iofile_id( "<INVALID FILE (fd==-1)>" );

        get_iolib_handle();

        VT_IOWRAP_INIT_FUNC(open);
	VT_IOWRAP_INIT_FUNC(creat);
	VT_IOWRAP_INIT_FUNC(close);
	VT_IOWRAP_INIT_FUNC(dup);
	VT_IOWRAP_INIT_FUNC(dup2);
	VT_IOWRAP_INIT_FUNC(lseek);
	VT_IOWRAP_INIT_FUNC(read);
	VT_IOWRAP_INIT_FUNC(write);
	VT_IOWRAP_INIT_FUNC(readv);
	VT_IOWRAP_INIT_FUNC(writev);
	VT_IOWRAP_INIT_FUNC(pread);
	VT_IOWRAP_INIT_FUNC(pwrite);
	VT_IOWRAP_INIT_FUNC(fdopen);
	VT_IOWRAP_INIT_FUNC(fopen);
	VT_IOWRAP_INIT_FUNC(fclose);
	VT_IOWRAP_INIT_FUNC(fseek);
#if defined(HAVE_FSEEKO) && HAVE_FSEEKO
	VT_IOWRAP_INIT_FUNC(fseeko);
#endif /* HAVE_FSEEKO */
	VT_IOWRAP_INIT_FUNC(rewind);
	VT_IOWRAP_INIT_FUNC(fsetpos);
	VT_IOWRAP_INIT_FUNC(fread);
	VT_IOWRAP_INIT_FUNC(fwrite);
	VT_IOWRAP_INIT_FUNC(fgetc);
	VT_IOWRAP_INIT_FUNC(getc);
	VT_IOWRAP_INIT_FUNC(fgets);
	VT_IOWRAP_INIT_FUNC(gets);
	VT_IOWRAP_INIT_FUNC(fputc);
	VT_IOWRAP_INIT_FUNC(putc);
	VT_IOWRAP_INIT_FUNC(fputs);
	VT_IOWRAP_INIT_FUNC(puts);
	VT_IOWRAP_INIT_FUNC(fscanf);
	VT_IOWRAP_INIT_FUNC(fprintf);
        VT_IOWRAP_INIT_FUNC(unlink);
        VT_IOWRAP_INIT_FUNC(flockfile);
        VT_IOWRAP_INIT_FUNC(ftrylockfile);
        VT_IOWRAP_INIT_FUNC(funlockfile);
        VT_IOWRAP_INIT_FUNC(lockf);
        VT_IOWRAP_INIT_FUNC(fcntl);
#if defined(HAVE_OPEN64) && HAVE_OPEN64
        VT_IOWRAP_INIT_FUNC(open64);
#endif /* HAVE_OPEN64 */
#if defined(HAVE_CREAT64) && HAVE_CREAT64
        VT_IOWRAP_INIT_FUNC(creat64);
#endif /* HAVE_CREAT64 */
#if defined(HAVE_LSEEK64) && HAVE_LSEEK64
        VT_IOWRAP_INIT_FUNC(lseek64);
#endif /* HAVE_LSEEK64 */
#if defined(HAVE_PREAD64) && HAVE_PREAD64
        VT_IOWRAP_INIT_FUNC(pread64);
#endif /* HAVE_PREAD64 */
#if defined(HAVE_PWRITE64) && HAVE_PWRITE64
        VT_IOWRAP_INIT_FUNC(pwrite64);
#endif /* HAVE_PWRITE64 */
#if defined(HAVE_FOPEN64) && HAVE_FOPEN64
        VT_IOWRAP_INIT_FUNC(fopen64);
#endif /* HAVE_FOPEN64 */
#if defined(HAVE_FSEEKO64) && HAVE_FSEEKO64
        VT_IOWRAP_INIT_FUNC(fseeko64);
#endif /* HAVE_FSEEKO64 */
#if defined(HAVE_FSETPOS64) && HAVE_FSETPOS64
        VT_IOWRAP_INIT_FUNC(fsetpos64);
#endif /* HAVE_FSETPOS64 */
#if defined(HAVE_FLOCKFILE) && HAVE_FLOCKFILE
        VT_IOWRAP_INIT_FUNC(flockfile);
#endif /* HAVE_FLOCKFILE */
#if defined(HAVE_FTRYLOCKFILE) && HAVE_FTRYLOCKFILE
        VT_IOWRAP_INIT_FUNC(ftrylockfile);
#endif /* HAVE_FTRYLOCKFILE */
#if defined(HAVE_FUNLOCKFILE) && HAVE_FUNLOCKFILE
        VT_IOWRAP_INIT_FUNC(funlockfile);
#endif /* HAVE_FLOCKFILE */
#if defined(HAVE_LOCKF) && HAVE_LOCKF
        VT_IOWRAP_INIT_FUNC(lockf)
#endif /* HAVE_LOCKF */
}

void vt_iowrap_reg()
{
	vt_debug_msg(DBG_INIT, "iowrap_reg: vt_def_scl_file()");
	vt_fid = vt_def_scl_file( "I/O" );

	VT_IOWRAP_REG_FUNC(open);
	VT_IOWRAP_REG_FUNC(creat);
	VT_IOWRAP_REG_FUNC(close);
	VT_IOWRAP_REG_FUNC(dup);
	VT_IOWRAP_REG_FUNC(dup2);
	VT_IOWRAP_REG_FUNC(lseek);
	VT_IOWRAP_REG_FUNC(read);
	VT_IOWRAP_REG_FUNC(write);
	VT_IOWRAP_REG_FUNC(readv);
	VT_IOWRAP_REG_FUNC(writev);
	VT_IOWRAP_REG_FUNC(pread);
	VT_IOWRAP_REG_FUNC(pwrite);
	VT_IOWRAP_REG_FUNC(fdopen);
	VT_IOWRAP_REG_FUNC(fopen);
	VT_IOWRAP_REG_FUNC(fclose);
	VT_IOWRAP_REG_FUNC(fseek);
#if defined(HAVE_FSEEKO) && HAVE_FSEEKO
	VT_IOWRAP_REG_FUNC(fseeko);
#endif /* HAVE_FSEEKO */
	VT_IOWRAP_REG_FUNC(rewind);
	VT_IOWRAP_REG_FUNC(fsetpos);
	VT_IOWRAP_REG_FUNC(fread);
	VT_IOWRAP_REG_FUNC(fwrite);
	VT_IOWRAP_REG_FUNC(fgetc);
	VT_IOWRAP_REG_FUNC(getc);
	VT_IOWRAP_REG_FUNC(fgets);
	VT_IOWRAP_REG_FUNC(gets);
	VT_IOWRAP_REG_FUNC(fputc);
	VT_IOWRAP_REG_FUNC(putc);
	VT_IOWRAP_REG_FUNC(fputs);
	VT_IOWRAP_REG_FUNC(puts);
	VT_IOWRAP_REG_FUNC(fscanf);
	VT_IOWRAP_REG_FUNC(fprintf);
        VT_IOWRAP_REG_FUNC(unlink);
        VT_IOWRAP_REG_FUNC(flockfile);
        VT_IOWRAP_REG_FUNC(ftrylockfile);
        VT_IOWRAP_REG_FUNC(funlockfile);
        VT_IOWRAP_REG_FUNC(lockf);
        VT_IOWRAP_REG_FUNC(fcntl);
#if defined(HAVE_OPEN64) && HAVE_OPEN64
        VT_IOWRAP_REG_FUNC(open64);
#endif /* HAVE_OPEN64 */
#if defined(HAVE_CREAT64) && HAVE_CREAT64
        VT_IOWRAP_REG_FUNC(creat64);
#endif /* HAVE_CREAT64 */
#if defined(HAVE_LSEEK64) && HAVE_LSEEK64
        VT_IOWRAP_REG_FUNC(lseek64);
#endif /* HAVE_LSEEK64 */
#if defined(HAVE_PREAD64) && HAVE_PREAD64
        VT_IOWRAP_REG_FUNC(pread64);
#endif /* HAVE_PREAD64 */
#if defined(HAVE_PWRITE64) && HAVE_PWRITE64
        VT_IOWRAP_REG_FUNC(pwrite64);
#endif /* HAVE_PWRITE64 */
#if defined(HAVE_FOPEN64) && HAVE_FOPEN64
        VT_IOWRAP_REG_FUNC(fopen64);
#endif /* HAVE_FOPEN64 */
#if defined(HAVE_FSEEKO64) && HAVE_FSEEKO64
        VT_IOWRAP_REG_FUNC(fseeko64);
#endif /* HAVE_FSEEKO64 */
#if defined(HAVE_FSETPOS64) && HAVE_FSETPOS64
        VT_IOWRAP_REG_FUNC(fsetpos64);
#endif /* HAVE_FSETPOS64 */
#if defined(HAVE_FLOCKFILE) && HAVE_FLOCKFILE
        VT_IOWRAP_REG_FUNC(flockfile);
#endif /* HAVE_FLOCKFILE */
#if defined(HAVE_FTRYLOCKFILE) && HAVE_FTRYLOCKFILE
        VT_IOWRAP_REG_FUNC(ftrylockfile);
#endif /* HAVE_FTRYLOCKFILE */
#if defined(HAVE_FUNLOCKFILE) && HAVE_FUNLOCKFILE
        VT_IOWRAP_REG_FUNC(funlockfile);
#endif /* HAVE_FLOCKFILE */
#if defined(HAVE_LOCKF) && HAVE_LOCKF
        VT_IOWRAP_REG_FUNC(lockf)
#endif /* HAVE_LOCKF */
}

void vt_iowrap_finalize()
{
        if( fd_to_vampirid!=NULL )
                free(fd_to_vampirid);
        dlclose(iolib_handle);
}

/*********************************************/
/*                                           */ 
/*     wrapper                               */
/*                                           */ 
/*********************************************/

int open(const char *path, int flags, ...)
{
#define VT_IOWRAP_THISFUNCNAME open
	mode_t mode = 0;
	int ret;
	uint64_t enter_time;

        /* checks whether tracing is enabled and initializes
	   if necessary */
	VT_IOWRAP_INIT_IOFUNC_OPEN();

	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		/* If mode_t is narrower than int, use the promoted type (int),
		   not mode_t. */
#if SIZEOF_MODE_T < SIZEOF_INT
		mode = va_arg(ap, int);
#else
		mode = va_arg(ap, mode_t);
#endif
		va_end(ap);
	}

        /* checks whether I/O tracing is enabled
	   if not, executes the I/O function and returns */
	VT_IOWRAP_CHECK_TRACING3(path, flags, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s, %i", path, mode);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, path, flags, mode);

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==-1, ret );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_OPEN64) && HAVE_OPEN64
int open64(const char *path, int flags, ...)
{
#define VT_IOWRAP_THISFUNCNAME open64
	mode_t mode = 0;
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC_OPEN();

	if (flags & O_CREAT) {
		va_list ap;
		va_start(ap, flags);
		/* If mode_t is narrower than int, use the promoted type (int),
		   not mode_t. */
#if SIZEOF_MODE_T < SIZEOF_INT
		mode = va_arg(ap, int);
#else
		mode = va_arg(ap, mode_t);
#endif
		va_end(ap);
	}

	VT_IOWRAP_CHECK_TRACING3(path, flags, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s", path);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, path, flags, mode);

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==-1, ret);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_OPEN64 */


int creat(const char *path, mode_t mode)
{
#define VT_IOWRAP_THISFUNCNAME creat
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC_OPEN();

	VT_IOWRAP_CHECK_TRACING2(path, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s", path);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, path, mode);

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==-1, ret);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_CREAT64) && HAVE_CREAT64
int creat64(const char *path, mode_t mode)
{
#define VT_IOWRAP_THISFUNCNAME creat64
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC_OPEN();

	VT_IOWRAP_CHECK_TRACING2(path, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s", path);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, path, mode);

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==-1, ret);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_CREAT64 */


int dup(int oldfd)
{
#define VT_IOWRAP_THISFUNCNAME dup
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(oldfd);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", oldfd);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, oldfd);

	VT_IOWRAP_LEAVE_IOFUNC_DUP( ret==-1, oldfd, ret);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int dup2(int oldfd, int newfd)
{
#define VT_IOWRAP_THISFUNCNAME dup2
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(oldfd, newfd);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i", oldfd, newfd);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, oldfd, newfd);

	VT_IOWRAP_LEAVE_IOFUNC_DUP( ret!=newfd, oldfd, ret);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int close(int fd)
{
#define VT_IOWRAP_THISFUNCNAME close
	int ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(fd);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", fd);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, fd);

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


off_t lseek(int fd, off_t offset, int whence)
{
#define VT_IOWRAP_THISFUNCNAME lseek
	off_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, offset, whence);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, " OFF_T_STRARG ", %i", fd, offset, whence);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, offset, whence);

	VT_IOWRAP_LEAVE_IOFUNC( ret==(off_t)-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_LSEEK64) && HAVE_LSEEK64
off64_t lseek64(int fd, off64_t offset, int whence)
{
#define VT_IOWRAP_THISFUNCNAME lseek64
	off64_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, offset, whence);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %lli, %i", fd, (long long)offset, whence);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, offset, whence);

	VT_IOWRAP_LEAVE_IOFUNC( ret==(off64_t)-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_LSEEK64 */


ssize_t read(int fd, void *buf, size_t count)
{
#define VT_IOWRAP_THISFUNCNAME read
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, buf, count);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu", fd, count);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, buf, count);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


ssize_t write(int fd, const void *buf, size_t count)
{
#define VT_IOWRAP_THISFUNCNAME write
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, buf, count);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu", fd, count);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, buf, count);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


ssize_t readv(int fd, const struct iovec *iov, int count)
{
#define VT_IOWRAP_THISFUNCNAME readv
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, iov, count);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i iovecs", fd, count);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, iov, count);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


ssize_t writev(int fd, const struct iovec *iov, int count)
{
#define VT_IOWRAP_THISFUNCNAME writev
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(fd, iov, count);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i iovecs", fd, count);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, iov, count);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


ssize_t pread(int fd, void *buf, size_t count, off_t offset)
{
#define VT_IOWRAP_THISFUNCNAME pread
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(fd, buf, count, offset);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu, " OFF_T_STRARG, fd, count, offset);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, fd, buf, count, offset);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


ssize_t pwrite(int fd, const void *buf, size_t count, off_t offset)
{
#define VT_IOWRAP_THISFUNCNAME pwrite
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(fd, buf, count, offset);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu, " OFF_T_STRARG, fd, count, offset);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, fd, buf, count, offset);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_PREAD64) && HAVE_PREAD64
ssize_t pread64(int fd, void *buf, size_t count, off64_t offset)
{
#define VT_IOWRAP_THISFUNCNAME pread64
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(fd, buf, count, offset);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu, %lli", fd, count, (long long)offset);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, fd, buf, count, offset);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_PREAD64 */


#if defined(HAVE_PWRITE64) && HAVE_PWRITE64
ssize_t pwrite64(int fd, const void *buf, size_t count, off64_t offset)
{
#define VT_IOWRAP_THISFUNCNAME pwrite64
	ssize_t ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(fd, buf, count, offset);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu, %lli", fd, count, (long long)offset);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, fd, buf, count, offset);
	num_bytes = ret;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, fd);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_PWRITE64 */


FILE *fopen(const char *path, const char *mode)
{
#define VT_IOWRAP_THISFUNCNAME fopen
	FILE *ret;
	uint64_t enter_time;
        int tmp;

	VT_IOWRAP_INIT_IOFUNC_OPEN();

	VT_IOWRAP_CHECK_TRACING2(path, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s, %s", path, mode);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, path, mode);
        tmp=(ret!=NULL) ? fileno(ret): 0;

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==NULL, tmp);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_FOPEN64) && HAVE_FOPEN64
FILE *fopen64(const char *path, const char *mode)
{
#define VT_IOWRAP_THISFUNCNAME fopen64
	FILE *ret;
	uint64_t enter_time;
        int tmp;

	VT_IOWRAP_INIT_IOFUNC_OPEN();

	VT_IOWRAP_CHECK_TRACING2(path, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s, %s", path, mode);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, path, mode);
        tmp=(ret!=NULL) ? fileno(ret): 0;

	VT_IOWRAP_LEAVE_IOFUNC_OPEN( ret==NULL, tmp);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FOPEN64 */


FILE *fdopen(int fd, const char *mode)
{
#define VT_IOWRAP_THISFUNCNAME fdopen
	FILE *ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(fd, mode);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %s", fd, mode);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, fd, mode);

	VT_IOWRAP_LEAVE_IOFUNC( ret==NULL, fd );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fclose(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fclose
	int ret;
        int tmp;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i",
		stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

        tmp = (stream!=NULL) ? fileno(stream) : 0;
	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fseek(FILE *stream, long offset, int whence)
{
#define VT_IOWRAP_THISFUNCNAME fseek
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(stream, offset, whence);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %li, %i",
	 	stream != NULL ? fileno(stream) : -1,
	 	offset, whence);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, stream, offset, whence);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_FSEEKO) && HAVE_FSEEKO
int fseeko(FILE *stream, off_t offset, int whence)
{
#define VT_IOWRAP_THISFUNCNAME fseeko
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(stream, offset, whence);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, " OFF_T_STRARG ", %i",
	 	stream != NULL ? fileno(stream) : -1,
	 	offset, whence);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, stream, offset, whence);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FSEEKO */


#if defined(HAVE_FSEEKO64) && HAVE_FSEEKO64
int fseeko64(FILE *stream, off64_t offset, int whence)
{
#define VT_IOWRAP_THISFUNCNAME fseeko64
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(stream, offset, whence);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %lli, %i",
	 	stream != NULL ? fileno(stream) : -1,
	 	(long long)offset, whence);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, stream, offset, whence);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FSEEKO64 */


void rewind(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME rewind
	uint64_t enter_time;
        int tmp;

	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING_VOID1(stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i",
	 	stream != NULL ? fileno(stream) : -1 );

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( 0, tmp );
#undef VT_IOWRAP_THISFUNCNAME
}


int fsetpos(FILE *stream, const fpos_t *pos) {
#define VT_IOWRAP_THISFUNCNAME fsetpos
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(stream, pos);

/*
	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, " OFF_T_STRARG,
	 	stream != NULL ? fileno(stream) : -1,
	 	pos->__pos);
*/
	/* pos->__pos does not exist on every platform */
	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i",
	 	stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, stream, pos);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


#if defined(HAVE_FSETPOS64) && HAVE_FSETPOS64
int fsetpos64(FILE *stream, const fpos64_t *pos) {
#define VT_IOWRAP_THISFUNCNAME fsetpos64
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(stream, pos);

/*
	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %lli",
	 	stream != NULL ? fileno(stream) : -1,
	 	(long long)pos->__pos);
*/
	/* pos->__pos does not exist on every platform */
	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i",
	 	stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, stream, pos);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==-1, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FSETPOS64 */

size_t fread(void *buf, size_t size, size_t nmemb, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fread
	size_t ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(buf, size, nmemb, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu x %zu", 
		stream != NULL ? fileno(stream) : -1,
		nmemb, size);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, buf, size, nmemb, stream);
        num_bytes = (ssize_t)(size*ret);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==(size_t) 0, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


size_t fwrite( const void *buf, size_t size, size_t nmemb, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fwrite
	size_t ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING4(buf, size, nmemb, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %zu x %zu", 
		stream != NULL ? fileno(stream) : -1,
		nmemb, size);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC4(VT_IOWRAP_THISFUNCNAME, buf, size, nmemb, stream);
        num_bytes = (ssize_t)(size*ret);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==(size_t)0, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fgetc(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fgetc
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
		stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        num_bytes=1;
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF , tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


/* getc may be defined as a macro, so we must disable it */
#if defined(getc)
# undef getc
#endif
int getc(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME getc
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
		stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        num_bytes=1;
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF , tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


char *fgets(char *s, int size, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fgets
	char *ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING3(s, size, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i bytes max, @%p", 
		stream != NULL ? fileno(stream) : -1, size, s);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, s, size, stream);
        num_bytes=strlen(s);
        tmp=(ret!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==NULL , tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


char *gets(char *s)
{
#define VT_IOWRAP_THISFUNCNAME gets
	char *ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(s);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": @%p", s);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, s);
        num_bytes = (ssize_t)strlen(s);

	VT_IOWRAP_LEAVE_IOFUNC( ret==NULL, fileno(stdin) );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fputc(int c, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fputc
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(c, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
		stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, c, stream);
        num_bytes=1;
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


/* putc may be defined as a macro, so we must disable it */
#if defined(putc)
# undef putc
#endif
int putc(int c, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME putc
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(c, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
		stream != NULL ? fileno(stream) : -1);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, c, stream);
        num_bytes=1;
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fputs(const char *s, FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME fputs
	int ret;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING2(s, stream);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %p", 
		stream != NULL ? fileno(stream) : -1, s);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC2(VT_IOWRAP_THISFUNCNAME, s, stream);
	num_bytes = (ssize_t)strlen(s);
        tmp=(stream!=NULL) ? fileno(stream): 0;

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF, tmp );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int puts(const char *s)
{
#define VT_IOWRAP_THISFUNCNAME puts
	int ret;
	uint64_t enter_time;
	
	VT_IOWRAP_INIT_IOFUNC();

	VT_IOWRAP_CHECK_TRACING1(s);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %p", s);

	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, s);
	num_bytes = (ssize_t)strlen(s);

	VT_IOWRAP_LEAVE_IOFUNC( ret==EOF, fileno(stdout) );

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}


int fscanf(FILE *stream, const char *format, ...)
{
#define VT_IOWRAP_THISFUNCNAME fscanf
	int ret;
	va_list arg;
	uint64_t enter_time;
        int tmp;

	VT_IOWRAP_INIT_IOFUNC();

	if( DO_TRACE() ) {
		vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %s", 
			stream != NULL ? fileno(stream) : -1, format);
	
		VT_IOWRAP_ENTER_IOFUNC();
	
		vt_debug_msg(DBG_IO, "vfscanf");
		va_start (arg, format);
		ret = vfscanf(stream, format, arg);
		va_end (arg);
        
		num_bytes = (ssize_t)ret;
                tmp=(stream!=NULL) ? fileno(stream): 0;
        
		VT_IOWRAP_LEAVE_IOFUNC( DISABLE_IORECORD, tmp );
	
		return ret;
	}
	else {
		va_start (arg, format);
		ret = vfscanf(stream, format, arg);
		va_end (arg);
		return ret;
	}
#undef VT_IOWRAP_THISFUNCNAME
}


int fprintf(FILE *stream, const char *format, ...)
{
#define VT_IOWRAP_THISFUNCNAME fprintf
	int ret;
	va_list arg;
	uint64_t enter_time;
        int tmp;
	
	VT_IOWRAP_INIT_IOFUNC();

	if( DO_TRACE() ) {
		vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %s", 
			stream != NULL ? fileno(stream) : -1, format);
	
		VT_IOWRAP_ENTER_IOFUNC();
	
		vt_debug_msg(DBG_IO, "vfprintf");
		va_start (arg, format);
		ret = vfprintf(stream, format, arg);
		va_end (arg);
	
		num_bytes = (ssize_t)ret;
                tmp=(stream!=NULL) ? fileno(stream): 0;
		VT_IOWRAP_LEAVE_IOFUNC( ret==0, tmp );
	
		return ret;
	}
	else {
		va_start (arg, format);
		ret = vfprintf(stream, format, arg);
		va_end (arg);
		return ret;
	}
#undef VT_IOWRAP_THISFUNCNAME
}


int unlink(const char *path)
{
#define VT_IOWRAP_THISFUNCNAME unlink
	int ret;
	uint64_t enter_time;

	VT_IOWRAP_INIT_IOFUNC();

	/* workaround for compiler warning (unused variable) */
	(void)num_bytes;

	VT_IOWRAP_CHECK_TRACING1(path);

	vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %s\n", path);
	  
	VT_IOWRAP_ENTER_IOFUNC();

	vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME) "\n");
	ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, path);

	VT_IOWRAP_LEAVE_IOFUNC_PATH(ret, path);

	return ret;
#undef VT_IOWRAP_THISFUNCNAME
}

#if defined(HAVE_FLOCKFILE) && HAVE_FLOCKFILE
void flockfile(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME flockfile
        uint64_t enter_time;
        int tmp;

        VT_IOWRAP_INIT_IOFUNC();

        /* workaround for compiler warning (unused variable) */
        (void)num_bytes;

        VT_IOWRAP_CHECK_TRACING_VOID1(stream);

        vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
                stream != NULL ? fileno(stream) : -1);

        VT_IOWRAP_ENTER_IOFUNC();

        vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
        VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        tmp=(stream!=NULL) ? fileno(stream): 0;

        VT_IOWRAP_LEAVE_IOFUNC( 0, tmp );
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FLOCKFILE */


#if defined(HAVE_FTRYLOCKFILE) && HAVE_FTRYLOCKFILE
int ftrylockfile(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME ftrylockfile
        int ret;
        uint64_t enter_time;
        int tmp;

        VT_IOWRAP_INIT_IOFUNC();

        /* workaround for compiler warning (unused variable) */
        (void)num_bytes;

        VT_IOWRAP_CHECK_TRACING1(stream);

        vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
                stream != NULL ? fileno(stream) : -1);

        VT_IOWRAP_ENTER_IOFUNC();

        vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
        ret = VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        tmp=(stream!=NULL) ? fileno(stream): 0;

        VT_IOWRAP_LEAVE_IOFUNC( ret!=0, tmp );

        return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FTRYLOCKFILE */


#if defined(HAVE_FUNLOCKFILE) && HAVE_FUNLOCKFILE
void funlockfile(FILE *stream)
{
#define VT_IOWRAP_THISFUNCNAME funlockfile
        uint64_t enter_time;
        int tmp;

        VT_IOWRAP_INIT_IOFUNC();

        /* workaround for compiler warning (unused variable) */
        (void)num_bytes;

        VT_IOWRAP_CHECK_TRACING_VOID1(stream);

        vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i", 
                stream != NULL ? fileno(stream) : -1);

        VT_IOWRAP_ENTER_IOFUNC();

        vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
        VT_IOWRAP_CALL_LIBFUNC1(VT_IOWRAP_THISFUNCNAME, stream);
        tmp=(stream!=NULL) ? fileno(stream): 0;

        VT_IOWRAP_LEAVE_IOFUNC( 0, tmp );
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_FUNLOCKFILE */


#if defined(HAVE_LOCKF) && HAVE_LOCKF
int lockf(int fd, int function, off_t size)
{
#define VT_IOWRAP_THISFUNCNAME lockf
        int ret;
        uint64_t enter_time;

        VT_IOWRAP_INIT_IOFUNC();

        /* workaround for compiler warning (unused variable) */
        (void)num_bytes;

        VT_IOWRAP_CHECK_TRACING3(fd, function, size);
        vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i, %llu", fd, function, (unsigned long long)size);

        if( function == F_TEST )
        {
                /* Do not record lock tests */
                if( enable_memhooks ) VT_MEMHOOKS_ON();
                return VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, function, size);
        }
        else
        {
                uint32_t ioop;
                uint64_t time;

                VT_IOWRAP_ENTER_IOFUNC();

                vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
                ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, function, size);

                switch( function ) {
                        case F_LOCK:
                        case F_TLOCK:
                                ioop = VT_IOOP_LOCK;
                                break;
                        case F_ULOCK:
                                ioop = VT_IOOP_UNLOCK;
                                break;
                        default:
                                ioop = VT_IOOP_OTHER;
                                break;
                }
                time = vt_pform_wtime();
                vt_debug_msg( DBG_INIT, "Macro VT_IOWRAP_LEAVE_IOFUNC(), Function " stringify(VT_IOWRAP_THISFUNCNAME) );
                if( was_recorded ) {
                        vampir_file_t* file = get_vampir_file( fd );
                        uint32_t fid = file->vampir_file_id;
                        if( fid ) {
                                if( ret != 0 ) {
                                        vt_debug_msg(DBG_VT_CALL, "vt_ioend(" stringify(VT_IOWRAP_THISFUNCNAME) "), stamp %llu", time);
                                        vt_ioend( &time, fid, handleid, ioop | VT_IOFLAG_IOFAILED, (uint64_t)num_bytes );
                                }
                                else {
                                        vt_ioend( &time, fid, handleid, ioop, (uint64_t)num_bytes );
                                }
                        }
                }
                vt_exit( &time );
                if( enable_memhooks ) VT_MEMHOOKS_ON();

                return ret;
        }
#undef VT_IOWRAP_THISFUNCNAME
}
#endif /* HAVE_LOCKF */


int fcntl(int fd, int cmd, ...)
{
#define VT_IOWRAP_THISFUNCNAME fcntl
        int ret;
        uint64_t enter_time;
        uint32_t ioop;
        uint64_t time;
        void *arg;
        struct flock *lock;
        va_list ap;

        VT_IOWRAP_INIT_IOFUNC();

        /* workaround for compiler warning (unused variable) */
        (void)num_bytes;

        va_start(ap, cmd);
        arg = va_arg(ap, void *);
        va_end(ap);

        VT_IOWRAP_CHECK_TRACING3(fd, cmd, arg);
        vt_debug_msg(DBG_IO, stringify(VT_IOWRAP_THISFUNCNAME) ": %i, %i, %p", fd, cmd, arg);

        switch(cmd)
        {
                case F_SETLK:
                case F_SETLKW:
                        break;
                default:
                        /* Ooperations other than locking are not traced */
                        if( enable_memhooks ) VT_MEMHOOKS_ON();
                        return VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, cmd, arg);
        }

        VT_IOWRAP_ENTER_IOFUNC();

        lock = (struct flock *)arg;
        switch( lock->l_type )
        {
                case F_RDLCK:
                        ioop = VT_IOOP_LOCK | VT_IOFLAG_ISREADLOCK;
                        break;
                case F_WRLCK:
                        ioop = VT_IOOP_LOCK;
                        break;
                case F_UNLCK:
                        ioop = VT_IOOP_UNLOCK;
                        break;
                default:
                        ioop = 0;
                        vt_error_msg("Unknown fcntl operation: %i", lock->l_type);
                        break;
        }

        vt_debug_msg(DBG_IO, "real_" stringify(VT_IOWRAP_THISFUNCNAME));
        ret = VT_IOWRAP_CALL_LIBFUNC3(VT_IOWRAP_THISFUNCNAME, fd, cmd, arg);

        time = vt_pform_wtime();
        vt_debug_msg( DBG_INIT, "Macro VT_IOWRAP_LEAVE_IOFUNC(), Function " stringify(VT_IOWRAP_THISFUNCNAME) );
        if( was_recorded ) {
                vampir_file_t* file = get_vampir_file( fd );
                uint32_t fid = file->vampir_file_id;
                if( fid ) {
                        if( ret == -1 ) {
                                vt_debug_msg(DBG_VT_CALL, "vt_ioend(" stringify(VT_IOWRAP_THISFUNCNAME) "), stamp %llu", (unsigned long long)time);
                                vt_ioend( &time, fid, handleid, ioop | VT_IOFLAG_IOFAILED, (uint64_t)num_bytes );
                        }
                        else {
                                vt_ioend( &time, fid, handleid, ioop, (uint64_t)num_bytes );
                        }
                }
        }
        vt_exit( &time );
        if( enable_memhooks ) VT_MEMHOOKS_ON();

        return ret;
#undef VT_IOWRAP_THISFUNCNAME
}
