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

#include "ompi/mpi/c/bindings.h"
#include "ompi/datatype/datatype.h"

#if OMPI_HAVE_WEAK_SYMBOLS && OMPI_PROFILING_DEFINES
#pragma weak MPI_Type_create_subarray = PMPI_Type_create_subarray
#endif

#if OMPI_PROFILING_DEFINES
#include "ompi/mpi/c/profile/defines.h"
#endif

static const char FUNC_NAME[] = "MPI_Type_create_subarray";


int MPI_Type_create_subarray(int ndims,
                             int size_array[],
                             int subsize_array[],
                             int start_array[],
                             int order,
                             MPI_Datatype oldtype,
                             MPI_Datatype *newtype)

{
    MPI_Datatype last_type; 
    int32_t i, step, start_loop, end_loop;
    MPI_Aint size, displ, extent;

    if (MPI_PARAM_CHECK) {
        OMPI_ERR_INIT_FINALIZE(FUNC_NAME);
        if( ndims < 0 ) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_COUNT, FUNC_NAME);
        } else if( (NULL == size_array) || (NULL == subsize_array) || (NULL == start_array) ) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, FUNC_NAME);
        } else if (NULL == newtype) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TYPE, FUNC_NAME);
        } else if( !(DT_FLAG_DATA & oldtype ->flags) ) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_TYPE, FUNC_NAME);
        } else if( (MPI_ORDER_C != order) && (MPI_ORDER_FORTRAN != order) ) {
            return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, FUNC_NAME);
        }
        for( i = 0; i < ndims; i++ ) {
            if( (subsize_array[i] < 1) || (subsize_array[i] > size_array[i]) ) {
                return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, FUNC_NAME);
            } else if( (start_array[i] < 0) || (start_array[i] > (size_array[i] - subsize_array[i])) ) {
                return OMPI_ERRHANDLER_INVOKE(MPI_COMM_WORLD, MPI_ERR_ARG, FUNC_NAME);
            } 
        }
    }
    /* If the ndims is zero then return the NULL datatype */
    if( ndims < 2 ) {
        if( 0 == ndims ) {
            *newtype = &ompi_mpi_datatype_null;
            return MPI_SUCCESS;
        }
        ompi_ddt_create_contiguous( subsize_array[0], oldtype, &last_type );
        ompi_ddt_create_resized( last_type, start_array[0] * extent, size_array[0] * extent, newtype );
        return MPI_SUCCESS;
    }

    if( MPI_ORDER_C == order ) {
        start_loop = i = ndims - 1;
        step = -1;
        end_loop = -1;
    } else {
        start_loop = i = 0;
        step = 1;
        end_loop = ndims - 1;
    }

    ompi_ddt_type_extent( oldtype, &extent );

    /* As we know that the ndims is at least 1 we can start by creating the first dimension data
     * outside the loop, such that we dont have to create a duplicate of the oldtype just to be able
     * to free it.
     */
    ompi_ddt_create_vector( subsize_array[i+step], subsize_array[i], size_array[i],
                            oldtype, newtype );

    last_type = *newtype;
    size = size_array[i] * size_array[i+step];
    displ = start_array[i] + start_array[i+step] * size_array[i];
    for( i += 2 * step; i != end_loop; i += step ) {
        ompi_ddt_create_vector( subsize_array[i], 1, size_array[i],
                                last_type, newtype );
        ompi_ddt_destroy( &last_type );
        displ += size * start_array[i];
        size *= size_array[i];
        last_type = *newtype;
    }
    
    ompi_ddt_create_resized( last_type, displ * extent,
                             (size - start_array[start_loop]) * extent, newtype );
    ompi_ddt_destroy( &last_type );
    
    {
        int* a_i[5];

        a_i[0] = &ndims;
        a_i[1] = size_array;
        a_i[2] = subsize_array;
        a_i[3] = start_array;
        a_i[4] = &order;

        ompi_ddt_set_args( *newtype, 3 * ndims + 2, a_i, 0, NULL, 1, &oldtype,
                           MPI_COMBINER_SUBARRAY );
    }

    return MPI_SUCCESS;
}
