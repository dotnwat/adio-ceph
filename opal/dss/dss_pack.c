/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2007 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2011      Cisco Systems, Inc.  All rights reserved.
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "opal_config.h"

#include "opal/types.h"
#include "opal/util/error.h"
#include "opal/util/output.h"
#include "opal/dss/dss_internal.h"

int opal_dss_pack(opal_buffer_t *buffer, const void *src, int32_t num_vals,
                  opal_data_type_t type)
{
    int rc;

    /* check for error */
    if (NULL == buffer) {
        return OPAL_ERR_BAD_PARAM;
    }

    /* Pack the number of values */
    if (OPAL_DSS_BUFFER_FULLY_DESC == buffer->type) {
        if (OPAL_SUCCESS != (rc = opal_dss_store_data_type(buffer, OPAL_INT32))) {
            return rc;
        }
    }
    if (OPAL_SUCCESS != (rc = opal_dss_pack_int32(buffer, &num_vals, 1, OPAL_INT32))) {
        return rc;
    }

    /* Pack the value(s) */
    return opal_dss_pack_buffer(buffer, src, num_vals, type);
}

int opal_dss_pack_buffer(opal_buffer_t *buffer, const void *src, int32_t num_vals,
                  opal_data_type_t type)
{
    int rc;
    opal_dss_type_info_t *info;

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_buffer( %p, %p, %lu, %d )\n",
                   (void*)buffer, src, (long unsigned int)num_vals, (int)type ) );

    /* Pack the declared data type */
    if (OPAL_DSS_BUFFER_FULLY_DESC == buffer->type) {
        if (OPAL_SUCCESS != (rc = opal_dss_store_data_type(buffer, type))) {
            return rc;
        }
    }

    /* Lookup the pack function for this type and call it */

    if (NULL == (info = (opal_dss_type_info_t*)opal_pointer_array_get_item(&opal_dss_types, type))) {
        return OPAL_ERR_PACK_FAILURE;
    }

    return info->odti_pack_fn(buffer, src, num_vals, type);
}


/* PACK FUNCTIONS FOR GENERIC SYSTEM TYPES */

/*
 * BOOL
 */
int opal_dss_pack_bool(opal_buffer_t *buffer, const void *src,
                       int32_t num_vals, opal_data_type_t type)
{
    int ret;

    /* System types need to always be described so we can properly
       unpack them.  If we aren't fully described, then add the
       description for this type... */
    if (OPAL_DSS_BUFFER_FULLY_DESC != buffer->type) {
        if (OPAL_SUCCESS != (ret = opal_dss_store_data_type(buffer, DSS_TYPE_BOOL))) {
            return ret;
        }
    }

    /* Turn around and pack the real type */
    return opal_dss_pack_buffer(buffer, src, num_vals, DSS_TYPE_BOOL);
}

/*
 * INT
 */
int opal_dss_pack_int(opal_buffer_t *buffer, const void *src,
                      int32_t num_vals, opal_data_type_t type)
{
    int ret;

    /* System types need to always be described so we can properly
       unpack them.  If we aren't fully described, then add the
       description for this type... */
    if (OPAL_DSS_BUFFER_FULLY_DESC != buffer->type) {
        if (OPAL_SUCCESS != (ret = opal_dss_store_data_type(buffer, DSS_TYPE_INT))) {
            return ret;
        }
    }

    /* Turn around and pack the real type */
    return opal_dss_pack_buffer(buffer, src, num_vals, DSS_TYPE_INT);
}

/*
 * SIZE_T
 */
int opal_dss_pack_sizet(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    int ret;

    /* System types need to always be described so we can properly
       unpack them.  If we aren't fully described, then add the
       description for this type... */
    if (OPAL_DSS_BUFFER_FULLY_DESC != buffer->type) {
        if (OPAL_SUCCESS != (ret = opal_dss_store_data_type(buffer, DSS_TYPE_SIZE_T))) {
            return ret;
        }
    }

    return opal_dss_pack_buffer(buffer, src, num_vals, DSS_TYPE_SIZE_T);
}

