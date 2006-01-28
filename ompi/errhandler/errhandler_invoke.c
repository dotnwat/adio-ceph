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

#include "communicator/communicator.h"
#include "win/win.h"
#include "file/file.h"
#include "errhandler/errhandler.h"
#include "mpi/f77/fint_2_int.h"


int ompi_errhandler_invoke(ompi_errhandler_t *errhandler, void *mpi_object, 
			   int object_type, int err_code, const char *message)
{
    MPI_Fint fortran_handle, fortran_err_code = OMPI_INT_2_FINT(err_code);
    ompi_communicator_t *comm;
    ompi_win_t *win;
    ompi_file_t *file;
    
    /* If we got no errorhandler, then just invoke errors_abort */
    if (NULL == errhandler) {
	ompi_mpi_errors_are_fatal_comm_handler(NULL, NULL, message);
    }
    
    /* Figure out what kind of errhandler it is, figure out if it's
       fortran or C, and then invoke it */
    
    switch (object_type) {
	case OMPI_ERRHANDLER_TYPE_COMM:
	    comm = (ompi_communicator_t *) mpi_object;
	    if (errhandler->eh_fortran_function) {
		fortran_handle = OMPI_INT_2_FINT(comm->c_f_to_c_index);
		errhandler->eh_fort_fn(&fortran_handle, &fortran_err_code);
	    } else {
		errhandler->eh_comm_fn(&comm, &err_code, message, NULL);
	  }
	  break;

	case OMPI_ERRHANDLER_TYPE_WIN:
	    win = (ompi_win_t *) mpi_object;
	    if (errhandler->eh_fortran_function) {
		fortran_handle = OMPI_INT_2_FINT(win->w_f_to_c_index);
		errhandler->eh_fort_fn(&fortran_handle, &fortran_err_code);
	    } else {
		errhandler->eh_win_fn(&win, &err_code, message, NULL);
	    }
	    break;
	    
	case OMPI_ERRHANDLER_TYPE_FILE:
	    file = (ompi_file_t *) mpi_object;
	    if (errhandler->eh_fortran_function) {
		fortran_handle = OMPI_INT_2_FINT(file->f_f_to_c_index);
		errhandler->eh_fort_fn(&fortran_handle, &fortran_err_code);
	    } else {
		errhandler->eh_file_fn(&file, &err_code, message, NULL);
	    }
	    break;
    }
    
    /* All done */
    return err_code;
}
