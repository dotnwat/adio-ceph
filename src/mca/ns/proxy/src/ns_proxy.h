/* -*- C -*-
 * 
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
 *
 */
#ifndef NS_PROXY_H
#define NS_PROXY_H


#include "orte_config.h"
#include "include/types.h"
#include "include/orte_constants.h"
#include "class/ompi_list.h"
#include "dps/dps.h"

#include "mca/ns/base/base.h"

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

struct orte_ns_proxy_cell_info_t {
    ompi_list_item_t item;  /**< Allows this item to be placed on a list */
    orte_cellid_t cellid;
    char *site;
    char *resource;
};
typedef struct orte_ns_proxy_cell_info_t orte_ns_proxy_cell_info_t;

OBJ_CLASS_DECLARATION(orte_ns_proxy_cell_info_t);

struct orte_ns_proxy_tagitem_t {
    ompi_list_item_t item;  /**< Allows this item to be placed on a list */
    orte_rml_tag_t tag;  /**< OOB tag */
    char *name;      /**< Name associated with tag */
};
typedef struct orte_ns_proxy_tagitem_t orte_ns_proxy_tagitem_t;

OBJ_CLASS_DECLARATION(orte_ns_proxy_tagitem_t);

struct orte_ns_proxy_dti_t {
    ompi_list_item_t item;  /**< Allows this item to be placed on a list */
    orte_data_type_t id;  /**< data type id */
    char *name;      /**< Name associated with data type */
};
typedef struct orte_ns_proxy_dti_t orte_ns_proxy_dti_t;

OBJ_CLASS_DECLARATION(orte_ns_proxy_dti_t);


/*
 * Module open / close
 */
int orte_ns_proxy_open(void);
int orte_ns_proxy_close(void);


/*
 * Startup / Shutdown
 */
mca_ns_base_module_t* orte_ns_proxy_init(int *priority);
int orte_ns_proxy_module_init(void);
int orte_ns_proxy_finalize(void);

/*
 * globals used within proxy component
 */

extern orte_process_name_t *orte_ns_my_replica;
extern int orte_ns_proxy_debug;
extern ompi_list_t orte_ns_proxy_cell_info_list;
extern ompi_list_t orte_ns_proxy_taglist;
extern ompi_list_t orte_ns_proxy_dtlist;
extern ompi_mutex_t orte_ns_proxy_mutex;

/*
 * proxy function prototypes
 */
int orte_ns_proxy_create_cellid(orte_cellid_t *cellid, char *site, char *resource);

int orte_ns_proxy_get_cell_info(orte_cellid_t cellid, char **site, char **resource);

int orte_ns_proxy_create_jobid(orte_jobid_t *jobid);

int orte_ns_proxy_reserve_range(orte_jobid_t job, orte_vpid_t range,
                                orte_vpid_t *startvpid);

int orte_ns_proxy_assign_rml_tag(orte_rml_tag_t *tag, char *name);

int orte_ns_proxy_define_data_type(const char *name,
                                   orte_data_type_t *type);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif
