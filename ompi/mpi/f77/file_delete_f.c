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

#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_DELETE = mpi_file_delete_f
#pragma weak pmpi_file_delete = mpi_file_delete_f
#pragma weak pmpi_file_delete_ = mpi_file_delete_f
#pragma weak pmpi_file_delete__ = mpi_file_delete_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_DELETE,
                           pmpi_file_delete,
                           pmpi_file_delete_,
                           pmpi_file_delete__,
                           pmpi_file_delete_f,
                           (char *filename, MPI_Fint *info, MPI_Fint *ierr),
                           (filename, info, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_DELETE = mpi_file_delete_f
#pragma weak mpi_file_delete = mpi_file_delete_f
#pragma weak mpi_file_delete_ = mpi_file_delete_f
#pragma weak mpi_file_delete__ = mpi_file_delete_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_DELETE,
                           mpi_file_delete,
                           mpi_file_delete_,
                           mpi_file_delete__,
                           mpi_file_delete_f,
                           (char *filename, MPI_Fint *info, MPI_Fint *ierr),
                           (filename, info, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_file_delete_f(char *filename, MPI_Fint *info, MPI_Fint *ierr)
{
    MPI_Info c_info;

    c_info = MPI_Info_f2c(*info);
    
    *ierr = OMPI_INT_2_FINT(MPI_File_delete(filename, c_info));
}
