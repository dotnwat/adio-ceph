#! /bin/sh

#
# Copyright (c) 2004-2006 The Trustees of Indiana University and Indiana
#                         University Research and Technology
#                         Corporation.  All rights reserved.
# Copyright (c) 2004-2005 The Regents of the University of California.
#                         All rights reserved.
# Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
# $COPYRIGHT$
# 
# Additional copyrights may follow
# 
# $HEADER$
#

#
# This file generates a Fortran code to bridge between an explicit F90
# generic interface and the F77 implementation.
#
# This file is automatically generated by either of the scripts
#   ../xml/create_mpi_f90_medium.f90.sh or
#   ../xml/create_mpi_f90_large.f90.sh
#

. "$1/fortran_kinds.sh"

# This entire file is only generated in medium/large modules.  So if
# we're not at least medium, bail now.

check_size medium
if test "$output" = "0"; then
    exit 0
fi

# Ok, we should continue.

allranks="0 $ranks"


output() {
    procedure=$1
    rank=$2
    type=$4
    proc="$1$2D$3"

    cat <<EOF

subroutine ${proc}(origin_addr, origin_count, origin_datatype, target_rank, target_disp, &
        target_count, target_datatype, win, ierr)
  include "mpif.h"
  ${type}, intent(in) :: origin_addr
  integer, intent(in) :: origin_count
  integer, intent(in) :: origin_datatype
  integer, intent(in) :: target_rank
  integer(kind=MPI_ADDRESS_KIND), intent(in) :: target_disp
  integer, intent(in) :: target_count
  integer, intent(in) :: target_datatype
  integer, intent(in) :: win
  integer, intent(out) :: ierr
  call ${procedure}(origin_addr, origin_count, origin_datatype, target_rank, target_disp, &
        target_count, target_datatype, win, ierr)
end subroutine ${proc}

EOF
}

for rank in $allranks
do
  case "$rank" in  0)  dim=''  ;  esac
  case "$rank" in  1)  dim=', dimension(:)'  ;  esac
  case "$rank" in  2)  dim=', dimension(:,:)'  ;  esac
  case "$rank" in  3)  dim=', dimension(:,:,:)'  ;  esac
  case "$rank" in  4)  dim=', dimension(:,:,:,:)'  ;  esac
  case "$rank" in  5)  dim=', dimension(:,:,:,:,:)'  ;  esac
  case "$rank" in  6)  dim=', dimension(:,:,:,:,:,:)'  ;  esac
  case "$rank" in  7)  dim=', dimension(:,:,:,:,:,:,:)'  ;  esac

  output MPI_Put ${rank} CH "character${dim}"
  output MPI_Put ${rank} L "logical${dim}"
  for kind in $ikinds
  do
    output MPI_Put ${rank} I${kind} "integer*${kind}${dim}"
  done
  for kind in $rkinds
  do
    output MPI_Put ${rank} R${kind} "real*${kind}${dim}"
  done
  for kind in $ckinds
  do
    output MPI_Put ${rank} C${kind} "complex*${kind}${dim}"
  done
done
