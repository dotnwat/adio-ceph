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
#include "portals_config.h"

#include "ompi/include/constants.h"

#include "opal/util/output.h"
#include "opal/threads/threads.h"

#include "ptl_portals.h"
#include "ptl_portals_compat.h"
#include "ptl_portals_recv.h"
#include "ptl_portals_send.h"


mca_ptl_portals_component_t mca_ptl_portals_component = {
    {
      /* First, the mca_base_module_t struct containing meta
         information about the module itself */
      {
        /* Indicate that we are a pml v1.0.0 module (which also
           implies a specific MCA version) */

        MCA_PTL_BASE_VERSION_1_0_0,

        "portals", /* MCA module name */
        OMPI_MAJOR_VERSION,  /* MCA module major version */
        OMPI_MINOR_VERSION,  /* MCA module minor version */
        OMPI_RELEASE_VERSION,  /* MCA module release version */
        mca_ptl_portals_component_open,  /* module open */
        mca_ptl_portals_component_close  /* module close */
      },
      
      /* Next the MCA v1.0.0 module meta data */
      
      {
        /* Whether the module is checkpointable or not */
        
        false
      },
      
      mca_ptl_portals_component_init,  
      mca_ptl_portals_component_control,
      mca_ptl_portals_component_progress,
    }
};


static opal_output_stream_t portals_output_stream;
{
    true,  /* is debugging */
    0,     /* verbose level */
    0,     /* want syslog */
    0,     /* syslog priority */
    NULL,  /* syslog ident */
    NULL,  /* prefix */
    true, /* want stdout */
    false,  /* want stderr */
    false, /* want file */
    false, /* file append */
    "ptl-portals"   /* file suffix */
};


static inline char*
param_register_string(const char* param_name, 
                                 const char* default_value)
{
    char *param_value;
    int id = mca_base_param_register_string("ptl", "portals",
                                            param_name, NULL, 
                                            default_value);
    mca_base_param_lookup_string(id, &param_value);
    return param_value;
}


static inline int
param_register_int(const char* param_name, 
                              int default_value)
{
    int id = mca_base_param_register_int("ptl", "portals", param_name,
                                         NULL, default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id, &param_value);
    return param_value;
}



int
mca_ptl_portals_component_open(void)
{
    int i;

    /* initialize state */
    mca_ptl_portals_component.portals_num_modules = 0;
    mca_ptl_portals_component.portals_modules = NULL;

    /* initialize objects */
    OBJ_CONSTRUCT(&mca_ptl_portals_component.portals_send_frags, 
                  ompi_free_list_t);
    OBJ_CONSTRUCT(&mca_ptl_portals_component.portals_recv_frags, 
                  ompi_free_list_t);
    OBJ_CONSTRUCT(&mca_ptl_portals_component.portals_pending_acks, 
                  opal_list_t);
    OBJ_CONSTRUCT(&mca_ptl_portals_component.portals_lock, 
                  opal_mutex_t);

    OBJ_CONSTRUCT(&portals_output_stream, opal_output_stream_t);
    portals_output_stream.lds_is_debugging = true;
    portals_output_stream.lds_want_stdout = true;
    portals_output_stream.lds_file_suffix = "ptl-portals";

    /* register portals module parameters */
#if PTL_PORTALS_UTCP
    mca_ptl_portals_component.portals_ifname = 
        param_register_string("ifname", "eth0");
#endif
    portals_output_stream.lds_verbose_level = 
        param_register_int("debug_level",
                           PTL_PORTALS_DEFAULT_DEBUG_LEVEL);

    mca_ptl_portals_component.portals_free_list_init_num = 
        param_register_int("free_list_init_num",
                           PTL_PORTALS_DEFAULT_FREE_LIST_INIT_NUM);
    mca_ptl_portals_component.portals_free_list_max_num = 
        param_register_int("free_list_max_num",
                           PTL_PORTALS_DEFAULT_FREE_LIST_MAX_NUM);
    mca_ptl_portals_component.portals_free_list_inc_num = 
        param_register_int("free_list_inc_num",
                           PTL_PORTALS_DEFAULT_FREE_LIST_inc_NUM);

    mca_ptl_portals_module.super.ptl_cache_size =
        param_register_int("request_cache_size",
                           PTL_PORTALS_DEFAULT_REQUEST_CACHE_SIZE);
    mca_ptl_portals_module.super.ptl_first_frag_size =
        param_register_int("first_frag_size",
                           PTL_PORTALS_DEFAULT_FIRST_FRAG_SIZE);
    mca_ptl_portals_module.super.ptl_min_frag_size =
        param_register_int("rndv_frag_min_size",
                           PTL_PORTALS_DEFAULT_RNDV_FRAG_MIN_SIZE);
    mca_ptl_portals_module.super.ptl_max_frag_size =
        param_register_int("rndv_frag_max_size",
                           PTL_PORTALS_DEFAULT_RNDV_FRAG_MAX_SIZE);

    mca_ptl_portals_module.first_frag_num_entries = 
        param_register_int("first_frag_num_entries",
                           PTL_PORTALS_DEFAULT_FIRST_FRAG_NUM_ENTRIES);
    mca_ptl_portals_module.first_frag_entry_size = 
        param_register_int("first_frag_entry_size",
                           PTL_PORTALS_DEFAULT_FIRST_FRAG_ENTRY_SIZE);

    mca_ptl_portals_module.eq_sizes[MCA_PTL_PORTALS_EQ_RECV] = 
        param_register_int("recv_queue_size",
                           PTL_PORTALS_DEFAULT_RECV_QUEUE_SIZE);
    mca_ptl_portals_module.eq_sizes[MCA_PTL_PORTALS_EQ_SEND] = 
        (param_register_int("send_queue_size",
                            PTL_PORTALS_DEFAULT_SEND_QUEUE_SIZE)) * 3;

    /* finish with objects */
    asprintf(&(portals_output_stream.lds_prefix), 
             "ptl: portals (%5d): ", getpid());

    mca_ptl_portals_component.portals_output = 
        opal_output_open(&portals_output_stream);

    /* fill in remaining defaults for module data */
    for (i = 0 ; i < MCA_PTL_PORTALS_EQ_SIZE ; ++i) {
        mca_ptl_portals_module.eq_handles[i] = PTL_EQ_NONE;
    }

    mca_ptl_portals_module.ni_handle = PTL_INVALID_HANDLE;
    mca_ptl_portals_module.dropped = 0;

    return OMPI_SUCCESS;
}


