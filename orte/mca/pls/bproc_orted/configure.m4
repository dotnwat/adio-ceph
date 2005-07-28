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

# MCA_pls_bproc_orted_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([MCA_pls_bproc_orted_CONFIG],[
    OMPI_CHECK_BPROC([pls_bproc_orted], [pls_bproc_orted_good=1], 
                     [pls_bproc_orted_good=1], [pls_bproc_orted_good=0])

    # if check worked, set wrapper flags if so.  
    # Evaluate succeed / fail
    AS_IF([test "$pls_bproc_orted_good" = "1"], 
          [pls_bproc_orted_WRAPPER_EXTRA_LDFLAGS="$pls_bproc_orted_LDFLAGS"
           pls_bproc_orted_WRAPPER_EXTRA_LIBS="$pls_bproc_orted_LIBS"
           $1],
          [$2])

    # set build flags to use in makefile
    AC_SUBST([pls_bproc_orted_CPPFLAGS])
    AC_SUBST([pls_bproc_orted_LDFLAGS])
    AC_SUBST([pls_bproc_orted_LIBS])
])dnl
