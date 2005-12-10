#include "ompi_config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */

#include "include/ompi_socket_errno.h"
#include "ompi/mca/btl/base/btl_base_error.h"
#include "btl_tcp_frag.h" 
#include "btl_tcp_endpoint.h"
#include "orte/util/proc_info.h"

static void mca_btl_tcp_frag_common_constructor(mca_btl_tcp_frag_t* frag) 
{ 
    frag->base.des_src = NULL;
    frag->base.des_src_cnt = 0;
    frag->base.des_dst = NULL;
    frag->base.des_dst_cnt = 0;
}

static void mca_btl_tcp_frag_eager_constructor(mca_btl_tcp_frag_t* frag) 
{ 
    frag->size = mca_btl_tcp_module.super.btl_eager_limit;   
    mca_btl_tcp_frag_common_constructor(frag); 
}

static void mca_btl_tcp_frag_max_constructor(mca_btl_tcp_frag_t* frag) 
{ 
    frag->size = mca_btl_tcp_module.super.btl_max_send_size; 
    mca_btl_tcp_frag_common_constructor(frag); 
}

static void mca_btl_tcp_frag_user_constructor(mca_btl_tcp_frag_t* frag) 
{ 
    frag->size = 0; 
    mca_btl_tcp_frag_common_constructor(frag); 
}


OBJ_CLASS_INSTANCE(
    mca_btl_tcp_frag_t, 
    mca_btl_base_descriptor_t, 
    NULL, 
    NULL); 

OBJ_CLASS_INSTANCE(
    mca_btl_tcp_frag_eager_t, 
    mca_btl_base_descriptor_t, 
    mca_btl_tcp_frag_eager_constructor, 
    NULL); 

OBJ_CLASS_INSTANCE(
    mca_btl_tcp_frag_max_t, 
    mca_btl_base_descriptor_t, 
    mca_btl_tcp_frag_max_constructor, 
    NULL); 

OBJ_CLASS_INSTANCE(
    mca_btl_tcp_frag_user_t, 
    mca_btl_base_descriptor_t, 
    mca_btl_tcp_frag_user_constructor, 
    NULL); 


bool mca_btl_tcp_frag_send(mca_btl_tcp_frag_t* frag, int sd)
{
    int cnt=-1;
    size_t i, num_vecs;

    /* non-blocking write, but continue if interrupted */
    while(cnt < 0) {
        cnt = writev(sd, frag->iov_ptr, frag->iov_cnt);
        if(cnt < 0) {
            switch(ompi_socket_errno) {
            case EINTR:
                continue;
            case EWOULDBLOCK:
                /* opal_output(0, "mca_btl_tcp_frag_send: EWOULDBLOCK\n"); */
                return false;
            case EFAULT:
                BTL_ERROR(("writev error (%p, %d)\n\t%s(%d)\n",
                    frag->iov_ptr[0].iov_base, frag->iov_ptr[0].iov_len,
                    strerror(ompi_socket_errno), frag->iov_cnt));
            default:
                {
                BTL_ERROR(("writev failed with errno=%d", ompi_socket_errno));
                mca_btl_tcp_endpoint_close(frag->endpoint);
                return false;
                }
            }
        }
    }

    /* if the write didn't complete - update the iovec state */
    num_vecs = frag->iov_cnt;
    for(i=0; i<num_vecs; i++) {
        if(cnt >= (int)frag->iov_ptr->iov_len) {
            cnt -= frag->iov_ptr->iov_len;
            frag->iov_ptr++;
            frag->iov_idx++;
            frag->iov_cnt--;
        } else {
            frag->iov_ptr->iov_base = (ompi_iov_base_ptr_t)
                (((unsigned char*)frag->iov_ptr->iov_base) + cnt);
            frag->iov_ptr->iov_len -= cnt;
            break;
        }
    }
    return (frag->iov_cnt == 0);
}


