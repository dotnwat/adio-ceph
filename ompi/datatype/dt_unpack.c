/* -*- Mode: C; c-basic-offset:4 ; -*- */
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
#include "datatype/datatype.h"
#include "datatype/convertor.h"
#include "datatype/datatype_internal.h"

#ifdef HAVE_ALLOCA_H
#include <alloca.h>
#endif
#include <stdlib.h>

void ompi_ddt_dump_stack( const dt_stack_t* pStack, int stack_pos,
                          const union dt_elem_desc* pDesc, const char* name )
{
    opal_output( 0, "\nStack %p stack_pos %d name %s\n", (void*)pStack, stack_pos, name );
    for( ; stack_pos >= 0; stack_pos-- ) {
        opal_output( 0, "%d: pos %d count %d disp %ld end_loop %d ", stack_pos, pStack[stack_pos].index,
                     pStack[stack_pos].count, pStack[stack_pos].disp, pStack[stack_pos].end_loop );
        if( pStack->index != -1 )
            opal_output( 0, "\t[desc count %d disp %ld extent %d]\n",
                         pDesc[pStack[stack_pos].index].elem.count,
                         pDesc[pStack[stack_pos].index].elem.disp,
                         pDesc[pStack[stack_pos].index].elem.extent );
        else
            opal_output( 0, "\n" );
    }
    opal_output( 0, "\n" );
}

/*
 *  Remember that the first item in the stack (ie. position 0) is the number
 * of times the datatype is involved in the operation (ie. the count argument
 * in the MPI_ call).
 */
/* Convert data from multiple input buffers (as received from the network layer)
 * to a contiguous output buffer with a predefined size.
 * return OMPI_SUCCESS if everything went OK and if there is still room before the complete
 *          conversion of the data (need additional call with others input buffers )
 *        1 if everything went fine and the data was completly converted
 *       -1 something wrong occurs.
 */
static int ompi_convertor_unpack_general( ompi_convertor_t* pConvertor,
					  struct iovec* iov,
					  uint32_t* out_size,
					  size_t* max_data,
					  int32_t* freeAfter )
{
    dt_stack_t* pStack;    /* pointer to the position on the stack */
    uint32_t pos_desc;     /* actual position in the description of the derived datatype */
    int count_desc;        /* the number of items already done in the actual pos_desc */
    int type = DT_CHAR;    /* type at current position */
    uint32_t advance;      /* number of bytes that we should advance the buffer */
    long disp_desc = 0;    /* compute displacement for truncated data */
    int bConverted = 0;    /* number of bytes converted this time */
    dt_elem_desc_t* pElems;
    int oCount = (pConvertor->pDesc->ub - pConvertor->pDesc->lb) * pConvertor->count;
    char* pInput;
    int iCount, rc;
    uint32_t iov_count, total_bytes_converted = 0;

    /* For the general case always use the user data description */
    pElems = pConvertor->pDesc->desc.desc;

    pStack = pConvertor->pStack + pConvertor->stack_pos;
    pos_desc   = pStack->index;
    count_desc = pStack->count;
    disp_desc  = pStack->disp;
    pStack--;
    pConvertor->stack_pos--;

    DDT_DUMP_STACK( pConvertor->pStack, pConvertor->stack_pos, pElems, "starting" );
    DUMP( "remember position on stack %d last_elem at %d\n", pConvertor->stack_pos, pos_desc );
    DUMP( "top stack info {index = %d, count = %d}\n", pStack->index, pStack->count );

    for( iov_count = 0; iov_count < (*out_size); iov_count++ ) {
        bConverted = 0;
        pInput = iov[iov_count].iov_base;
        iCount = iov[iov_count].iov_len;
        while( 1 ) {
            if( DT_END_LOOP == pElems[pos_desc].elem.common.type ) { /* end of the current loop */
                if( --(pStack->count) == 0 ) { /* end of loop */
                    if( pConvertor->stack_pos == 0 ) {
                        pConvertor->flags |= CONVERTOR_COMPLETED;
                        goto save_and_return;  /* completed */
                    }
                    pConvertor->stack_pos--;
                    pStack--;
                }

                if( pStack->index == -1 ) {
                    pStack->disp += (pConvertor->pDesc->ub - pConvertor->pDesc->lb);
                } else {
                    assert( DT_LOOP == pElems[pStack->index].elem.common.type );
                    pStack->disp += pElems[pStack->index].loop.extent;
                }
                pos_desc = pStack->index + 1;
                count_desc = pElems[pos_desc].elem.count;
                disp_desc = pElems[pos_desc].elem.disp;
            }
            if( DT_LOOP == pElems[pos_desc].elem.common.type ) {
                do {
                    PUSH_STACK( pStack, pConvertor->stack_pos,
                                pos_desc, DT_LOOP, pElems[pos_desc].loop.loops,
                                pStack->disp, pos_desc + pElems[pos_desc].loop.items + 1 );
                    pos_desc++;
                } while( DT_LOOP == pElems[pos_desc].loop.common.type ); /* let's start another loop */
                DDT_DUMP_STACK( pConvertor->pStack, pConvertor->stack_pos, pElems, "advance loops" );
                /* update the current state */
                count_desc = pElems[pos_desc].elem.count;
                disp_desc = pElems[pos_desc].elem.disp;
            }
            while( pElems[pos_desc].elem.common.flags & DT_FLAG_DATA ) {
                /* now here we have a basic datatype */
                type = pElems[pos_desc].elem.common.type;
                rc = pConvertor->pFunctions[type]( count_desc,
                                                   pInput, iCount, ompi_ddt_basicDatatypes[type]->size,
                                                   pConvertor->pBaseBuf + pStack->disp + disp_desc,
                                                   oCount, pElems[pos_desc].elem.extent );
                advance = rc * ompi_ddt_basicDatatypes[type]->size;
                iCount -= advance;      /* decrease the available space in the buffer */
                pInput += advance;      /* increase the pointer to the buffer */
                bConverted += advance;
                if( rc != count_desc ) {
                    /* not all data has been converted. Keep the state */
                    count_desc -= rc;
                    disp_desc += rc * pElems[pos_desc].elem.extent;
                    if( iCount != 0 )
                        printf( "unpack there is still room in the input buffer %d bytes\n", iCount );
                    goto save_and_return;
                }
                pos_desc++;  /* advance to the next data */
                count_desc = pElems[pos_desc].elem.count;
                disp_desc = pElems[pos_desc].elem.disp;
                if( iCount == 0 )
                    goto save_and_return;  /* break if there is no more data in the buffer */
            }
        }
    save_and_return:
        pConvertor->bConverted += bConverted;  /* update the # of bytes already converted */
        iov[iov_count].iov_len = bConverted;           /* update the iovec length */
        total_bytes_converted += bConverted;
    }
    *max_data = total_bytes_converted;
    /* out of the loop: we have complete the data conversion or no more space
     * in the buffer.
     */
    if( pConvertor->flags & CONVERTOR_COMPLETED ) return 1;  /* data succesfully converted */

    /* I complete an element, next step I should go to the next one */
    PUSH_STACK( pStack, pConvertor->stack_pos, pos_desc, type,
                count_desc, disp_desc, pos_desc );

    return 0;
}

