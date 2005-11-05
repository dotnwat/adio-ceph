/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#include "datatype/datatype.h"

int32_t ompi_ddt_create_subarray( int ndims, const int* pSizes, const int* pSubSizes, const int* pStarts,
                             int order, const ompi_datatype_t* oldType, ompi_datatype_t** newType )
{
   return OMPI_ERR_NOT_IMPLEMENTED;
}

int32_t ompi_ddt_create_darray( int size, int rank, int ndims, const int* pGSizes, const int *pDistrib,
                            const int* pDArgs, const int* pPSizes, int order, const ompi_datatype_t* oldType,
                            ompi_datatype_t** newType )
{
   return OMPI_ERR_NOT_IMPLEMENTED;
}

