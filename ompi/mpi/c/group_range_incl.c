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
 * Copyright (c) 2006      University of Houston. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */
#include "ompi_config.h"
#include <stdio.h>

#include "ompi/mpi/c/bindings.h"
#include "ompi/group/group.h"
#include "ompi/errhandler/errhandler.h"
#include "ompi/communicator/communicator.h"
#include "ompi/proc/proc.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Group_range_incl = PMPI_Group_range_incl
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Group_range_incl";


int MPI_Group_range_incl(MPI_Group group, int n_triplets, int ranges[][3],
                         MPI_Group *new_group) 
{
    int err, i;
    int group_size;

    group_size = ompi_group_size ( group);
    /* can't act on NULL group */
    if( MPI_PARAM_CHECK ) {
	OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
	
	if ( (MPI_GROUP_NULL == group) || (NULL == group) ) {
	    return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_GROUP,
					  FUNC_NAME);
	}

	for ( i=0; i < n_triplets; i++) {
	    if(( 0 > ranges[i][0] ) || (ranges[i][0] > group_size )) {
		return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_RANK, 
					      FUNC_NAME);
	    }
	    if((0 > ranges[i][1]) || (ranges[i][1] > group_size)) {
		return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_RANK, 
					      FUNC_NAME);
	    }
	    if (ranges[i][2] == 0) {
		return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_RANK, 
					      FUNC_NAME);
	    }
	    if ( (ranges[i][0] < ranges[i][1]) && (ranges[i][2] < 0)) {
		return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_RANK, 
					      FUNC_NAME);
	    }
	    if ( (ranges[i][0] > ranges[i][1]) && (ranges[i][2] > 0) ){
		return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_RANK, 
					      FUNC_NAME);
	    }
	}
    }

   err = ompi_group_range_incl ( group, n_triplets, ranges, new_group );
   OMPI_ERRHANDLER_RETURN(err, MPI_COMM_WORLD, err, FUNC_NAME );
}