int
mca_ptl_portals_component_close(void)
{
    /* print out debugging if anything is pending */
    /* BWB - implement me, if possible */

    /* release resources */
    OBJ_DESTRUCT(&mca_ptl_portals_component.portals_lock);
    OBJ_DESTRUCT(&mca_ptl_portals_component.portals_recv_frags);
    OBJ_DESTRUCT(&mca_ptl_portals_component.portals_pending_acks);
    OBJ_DESTRUCT(&mca_ptl_portals_component.portals_lock);

    if (NULL != mca_ptl_portals_component.portals_ifname) {
        free(mca_ptl_portals_component.portals_ifname);
    }

    if (NULL != portals_output_stream.lds_prefix) {
        portals_output_stream.lds_prefix = NULL;
    }

    opal_output_close(mca_ptl_portals_component.portals_output);
    mca_ptl_portals_component.portals_output = -1;

    return OMPI_SUCCESS;
}


mca_ptl_base_module_t**
mca_ptl_portals_component_init(int *num_ptls, 
                               bool enable_progress_threads,
                               bool enable_mpi_threads)
{
    mca_ptl_base_module_t** ptls;
    *num_ptls = 0;

    if (enable_progress_threads) {
        opal_output_verbose(20, mca_ptl_portals_component.portals_output,
                            "disabled because progress threads enabled");
        return NULL;
    }

    ompi_free_list_init(&mca_ptl_portals_component.portals_send_frags, 
        sizeof(mca_ptl_portals_send_frag_t),
        OBJ_CLASS(mca_ptl_portals_send_frag_t),
        mca_ptl_portals_component.portals_free_list_init_num,
        mca_ptl_portals_component.portals_free_list_max_num,
        mca_ptl_portals_component.portals_free_list_inc_num,
        NULL); /* use default allocator */

    ompi_free_list_init(&mca_ptl_portals_component.portals_recv_frags, 
        sizeof(mca_ptl_portals_recv_frag_t),
        OBJ_CLASS(mca_ptl_portals_recv_frag_t),
        mca_ptl_portals_component.portals_free_list_init_num,
        mca_ptl_portals_component.portals_free_list_max_num,
        mca_ptl_portals_component.portals_free_list_inc_num,
        NULL); /* use default allocator */

    /* initialize portals ptl.  note that this is in the compat code because
       it's fairly non-portable between implementations */
    if (OMPI_SUCCESS != mca_ptl_portals_init(&mca_ptl_portals_component)) {
        opal_output_verbose(20, mca_ptl_portals_component.portals_output,
                            "disabled because compatibility init failed");
        return NULL;
    }

    /* return array of ptls */
    ptls = malloc(mca_ptl_portals_component.portals_num_modules * 
                  sizeof(mca_ptl_base_module_t*));
    if (NULL == ptls) return NULL;

    memcpy(ptls, 
           mca_ptl_portals_component.portals_modules, 
           mca_ptl_portals_component.portals_num_modules * 
           sizeof(mca_ptl_base_module_t*));
    *num_ptls = mca_ptl_portals_component.portals_num_modules;

    opal_output_verbose(20, mca_ptl_portals_component.portals_output,
                        "initialized %d modules",
                        *num_ptls);

    return ptls;
}


