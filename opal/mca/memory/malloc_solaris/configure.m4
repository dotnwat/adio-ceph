# -*- shell-script -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The University of Tennessee and The University
#                         of Tennessee Research Foundation.  All rights
#                         reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2007-2008 Sun Microsystems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

AC_DEFUN([MCA_memory_malloc_solaris_COMPILE_MODE], [
    AC_MSG_CHECKING([for MCA component $2:$3 compile mode])
    $4="static"
    AC_MSG_RESULT([$$4])
])

# MCA_memory_malloc_solaris_CONFIG(action-if-can-compile, 
#                        [action-if-cant-compile])
# ------------------------------------------------
AC_DEFUN([MCA_memory_malloc_solaris_CONFIG],[
    OMPI_VAR_SCOPE_PUSH([memory_malloc_solaris_happy],
                        [memory_malloc_solaris_should_use],
                        [memory_malloc_solaris_munmap])

    AC_MSG_CHECKING([for Solaris])

    case "$host" in
    *solaris*)

        AC_MSG_RESULT([yes])

        AS_IF([test "$with_memory_manager" = "malloc_solaris"],
              [memory_malloc_solaris_happy="yes"
                memory_malloc_solaris_should_use=1],
              [memory_malloc_solaris_should_use=0
                AS_IF([test "$with_memory_manager" = ""],
                      [memory_malloc_solaris_happy="yes"],
                      [memory_malloc_solaris_happy="no"])])

        AS_IF([test "$memory_malloc_solaris_happy" = "yes"],
              [AC_CHECK_HEADER([malloc.h], [], 
              [memory_malloc_solaris_happy="no"])])

        AS_IF([test "$memory_malloc_solaris_happy" = "yes"],
              [memory_malloc_solaris_munmap=0

              AC_CHECK_HEADER([sys/syscall.h], 
                              [AC_CHECK_FUNCS([syscall], 
                                              [memory_malloc_solaris_munmap=1])])

              AC_CHECK_FUNCS([__munmap], [memory_malloc_solaris_munmap=1])

              # only allow dlsym (and therefore add -ldl) if we
              # really need to
              AS_IF([test "$memory_malloc_solaris_munmap" = "0"],
                    [memory_malloc_solaris_LIBS_SAVE="$LIBS"
                      AC_CHECK_LIB([dl],
                                   [dlsym],
                                   [memory_malloc_solaris_LIBS="-ldl"
                                     memory_malloc_solaris_munmap=1])
                      AC_CHECK_FUNCS([dlsym])
                      LIBS="$memory_malloc_solaris_LIBS_SAVE"])

              AS_IF([test "$memory_malloc_solaris_munmap" = "0"],
                    [memory_malloc_solaris_happy="no"])])

        AS_IF([test "$memory_malloc_solaris_happy" = "yes"],
              [memory_malloc_solaris_WRAPPER_EXTRA_LIBS="$memory_malloc_solaris_LIBS"])

        AS_IF([test "$memory_malloc_solaris_happy" = "no" -a \
                "$memory_malloc_solaris_should_use" = "1"],
              [AC_MSG_ERROR([malloc_solaris memory management requested but not available.  Aborting.])])

        AC_SUBST(memory_malloc_solaris_LIBS)

    ;;
    *)
        AC_MSG_RESULT([no])
        AS_IF([test "$with_memory_manager" = "malloc_solaris"],
              [memory_malloc_solaris_happy="no"
                memory_malloc_solaris_should_use=0])
    ;;
    esac 

    AS_IF([test "$memory_malloc_solaris_happy" = "yes"],
          [memory_base_found=1
           $1], [$2])

    OMPI_VAR_SCOPE_POP
])
