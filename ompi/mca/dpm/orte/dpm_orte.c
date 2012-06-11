/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
 *                         University Research and Technology
 *                         Corporation.  All rights reserved.
 * Copyright (c) 2004-2011 The University of Tennessee and The University
 *                         of Tennessee Research Foundation.  All rights
 *                         reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart,
 *                         University of Stuttgart.  All rights reserved.
 * Copyright (c) 2004-2005 The Regents of the University of California.
 *                         All rights reserved.
 * Copyright (c) 2007-2011 Cisco Systems, Inc.  All rights reserved.
 * Copyright (c) 2006-2009 University of Houston.  All rights reserved.
 * Copyright (c) 2009      Sun Microsystems, Inc.  All rights reserved.
 * Copyright (c) 2011-2012 Los Alamos National Security, LLC.  All rights
 *                         reserved. 
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#include "ompi_config.h"
#include "ompi/constants.h"

#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "opal/util/argv.h"
#include "opal/util/opal_getcwd.h"

#include "opal/dss/dss.h"
#include "orte/mca/errmgr/errmgr.h"
#include "orte/mca/grpcomm/grpcomm.h"
#include "orte/mca/plm/plm.h"
#include "orte/mca/rml/rml.h"
#include "orte/mca/rml/rml_types.h"
#include "orte/mca/rmaps/rmaps.h"
#include "orte/mca/rmaps/rmaps_types.h"
#include "orte/mca/rmaps/base/base.h"
#include "orte/mca/rml/base/rml_contact.h"
#include "orte/mca/routed/routed.h"
#include "orte/util/name_fns.h"
#include "orte/runtime/orte_globals.h"
#include "orte/runtime/orte_wait.h"

#include "ompi/communicator/communicator.h"
#include "ompi/group/group.h"
#include "ompi/proc/proc.h"
#include "ompi/mca/pml/pml.h"
#include "ompi/info/info.h"

#include "ompi/mca/dpm/base/base.h"
#include "dpm_orte.h"

/* Local static variables */
static opal_mutex_t ompi_dpm_port_mutex;
static orte_rml_tag_t next_tag;
static bool waiting_for_recv = false;
static opal_buffer_t *cabuf=NULL;
static orte_process_name_t carport;

/* Local static functions */
static void recv_cb(int status, orte_process_name_t* sender,
                    opal_buffer_t *buffer,
                    orte_rml_tag_t tag, void *cbdata);

/* API functions */
static int init(void);
static int connect_accept ( ompi_communicator_t *comm, int root,
                            char *port_string, bool send_first,
                            ompi_communicator_t **newcomm );
static void disconnect(ompi_communicator_t *comm);
static int spawn(int count, char **array_of_commands,
                 char ***array_of_argv,
                 int *array_of_maxprocs,
                 MPI_Info *array_of_info,
                 char *port_name);
static int dyn_init(void);
static int open_port(char *port_name, orte_rml_tag_t given_tag);
static int parse_port_name(char *port_name, char **hnp_uri, char **rml_uri,
                           orte_rml_tag_t *tag);
static int route_to_port(char *rml_uri, orte_process_name_t *rproc);
static int close_port(char *port_name);
static int finalize(void);

/*
 * instantiate the module
 */
ompi_dpm_base_module_t ompi_dpm_orte_module = {
    init,
    connect_accept,
    disconnect,
    spawn,
    dyn_init,
    ompi_dpm_base_dyn_finalize,
    ompi_dpm_base_mark_dyncomm,
    open_port,
    parse_port_name,
    route_to_port, 
    close_port,
    finalize
};

static void rml_cbfunc(int status, orte_process_name_t* sender,
                       opal_buffer_t* buffer, orte_rml_tag_t tag,
                       void* cbdata)
{
    OBJ_RELEASE(buffer);
}


/*
 * Init the module
 */
static int init(void)
{    
    OBJ_CONSTRUCT(&ompi_dpm_port_mutex, opal_mutex_t);
    next_tag = OMPI_RML_TAG_DYNAMIC;

    return OMPI_SUCCESS;
}

