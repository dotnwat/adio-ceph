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
#pragma weak PMPI_STATUS_SET_ELEMENTS = mpi_status_set_elements_f
#pragma weak pmpi_status_set_elements = mpi_status_set_elements_f
#pragma weak pmpi_status_set_elements_ = mpi_status_set_elements_f
#pragma weak pmpi_status_set_elements__ = mpi_status_set_elements_f
#elif OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (PMPI_STATUS_SET_ELEMENTS,
                           pmpi_status_set_elements,
                           pmpi_status_set_elements_,
                           pmpi_status_set_elements__,
                           pmpi_status_set_elements_f,
                           (MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr),
                           (status, datatype, count, ierr) )
#endif

#if OMPI_HAVE_WEAK_SYMBOLS
#pragma weak MPI_STATUS_SET_ELEMENTS = mpi_status_set_elements_f
#pragma weak mpi_status_set_elements = mpi_status_set_elements_f
#pragma weak mpi_status_set_elements_ = mpi_status_set_elements_f
#pragma weak mpi_status_set_elements__ = mpi_status_set_elements_f
#endif

#if ! OMPI_HAVE_WEAK_SYMBOLS && ! OMPI_PROFILE_LAYER
OMPI_GENERATE_F77_BINDINGS (MPI_STATUS_SET_ELEMENTS,
                           mpi_status_set_elements,
                           mpi_status_set_elements_,
                           mpi_status_set_elements__,
                           mpi_status_set_elements_f,
                           (MPI_Fint *status, MPI_Fint *datatype, MPI_Fint *count, MPI_Fint *ierr),
                           (status, datatype, count, ierr) )
#endif


#if OMPI_PROFILE_LAYER && ! OMPI_HAVE_WEAK_SYMBOLS
#include "mpi/f77/profile/defines.h"
#endif

void mpi_status_set_elements_f(MPI_Fint *status, MPI_Fint *datatype, 
			       MPI_Fint *count, MPI_Fint *ierr)
{
    MPI_Datatype c_type = MPI_Type_f2c(*datatype);
    MPI_Status c_status;

    MPI_Status_f2c( status, &c_status );

    *ierr = OMPI_INT_2_FINT(MPI_Status_set_elements(&c_status, c_type, 
						    OMPI_FINT_2_INT(*count)));

    /* If datatype is really being set, then that needs to be converted.... */
    if (MPI_SUCCESS == *ierr) {
        MPI_Status_c2f(&c_status, status);
    }

}