static int ompi_convertor_unpack_homogeneous( ompi_convertor_t* pConv,
					      struct iovec* iov,
					      uint32_t* out_size,
					      size_t* max_data,
					      int32_t* freeAfter )
{
    dt_stack_t* pStack;    /* pointer to the position on the stack */
    uint32_t pos_desc;     /* actual position in the description of the derived datatype */
    uint32_t i;            /* counter for basic datatype with extent */
    int bConverted = 0;    /* number of bytes converted this time */
    long lastDisp = 0;
    size_t space = iov[0].iov_len, last_count = 0, last_blength = 0;
    char* pSrcBuf;
    const ompi_datatype_t* pData = pConv->pDesc;
    dt_elem_desc_t* pElems;

    pSrcBuf = iov[0].iov_base;

    pElems = pConv->use_desc->desc;
    pStack = pConv->pStack + pConv->stack_pos;
    pos_desc = pStack->index;
    lastDisp = pStack->disp;
    last_count = pStack->count;
    /*opal_output( 0, "ompi_convertor_unpack_homogeneous stack_pos %d index %d count %d lastDisp %ld bConverted %d\n",
                   pConv->stack_pos, pStack->index, pStack->count, lastDisp, pConv->bConverted );*/
    pStack--;
    pConv->stack_pos--;

    while( 1 ) {  /* loop forever. The exit condition is detected inside the while loop */
        if( DT_END_LOOP == pElems[pos_desc].elem.common.type ) { /* end of the current loop */
            if( --(pStack->count) == 0 ) { /* end of loop */
                if( pConv->stack_pos == 0 ) {
                    last_blength = 0;  /* nothing to copy anymore */
                    pConv->flags |= CONVERTOR_COMPLETED;
                    goto end_loop;
                }
                pStack--;
                pConv->stack_pos--;
                pos_desc++;
            } else {
                if( pStack->index == -1 ) {
                    pStack->disp += (pData->ub - pData->lb);
                } else {
                    assert( DT_LOOP == pElems[pStack->index].elem.common.type );
                    pStack->disp += pElems[pStack->index].loop.extent;
                }
                pos_desc = pStack->index + 1;
            }
            lastDisp = pStack->disp + pElems[pos_desc].elem.disp;
            last_count = pElems[pos_desc].elem.count;
            continue;
        }
        while( DT_LOOP == pElems[pos_desc].elem.common.type ) {
            int stop_in_loop = 0;
            if( pElems[pos_desc].loop.common.flags & DT_FLAG_CONTIGUOUS ) {
                ddt_endloop_desc_t* end_loop = &(pElems[pos_desc + pElems[pos_desc].loop.items].end_loop);
                last_count = pElems[pos_desc].loop.loops;
                if( (end_loop->size * last_count) > space ) {
                    stop_in_loop = last_count;
                    last_count = space / end_loop->size;
                }
                for( i = 0; i < last_count; i++ ) {
                    OMPI_DDT_SAFEGUARD_POINTER( pConv->pBaseBuf + lastDisp, end_loop->size,
                                                pConv->pBaseBuf, pData, pConv->count );
                    /*opal_output( 0, "3. memcpy %p, %p, %d", pConv->pBaseBuf + lastDisp, pSrcBuf, end_loop->size );*/
                    MEMCPY( pConv->pBaseBuf + lastDisp, pSrcBuf, end_loop->size );
                    pSrcBuf += end_loop->size;
                    lastDisp += pElems[pos_desc].loop.extent;
                }
                space -= (end_loop->size * last_count);
                bConverted += (end_loop->size * last_count);
                if( stop_in_loop == 0 ) {
                    pos_desc += pElems[pos_desc].loop.items + 1;
                    last_count = pElems[pos_desc].elem.count;
                    continue;
                }
                last_count = stop_in_loop - last_count;
                last_blength = 0;
                /* Save the stack with the correct last_count value. */
            }
            PUSH_STACK( pStack, pConv->stack_pos, pos_desc, DT_LOOP, last_count,
                        pStack->disp, pos_desc + pElems[pos_desc].loop.items );
            pos_desc++;
            lastDisp = pStack->disp + pElems[pos_desc].elem.disp;
            last_count = pElems[pos_desc].elem.count;
        }
        /* now here we have a basic datatype */
        while( pElems[pos_desc].elem.common.flags & DT_FLAG_DATA ) {
            const ompi_datatype_t* basic_type = BASIC_DDT_FROM_ELEM(pElems[pos_desc]);
            /* do we have enough space in the buffer ? */
            last_blength = last_count * basic_type->size;
            if( pElems[pos_desc].elem.common.flags & DT_FLAG_CONTIGUOUS ) {
                if( space < last_blength ) {
                    last_blength = space / basic_type->size;
                    last_count -= last_blength;
                    last_blength *= basic_type->size;
                    space -= last_blength;
                    goto end_loop;  /* or break whatever but go out of this while */
                }
                OMPI_DDT_SAFEGUARD_POINTER( pConv->pBaseBuf + lastDisp, last_blength,
                                            pConv->pBaseBuf, pData, pConv->count );
                /*opal_output( 0, "1. memcpy %p, %p, %d -> %d", pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength, bConverted );*/
                MEMCPY( pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength );
                bConverted += last_blength;
                space -= last_blength;
                pSrcBuf += last_blength;
            } else {
                uint32_t i;

                last_blength = basic_type->size;
                for( i = 0; i < last_count; i++ ) {
                    OMPI_DDT_SAFEGUARD_POINTER( pConv->pBaseBuf + lastDisp, last_blength,
                                                pConv->pBaseBuf, pData, pConv->count );
                    /*opal_output( 0, "2. memcpy %p, %p, %d", pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength );*/
                    MEMCPY( pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength );
                    lastDisp += pElems[pos_desc].elem.extent;
                    pSrcBuf += basic_type->size;
                }
                bConverted += basic_type->size * last_count;
            }
            pos_desc++;  /* advance to the next data */
            lastDisp = pStack->disp + pElems[pos_desc].elem.disp;
            last_count = pElems[pos_desc].elem.count;
        }
    }
 end_loop:
    if( last_blength != 0 ) { /* save the internal state */
        /* update corresponding the the datatype length */
        OMPI_DDT_SAFEGUARD_POINTER( pConv->pBaseBuf + lastDisp, last_blength,
                                    pConv->pBaseBuf, pData, pConv->count );
        MEMCPY( pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength );
        /*opal_output( 0, "1. memcpy %p, %p, %d -> %d", pConv->pBaseBuf + lastDisp, pSrcBuf, last_blength, bConverted );*/
        bConverted += last_blength;
        lastDisp += last_blength;
    }

    pConv->bConverted += bConverted;  /* update the converted field */
    iov[0].iov_len = bConverted;      /* update the iovec length */
    *max_data = bConverted;

    if( pConv->flags & CONVERTOR_COMPLETED ) {  /* finish thus do not update the stack */
        return 1;
    }
    PUSH_STACK( pStack, pConv->stack_pos, pos_desc, pElems[pos_desc].elem.common.type,
                last_count, lastDisp, pos_desc );
    return 0;
}

