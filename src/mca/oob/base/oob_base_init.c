/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>

#include "runtime/runtime.h"
#include "util/output.h"
#include "util/proc_info.h"
#include "util/argv.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pcmclient/pcmclient.h"
#include "mca/pcmclient/base/base.h"
#include "mca/ns/base/base.h"
#include "mca/pcm/pcm.h"
#include "mca/oob/oob.h"
#include "mca/oob/base/base.h"


OBJ_CLASS_INSTANCE(
    mca_oob_t,
    ompi_list_item_t,
    NULL,
    NULL
);

OBJ_CLASS_INSTANCE(
    mca_oob_base_info_t,
    ompi_list_item_t,
    NULL,
    NULL
);

ompi_process_name_t mca_oob_name_self  = { MCA_NS_BASE_CELLID_MAX, MCA_NS_BASE_JOBID_MAX, MCA_NS_BASE_VPID_MAX };
ompi_process_name_t mca_oob_name_seed  = { 0, 0, 0 };
ompi_process_name_t mca_oob_name_any  = { MCA_NS_BASE_CELLID_MAX, MCA_NS_BASE_JOBID_MAX, MCA_NS_BASE_VPID_MAX };

/**
 * Parse contact info string into process name and list of uri strings.
 */

int mca_oob_parse_contact_info(
    const char* contact_info,
    ompi_process_name_t* name,
    char*** uri)
{
    ompi_process_name_t* proc_name;

    /* parse the process name */
    char* cinfo = strdup(contact_info);
    char* ptr = strchr(cinfo, ';');
    if(NULL == ptr) {
        free(cinfo);
        return OMPI_ERR_BAD_PARAM;
    }
    *ptr = '\0';
    ptr++;
    proc_name = ns_base_convert_string_to_process_name(cinfo);
    *name = *proc_name;
    free(proc_name);

    if (NULL != uri) {
	/* parse the remainder of the string into an array of uris */
	*uri = ompi_argv_split(ptr, ';');
    }
    free(cinfo);
    return OMPI_SUCCESS;
}


/**
 * Function for selecting one module from all those that are
 * available.
 *
 * Call the init function on all available modules.
 */
int mca_oob_base_init(bool *user_threads, bool *hidden_threads)
{
    ompi_list_item_t *item;
    mca_base_component_list_item_t *cli;
    mca_oob_base_component_t *component;
    mca_oob_t *module;
    extern ompi_list_t mca_oob_base_components;
    mca_oob_t *s_module = NULL;
    bool s_user_threads = true;
    bool s_hidden_threads = false;
    int  s_priority = -1;

    char** include = ompi_argv_split(mca_oob_base_include, ',');
    char** exclude = ompi_argv_split(mca_oob_base_exclude, ',');

    /* Traverse the list of available modules; call their init functions. */
    for (item = ompi_list_get_first(&mca_oob_base_components);
        item != ompi_list_get_end(&mca_oob_base_components);
        item = ompi_list_get_next(item)) {
        mca_oob_base_info_t *inited;

        cli = (mca_base_component_list_item_t *) item;
        component = (mca_oob_base_component_t *) cli->cli_component;

        /* if there is an include list - item must be in the list to be included */
        if ( NULL != include ) {
            char** argv = include;
            bool found = false;
            while(argv && *argv) {
                if(strcmp(component->oob_base.mca_component_name,*argv) == 0) {
                    found = true;
                    break;
                }
                argv++;
            }
            if(found == false) {
                continue;
            }
                                                                                                                     
        /* otherwise - check the exclude list to see if this item has been specifically excluded */
        } else if ( NULL != exclude ) {
            char** argv = exclude;
            bool found = false;
            while(argv && *argv) {
                if(strcmp(component->oob_base.mca_component_name,*argv) == 0) {
                    found = true;
                    break;
                }
                argv++;
            }
            if(found == true) {
                continue;
            }
        }
                                                                                                                     

        if (NULL == component->oob_init) {
            ompi_output_verbose(10, mca_oob_base_output, "mca_oob_base_init: no init function; ignoring component");
        } else {
            int priority = -1;
            bool u_threads;
            bool h_threads;
            module = component->oob_init(&priority, &u_threads, &h_threads);
            if (NULL != module) {
                inited = OBJ_NEW(mca_oob_base_info_t);
                inited->oob_component = component;
                inited->oob_module = module;
                ompi_list_append(&mca_oob_base_modules, &inited->super);

                /* setup highest priority oob channel */
                if(priority > s_priority) {
                    s_priority = priority;
                    s_module = module;
                    s_user_threads = u_threads;
                    s_hidden_threads = h_threads;
                }
            }
        }
    }
    /* set the global variable to point to the first initialize module */
    if(s_module == NULL) {
      ompi_output(0, "mca_oob_base_init: no OOB modules available\n");
      return OMPI_ERROR;
   }

   mca_oob = *s_module;
   *user_threads &= s_user_threads;
   *hidden_threads |= s_hidden_threads;
   return OMPI_SUCCESS;
}


                                                                                  
/**
*  Obtains the contact info (oob implementation specific) URI strings through
*  which this process can be contacted on an OOB channel.
*
*  @return  A null terminated string.
*
*  The caller is responsible for freeing the returned string.
*/
                                                                                                             
char* mca_oob_get_contact_info()
{
    char *proc_name = ns_base_get_proc_name_string(MCA_OOB_NAME_SELF);
    char *proc_addr = mca_oob.oob_get_addr();
    size_t size = strlen(proc_name) + 1 + strlen(proc_addr) + 1;
    char *contact_info = malloc(size);
    sprintf(contact_info, "%s;%s", proc_name, proc_addr);
    free(proc_name);
    free(proc_addr);
    return contact_info;
}


/**
*  Setup the contact information for the seed daemon - which
*  is passed as an MCA parameter. 
*
*  @param  seed  
*/
                                                                                                             
int mca_oob_set_contact_info(const char* contact_info)
{
    ompi_process_name_t name;
    char** uri;
    char** ptr;
    int rc = mca_oob_parse_contact_info(contact_info, &name, &uri);
    if(rc != OMPI_SUCCESS)
        return rc;

    for(ptr = uri; ptr != NULL && *ptr != NULL; ptr++) {
        ompi_list_item_t* item;
        for (item =  ompi_list_get_first(&mca_oob_base_modules);
             item != ompi_list_get_end(&mca_oob_base_modules);
             item =  ompi_list_get_next(item)) {
            mca_oob_base_info_t* base = (mca_oob_base_info_t *) item;
            if (strncmp(base->oob_component->oob_base.mca_component_name, *ptr,
                strlen(base->oob_component->oob_base.mca_component_name)) == 0)
                base->oob_module->oob_set_addr(&name, *ptr);
        }
    }

    if(uri != NULL) {
        ompi_argv_free(uri);
    }
    return OMPI_SUCCESS;
}

/**
* Called to request the selected oob components to
* register their address with the seed deamon.
*/

int mca_oob_base_module_init(void)
{
  ompi_list_item_t* item;

  /* setup self to point to actual process name */
  mca_oob_name_self = *ompi_process_info.name;

  /* Initialize all modules after oob/gpr/ns have initialized */
  for (item =  ompi_list_get_first(&mca_oob_base_modules);
       item != ompi_list_get_end(&mca_oob_base_modules);
       item =  ompi_list_get_next(item)) {
    mca_oob_base_info_t* base = (mca_oob_base_info_t *) item;
    if (NULL != base->oob_module->oob_init)
        base->oob_module->oob_init();
  }
  return OMPI_SUCCESS;
}