static int connect_accept ( ompi_communicator_t *comm, int root,
                            char *port_string, bool send_first,
                            ompi_communicator_t **newcomm )
{
    int size, rsize, rank, rc;
    orte_std_cntr_t num_vals;
    orte_std_cntr_t rnamebuflen = 0;
    int rnamebuflen_int = 0;
    void *rnamebuf=NULL;

    ompi_communicator_t *newcomp=MPI_COMM_NULL;
    ompi_proc_t **rprocs=NULL;
    ompi_group_t *group=comm->c_local_group;
    orte_process_name_t port;
    orte_rml_tag_t tag=ORTE_RML_TAG_INVALID;
    opal_buffer_t *nbuf=NULL, *nrbuf=NULL;
    ompi_proc_t **proc_list=NULL, **new_proc_list;
    int i,j, new_proc_len;
    ompi_group_t *new_group_pointer;

    orte_grpcomm_coll_id_t id;
    orte_grpcomm_collective_t modex;
    opal_list_item_t *item;
    orte_namelist_t *nm;

    OPAL_OUTPUT_VERBOSE((1, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept with port %s %s",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         port_string, send_first ? "sending first" : "recv first"));
    
    /* set default error return */
    *newcomm = MPI_COMM_NULL;
    
    size = ompi_comm_size ( comm );
    rank = ompi_comm_rank ( comm );

    /* extract the process name from the port string, if given, and
     * set us up to communicate with it
     */
    if (NULL != port_string && 0 < strlen(port_string)) {
        char *hnp_uri, *rml_uri;

        /* separate the string into the HNP and RML URI and tag */
        if (ORTE_SUCCESS != (rc = parse_port_name(port_string, &hnp_uri, &rml_uri, &tag))) {
            ORTE_ERROR_LOG(rc);
            return rc;
        }
        /* extract the originating proc's name */
        if (ORTE_SUCCESS != (rc = orte_rml_base_parse_uris(rml_uri, &port, NULL))) {
            ORTE_ERROR_LOG(rc);
            free(hnp_uri); free(rml_uri);
            return rc;
        }
        /* make sure we can route rml messages to the destination job */
        if (ORTE_SUCCESS != (rc = route_to_port(hnp_uri, &port))) {
            ORTE_ERROR_LOG(rc);
            free(hnp_uri); free(rml_uri);
            return rc;
        }
        free(hnp_uri); free(rml_uri);
    }
    
    /* tell the progress engine to tick the event library more
       often, to make sure that the OOB messages get sent */
    opal_progress_event_users_increment();

    if ( rank == root ) {
        if (send_first) {
            /* Get a collective id for the modex we need later on - we
             * have to get a globally unique id for this purpose as
             * multiple threads can do simultaneous connect/accept, 
             * and the same processes can be engaged in multiple
             * connect/accepts at the same time. Only one side
             * needs to do this, so have it be send_first
             */
            nbuf = OBJ_NEW(opal_buffer_t);
            if (NULL == nbuf) {
                return OMPI_ERROR;
            }
            /* send the request - doesn't have to include any data */
            rc = orte_rml.send_buffer_nb(ORTE_PROC_MY_HNP, nbuf, ORTE_RML_TAG_COLL_ID_REQ, 0, rml_cbfunc, NULL);
            /* wait for the id */
            waiting_for_recv = true;
            cabuf = OBJ_NEW(opal_buffer_t);
            rc = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, ORTE_RML_TAG_COLL_ID,
                                         ORTE_RML_NON_PERSISTENT, recv_cb, NULL);
            /* wait for response */
            ORTE_WAIT_FOR_COMPLETION(waiting_for_recv);
            i=1;
            if (OPAL_SUCCESS != (rc = opal_dss.unpack(cabuf, &id, &i, ORTE_GRPCOMM_COLL_ID_T))) {
                ORTE_ERROR_LOG(rc);
                OBJ_RELEASE(cabuf);
                return OMPI_ERROR;
            }
            OBJ_RELEASE(cabuf);
            /* send it to my peer on the other side */
            nbuf = OBJ_NEW(opal_buffer_t);
            if (NULL == nbuf) {
                return OMPI_ERROR;
            }
            if (ORTE_SUCCESS != (rc = opal_dss.pack(nbuf, &id, 1, ORTE_GRPCOMM_COLL_ID_T))) {
                ORTE_ERROR_LOG(rc);
                goto exit;
            }
            rc = orte_rml.send_buffer_nb(&port, nbuf, tag, 0, rml_cbfunc, NULL);
        } else {
            /* wait to recv the collective id */
            waiting_for_recv = true;
            cabuf = OBJ_NEW(opal_buffer_t);
            rc = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, tag,
                                         ORTE_RML_NON_PERSISTENT, recv_cb, NULL);
            /* wait for response */
            ORTE_WAIT_FOR_COMPLETION(waiting_for_recv);
            i=1;
            if (OPAL_SUCCESS != (rc = opal_dss.unpack(cabuf, &id, &i, ORTE_GRPCOMM_COLL_ID_T))) {
                ORTE_ERROR_LOG(rc);
                OBJ_RELEASE(cabuf);
                return OMPI_ERROR;
            }
            OBJ_RELEASE(cabuf);
        }

        /* Generate the message buffer containing the number of processes and the list of
           participating processes */
        nbuf = OBJ_NEW(opal_buffer_t);
        if (NULL == nbuf) {
            return OMPI_ERROR;
        }
        
        /* pass the collective id so we can all use it */
        if (ORTE_SUCCESS != (rc = opal_dss.pack(nbuf, &id, 1, ORTE_GRPCOMM_COLL_ID_T))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }
        
        if (OPAL_SUCCESS != (rc = opal_dss.pack(nbuf, &size, 1, OPAL_INT))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }
        
        if(OMPI_GROUP_IS_DENSE(group)) {
            ompi_proc_pack(group->grp_proc_pointers, size, nbuf);
        } else {
            proc_list = (ompi_proc_t **) calloc (group->grp_proc_count, 
                                                 sizeof (ompi_proc_t *));
            for(i=0 ; i<group->grp_proc_count ; i++)
                proc_list[i] = ompi_group_peer_lookup(group,i);
            
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept adding %s to proc list",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 ORTE_NAME_PRINT(&proc_list[i]->proc_name)));
            ompi_proc_pack(proc_list, size, nbuf);
        }
        
        /* pack wireup info - this is required so that all involved parties can
         * discover how to talk to each other. For example, consider the case
         * where we connect_accept to one independent job (B), and then connect_accept
         * to another one (C) to wire all three of us together. Job B will not know
         * how to talk to job C at the OOB level because the two of them didn't
         * directly connect_accept to each other. Hence, we include the required
         * wireup info at this first exchange
         */
        if (ORTE_SUCCESS != (rc = orte_routed.get_wireup_info(nbuf))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }

        if (NULL != cabuf) {
            OBJ_RELEASE(cabuf);
        }
        
        cabuf = OBJ_NEW(opal_buffer_t);
        if (NULL == cabuf ) {
            rc = OMPI_ERROR;
            goto exit;
        }

        /* Exchange the number and the list of processes in the groups */
        if ( send_first ) {
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept sending first to %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 ORTE_NAME_PRINT(&port)));
            rc = orte_rml.send_buffer(&port, nbuf, tag, 0);
            /* setup to recv */
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept waiting for response",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
            waiting_for_recv = true;
            rc = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, tag,
                                         ORTE_RML_NON_PERSISTENT, recv_cb, NULL);
            /* wait for response */
            ORTE_WAIT_FOR_COMPLETION(waiting_for_recv);
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept got data from %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 ORTE_NAME_PRINT(&carport)));
            
        } else {
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept recving first",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
            /* setup to recv */
            waiting_for_recv = true;
            rc = orte_rml.recv_buffer_nb(ORTE_NAME_WILDCARD, tag,
                                         ORTE_RML_NON_PERSISTENT, recv_cb, NULL);
            /* wait for response */
            ORTE_WAIT_FOR_COMPLETION(waiting_for_recv);
            /* now send our info */
            OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                 "%s dpm:orte:connect_accept sending info to %s",
                                 ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                 ORTE_NAME_PRINT(&carport)));
            rc = orte_rml.send_buffer(&carport, nbuf, tag, 0);
        }

        if (OPAL_SUCCESS != (rc = opal_dss.unload(cabuf, &rnamebuf, &rnamebuflen))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }
    }

    /* First convert the size_t to an int so we can cast in the bcast to a void *
     * if we don't then we will get badness when using big vs little endian
     * THIS IS NO LONGER REQUIRED AS THE LENGTH IS NOW A STD_CNTR_T, WHICH
     * CORRELATES TO AN INT32
     */
    rnamebuflen_int = (int)rnamebuflen;

    /* bcast the buffer-length to all processes in the local comm */
    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept bcast buffer length",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    rc = comm->c_coll.coll_bcast (&rnamebuflen_int, 1, MPI_INT, root, comm,
                                  comm->c_coll.coll_bcast_module);
    if ( OMPI_SUCCESS != rc ) {
        goto exit;
    }
    rnamebuflen = rnamebuflen_int;

    if ( rank != root ) {
        /* non root processes need to allocate the buffer manually */
        rnamebuf = (char *) malloc(rnamebuflen);
        if ( NULL == rnamebuf ) {
            rc = OMPI_ERR_OUT_OF_RESOURCE;
            goto exit;
        }
    }

    /* bcast list of processes to all procs in local group
       and reconstruct the data. Note that proc_get_proclist
       adds processes, which were not known yet to our
       process pool.
    */
    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept bcast proc list",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    rc = comm->c_coll.coll_bcast (rnamebuf, rnamebuflen_int, MPI_BYTE, root, comm,
                                  comm->c_coll.coll_bcast_module);
    if ( OMPI_SUCCESS != rc ) {
        goto exit;
    }

    nrbuf = OBJ_NEW(opal_buffer_t);
    if (NULL == nrbuf) {
        goto exit;
    }
    if ( OPAL_SUCCESS != ( rc = opal_dss.load(nrbuf, rnamebuf, rnamebuflen))) {
        ORTE_ERROR_LOG(rc);
        goto exit;
    }

    /* unload the collective id */
    num_vals = 1;
    if (ORTE_SUCCESS != (rc = opal_dss.unpack(nrbuf, &id, &num_vals, ORTE_GRPCOMM_COLL_ID_T))) {
        ORTE_ERROR_LOG(rc);
        goto exit;
    }

    num_vals = 1;
    if (OPAL_SUCCESS != (rc = opal_dss.unpack(nrbuf, &rsize, &num_vals, OPAL_INT))) {
        ORTE_ERROR_LOG(rc);
        goto exit;
    }

    rc = ompi_proc_unpack(nrbuf, rsize, &rprocs, &new_proc_len, &new_proc_list);
    if ( OMPI_SUCCESS != rc ) {
        goto exit;
    }

    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept unpacked %d new procs",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), new_proc_len));
    
    /* If we added new procs, we need to do the modex and then call
       PML add_procs */
    if (new_proc_len > 0) {
        opal_list_t all_procs;
        orte_namelist_t *name;

        /* we first need to give the wireup info to our routed module.
         * Not every routed module will need it, but some do require
         * this info before we can do any comm
         */
        if (ORTE_SUCCESS != (rc = orte_routed.init_routes(rprocs[0]->proc_name.jobid, nrbuf))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }

        OBJ_CONSTRUCT(&all_procs, opal_list_t);

        if (send_first) {
            for (i = 0 ; i < rsize ; ++i) {
                name = OBJ_NEW(orte_namelist_t);
                name->name = rprocs[i]->proc_name;
                opal_list_append(&all_procs, &name->super);
                OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                     "%s dpm:orte:connect_accept send first adding %s to allgather list",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     ORTE_NAME_PRINT(&name->name)));
            }
            for (i = 0 ; i < group->grp_proc_count ; ++i) {
                name = OBJ_NEW(orte_namelist_t);
                name->name = ompi_group_peer_lookup(group, i)->proc_name;
                opal_list_append(&all_procs, &name->super);
                OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                     "%s dpm:orte:connect_accept send first adding %s to allgather list",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     ORTE_NAME_PRINT(&name->name)));
            }

        } else {
            for (i = 0 ; i < group->grp_proc_count ; ++i) {
                name = OBJ_NEW(orte_namelist_t);
                name->name = ompi_group_peer_lookup(group, i)->proc_name;
                opal_list_append(&all_procs, &name->super);
                OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                     "%s dpm:orte:connect_accept recv first adding %s to allgather list",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     ORTE_NAME_PRINT(&name->name)));
            }
            for (i = 0 ; i < rsize ; ++i) {
                name = OBJ_NEW(orte_namelist_t);
                name->name = rprocs[i]->proc_name;
                opal_list_append(&all_procs, &name->super);
                OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                                     "%s dpm:orte:connect_accept recv first adding %s to allgather list",
                                     ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                                     ORTE_NAME_PRINT(&name->name)));
            }

        }

        OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                             "%s dpm:orte:connect_accept executing modex",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

        /* setup the modex */
        OBJ_CONSTRUCT(&modex, orte_grpcomm_collective_t);
        modex.id = id;
        /* copy across the list of participants */
        for (item = opal_list_get_first(&all_procs);
             item != opal_list_get_end(&all_procs);
             item = opal_list_get_next(item)) {
            nm = (orte_namelist_t*)item;
            name = OBJ_NEW(orte_namelist_t);
            name->name = nm->name;
            opal_list_append(&modex.participants, &name->super);
        }

        /* perform it */
        if (OMPI_SUCCESS != (rc = orte_grpcomm.modex(&modex))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }
        while (modex.active) {
            opal_progress();
        }
        OBJ_DESTRUCT(&modex);

        OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                             "%s dpm:orte:connect_accept modex complete",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

        /*
          while (NULL != (item = opal_list_remove_first(&all_procs))) {
          OBJ_RELEASE(item);
          }
          OBJ_DESTRUCT(&all_procs);
        */

        OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                             "%s dpm:orte:connect_accept adding procs",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

        if (OMPI_SUCCESS != (rc = MCA_PML_CALL(add_procs(new_proc_list, new_proc_len)))) {
            ORTE_ERROR_LOG(rc);
            goto exit;
        }

        OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                             "%s dpm:orte:connect_accept new procs added",
                             ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));
    }

    OBJ_RELEASE(nrbuf);
    if ( rank == root ) {
        OBJ_RELEASE(nbuf);
    }

    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept allocating group size %d",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME), rsize));

    new_group_pointer=ompi_group_allocate(rsize);
    if( NULL == new_group_pointer ) {
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto exit;
    }

    /* put group elements in the list */
    for (j = 0; j < rsize; j++) {
        new_group_pointer->grp_proc_pointers[j] = rprocs[j]; 
    }                           /* end proc loop */

    /* increment proc reference counters */
    ompi_group_increment_proc_count(new_group_pointer);
    
    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept setting up communicator",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    /* set up communicator structure */
    rc = ompi_comm_set ( &newcomp,                 /* new comm */
                         comm,                     /* old comm */
                         group->grp_proc_count,    /* local_size */
                         NULL,                     /* local_procs */
                         rsize,                    /* remote_size */
                         NULL  ,                   /* remote_procs */
                         NULL,                     /* attrs */
                         comm->error_handler,      /* error handler */
                         NULL,                     /* topo component */
                         group,                    /* local group */
                         new_group_pointer         /* remote group */
                         );
    if ( NULL == newcomp ) {
        rc = OMPI_ERR_OUT_OF_RESOURCE;
        goto exit;
    }

    ompi_group_decrement_proc_count (new_group_pointer);
    OBJ_RELEASE(new_group_pointer);
    new_group_pointer = MPI_GROUP_NULL;

    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept allocate comm_cid",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    /* allocate comm_cid */
    rc = ompi_comm_nextcid ( newcomp,                 /* new communicator */
                             comm,                    /* old communicator */
                             NULL,                    /* bridge comm */
                             &root,                   /* local leader */
                             &carport,                /* remote leader */
                             OMPI_COMM_CID_INTRA_OOB, /* mode */
                             send_first );            /* send or recv first */
    if ( OMPI_SUCCESS != rc ) {
        goto exit;
    }

    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept activate comm",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    /* activate comm and init coll-component */
    rc = ompi_comm_activate ( &newcomp,               /* new communicator */
                              comm,                    /* old communicator */
                              NULL,                    /* bridge comm */
                              &root,                   /* local leader */
                              &carport,                /* remote leader */
                              OMPI_COMM_CID_INTRA_OOB, /* mode */
                              send_first );            /* send or recv first */
    if ( OMPI_SUCCESS != rc ) {
        goto exit;
    }

    /* Question: do we have to re-start some low level stuff
       to enable the usage of fast communication devices
       between the two worlds ?
    */

 exit:
    /* done with OOB and such - slow our tick rate again */
    opal_progress();
    opal_progress_event_users_decrement();

    if ( NULL != rprocs ) {
        free ( rprocs );
    }
    if ( NULL != proc_list ) {
        free ( proc_list );
    }
    if ( OMPI_SUCCESS != rc ) {
        if ( MPI_COMM_NULL != newcomp && NULL != newcomp ) {
            OBJ_RETAIN(newcomp);
            newcomp = MPI_COMM_NULL;
        }
    }

    *newcomm = newcomp;
    OPAL_OUTPUT_VERBOSE((3, ompi_dpm_base_output,
                         "%s dpm:orte:connect_accept complete",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME)));

    return rc;
}

