/*
 * $HEADER$
 */

#include "ompi_config.h"

#include <stdio.h>
#include <stdlib.h>

#include "mpi.h"
#include "include/constants.h"
#include "class/ompi_list.h"
#include "util/output.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/io/io.h"
#include "mca/io/base/base.h"


/*
 * Global variables
 */
bool mca_io_base_components_available_valid = false;
ompi_list_t mca_io_base_components_available;
const mca_io_base_component_1_0_0_t *mca_io_base_basic_component = NULL;


/*
 * Private functions
 */
static int init_query(const mca_base_component_t *ls, 
                      mca_base_component_priority_list_item_t *entry);
static int init_query_1_0_0(const mca_base_component_t *ls, 
                            mca_base_component_priority_list_item_t *entry);

/*
 * Scan down the list of successfully opened components and query each of
 * them (the opened list will be one or more components.  If the user
 * requested a specific component, it will be the only component in the
 * opened list).  Create and populate the available list of all
 * components who indicate that they want to be considered for selection.
 * Close all components who do not want to be considered for selection,
 * and destroy the opened list.
 *
 * It is *not* an error if there are no io components available.
 * Appropriate run-time MPI exceptions will be invoked during
 * MPI_FILE_OPEN and MPI_FILE_DELETE.
 */
int mca_io_base_find_available(bool *allow_multi_user_threads,
                               bool *have_hidden_threads)
{
    bool found = false;
    mca_base_component_priority_list_item_t *entry;
    ompi_list_item_t *p;
    const mca_base_component_t *component;

    /* Initialize the list */

    OBJ_CONSTRUCT(&mca_io_base_components_available, ompi_list_t);
    mca_io_base_components_available_valid = true;

    /* The list of components that we should check has already been
       established in mca_io_base_open. */
  
    for (found = false, 
             p = ompi_list_remove_first(&mca_io_base_components_opened);
         p != NULL;
         p = ompi_list_remove_first(&mca_io_base_components_opened)) {
        component = ((mca_base_component_list_item_t *) p)->cli_component;
        
        /* Call a subroutine to do the work, because the component may
           represent different versions of the io MCA. */
    
        entry = OBJ_NEW(mca_base_component_priority_list_item_t);
        entry->cpli_component = component;
        entry->cpli_priority = 0;
        if (OMPI_SUCCESS == init_query(component, entry)) {
      
            /* Save the results in the list.  The priority isn't
               relevant, because selection is decided at
               communicator-constructor time.  But we save the thread
               arguments (set in the init_query() function) so that
               the initial selection algorithm can negotiate the
               overall thread level for this process. */
      
            ompi_list_append(&mca_io_base_components_available, 
                             (ompi_list_item_t *) entry);

            /* We found something :-) */

            found = true;
        } else {
      
            /* If the component doesn't want to run, then close it.
               It's already had its close() method invoked; now close
               it out of the DSO repository (if it's there). */
      
            mca_base_component_repository_release(component);
            OBJ_RELEASE(entry);
        }

        /* Free the entry from the "opened" list */

        OBJ_RELEASE(p);
    }

    /* The opened list is now no longer useful and we can free it */
    
    OBJ_DESTRUCT(&mca_io_base_components_opened);
    mca_io_base_components_opened_valid = false;

    /* All done */

    return OMPI_SUCCESS;
}


/*
 * Query a component, see if it wants to run at all.  If it does, save
 * some information.  If it doesn't, close it.
 */
static int init_query(const mca_base_component_t *m, 
                      mca_base_component_priority_list_item_t *entry)
{
    int ret;

    ompi_output_verbose(10, mca_io_base_output,
                        "io:find_available: querying io component %s", 
                        m->mca_component_name);

    /* This component has already been successfully opened.  So now
       query it. */

    if (1 == m->mca_type_major_version &&
        0 == m->mca_type_minor_version &&
        0 == m->mca_type_release_version) {
        ret = init_query_1_0_0(m, entry);
    } else {
        /* Unrecognized io API version */

        ompi_output_verbose(10, mca_io_base_output,
                            "io:find_available: unrecognized io API version (%d.%d.%d)", 
                            m->mca_type_major_version,
                            m->mca_type_minor_version,
                            m->mca_type_release_version);
        /* JMS show_help */
        return OMPI_ERROR;
    }

    /* Query done -- look at the return value to see what happened */

    if (OMPI_SUCCESS != ret) {
        ompi_output_verbose(10, mca_io_base_output, 
                            "io:find_available: io component %s is not available", 
                            m->mca_component_name);
        if (NULL != m->mca_close_component) {
            m->mca_close_component();
        }
    } else {
        ompi_output_verbose(10, mca_io_base_output, 
                            "io:find_available: io component %s is available", 
                            m->mca_component_name);
    }

    /* All done */

    return ret;
}


/*
 * Query a specific component, io v1.0.0
 */
static int 
init_query_1_0_0(const mca_base_component_t *component, 
                 mca_base_component_priority_list_item_t *entry)
{
    mca_io_base_component_1_0_0_t *io = 
	(mca_io_base_component_1_0_0_t *) component;

    return io->io_init_query(&(entry->cpli_allow_multi_user_threads), 
                             &(entry->cpli_have_hidden_threads));
}