static int ompi_convertor_unpack_homogeneous_contig( ompi_convertor_t* pConv,
						     struct iovec* iov,
						     uint32_t* out_size,
						     size_t* max_data,
						     int32_t* freeAfter )
{
    const ompi_datatype_t *pData = pConv->pDesc;
    char* pDstBuf = pConv->pBaseBuf, *pSrcBuf;
    uint32_t iov_count, initial_bytes_converted = pConv->bConverted;
    long extent = pData->ub - pData->lb;
    uint32_t bConverted, length, remaining, i;
    dt_stack_t* stack = &(pConv->pStack[1]);

    for( iov_count = 0; iov_count < (*out_size); iov_count++ ) {
        pSrcBuf = (char*)iov[iov_count].iov_base;
        remaining = pConv->count * pData->size - pConv->bConverted;
        if( remaining > iov[iov_count].iov_len )
            remaining = iov[iov_count].iov_len;
        bConverted = remaining; /* how much will get unpacked this time */

        if( (long)pData->size == extent ) {
            pDstBuf += pData->true_lb + pConv->bConverted;

            /* contiguous data or basic datatype with count */
            OMPI_DDT_SAFEGUARD_POINTER( pDstBuf, remaining,
                                        pConv->pBaseBuf, pData, pConv->count );
            MEMCPY( pDstBuf, pSrcBuf, remaining);
        } else {
            pDstBuf += stack->disp;

            length = pConv->bConverted / pData->size;  /* already done */
            length = pConv->bConverted - length * pData->size;  /* still left on the last element */
            /* complete the last copy */
            if( length != 0 ) {
                OMPI_DDT_SAFEGUARD_POINTER( pDstBuf, length, pConv->pBaseBuf, pData, pConv->count );
                MEMCPY( pDstBuf, pSrcBuf, length );
                pSrcBuf += length;
                pDstBuf += (extent - length);
                remaining -= length;
            }
            for( i = 0; pData->size <= remaining; i++ ) {
                OMPI_DDT_SAFEGUARD_POINTER( pDstBuf, pData->size, pConv->pBaseBuf, pData, pConv->count );
                MEMCPY( pDstBuf, pSrcBuf, pData->size );
                pSrcBuf += pData->size;
                pDstBuf += extent;
                remaining -= pData->size;
            }
            /* copy the last bits */
            if( remaining != 0 ) {
                OMPI_DDT_SAFEGUARD_POINTER( pDstBuf, remaining, pConv->pBaseBuf, pData, pConv->count );
                MEMCPY( pDstBuf, pSrcBuf, remaining );
                pDstBuf += remaining;
            }
            stack->disp = pDstBuf - pConv->pBaseBuf;  /* save the position */
        }
        pConv->bConverted += bConverted;
    }
    *out_size = iov_count;
    *max_data = (pConv->bConverted - initial_bytes_converted);
    if( pConv->bConverted == (pData->size * pConv->count) ) {
        pConv->flags |= CONVERTOR_COMPLETED;
        return 1;
    }
    return 0;
}