static void disconnect(ompi_communicator_t *comm)
{
    ompi_dpm_base_disconnect_obj *dobj;
    
    dobj = ompi_dpm_base_disconnect_init (comm);
    ompi_dpm_base_disconnect_waitall(1, &dobj);
    
}

static int spawn(int count, char **array_of_commands,
                 char ***array_of_argv,
                 int *array_of_maxprocs,
                 MPI_Info *array_of_info,
                 char *port_name)
{
    int rc, i, j, counter;
    int have_wdir=0;
    int flag=0;
    char cwd[OPAL_PATH_MAX];
    char host[OPAL_PATH_MAX];  /*** should define OMPI_HOST_MAX ***/
    char prefix[OPAL_PATH_MAX];
    char stdin_target[OPAL_PATH_MAX];
    char params[OPAL_PATH_MAX];
    char mapper[OPAL_PATH_MAX];
    int npernode;
    char slot_list[OPAL_PATH_MAX];

    orte_job_t *jdata;
    orte_app_context_t *app;
    bool local_spawn, non_mpi;
    bool local_bynode = false;
    
    /* parse the info object */
    /* check potentially for:
       - "host": desired host where to spawn the processes
       - "hostfile": hostfile containing hosts where procs are
       to be spawned
       - "add-host": add the specified hosts to the known list
       of available resources and spawn these
       procs on them
       - "add-hostfile": add the hosts in the hostfile to the
       known list of available resources and spawn
       these procs on them
       - "prefix": the path to the root of the directory tree where ompi
       executables and libraries can be found on all nodes
       used to spawn these procs
       - "arch": desired architecture
       - "wdir": directory, where executable can be found
       - "path": list of directories where to look for the executable
       - "file": filename, where additional information is provided.
       - "soft": see page 92 of MPI-2.
    */

    /* make sure the progress engine properly trips the event library */
    opal_progress_event_users_increment();
    
    /* setup the job object */
    jdata = OBJ_NEW(orte_job_t);
    
    /* Convert the list of commands to an array of orte_app_context_t
       pointers */
    for (i = 0; i < count; ++i) {
        app = OBJ_NEW(orte_app_context_t);
        if (NULL == app) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_RELEASE(jdata);
            opal_progress_event_users_decrement();
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        /* add the app to the job data */
        opal_pointer_array_add(jdata->apps, app);
        app->idx = i;
        jdata->num_apps++;
        
        /* copy over the name of the executable */
        app->app = strdup(array_of_commands[i]);
        if (NULL == app->app) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_RELEASE(jdata);
            opal_progress_event_users_decrement();
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        /* record the number of procs to be generated */
        app->num_procs = array_of_maxprocs[i];
        jdata->num_procs += app->num_procs;
        
        /* copy over the argv array */
        counter = 1;

        if (MPI_ARGVS_NULL != array_of_argv &&
            MPI_ARGV_NULL != array_of_argv[i]) {
            /* first need to find out how many entries there are */
            j=0;
            while (NULL != array_of_argv[i][j]) {
                j++;
            }
            counter += j;
        }

        /* now copy them over, ensuring to NULL terminate the array */
        app->argv = (char**)malloc((1 + counter) * sizeof(char*));
        if (NULL == app->argv) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_RELEASE(jdata);
            opal_progress_event_users_decrement();
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        app->argv[0] = strdup(array_of_commands[i]);
        for (j=1; j < counter; j++) {
            app->argv[j] = strdup(array_of_argv[i][j-1]);
        }
        app->argv[counter] = NULL;


        /* the environment gets set by the launcher
         * all we need to do is add the specific values
         * needed for comm_spawn
         */
        /* Add environment variable with the contact information for the
           child processes.
        */
        app->env = (char**)malloc(2 * sizeof(char*));
        if (NULL == app->env) {
            ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
            OBJ_RELEASE(jdata);
            opal_progress_event_users_decrement();
            return ORTE_ERR_OUT_OF_RESOURCE;
        }
        asprintf(&(app->env[0]), "OMPI_PARENT_PORT=%s", port_name);
        app->env[1] = NULL;
        for (j = 0; NULL != environ[j]; ++j) {
            if (0 == strncmp("OMPI_", environ[j], 5)) {
                opal_argv_append_nosize(&app->env, environ[j]);
            }
        }

        /* Check for well-known info keys */
        have_wdir = 0;
        if ( array_of_info != NULL && array_of_info[i] != MPI_INFO_NULL ) {

            /* check for 'host' */
            ompi_info_get (array_of_info[i], "host", sizeof(host) - 1, host, &flag);
            if ( flag ) {
                app->dash_host = opal_argv_split(host, ',');
            }
 
            /* check for 'hostfile' */
            ompi_info_get (array_of_info[i], "hostfile", sizeof(host) - 1, host, &flag);
            if ( flag ) {
                app->hostfile = strdup(host);
            }
            
            /* check for 'add-hostfile' */
            ompi_info_get (array_of_info[i], "add-hostfile", sizeof(host) - 1, host, &flag);
            if ( flag ) {
                app->add_hostfile = strdup(host);
            }
            
            /* check for 'add-host' */
            ompi_info_get (array_of_info[i], "add-host", sizeof(host) - 1, host, &flag);
            if ( flag ) {
                app->add_host = opal_argv_split(host, ',');
            }
            
            /* 'path', 'arch', 'file', 'soft'  -- to be implemented */ 
            
            /* check for 'ompi_prefix' (OMPI-specific -- to effect the same
             * behavior as --prefix option to orterun)
             */
            ompi_info_get (array_of_info[i], "ompi_prefix", sizeof(prefix) - 1, prefix, &flag);
            if ( flag ) {
                app->prefix_dir = strdup(prefix);
            }

            /* check for 'wdir' */ 
            ompi_info_get (array_of_info[i], "wdir", sizeof(cwd) - 1, cwd, &flag);
            if ( flag ) {
                app->cwd = strdup(cwd);
                have_wdir = 1;
            }
            
            /* check for 'mapper' */ 
            ompi_info_get(array_of_info[i], "mapper", sizeof(mapper) - 1, mapper, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                jdata->map->req_mapper = strdup(mapper);
            }

            /* check for 'display_map' */ 
            ompi_info_get_bool(array_of_info[i], "display_map", &local_spawn, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                jdata->map->display_map = true;
            }

            /* check for 'npernode' and 'ppr' */ 
            ompi_info_get (array_of_info[i], "npernode", sizeof(slot_list) - 1, slot_list, &flag);
            if ( flag ) {
                if (ORTE_SUCCESS != ompi_info_value_to_int(slot_list, &npernode)) {
                    ORTE_ERROR_LOG(ORTE_ERR_BAD_PARAM);
                    return ORTE_ERR_BAD_PARAM;
                }
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_PPR;
                asprintf(&(jdata->map->ppr), "%d:n", npernode);
            }
            ompi_info_get (array_of_info[i], "pernode", sizeof(slot_list) - 1, slot_list, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_PPR;
                jdata->map->ppr = strdup("1:n");
            }
            ompi_info_get (array_of_info[i], "ppr", sizeof(slot_list) - 1, slot_list, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_PPR;
                jdata->map->ppr = strdup(slot_list);
            }

            /* check for 'map_byxxx' */
            ompi_info_get_bool(array_of_info[i], "map_by_node", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYNODE;
            }
#if OPAL_HAVE_HWLOC
            ompi_info_get_bool(array_of_info[i], "map_by_board", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYBOARD;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_numa", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYNUMA;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_socket", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYSOCKET;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_l3cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYL3CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_l2cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYL2CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_l1cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYL1CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_core", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYCORE;
            }
            ompi_info_get_bool(array_of_info[i], "map_by_hwthread", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (ORTE_MAPPING_POLICY_IS_SET(jdata->map->mapping)) {
                    return OMPI_ERROR;
                }
                jdata->map->mapping |= ORTE_MAPPING_BYHWTHREAD;
            }
#endif

            /* check for 'rank_byxxx' */
            ompi_info_get_bool(array_of_info[i], "rank_by_node", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_NODE;
            }
#if OPAL_HAVE_HWLOC
            ompi_info_get_bool(array_of_info[i], "rank_by_board", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_BOARD;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_numa", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_NUMA;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_socket", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_SOCKET;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_l3cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_L3CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_l2cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_L2CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_l1cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_L1CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_core", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_CORE;
            }
            ompi_info_get_bool(array_of_info[i], "rank_by_hwthread", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (0 != jdata->map->ranking) {
                    return OMPI_ERROR;
                }
                jdata->map->ranking = ORTE_RANK_BY_HWTHREAD;
            }

            /* check for 'bind_toxxx' */
            ompi_info_get_bool(array_of_info[i], "bind_if_supported", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                jdata->map->binding |= OPAL_BIND_IF_SUPPORTED;
            }
            ompi_info_get_bool(array_of_info[i], "bind_overload_allowed", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                jdata->map->binding |= OPAL_BIND_ALLOW_OVERLOAD;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_none", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_NONE;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_board", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_BOARD;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_numa", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_NUMA;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_socket", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_SOCKET;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_l3cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_L3CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_l2cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_L2CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_l1cache", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_L1CACHE;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_core", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_CORE;
            }
            ompi_info_get_bool(array_of_info[i], "bind_to_hwthread", &local_bynode, &flag);
            if ( flag ) {
                if (NULL == jdata->map) {
                    jdata->map = OBJ_NEW(orte_job_map_t);
                    if (NULL == jdata->map) {
                        ORTE_ERROR_LOG(ORTE_ERR_OUT_OF_RESOURCE);
                        return ORTE_ERR_OUT_OF_RESOURCE;
                    }
                }
                if (OPAL_BINDING_POLICY_IS_SET(jdata->map->binding)) {
                    return OMPI_ERROR;
                }
                jdata->map->binding |= OPAL_BIND_TO_HWTHREAD;
            }
#endif

            /* check for 'preload_binary' */
            ompi_info_get_bool(array_of_info[i], "ompi_preload_binary", &local_spawn, &flag);
            if ( flag ) {
                app->preload_binary = true;
            }
            
            /* check for 'preload_libraries' */
            ompi_info_get_bool(array_of_info[i], "ompi_preload_libraries", &local_spawn, &flag);
            if ( flag ) {
                app->preload_libs = true;
            }
            
            /* check for 'preload_files' */ 
            ompi_info_get (array_of_info[i], "ompi_preload_files", sizeof(cwd) - 1, cwd, &flag);
            if ( flag ) {
                app->preload_files = strdup(cwd);
            }
            
            /* check for 'preload_files_dest_dir' */ 
            ompi_info_get (array_of_info[i], "ompi_preload_files_dest_dir", sizeof(cwd) - 1, cwd, &flag);
            if ( flag ) {
                app->preload_files_dest_dir = strdup(cwd);
            }
            
            /* check for 'preload_files_src_dir' */ 
            ompi_info_get (array_of_info[i], "ompi_preload_files_src_dir", sizeof(cwd) - 1, cwd, &flag);
            if ( flag ) {
                app->preload_files_src_dir = strdup(cwd);
            }
            
            /* see if this is a non-mpi job - if so, then set the flag so ORTE
             * knows what to do
             */
            ompi_info_get_bool(array_of_info[i], "ompi_non_mpi", &non_mpi, &flag);
            if (flag && non_mpi) {
                jdata->controls |= ORTE_JOB_CONTROL_NON_ORTE_JOB;
            }
            
            /* see if this is an MCA param that the user wants applied to the child job */
            ompi_info_get (array_of_info[i], "ompi_param", sizeof(params) - 1, params, &flag);
            if ( flag ) {
                opal_argv_append_unique_nosize(&app->env, params, true);
            }
            
            /* see if user specified what to do with stdin - defaults to
             * not forwarding stdin to child processes
             */
            ompi_info_get (array_of_info[i], "ompi_stdin_target", sizeof(stdin_target) - 1, stdin_target, &flag);
            if ( flag ) {
                if (0 == strcmp(stdin_target, "all")) {
                    jdata->stdin_target = ORTE_VPID_WILDCARD;
                } else if (0 == strcmp(stdin_target, "none")) {
                    jdata->stdin_target = ORTE_VPID_INVALID;
                } else {
                    jdata->stdin_target = strtoul(stdin_target, NULL, 10);
                }
            }
        }

        /* default value: If the user did not tell us where to look for the
         * executable, we assume the current working directory, or the preload destination
         * if it was given
         */
        if ( !have_wdir ) {
            if (NULL != app->preload_files_dest_dir) {
                app->cwd = strdup(app->preload_files_dest_dir);
            } else {
                if (OMPI_SUCCESS != (rc = opal_getcwd(cwd, OPAL_PATH_MAX))) {
                    ORTE_ERROR_LOG(rc);
                    OBJ_RELEASE(jdata);
                    opal_progress_event_users_decrement();
                    return rc;
                }
                app->cwd = strdup(cwd);
            }
        }
        
        /* leave the map info alone - the launcher will
         * decide where to put things
         */
    } /* for (i = 0 ; i < count ; ++i) */

    /* spawn procs */
    rc = orte_plm.spawn(jdata);
    OBJ_RELEASE(jdata);

    if (ORTE_SUCCESS != rc) {
        ORTE_ERROR_LOG(rc);
        opal_progress_event_users_decrement();
        return MPI_ERR_SPAWN;
    }

    /* clean up */
    opal_progress_event_users_decrement();

    return OMPI_SUCCESS;
}

