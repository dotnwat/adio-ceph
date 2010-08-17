# -*- shell-script -*-
#
# Copyright (c) 2004-2010 The Trustees of Indiana University.
#                         All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# OMPI_MPIEXT_cr_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([OMPI_MPIEXT_cr_CONFIG],[
    # If we don't want FT, don't compile this component
    AS_IF([test "$opal_want_ft_cr" = "1"],
        [$1],
        [$2])
])dnl