/* Return value:
 *     0 : nothing has been done
 * positive value: number of item converted.
 * negative value: -1 * number of items converted, less data provided than expected
 *                and there are less data than the size on the remote host of the
 *                basic datatype.
 */
#define COPY_TYPE( TYPENAME, TYPE, COUNT )                              \
static int copy_##TYPENAME( uint32_t count,                             \
                            char* from, uint32_t from_len, long from_extent, \
                            char* to, uint32_t to_len, long to_extent ) \
{                                                                       \
    uint32_t i;                                                         \
    uint32_t remote_TYPE_size = sizeof(TYPE) * (COUNT); /* TODO */      \
    uint32_t local_TYPE_size = (COUNT) * sizeof(TYPE);                  \
                                                                        \
    if( (remote_TYPE_size * count) > from_len ) {                       \
        count = from_len / remote_TYPE_size;                            \
        if( (count * remote_TYPE_size) != from_len ) {                  \
            DUMP( "oops should I keep this data somewhere (excedent %d bytes)?\n", \
                  from_len - (count * remote_TYPE_size) );              \
        }                                                               \
        DUMP( "correct: copy %s count %d from buffer %p with length %d to %p space %d\n", \
              #TYPE, count, from, from_len, to, to_len );               \
    } else                                                              \
        DUMP( "         copy %s count %d from buffer %p with length %d to %p space %d\n", \
              #TYPE, count, from, from_len, to, to_len );               \
                                                                        \
    if( (from_extent == (long)local_TYPE_size) &&                       \
        (to_extent == (long)remote_TYPE_size) ) {                       \
        MEMCPY( to, from, count * local_TYPE_size );                    \
    } else {                                                            \
        for( i = 0; i < count; i++ ) {                                  \
            MEMCPY( to, from, local_TYPE_size );                        \
            to += to_extent;                                            \
            from += from_extent;                                        \
        }                                                               \
    }                                                                   \
    return count;                                                       \
}

#define COPY_CONTIGUOUS_BYTES( TYPENAME, COUNT )                        \
static int copy_##TYPENAME##_##COUNT( uint32_t count,                   \
                                      char* from, uint32_t from_len, long from_extent, \
                                      char* to, uint32_t to_len, long to_extent) \
{                                                                       \
    uint32_t i;                                                         \
    uint32_t remote_TYPE_size = (COUNT); /* TODO */                     \
    uint32_t local_TYPE_size = (COUNT);                                 \
                                                                        \
    if( (remote_TYPE_size * count) > from_len ) {                       \
        count = from_len / remote_TYPE_size;                            \
        if( (count * remote_TYPE_size) != from_len ) {                  \
            DUMP( "oops should I keep this data somewhere (excedent %d bytes)?\n", \
                  from_len - (count * remote_TYPE_size) );              \
        }                                                               \
        DUMP( "correct: copy %s count %d from buffer %p with length %d to %p space %d\n", \
              #TYPENAME, count, from, from_len, to, to_len );           \
    } else                                                              \
        DUMP( "         copy %s count %d from buffer %p with length %d to %p space %d\n", \
              #TYPENAME, count, from, from_len, to, to_len );           \
                                                                        \
    if( (from_extent == (long)local_TYPE_size) &&                       \
        (to_extent == (long)remote_TYPE_size) ) {                       \
        MEMCPY( to, from, count * local_TYPE_size );                    \
    } else {                                                            \
        for( i = 0; i < count; i++ ) {                                  \
            MEMCPY( to, from, local_TYPE_size );                        \
            to += to_extent;                                            \
            from += from_extent;                                        \
        }                                                               \
    }                                                                   \
    return count;                                                       \
}

COPY_TYPE( char, char, 1 )
COPY_TYPE( short, short, 1 )
COPY_TYPE( int, int, 1 )
COPY_TYPE( float, float, 1 )
COPY_TYPE( long, long, 1 )
COPY_TYPE( double, double, 1 )
COPY_TYPE( long_long, long long, 1 )
COPY_TYPE( long_double, long double, 1 )
COPY_TYPE( complex_float, ompi_complex_float_t, 1 )
COPY_TYPE( complex_double, ompi_complex_double_t, 1 )
COPY_TYPE( complex_long_double, ompi_complex_long_double_t, 1 )
COPY_TYPE( wchar, wchar_t, 1 )
COPY_TYPE( 2int, int, 2 )
COPY_TYPE( 2float, float, 2 )
COPY_TYPE( 2double, double, 2 )
COPY_TYPE( 2complex_float, ompi_complex_float_t, 2 )
COPY_TYPE( 2complex_double, ompi_complex_double_t, 2 )

#if OMPI_SIZEOF_FORTRAN_LOGICAL == 1 || SIZEOF_BOOL == 1
#define REQUIRE_COPY_BYTES_1 1
#else
#define REQUIRE_COPY_BYTES_1 0
#endif

#if OMPI_SIZEOF_FORTRAN_LOGICAL == 2 || SIZEOF_BOOL == 2
#define REQUIRE_COPY_BYTES_2 1
#else
#define REQUIRE_COPY_BYTES_2 0
#endif

#if OMPI_SIZEOF_FORTRAN_LOGICAL == 4 || SIZEOF_BOOL == 4
#define REQUIRE_COPY_BYTES_4 1
#else
#define REQUIRE_COPY_BYTES_4 0
#endif

#if (SIZEOF_FLOAT + SIZEOF_INT) == 8 || (SIZEOF_LONG + SIZEOF_INT) == 8 || SIZEOF_BOOL == 8
#define REQUIRE_COPY_BYTES_8 1
#else
#define REQUIRE_COPY_BYTES_8 0
#endif

#if (SIZEOF_DOUBLE + SIZEOF_INT) == 12 || (SIZEOF_LONG + SIZEOF_INT) == 12
#define REQUIRE_COPY_BYTES_12 1
#else
#define REQUIRE_COPY_BYTES_12 0
#endif

#if (SIZEOF_LONG_DOUBLE + SIZEOF_INT) == 16
#define REQUIRE_COPY_BYTES_16 1
#else
#define REQUIRE_COPY_BYTES_16 0
#endif

#if (SIZEOF_LONG_DOUBLE + SIZEOF_INT) == 20
#define REQUIRE_COPY_BYTES_20 1
#else
#define REQUIRE_COPY_BYTES_20 0
#endif

#if REQUIRE_COPY_BYTES_1
COPY_CONTIGUOUS_BYTES( bytes, 1 )
#endif  /* REQUIRE_COPY_BYTES_1 */
#if REQUIRE_COPY_BYTES_2
COPY_CONTIGUOUS_BYTES( bytes, 2 )
#endif  /* REQUIRE_COPY_BYTES_2 */
#if REQUIRE_COPY_BYTES_4
COPY_CONTIGUOUS_BYTES( bytes, 4 )
#endif  /* REQUIRE_COPY_BYTES_4 */
#if REQUIRE_COPY_BYTES_8
COPY_CONTIGUOUS_BYTES( bytes, 8 )
#endif  /* REQUIRE_COPY_BYTES_8 */
#if REQUIRE_COPY_BYTES_12
COPY_CONTIGUOUS_BYTES( bytes, 12 )
#endif  /* REQUIRE_COPY_BYTES_12 */
#if REQUIRE_COPY_BYTES_16
COPY_CONTIGUOUS_BYTES( bytes, 16 )
#endif  /* REQUIRE_COPY_BYTES_16 */
#if REQUIRE_COPY_BYTES_20
COPY_CONTIGUOUS_BYTES( bytes, 20 )
#endif  /* REQUIRE_COPY_BYTES_20 */

conversion_fct_t ompi_ddt_copy_functions[DT_MAX_PREDEFINED] = {
   (conversion_fct_t)NULL,                      /* DT_LOOP                */
   (conversion_fct_t)NULL,                      /* DT_END_LOOP            */
   (conversion_fct_t)NULL,                      /* DT_LB                  */
   (conversion_fct_t)NULL,                      /* DT_UB                  */
   (conversion_fct_t)copy_char,                 /* DT_CHAR                */
   (conversion_fct_t)copy_char,                 /* DT_CHARACTER           */
   (conversion_fct_t)copy_char,                 /* DT_UNSIGNED_CHAR       */
   (conversion_fct_t)copy_char,                 /* DT_BYTE                */
   (conversion_fct_t)copy_short,                /* DT_SHORT               */
   (conversion_fct_t)copy_short,                /* DT_UNSIGNED_SHORT      */
   (conversion_fct_t)copy_int,                  /* DT_INT                 */
   (conversion_fct_t)copy_int,                  /* DT_UNSIGNED_INT        */
   (conversion_fct_t)copy_long,                 /* DT_LONG                */
   (conversion_fct_t)copy_long,                 /* DT_UNSIGNED_LONG       */
   (conversion_fct_t)copy_long_long,            /* DT_LONG_LONG           */
   (conversion_fct_t)copy_long_long,            /* DT_LONG_LONG_INT       */
   (conversion_fct_t)copy_long_long,            /* DT_UNSIGNED_LONG_LONG  */
   (conversion_fct_t)copy_float,                /* DT_FLOAT               */
   (conversion_fct_t)copy_double,               /* DT_DOUBLE              */
   (conversion_fct_t)copy_long_double,          /* DT_LONG_DOUBLE         */
   (conversion_fct_t)copy_complex_float,        /* DT_COMPLEX_FLOAT       */
   (conversion_fct_t)copy_complex_double,       /* DT_COMPLEX_DOUBLE      */
   (conversion_fct_t)copy_complex_long_double,  /* DT_COMPLEX_LONG_DOUBLE */
   (conversion_fct_t)NULL,                      /* DT_PACKED              */
#if OMPI_SIZEOF_FORTRAN_LOGICAL == 1
   (conversion_fct_t)copy_bytes_1,              /* DT_LOGIC               */
#elif OMPI_SIZEOF_FORTRAN_LOGICAL == 4
   (conversion_fct_t)copy_bytes_4,              /* DT_LOGIC               */
#elif
   NULL,                                        /* DT_LOGIC               */
#endif
#if (SIZEOF_FLOAT + SIZEOF_INT) == 8
   (conversion_fct_t)copy_bytes_8,              /* DT_FLOAT_INT           */
#else
#error Complete me please
#endif
#if (SIZEOF_DOUBLE + SIZEOF_INT) == 12
   (conversion_fct_t)copy_bytes_12,             /* DT_DOUBLE_INT          */
#else
#error Complete me please
#endif
#if (SIZEOF_LONG_DOUBLE + SIZEOF_INT) == 12
   (conversion_fct_t)copy_bytes_12,             /* DT_LONG_DOUBLE_INT     */
#elif (SIZEOF_LONG_DOUBLE + SIZEOF_INT) == 16
   (conversion_fct_t)copy_bytes_16,             /* DT_LONG_DOUBLE_INT     */
#elif (SIZEOF_LONG_DOUBLE + SIZEOF_INT) == 20
   (conversion_fct_t)copy_bytes_20,             /* DT_LONG_DOUBLE_INT     */
#else
#error Complete me please
#endif
#if (SIZEOF_LONG + SIZEOF_INT) == 8
   (conversion_fct_t)copy_bytes_8,              /* DT_LONG_INT            */
#elif (SIZEOF_LONG + SIZEOF_INT) == 12
   (conversion_fct_t)copy_bytes_12,             /* DT_LONG_INT            */
#else
#error Complete me please
#endif
   (conversion_fct_t)copy_2int,                 /* DT_2INT                */
   (conversion_fct_t)NULL,                      /* DT_SHORT_INT           */
   (conversion_fct_t)copy_int,                  /* DT_INTEGER             */
   (conversion_fct_t)copy_float,                /* DT_REAL                */
   (conversion_fct_t)copy_double,               /* DT_DBLPREC             */
   (conversion_fct_t)copy_2float,               /* DT_2REAL               */
   (conversion_fct_t)copy_2double,              /* DT_2DBLPREC            */
   (conversion_fct_t)copy_2int,                 /* DT_2INTEGER            */
   (conversion_fct_t)copy_wchar,                /* DT_WCHAR               */
   (conversion_fct_t)copy_2complex_float,       /* DT_2COMPLEX            */
   (conversion_fct_t)copy_2complex_double,      /* DT_2DOUBLE_COMPLEX     */
#if SIZEOF_BOOL == 1
   (conversion_fct_t)copy_bytes_1,              /* DT_CXX_BOOL            */
#elif SIZEOF_BOOL == 4
   (conversion_fct_t)copy_bytes_4,              /* DT_CXX_BOOL            */
#elif SIZEOF_BOOL == 8
   (conversion_fct_t)copy_bytes_8,              /* DT_CXX_BOOL            */
#else
#error Complete me please
#endif
   (conversion_fct_t)NULL,                      /* DT_UNAVAILABLE         */
};

/* Should we supply buffers to the convertor or can we use directly
 * the user buffer ?
 */
int32_t ompi_convertor_need_buffers( ompi_convertor_t* pConvertor )
{
    const ompi_datatype_t* pData = pConvertor->pDesc;
    if( !(pData->flags & DT_FLAG_CONTIGUOUS) ) return 1;
    if( pConvertor->count == 1 ) return 0;  /* only one data ignore the gaps around */
    if( (long)pData->size != (pData->ub - pData->lb) ) return 1;
    return 0;
}

extern int ompi_ddt_local_sizes[DT_MAX_PREDEFINED];

inline int32_t
ompi_convertor_prepare_for_recv( ompi_convertor_t* convertor,
                                 const struct ompi_datatype_t* datatype,
                                 int32_t count,
                                 const void* pUserBuf )
{
    /* Here I should check that the data is not overlapping */

    if( OMPI_SUCCESS != ompi_convertor_prepare( convertor, datatype,
                                                count, pUserBuf ) ) {
        return OMPI_ERROR;
    }

    convertor->flags      |= CONVERTOR_RECV;
    convertor->memAlloc_fn = NULL;
    convertor->fAdvance    = ompi_convertor_unpack_general;     /* TODO: just stop complaining */
    convertor->fAdvance    = ompi_convertor_unpack_homogeneous; /* default behaviour */
    convertor->fAdvance    = ompi_convertor_generic_simple_unpack;

    /* TODO: work only on homogeneous architectures */
    if( convertor->pDesc->flags & DT_FLAG_CONTIGUOUS ) {
        convertor->flags |= DT_FLAG_CONTIGUOUS;
        convertor->fAdvance = ompi_convertor_unpack_homogeneous_contig;
    }
    return OMPI_SUCCESS;
}

int32_t
ompi_convertor_copy_and_prepare_for_recv( const ompi_convertor_t* pSrcConv,
                                          const struct ompi_datatype_t* datatype,
                                          int32_t count,
                                          const void* pUserBuf,
                                          ompi_convertor_t* convertor )
{
    convertor->remoteArch      = pSrcConv->remoteArch;
    convertor->pFunctions      = pSrcConv->pFunctions;
    convertor->flags           = pSrcConv->flags & ~CONVERTOR_STATE_MASK;

    return ompi_convertor_prepare_for_recv( convertor, datatype, count, pUserBuf );
}

/* Get the number of elements from the data associated with this convertor that can be
 * retrieved from a recevied buffer with the size iSize.
 * To spped-up this function you should use it with a iSize == to the modulo
 * of the original size and the size of the data.
 * This function should be called with a initialized clean convertor.
 * Return value:
 *   positive = number of basic elements inside
 *   negative = some error occurs
 */
int32_t ompi_ddt_get_element_count( const ompi_datatype_t* datatype, int32_t iSize )
{
    dt_stack_t* pStack;   /* pointer to the position on the stack */
    uint32_t pos_desc;    /* actual position in the description of the derived datatype */
    int rc, nbElems = 0;
    int stack_pos = 0;
    dt_elem_desc_t* pElems;

    /* Normally the size should be less or equal to the size of the datatype.
     * This function does not support a iSize bigger than the size of the datatype.
     */
    assert( (uint32_t)iSize <= datatype->size );
    DUMP( "dt_count_elements( %p, %d )\n", (void*)datatype, iSize );
    pStack = alloca( sizeof(dt_stack_t) * (datatype->btypes[DT_LOOP] + 2) );
    pStack->count    = 1;
    pStack->index    = -1;
    pStack->disp     = 0;
    pElems           = datatype->desc.desc;
    pStack->end_loop = datatype->desc.used;
    pos_desc         = 0;

    while( 1 ) {  /* loop forever the exit conditionis on the last section */
        if( DT_END_LOOP == pElems[pos_desc].elem.common.type ) { /* end of the current loop */
            if( --(pStack->count) == 0 ) { /* end of loop */
                stack_pos--;
                pStack--;
                if( stack_pos == -1 )
                    return nbElems;  /* completed */
            }
            if( pStack->index == -1 ) {
                pStack->disp += (datatype->ub - datatype->lb);
            } else {
                assert( DT_LOOP == pElems[pStack->index].elem.common.type );
                pStack->disp += pElems[pStack->index].loop.extent;
            }
            pos_desc = pStack->index + 1;
            continue;
        }
        if( DT_LOOP == pElems[pos_desc].elem.common.type ) {
            ddt_loop_desc_t* loop = &(pElems[pos_desc].loop);
            do {
                PUSH_STACK( pStack, stack_pos, pos_desc, DT_LOOP, loop->loops,
                            0, pos_desc + loop->items );
                pos_desc++;
            } while( DT_LOOP == pElems[pos_desc].elem.common.type ); /* let's start another loop */
            DDT_DUMP_STACK( pStack, stack_pos, pElems, "advance loops" );
            continue;
        }
        while( pElems[pos_desc].elem.common.flags & DT_FLAG_DATA ) {
            /* now here we have a basic datatype */
            const ompi_datatype_t* basic_type = BASIC_DDT_FROM_ELEM(pElems[pos_desc]);
            rc = pElems[pos_desc].elem.count * basic_type->size;
            if( rc >= iSize ) {
                rc = iSize / basic_type->size;
                nbElems += rc;
                iSize -= rc * basic_type->size;
                return (iSize == 0 ? nbElems : -1);
            }
            nbElems += pElems[pos_desc].elem.count;
            iSize -= rc;
            pos_desc++;  /* advance to the next data */
        }
    }

    /* cleanup the stack */
    return -1;  /* never reached */
}

int32_t ompi_ddt_copy_content_same_ddt( const ompi_datatype_t* datatype, int32_t count,
                                        char* pDestBuf, const char* pSrcBuf )
{
    dt_stack_t* pStack;   /* pointer to the position on the stack */
    int pos_desc;         /* actual position in the description of the derived datatype */
    int stack_pos = 0, i;
    long lastDisp = 0, lastLength = 0;
    dt_elem_desc_t* pElems;

    if( !(datatype->flags & DT_FLAG_COMMITED) ) {  /* datatype not committed */
        return OMPI_ERROR;
    }
    /* empty data ? then do nothing. This should normally be trapped
     * at a higher level.
     */
    if( count == 0 ) return OMPI_SUCCESS;

    /* If we have to copy a contiguous datatype then simply
     * do a memcpy.
     */
    if( (datatype->flags & DT_FLAG_CONTIGUOUS) == DT_FLAG_CONTIGUOUS ) {
        long extent = (datatype->ub - datatype->lb);
	/* Now that we know the datatype is contiguous, we should move the 2 pointers
	 * source and destination to the correct displacement.
	 */
	pDestBuf += datatype->lb;
	pSrcBuf += datatype->lb;
	if( (long)datatype->size == extent ) {  /* all contiguous == no gaps around */
            int total_length = datatype->size * count;
            lastLength = 128 * 1024;
            if( lastLength > total_length ) lastLength = total_length;
            while( total_length > 0 ) {
                OMPI_DDT_SAFEGUARD_POINTER( pDestBuf, lastLength,
                                            pDestBuf, datatype, count );
                MEMCPY( pDestBuf, pSrcBuf, lastLength );
                pDestBuf += lastLength;
                pSrcBuf += lastLength;
                total_length -= lastLength;
                if( lastLength > total_length ) lastLength = total_length;
            }
        } else {
            for( pos_desc = 0; pos_desc < count; pos_desc++ ) {
                OMPI_DDT_SAFEGUARD_POINTER( pDestBuf, datatype->size,
                                            pDestBuf, datatype, count );
                MEMCPY( pDestBuf, pSrcBuf, datatype->size );
                pDestBuf += extent;
                pSrcBuf += extent;
            }
        }
        return 0;
    }

    pStack = alloca( sizeof(dt_stack_t) * (datatype->btypes[DT_LOOP] + 1) );
    pStack->count = count;
    pStack->index = -1;
    pStack->disp = 0;
    pos_desc  = 0;

    if( datatype->opt_desc.desc != NULL ) {
        pElems = datatype->opt_desc.desc;
        pStack->end_loop = datatype->opt_desc.used;
    } else {
        pElems = datatype->desc.desc;
        pStack->end_loop = datatype->desc.used;
    }

    while( 1 ) {
        if( DT_END_LOOP == pElems[pos_desc].elem.common.type ) { /* end of the current loop */
            if( --(pStack->count) == 0 ) { /* end of loop */
                pStack--;
                if( --stack_pos == -1 ) {
                    goto end_loop;
                }
                DDT_DUMP_STACK( pStack, stack_pos, pElems, "loop finish" );
                pos_desc++;
            } else {
                DDT_DUMP_STACK( pStack, stack_pos, pElems, "decrease loop count" );
                if( pStack->index == -1 ) {
                    pStack->disp += (datatype->ub - datatype->lb);
                } else {
                    assert( DT_LOOP == pElems[pStack->index].elem.common.type );
                    pStack->disp += pElems[pStack->index].loop.extent;
                }
                pos_desc = pStack->index + 1;
            }
        }
        if( DT_LOOP == pElems[pos_desc].elem.common.type ) {
            do {
                PUSH_STACK( pStack, stack_pos, pos_desc, DT_LOOP, pElems[pos_desc].loop.loops,
                            pStack->disp, pos_desc + pElems[pos_desc].loop.items );
                pos_desc++;
            } while( DT_LOOP == pElems[pos_desc].elem.common.type ); /* let's start another loop */
            DDT_DUMP_STACK( pStack, stack_pos, pElems, "advance loops" );
        }
        while( pElems[pos_desc].elem.common.flags & DT_FLAG_DATA ) {
            if( (lastDisp + lastLength) != (pStack->disp + pElems[pos_desc].elem.disp) ) {
                /* If now contiguous with the previous piece of data then first save
                 * the previous one...
                 */
                OMPI_DDT_SAFEGUARD_POINTER( pDestBuf + lastDisp, lastLength,
                                            pDestBuf, datatype, count );
                MEMCPY( pDestBuf + lastDisp, pSrcBuf + lastDisp, lastLength );
                lastDisp = pStack->disp + pElems[pos_desc].elem.disp;
                lastLength = 0;
            }
            if( pElems[pos_desc].elem.common.flags & DT_FLAG_CONTIGUOUS ) {
                /* a contiguous piece of memory. Just add it to the actual one. Notice that if this
                 * datatype is not contiguous with the previous one, then the old one is already
                 * copied. Thus we just have to increase the amount ...
                 */
                lastLength += pElems[pos_desc].elem.count * BASIC_DDT_FROM_ELEM(pElems[pos_desc])->size;
            } else {
                /* basic datatype but with an extent different that the size. Try to add the first
                 * one to the previous piece of memory ...
                 */
                lastLength += BASIC_DDT_FROM_ELEM(pElems[pos_desc])->size;
                for( i = 0; i < ((int)pElems[pos_desc].elem.count - 1); i++ ) {
                    OMPI_DDT_SAFEGUARD_POINTER( pDestBuf + lastDisp, lastLength,
                                                pDestBuf, datatype, count );
                    MEMCPY( pDestBuf + lastDisp, pSrcBuf + lastDisp, lastLength );
                    lastDisp += pElems[pos_desc].elem.disp;
                    lastLength = BASIC_DDT_FROM_ELEM(pElems[pos_desc])->size;
                }
            }
            pos_desc++;  /* advance to the next data */
        }
    }
 end_loop:
    if( lastLength != 0 ) {
        OMPI_DDT_SAFEGUARD_POINTER( pDestBuf + lastDisp, lastLength,
                                    pDestBuf, datatype, count );
        MEMCPY( pDestBuf + lastDisp, pSrcBuf + lastDisp, lastLength );
    }
    /* cleanup the stack */
    return OMPI_SUCCESS;
}
