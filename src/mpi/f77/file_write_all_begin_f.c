/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include "mpi.h"
#include "mpi/f77/bindings.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILE_LAYER
#pragma weak PMPI_FILE_WRITE_ALL_BEGIN = mpi_file_write_all_begin_f
#pragma weak pmpi_file_write_all_begin = mpi_file_write_all_begin_f
#pragma weak pmpi_file_write_all_begin_ = mpi_file_write_all_begin_f
#pragma weak pmpi_file_write_all_begin__ = mpi_file_write_all_begin_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_FILE_WRITE_ALL_BEGIN,
                           pmpi_file_write_all_begin,
                           pmpi_file_write_all_begin_,
                           pmpi_file_write_all_begin__,
                           pmpi_file_write_all_begin_f,
                           (MPI_Fint *fh, char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *ierr),
                           (fh, buf, count, datatype, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_FILE_WRITE_ALL_BEGIN = mpi_file_write_all_begin_f
#pragma weak mpi_file_write_all_begin = mpi_file_write_all_begin_f
#pragma weak mpi_file_write_all_begin_ = mpi_file_write_all_begin_f
#pragma weak mpi_file_write_all_begin__ = mpi_file_write_all_begin_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_FILE_WRITE_ALL_BEGIN,
                           mpi_file_write_all_begin,
                           mpi_file_write_all_begin_,
                           mpi_file_write_all_begin__,
                           mpi_file_write_all_begin_f,
                           (MPI_Fint *fh, char *buf, MPI_Fint *count, MPI_Fint *datatype, MPI_Fint *ierr),
                           (fh, buf, count, datatype, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_file_write_all_begin_f(MPI_Fint *fh, char *buf, 
				MPI_Fint *count, MPI_Fint *datatype,
				MPI_Fint *ierr)
{
    MPI_File c_fh = MPI_File_f2c(*fh);
    MPI_Datatype c_type = MPI_Type_f2c(*datatype);

    *ierr = OMPI_INT_2_FINT(MPI_File_write_all_begin(c_fh, buf, 
						    OMPI_FINT_2_INT(*count),
						    c_type));
}