/*
 * PID_T
 */
int opal_dss_pack_pid(opal_buffer_t *buffer, const void *src,
                      int32_t num_vals, opal_data_type_t type)
{
    int ret;

    /* System types need to always be described so we can properly
       unpack them.  If we aren't fully described, then add the
       description for this type... */
    if (OPAL_DSS_BUFFER_FULLY_DESC != buffer->type) {
        if (OPAL_SUCCESS != (ret = opal_dss_store_data_type(buffer, DSS_TYPE_PID_T))) {
            return ret;
        }
    }

    /* Turn around and pack the real type */
    return opal_dss_pack_buffer(buffer, src, num_vals, DSS_TYPE_PID_T);
}


/* PACK FUNCTIONS FOR NON-GENERIC SYSTEM TYPES */

/*
 * NULL
 */
int opal_dss_pack_null(opal_buffer_t *buffer, const void *src,
                       int32_t num_vals, opal_data_type_t type)
{
    char null=0x00;
    char *dst;

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_null * %d\n", num_vals ) );
    /* check to see if buffer needs extending */
    if (NULL == (dst = opal_dss_buffer_extend(buffer, num_vals))) {
        return OPAL_ERR_OUT_OF_RESOURCE;
    }

    /* store the nulls */
    memset(dst, (int)null, num_vals);

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return OPAL_SUCCESS;
}

/*
 * BYTE, CHAR, INT8
 */
int opal_dss_pack_byte(opal_buffer_t *buffer, const void *src,
                       int32_t num_vals, opal_data_type_t type)
{
    char *dst;

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_byte * %d\n", num_vals ) );
    /* check to see if buffer needs extending */
    if (NULL == (dst = opal_dss_buffer_extend(buffer, num_vals))) {
        return OPAL_ERR_OUT_OF_RESOURCE;
    }

    /* store the data */
    memcpy(dst, src, num_vals);

    /* update buffer pointers */
    buffer->pack_ptr += num_vals;
    buffer->bytes_used += num_vals;

    return OPAL_SUCCESS;
}

/*
 * INT16
 */
