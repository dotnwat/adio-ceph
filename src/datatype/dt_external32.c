/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#include "datatype.h"
#include "datatype_internal.h"
#include "dt_arch.h"

/* From the MPI standard. external32 use the following types:
 *   Type Length
 * MPI_PACKED               1
 * MPI_BYTE                 1
 * MPI_CHAR                 1
 * MPI_UNSIGNED_CHAR        1
 * MPI_SIGNED_CHAR          1
 * MPI_WCHAR                2
 * MPI_SHORT                2
 * MPI_UNSIGNED_SHORT       2
 * MPI_INT                  4
 * MPI_UNSIGNED             4
 * MPI_LONG                 4
 * MPI_UNSIGNED_LONG        4
 * MPI_FLOAT                4
 * MPI_DOUBLE               8
 * MPI_LONG_DOUBLE         16
 * Fortran types
 * MPI_CHARACTER            1
 * MPI_LOGICAL              4
 * MPI_INTEGER              4
 * MPI_REAL                 4
 * MPI_DOUBLE_PRECISION     8
 * MPI_COMPLEX              2*4
 * MPI_DOUBLE_COMPLEX       2*8
 * Optional types
 * MPI_INTEGER1             1
 * MPI_INTEGER2             2
 * MPI_INTEGER4             4
 * MPI_INTEGER8             8
 * MPI_LONG_LONG            8
 * MPI_UNSIGNED_LONG_LONG   8
 * MPI_REAL4                4
 * MPI_REAL8                8
 * MPI_REAL16              16
 *
 * All floating point values are in big-endian IEEE format. Double extended use 16 bytes, with
 * 15 exponent bits (bias = 10383), 112 mantissa bits and the same encoding as double.All
 * integers are in two's complement big-endian format.
 * 
 * All data are byte aligned, regardless of type. That's exactly what we expect as we can
 * consider the data stored in external32 as being packed.
 */

uint32_t ompi_ddt_external32_arch_id = OMPI_ARCH_LDEXPSIZEIS15 | OMPI_ARCH_LDMANTDIGIS113 |
                                       OMPI_ARCH_LONGDOUBLEIS128 | OMPI_ARCH_ISBIGENDIAN |
                                       OMPI_ARCH_HEADERMASK | OMPI_ARCH_HEADERMASK2;

