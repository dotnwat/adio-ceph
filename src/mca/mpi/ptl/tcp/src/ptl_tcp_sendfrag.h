/* @file
 *
 * $HEADER$
 */

#ifndef MCA_PTL_TCP_SEND_FRAG_H
#define MCA_PTL_TCP_SEND_FRAG_H

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include "lam_config.h"
#include "mca/mpi/ptl/base/ptl_base_sendfrag.h"
#include "ptl_tcp.h"

extern lam_class_info_t mca_ptl_tcp_send_frag_t_class_info;


struct mca_ptl_tcp_send_frag_t {
   mca_ptl_base_send_frag_t super;
   struct iovec *frag_vec_ptr;
   size_t frag_vec_cnt;
   struct iovec frag_vec[2];
#define frag_peer super.super.frag_peer
#define frag_header super.super.frag_header
#define frag_owner super.super.frag_owner
};
typedef struct mca_ptl_tcp_send_frag_t mca_ptl_tcp_send_frag_t;


static inline mca_ptl_tcp_send_frag_t* mca_ptl_tcp_send_frag_alloc(int* rc) 
{
    return (mca_ptl_tcp_send_frag_t*)lam_free_list_get(&mca_ptl_tcp_module.tcp_send_frags, rc);
}


static inline void mca_ptl_tcp_send_frag_return(mca_ptl_tcp_send_frag_t* frag)
{
    lam_free_list_return(&mca_ptl_tcp_module.tcp_send_frags, (lam_list_item_t*)frag);
}


bool mca_ptl_tcp_send_frag_handler(mca_ptl_tcp_send_frag_t*, int sd);
void mca_ptl_tcp_send_frag_reinit(
    mca_ptl_tcp_send_frag_t*, 
    struct mca_ptl_base_peer_t*, 
    struct mca_ptl_base_send_request_t*, 
    size_t);

#endif