int opal_dss_pack_int16(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    int32_t i;
    uint16_t tmp, *srctmp = (uint16_t*) src;
    char *dst;

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_int16 * %d\n", num_vals ) );
    /* check to see if buffer needs extending */
    if (NULL == (dst = opal_dss_buffer_extend(buffer, num_vals*sizeof(tmp)))) {
        return OPAL_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = htons(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return OPAL_SUCCESS;
}

/*
 * INT32
 */
int opal_dss_pack_int32(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    int32_t i;
    uint32_t tmp, *srctmp = (uint32_t*) src;
    char *dst;

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_int32 * %d\n", num_vals ) );
    /* check to see if buffer needs extending */
    if (NULL == (dst = opal_dss_buffer_extend(buffer, num_vals*sizeof(tmp)))) {
        return OPAL_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = htonl(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += num_vals * sizeof(tmp);
    buffer->bytes_used += num_vals * sizeof(tmp);

    return OPAL_SUCCESS;
}

/*
 * INT64
 */
int opal_dss_pack_int64(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    int32_t i;
    uint64_t tmp, *srctmp = (uint64_t*) src;
    char *dst;
    size_t bytes_packed = num_vals * sizeof(tmp);

    OPAL_OUTPUT( ( opal_dss_verbose, "opal_dss_pack_int64 * %d\n", num_vals ) );
    /* check to see if buffer needs extending */
    if (NULL == (dst = opal_dss_buffer_extend(buffer, bytes_packed))) {
        return OPAL_ERR_OUT_OF_RESOURCE;
    }

    for (i = 0; i < num_vals; ++i) {
        tmp = hton64(srctmp[i]);
        memcpy(dst, &tmp, sizeof(tmp));
        dst += sizeof(tmp);
    }
    buffer->pack_ptr += bytes_packed;
    buffer->bytes_used += bytes_packed;

    return OPAL_SUCCESS;
}

/*
 * STRING
 */
int opal_dss_pack_string(opal_buffer_t *buffer, const void *src,
                         int32_t num_vals, opal_data_type_t type)
{
    int ret = OPAL_SUCCESS;
    int32_t i, len;
    char **ssrc = (char**) src;

    for (i = 0; i < num_vals; ++i) {
        if (NULL == ssrc[i]) {  /* got zero-length string/NULL pointer - store NULL */
            len = 0;
            if (OPAL_SUCCESS != (ret = opal_dss_pack_int32(buffer, &len, 1, OPAL_INT32))) {
                return ret;
            }
        } else {
            len = (int32_t)strlen(ssrc[i]) + 1;
            if (OPAL_SUCCESS != (ret = opal_dss_pack_int32(buffer, &len, 1, OPAL_INT32))) {
                return ret;
            }
            if (OPAL_SUCCESS != (ret =
                opal_dss_pack_byte(buffer, ssrc[i], len, OPAL_BYTE))) {
                return ret;
            }
        }
    }

    return OPAL_SUCCESS;
}

/* PACK FUNCTIONS FOR GENERIC OPAL TYPES */

/*
 * OPAL_DATA_TYPE
 */
int opal_dss_pack_data_type(opal_buffer_t *buffer, const void *src, int32_t num_vals,
                            opal_data_type_t type)
{
    int ret;
    
    /* Turn around and pack the real type */
    if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, src, num_vals, OPAL_DATA_TYPE_T))) {
    }
    
    return ret;
}

/*
 * OPAL_BYTE_OBJECT
 */
int opal_dss_pack_byte_object(opal_buffer_t *buffer, const void *src, int32_t num,
                             opal_data_type_t type)
{
    opal_byte_object_t **sbyteptr;
    int32_t i, n;
    int ret;

    sbyteptr = (opal_byte_object_t **) src;

    for (i = 0; i < num; ++i) {
        n = sbyteptr[i]->size;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_int32(buffer, &n, 1, OPAL_INT32))) {
            return ret;
        }
        if (0 < n) {
            if (OPAL_SUCCESS != (ret =
                opal_dss_pack_byte(buffer, sbyteptr[i]->bytes, n, OPAL_BYTE))) {
                return ret;
            }
        }
    }

    return OPAL_SUCCESS;
}

static int opal_dss_pack_float(opal_buffer_t *buffer, float val)
{
    int32_t tmp1, tmp2;
    int ret;

    tmp1 = (int)val;
    if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp1, 1, OPAL_INT32))) {
        return ret;
    }
    tmp2 = (int)(100.0 * (val - (int)val));
    if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp2, 1, OPAL_INT32))) {
        return ret;
    }
    return OPAL_SUCCESS;
}

/*
 * OPAL_PSTAT
 */
int opal_dss_pack_pstat(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    opal_pstats_t **ptr;
    int32_t i, tmp;
    int ret;
    char *cptr;
    
    ptr = (opal_pstats_t **) src;
    
    for (i = 0; i < num_vals; ++i) {
        cptr = ptr[i]->node;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &cptr, 1, OPAL_STRING))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->rank, 1, OPAL_INT32))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->pid, 1, OPAL_PID))) {
            return ret;
        }
        cptr = ptr[i]->cmd;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &cptr, 1, OPAL_STRING))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->state[0], 1, OPAL_BYTE))) {
            return ret;
        }
        tmp = ptr[i]->time.tv_sec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
        tmp = ptr[i]->time.tv_usec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->priority, 1, OPAL_INT32))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->num_threads, 1, OPAL_INT16))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->vsize))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->rss))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->peak_vsize))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->processor, 1, OPAL_INT16))) {
            return ret;
        }
        tmp = ptr[i]->sample_time.tv_sec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
        tmp = ptr[i]->sample_time.tv_usec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
    }

    return OPAL_SUCCESS;
}

/*
 * OPAL_NODE_STAT
 */
int opal_dss_pack_node_stat(opal_buffer_t *buffer, const void *src,
                            int32_t num_vals, opal_data_type_t type)
{
    opal_node_stats_t **ptr;
    int32_t i, tmp;
    int ret;
    
    ptr = (opal_node_stats_t **) src;
    
    for (i = 0; i < num_vals; ++i) {
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->la))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->la5))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->la15))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->total_mem))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->free_mem))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->buffers))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->cached))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->swap_cached))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->swap_total))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->swap_free))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_float(buffer, ptr[i]->mapped))) {
            return ret;
        }
        tmp = ptr[i]->sample_time.tv_sec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
        tmp = ptr[i]->sample_time.tv_usec;
        if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &tmp, 1, OPAL_INT32))) {
            return ret;
        }
    }

    return OPAL_SUCCESS;
}

/*
 * OPAL_VALUE
 */
int opal_dss_pack_value(opal_buffer_t *buffer, const void *src,
                        int32_t num_vals, opal_data_type_t type)
{
    opal_value_t **ptr;
    int32_t i, n;
    int ret;

    ptr = (opal_value_t **) src;
    
    for (i = 0; i < num_vals; ++i) {
        /* pack the key and type */
        if (OPAL_SUCCESS != (ret = opal_dss_pack_string(buffer, &ptr[i]->key, 1, OPAL_STRING))) {
            return ret;
        }
        if (OPAL_SUCCESS != (ret = opal_dss_pack_data_type(buffer, &ptr[i]->type, 1, OPAL_DATA_TYPE))) {
            return ret;
        }
        /* now pack the right field */
        switch (ptr[i]->type) {
        case OPAL_BYTE:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.byte, 1, OPAL_BYTE))) {
                return ret;
            }
            break;
        case OPAL_STRING:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.string, 1, OPAL_STRING))) {
                return ret;
            }
            break;
        case OPAL_PID:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.pid, 1, OPAL_PID))) {
                return ret;
            }
            break;
        case OPAL_INT:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.integer, 1, OPAL_INT))) {
                return ret;
            }
            break;
        case OPAL_INT8:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.int8, 1, OPAL_INT8))) {
                return ret;
            }
            break;
        case OPAL_INT16:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.int16, 1, OPAL_INT16))) {
                return ret;
            }
            break;
        case OPAL_INT32:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.int32, 1, OPAL_INT32))) {
                return ret;
            }
            break;
        case OPAL_INT64:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.int64, 1, OPAL_INT64))) {
                return ret;
            }
            break;
        case OPAL_UINT:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.uint, 1, OPAL_UINT))) {
                return ret;
            }
            break;
        case OPAL_UINT8:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.uint8, 1, OPAL_UINT8))) {
                return ret;
            }
            break;
        case OPAL_UINT16:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.uint16, 1, OPAL_UINT16))) {
                return ret;
            }
            break;
        case OPAL_UINT32:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.uint32, 1, OPAL_UINT32))) {
                return ret;
            }
            break;
        case OPAL_UINT64:
            if (OPAL_SUCCESS != (ret = opal_dss_pack_buffer(buffer, &ptr[i]->data.uint64, 1, OPAL_UINT64))) {
                return ret;
            }
            break;
        case OPAL_BYTE_OBJECT:
            /* have to pack by hand so we can match unpack without allocation */
            n = ptr[i]->data.bo.size;
            if (OPAL_SUCCESS != (ret = opal_dss_pack_int32(buffer, &n, 1, OPAL_INT32))) {
                return ret;
            }
            if (0 < n) {
                if (OPAL_SUCCESS != (ret = opal_dss_pack_byte(buffer, ptr[i]->data.bo.bytes, n, OPAL_BYTE))) {
                    return ret;
                }
            }
            break;
        default:
            opal_output(0, "PACK-OPAL-VALUE: UNSUPPORTED TYPE %d", (int)ptr[i]->type);
            return OPAL_ERROR;
        }
    }

    return OPAL_SUCCESS;
}
