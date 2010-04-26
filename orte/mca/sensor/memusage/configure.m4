# -*- shell-script -*-
#
# Copyright (c) 2010      Cisco Systems, Inc. All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

# MCA_sensor_memusage_CONFIG([action-if-found], [action-if-not-found])
# -----------------------------------------------------------
AC_DEFUN([MCA_sensor_memusage_CONFIG], [
    # if we don't want sensors, don't compile
    # this component
    AS_IF([test "$orte_want_sensors" = "1"],
        [$1], [$2])
])dnl