/*
 * The port_name is constructed to support the ability
 * to route messages between different jobs. Messages
 * between job families are routed via their respective HNPs
 * to reduce connection count and to support connect/accept.
 * Thus, the port_name consists of three fields:
 * (a) the contact info of the process opening the port. This
 *     is provided in case the routed module wants to communicate
 *     directly between the procs.
 * (b) the tag of the port. The reason for adding the tag is
 *     to make the port unique for multi-threaded scenarios.
 * (c) the contact info for the job's HNP. This will be
 *     used to route messages between job families
 *
 * Construction of the port name is done here - as opposed to
 * in the routed module itself - because two mpiruns using different
 * routed modules could exchange the port name (via pubsub). The
 * format of the port name must, therefore, be universal.
 *
 * Optionally can provide a tag to be used - otherwise, we supply the
 * next dynamically assigned tag
 */
static int open_port(char *port_name, orte_rml_tag_t given_tag)
{
    char *rml_uri=NULL;
    int rc, len;
    char tag[12];
    
    OPAL_THREAD_LOCK(&ompi_dpm_port_mutex);

    if (NULL == orte_process_info.my_hnp_uri) {
        rc = OMPI_ERR_NOT_AVAILABLE;
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }
    
    if (NULL == (rml_uri = orte_rml.get_contact_info())) {
        rc = OMPI_ERROR;
        ORTE_ERROR_LOG(rc);
        goto cleanup;
    }
    
    if (ORTE_RML_TAG_INVALID == given_tag) {
        snprintf(tag, 12, "%d", next_tag);
        next_tag++;
    } else {
        snprintf(tag, 12, "%d", given_tag);
    }
    
    
    len = strlen(orte_process_info.my_hnp_uri) + strlen(rml_uri) + strlen(tag);
    
    /* if the overall port name is too long, we abort */
    if (len > (MPI_MAX_PORT_NAME-1)) {
        rc = OMPI_ERR_VALUE_OUT_OF_BOUNDS;
        goto cleanup;
    }
    
    /* assemble the port name */
    snprintf(port_name, MPI_MAX_PORT_NAME, "%s+%s:%s", orte_process_info.my_hnp_uri, rml_uri, tag);
    rc = OMPI_SUCCESS;

cleanup:
    if (NULL != rml_uri) {
        free(rml_uri);
    }
    
    OPAL_THREAD_UNLOCK(&ompi_dpm_port_mutex);
    return rc;
}