int
mca_ptl_portals_component_control(int param, void* value, size_t size)
{
    uint32_t i;
    int ret = OMPI_SUCCESS;

    opal_output_verbose(30, mca_ptl_portals_component.portals_output,
                        "component control: %d, %d", 
                        param, (*(int*) value));

    switch(param) {
        case MCA_PTL_ENABLE:
            for (i = 0 ; 
                 i < mca_ptl_portals_component.portals_num_modules ; 
                 ++i) {
                ret = mca_ptl_portals_module_enable(
                                mca_ptl_portals_component.portals_modules[i],
                                *(int*)value);                  
                if (ret != OMPI_SUCCESS) break;
            }
            break;
        default:
            break;
    }
    return ret;
}


int
mca_ptl_portals_component_progress(mca_ptl_tstamp_t tstamp)
{
    int num_progressed = 0;
    size_t i;
    tstamp = 10;

    for (i = 0 ; i < mca_ptl_portals_component.portals_num_modules ; ++i) {
        struct mca_ptl_portals_module_t *module = 
            mca_ptl_portals_component.portals_modules[i];
        ptl_event_t ev;
        ptl_sr_value_t numdropped;
        int which;
        int ret;

        if (module->eq_handles[MCA_PTL_PORTALS_EQ_SIZE - 1] == 
            PTL_EQ_NONE) continue; /* they are all initialized at once */

#if OMPI_ENABLE_DEBUG
        /* BWB - this is going to kill performance */
        PtlNIStatus(module->ni_handle,
                    PTL_SR_DROP_COUNT,
                    &numdropped);
        if (numdropped != module->dropped) {
            opal_output_verbose(30, mca_ptl_portals_component.portals_output,
                                "*** Dropped message count changed.  %lld, %lld",
                                module->dropped, numdropped);
            module->dropped = numdropped;
        }
#endif

        ret = PtlEQPoll(module->eq_handles,
                        MCA_PTL_PORTALS_EQ_SIZE, /* number of eq handles */
                        (int) tstamp,
                        &ev,
                        &which);
        if (PTL_EQ_EMPTY == ret) {
            /* nothing to see here - move along */
            continue;
        } else if (!(PTL_OK == ret || PTL_EQ_DROPPED == ret)) {
            /* BWB - how can we report errors? */
            opal_output(mca_ptl_portals_component.portals_output,
                        "Error calling PtlEQGet: %d", ret);
            continue;
        } else if (PTL_EQ_DROPPED == ret) {
            opal_output_verbose(10, mca_ptl_portals_component.portals_output,
                                "*** Event queue entries were dropped");
        }

#if PTL_PORTALS_HAVE_EVENT_UNLINK
        /* not everyone has UNLINK.  Use it only to print the event,
           so we can make sure we properly re-initialize the ones that
           need to be re-initialized */
        if (PTL_EVENT_UNLINK == ev.type) {
            OPAL_OUTPUT_VERBOSE((100, mca_ptl_portals_component.portals_output,
                                 "unlink event occurred"));
            continue;
        }
#endif

        if (ev.md.user_ptr == NULL) {
            /* no fragment associated with it - it's a receive */
            assert(which == MCA_PTL_PORTALS_EQ_RECV);
            mca_ptl_portals_process_recv_event(module, &ev);
        } else {
            /* there's a fragment associated with it - choose based on
               frag type */
            mca_ptl_base_frag_t *frag = 
                (mca_ptl_base_frag_t*) ev.md.user_ptr;
            if (frag->frag_type == MCA_PTL_FRAGMENT_SEND) {
                assert(which == MCA_PTL_PORTALS_EQ_SEND);
                mca_ptl_portals_process_send_event(&ev);
            } else {
                assert(which == MCA_PTL_PORTALS_EQ_RECV);
                mca_ptl_portals_process_recv_event(module, &ev);
            }
        }

        num_progressed++;
    }

    return num_progressed;
}

