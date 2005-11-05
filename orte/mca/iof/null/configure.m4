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
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# MCA_iof_null_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([MCA_iof_null_CONFIG],[

    # README README README README README README README README README
    #   
    # The NULL iof component is here for debugging some things when
    # using TM.  It should not be used anywhere else, as you won't
    # get I/O.  So check for tm and follow the tm pls lead.
    #
    # README README README README README README README README README

    OMPI_CHECK_TM([iof_null], [iof_null_good=1], [iof_null_good=0])
         
    # don't need to set any flags - there's no TM-using code in this
    # component
    AS_IF([test "$iof_null_good" = "1"], [$1], [$2])
])dnl