static int route_to_port(char *rml_uri, orte_process_name_t *rproc)
{    
    opal_buffer_t route;
    int rc;
    
    /* We need to ask the routed module to init_routes so it can do the
     * right thing. In most cases, it will route any messages to the
     * proc through our HNP - however, this is NOT the case in all
     * circumstances, so we need to let the routed module decide what
     * to do.
     */
    /* pack a cmd so the buffer can be unpacked correctly */
    OBJ_CONSTRUCT(&route, opal_buffer_t);
    
    /* pack the provided uri */
    opal_dss.pack(&route, &rml_uri, 1, OPAL_STRING);
    
    /* init the route */
    if (ORTE_SUCCESS != (rc = orte_routed.init_routes(rproc->jobid, &route))) {
        ORTE_ERROR_LOG(rc);
    }
    OBJ_DESTRUCT(&route);
    
    /* nothing more to do here */
    return rc;
}

static int parse_port_name(char *port_name,
                           char **hnp_uri, 
                           char **rml_uri,
                           orte_rml_tag_t *ptag)
{
    char *tmpstring=NULL, *ptr;
    int tag;
    int rc;
    
    /* don't mangle the port name */
    tmpstring = strdup(port_name);
    
    /* find the ':' demarking the RML tag we added to the end */
    if (NULL == (ptr = strrchr(tmpstring, ':'))) {
        rc = OMPI_ERR_NOT_FOUND;
        goto cleanup;
    }
    
    /* terminate the port_name at that location */
    *ptr = '\0';
    ptr++;
    
    /* convert the RML tag */
    sscanf(ptr,"%d", &tag);
    
    /* now split out the second field - the uri of the remote proc */
    if (NULL == (ptr = strchr(tmpstring, '+'))) {
        rc = OMPI_ERR_NOT_FOUND;
        goto cleanup;
    }
    *ptr = '\0';
    ptr++;
    
    /* save that info */
    if(NULL != hnp_uri) *hnp_uri = tmpstring;
    else free(tmpstring);
    if(NULL != rml_uri) *rml_uri = strdup(ptr);
    if(NULL != ptag) *ptag = tag;
    
    return OMPI_SUCCESS;
    
cleanup:
    /* release the tmp storage */
    if (NULL != tmpstring) {
        free(tmpstring);
    }
    return rc;
}

