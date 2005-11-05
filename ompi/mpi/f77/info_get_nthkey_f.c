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
#pragma weak PMPI_INFO_GET_NTHKEY = mpi_info_get_nthkey_f
#pragma weak pmpi_info_get_nthkey = mpi_info_get_nthkey_f
#pragma weak pmpi_info_get_nthkey_ = mpi_info_get_nthkey_f
#pragma weak pmpi_info_get_nthkey__ = mpi_info_get_nthkey_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_INFO_GET_NTHKEY,
                           pmpi_info_get_nthkey,
                           pmpi_info_get_nthkey_,
                           pmpi_info_get_nthkey__,
                           pmpi_info_get_nthkey_f,
                           (MPI_Fint *info, MPI_Fint *n, char *key, MPI_Fint *ierr),
                           (info, n, key, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_INFO_GET_NTHKEY = mpi_info_get_nthkey_f
#pragma weak mpi_info_get_nthkey = mpi_info_get_nthkey_f
#pragma weak mpi_info_get_nthkey_ = mpi_info_get_nthkey_f
#pragma weak mpi_info_get_nthkey__ = mpi_info_get_nthkey_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_INFO_GET_NTHKEY,
                           mpi_info_get_nthkey,
                           mpi_info_get_nthkey_,
                           mpi_info_get_nthkey__,
                           mpi_info_get_nthkey_f,
                           (MPI_Fint *info, MPI_Fint *n, char *key, MPI_Fint *ierr),
                           (info, n, key, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_info_get_nthkey_f(MPI_Fint *info, MPI_Fint *n, char *key,
			   MPI_Fint *ierr)
{
    MPI_Info c_info;

    c_info = MPI_Info_f2c(*info);
    
    *ierr = OMPI_INT_2_FINT(MPI_Info_get_nthkey(c_info, 
						OMPI_FINT_2_INT(*n),
						key));
}
