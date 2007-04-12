/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University and Indiana
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

#include "orte_config.h"
#include "orte/util/proc_info.h"
#include "opal/util/output.h"
#include "opal/mca/base/base.h"
#include "opal/mca/base/mca_base_param.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/rml/rml_types.h"
#include "iof_svc.h"
#include "iof_svc_proxy.h"
#include "iof_svc_pub.h"
#include "iof_svc_sub.h"

/*
 * Local functions
 */
static int orte_iof_svc_open(void);
static int orte_iof_svc_close(void);

static orte_iof_base_module_t* orte_iof_svc_init(
    int* priority, 
    bool *allow_multi_user_threads,
    bool *have_hidden_threads);

/*
 * Local variables
 */
static bool initialized = false;


orte_iof_svc_component_t mca_iof_svc_component = {
    {
      /* First, the mca_base_component_t struct containing meta
         information about the component itself */

      {
        /* Indicate that we are a iof v1.0.0 component (which also
           implies a specific MCA version) */

        ORTE_IOF_BASE_VERSION_1_0_0,

        "svc", /* MCA component name */
        ORTE_MAJOR_VERSION,  /* MCA component major version */
        ORTE_MINOR_VERSION,  /* MCA component minor version */
        ORTE_RELEASE_VERSION,  /* MCA component release version */
        orte_iof_svc_open,  /* component open  */
        orte_iof_svc_close  /* component close */
      },

      /* Next the MCA v1.0.0 component meta data */
      {
          /* The component is checkpoint ready */
          MCA_BASE_METADATA_PARAM_CHECKPOINT
      },

      orte_iof_svc_init
    }
};

static  int orte_iof_svc_param_register_int(
    const char* param_name,
    int default_value)
{
    int id = mca_base_param_register_int("iof","svc",param_name,NULL,default_value);
    int param_value = default_value;
    mca_base_param_lookup_int(id,&param_value);
    return param_value;
}


/**
  * component open/close/init function
  */
static int orte_iof_svc_open(void)
{
    mca_iof_svc_component.svc_debug = orte_iof_svc_param_register_int("debug", 1);
    return ORTE_SUCCESS;
}


static int orte_iof_svc_close(void)
{
    opal_list_item_t* item;

    if (initialized) {
        OPAL_THREAD_LOCK(&mca_iof_svc_component.svc_lock);
        while((item = opal_list_remove_first(&mca_iof_svc_component.svc_subscribed)) != NULL) {
            OBJ_RELEASE(item);
        }
        while((item = opal_list_remove_first(&mca_iof_svc_component.svc_published)) != NULL) {
            OBJ_RELEASE(item);
        }
        OPAL_THREAD_UNLOCK(&mca_iof_svc_component.svc_lock);
        orte_rml.recv_cancel(ORTE_NAME_WILDCARD, ORTE_RML_TAG_IOF_SVC);
    }

    return ORTE_SUCCESS;
}


/**
 * Callback when peer is disconnected
 */

static void
orte_iof_svc_exception_handler(const orte_process_name_t* peer, orte_rml_exception_t reason)
{
    orte_iof_svc_sub_delete_all(peer);
    orte_iof_svc_pub_delete_all(peer);
}


/**
 * Module Initialization
 */

static orte_iof_base_module_t* 
orte_iof_svc_init(int* priority, bool *allow_multi_user_threads, bool *have_hidden_threads)
{
    int rc;

    if (false == orte_process_info.seed) {
        return NULL;
    }

    *priority = 1;
    *allow_multi_user_threads = true;
    *have_hidden_threads = false;

    OBJ_CONSTRUCT(&mca_iof_svc_component.svc_subscribed, opal_list_t);
    OBJ_CONSTRUCT(&mca_iof_svc_component.svc_published, opal_list_t);
    OBJ_CONSTRUCT(&mca_iof_svc_component.svc_lock, opal_mutex_t);

    /* post non-blocking recv */
    mca_iof_svc_component.svc_iov[0].iov_base = NULL;
    mca_iof_svc_component.svc_iov[0].iov_len = 0;

    rc = orte_rml.recv_nb(
        ORTE_NAME_WILDCARD,
        mca_iof_svc_component.svc_iov,
        1,
        ORTE_RML_TAG_IOF_SVC,
        ORTE_RML_ALLOC|ORTE_RML_PERSISTENT,
        orte_iof_svc_proxy_recv,
        NULL
    );
    if(rc != ORTE_SUCCESS) {
        opal_output(0, "orte_iof_svc_init: unable to post non-blocking recv");
        return NULL;
    }

    rc = orte_rml.add_exception_handler(orte_iof_svc_exception_handler);
    initialized = true;
    return &orte_iof_svc_module;
}


int
orte_iof_svc_finalize(void)
{
    orte_rml.recv_cancel(ORTE_NAME_WILDCARD, ORTE_RML_TAG_IOF_SVC );
    orte_rml.del_exception_handler(orte_iof_svc_exception_handler);

    return ORTE_SUCCESS;
}

int orte_iof_svc_ft_event(int state) {
    /*
     * Replica doesn't need to do anything for a checkpoint
     */
    return ORTE_SUCCESS;
}