static int close_port(char *port_name)
{
    /* nothing to do here - user is responsible for the memory */
    return OMPI_SUCCESS;
}

static int dyn_init(void)
{
    char *port_name=NULL;
    int root=0, rc;
    bool send_first = true;
    ompi_communicator_t *newcomm=NULL;
    
    /* if env-variable is set, we are a dynamically spawned
        * child - parse port and call comm_connect_accept */
    if (NULL == (port_name = ompi_dpm_base_dyn_init())) {
        /* nothing to do */
        return OMPI_SUCCESS;
    }
    
    OPAL_OUTPUT_VERBOSE((1, ompi_dpm_base_output,
                         "%s dpm:orte:dyn_init with port %s",
                         ORTE_NAME_PRINT(ORTE_PROC_MY_NAME),
                         port_name));
    
    rc = connect_accept (MPI_COMM_WORLD, root, port_name, send_first, &newcomm);
    if (OMPI_SUCCESS != rc) {
        return rc;
    }
    
    /* originally, we set comm_parent to comm_null (in comm_init),
     * now we have to decrease the reference counters to the according
     * objects
     */
    OBJ_RELEASE(ompi_mpi_comm_parent->c_local_group);
    OBJ_RELEASE(ompi_mpi_comm_parent->error_handler);
    OBJ_RELEASE(ompi_mpi_comm_parent);

    /* Set the parent communicator */
    ompi_mpi_comm_parent = newcomm;

    /* Set name for debugging purposes */
    snprintf(newcomm->c_name, MPI_MAX_OBJECT_NAME, "MPI_COMM_PARENT");
    newcomm->c_flags |= OMPI_COMM_NAMEISSET;
    
    return OMPI_SUCCESS;
}


/*
 * finalize the module
 */
static int finalize(void)
{
    OBJ_DESTRUCT(&ompi_dpm_port_mutex);
    return OMPI_SUCCESS;
}


static void recv_cb(int status, orte_process_name_t* sender,
                    opal_buffer_t *buffer,
                    orte_rml_tag_t tag, void *cbdata)
{
    /* copy the payload to the global buffer */
    opal_dss.copy_payload(cabuf, buffer);
    
    /* flag the identity of the remote proc */
    carport.jobid = sender->jobid;
    carport.vpid = sender->vpid;
    
    /* flag complete */
    waiting_for_recv = false;
}

