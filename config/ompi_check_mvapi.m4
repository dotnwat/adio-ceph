# -*- shell-script -*-
#
# Copyright (c) 2004-2005 The Trustees of Indiana University.
#                         All rights reserved.
# Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
#                         All rights reserved.
# Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
#                         University of Stuttgart.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#


# OMPI_CHECK_MVAPI(prefix, [action-if-found], [action-if-not-found])
# --------------------------------------------------------
# check if MVAPI support can be found.  sets prefix_{CPPFLAGS, 
# LDFLAGS, LIBS} as needed and runs action-if-found if there is
# support, otherwise executes action-if-not-found
AC_DEFUN([OMPI_CHECK_MVAPI],[
    AC_ARG_WITH([mvapi],
                [AC_HELP_STRING([--with-btl-mvapi=MVAPI_DIR],
                                [Additional directory to search for MVAPI installation])])
    AC_ARG_WITH([mvapi-libdir],
       [AC_HELP_STRING([--with-btl-mvapi-libdir=IBLIBDIR],
	       [directory where the IB library can be found, if it is not in MVAPI_DIR/lib or MVAPI_DIR/lib64])])

    AS_IF([test ! -z "$with_btl_mvapi" -a "$with_btl_mvapi" != "yes"],
          [ompi_check_mvapi_dir="$with_btl_mvapi"])
    AS_IF([test ! -z "$with_btl_mvapi_libdir" -a "$with_btl_mvapi_libdir" != "yes"],
          [ompi_check_mvapi_libdir="$with_btl_mvapi_libdir"])

    # check for pthreads and emit a warning that things might go south...
    AS_IF([test "$HAVE_POSIX_THREADS" != "1"],
          [AC_MSG_WARN([POSIX threads not enabled.  May not be able to link with mvapi])])

    OMPI_CHECK_PACKAGE([$1],
                       [vapi.h],
                       [vapi],
                       [VAPI_open_hca],
                       [-lmosal -lmpga -lmtl_common],
                       [$ompi_check_mvapi_dir],
                       [$ompi_check_mvapi_libdir],
                       [ompi_check_mvapi_happy="yes"],
                       [ompi_check_mvapi_happy="no"])

    AS_IF([test "$ompi_check_mvapi_happy" = "yes"],
          [$2],
          [AS_IF([test ! -z "$with_btl_mvapi" -a "$with_btl_mvapi" != "no"],
                 [AC_MSG_ERROR([MVAPI support requested but not found.  Aborting])])
           $3])
])

