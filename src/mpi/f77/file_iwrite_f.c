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

#include "ompi_config.h"

#include "mpi/f77/bindings.h"
#include "mpi/f77/constants.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_IWRITE = mpi_file_iwrite_f
#pragma weak pmpi_file_iwrite = mpi_file_iwrite_f
#pragma weak pmpi_file_iwrite_ = mpi_file_iwrite_f
#pragma weak pmpi_file_iwrite__ = mpi_file_iwrite_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_IWRITE,
                           pmpi_file_iwrite,
                           pmpi_file_iwrite_,
                           pmpi_file_iwrite__,
                           pmpi_file_iwrite_f,
                           (MPI_Fint *fh, char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *request, MPI_Fint *ierr),
                           (fh, buf, count, datatype, request, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_IWRITE = mpi_file_iwrite_f
#pragma weak mpi_file_iwrite = mpi_file_iwrite_f
#pragma weak mpi_file_iwrite_ = mpi_file_iwrite_f
#pragma weak mpi_file_iwrite__ = mpi_file_iwrite_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_IWRITE,
                           mpi_file_iwrite,
                           mpi_file_iwrite_,
                           mpi_file_iwrite__,
                           mpi_file_iwrite_f,
                           (MPI_Fint *fh, char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *request, MPI_Fint *ierr),
                           (fh, buf, count, datatype, request, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_file_iwrite_f(MPI_Fint *fh, char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *request, MPI_Fint *ierr)
{
   MPI_File c_fh = MPI_File_f2c(*fh);
   MPI_Datatype c_type = MPI_Type_f2c(*datatype);
   MPI_Request c_request;
   
   *ierr = OMPI_INT_2_FINT(MPI_File_iwrite(c_fh, OMPI_ADDR(buf),
					   OMPI_FINT_2_INT(*count),
					   c_type, &c_request));
   
   if (MPI_SUCCESS == *ierr) {
      *request = MPI_Request_c2f(c_request);
   }
}