bool mca_btl_tcp_frag_recv(mca_btl_tcp_frag_t* frag, int sd)
{
    int cnt;
    size_t i, num_vecs = frag->iov_cnt;
    mca_btl_base_endpoint_t* btl_endpoint = frag->endpoint;

 repeat:

#if MCA_BTL_TCP_ENDPOINT_CACHE
    if( 0 != btl_endpoint->endpoint_cache_length ) {
        size_t length = btl_endpoint->endpoint_cache_length;
        cnt = btl_endpoint->endpoint_cache_length;
        for( i = 0; i < frag->iov_cnt; i++ ) {
            if( length > frag->iov_ptr[i].iov_len )
                length = frag->iov_ptr[0].iov_len;
            memcpy( frag->iov_ptr[i].iov_base,
                    btl_endpoint->endpoint_cache + btl_endpoint->endpoint_cache_pos,
                    length );
            btl_endpoint->endpoint_cache_pos += length;
            btl_endpoint->endpoint_cache_length -= length;
            length = btl_endpoint->endpoint_cache_length;
            if( 0 == length ) break;
        }
        goto advance_iov_position;
    }

    frag->iov_ptr[num_vecs].iov_base = btl_endpoint->endpoint_cache;
    frag->iov_ptr[num_vecs].iov_len  = mca_btl_tcp_component.tcp_endpoint_cache;
    num_vecs++;
#endif  /* MCA_BTL_TCP_ENDPOINT_CACHE */

    /* non-blocking read, but continue if interrupted */
    cnt = -1;
    while( cnt < 0 ) {
        cnt = readv(sd, frag->iov_ptr, num_vecs);
        if(cnt < 0) {
            switch(ompi_socket_errno) {
            case EINTR:
                continue;
            case EWOULDBLOCK:
                return false;
            case EFAULT:
                opal_output( 0, "mca_btl_tcp_frag_send: writev error (%p, %d)\n\t%s(%d)\n",
                             frag->iov_ptr[0].iov_base, frag->iov_ptr[0].iov_len,
                             strerror(ompi_socket_errno), frag->iov_cnt );
            default:
                {
                    opal_output(0, "mca_btl_tcp_frag_send: writev failed with errno=%d", 
                                ompi_socket_errno);
                    mca_btl_tcp_endpoint_close(btl_endpoint);
                    return false;
                }
            }
        }
        if(cnt == 0) {
            mca_btl_tcp_endpoint_close(btl_endpoint);
            return false;
        }
        goto advance_iov_position;
    };

 advance_iov_position:
    /* if the write didn't complete - update the iovec state */
    num_vecs = frag->iov_cnt;
    for(i=0; i<num_vecs; i++) {
        if(cnt >= (int)frag->iov_ptr->iov_len) {
            cnt -= frag->iov_ptr->iov_len;
            frag->iov_idx++;
            frag->iov_ptr++;
            frag->iov_cnt--;
        } else {
            frag->iov_ptr->iov_base = (ompi_iov_base_ptr_t)
                (((unsigned char*)frag->iov_ptr->iov_base) + cnt);
            frag->iov_ptr->iov_len -= cnt;
	    cnt = 0;
            break;
        }
    }

#if MCA_BTL_TCP_ENDPOINT_CACHE
    btl_endpoint->endpoint_cache_length = cnt;
#endif  /* MCA_BTL_TCP_ENDPOINT_CACHE */

    /* read header */
    if(frag->iov_cnt == 0) {
        switch(frag->hdr.type) {
        case MCA_BTL_TCP_HDR_TYPE_SEND:
            if(frag->iov_idx == 1 && frag->hdr.size) {
                frag->iov[1].iov_base = (void*)(frag+1);
                frag->iov[1].iov_len = frag->hdr.size;
                frag->segments[0].seg_addr.pval = frag+1;
                frag->segments[0].seg_len = frag->hdr.size;
                frag->iov_cnt++;
                goto repeat;
            }
            break;
        case MCA_BTL_TCP_HDR_TYPE_PUT:
            if(frag->iov_idx == 1) {
                frag->iov[1].iov_base = (void*)frag->segments;
                frag->iov[1].iov_len = frag->hdr.count * sizeof(mca_btl_base_segment_t);
                frag->iov_cnt++;
                goto repeat;
            } else if (frag->iov_idx == 2) {
                for(i=0; i<frag->hdr.count; i++) {
                    frag->iov[i+2].iov_base = frag->segments[i].seg_addr.pval;
                    frag->iov[i+2].iov_len = frag->segments[i].seg_len;
                    frag->iov_cnt++;
                }
                goto repeat;
            }
            break;
        case MCA_BTL_TCP_HDR_TYPE_GET:
        default:
            break;
        }
        return true;
    }
    return false;
}

