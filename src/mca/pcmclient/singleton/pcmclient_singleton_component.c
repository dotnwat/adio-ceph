/*
 * $HEADER$
 */


#include "ompi_config.h"
#include "pcmclient-singleton-version.h"

#include "include/constants.h"
#include "include/types.h"
#include "mca/mca.h"
#include "mca/pcmclient/pcmclient.h"
#include "mca/pcmclient/singleton/pcmclient_singleton.h"
#include "util/proc_info.h"
#include "mca/base/mca_base_param.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

/*
 * Struct of function pointers and all that to let us be initialized
 */
mca_pcmclient_base_component_1_0_0_t mca_pcmclient_singleton_component = {
  {
    MCA_PCMCLIENT_BASE_VERSION_1_0_0,

    "singleton", /* MCA component name */
    MCA_pcmclient_singleton_MAJOR_VERSION,  /* MCA component major version */
    MCA_pcmclient_singleton_MINOR_VERSION,  /* MCA component minor version */
    MCA_pcmclient_singleton_RELEASE_VERSION,  /* MCA component release version */
    mca_pcmclient_singleton_open,  /* component open */
    mca_pcmclient_singleton_close /* component close */
  },
  {
    false /* checkpoint / restart */
  },
  mca_pcmclient_singleton_init,    /* component init */
  mca_pcmclient_singleton_finalize
};


struct mca_pcmclient_base_module_1_0_0_t mca_pcmclient_singleton_1_0_0 = {
    mca_pcmclient_singleton_get_self,
    mca_pcmclient_singleton_get_peers,
};

ompi_process_name_t *mca_pcmclient_singleton_procs = NULL;


int
mca_pcmclient_singleton_open(void)
{
    return OMPI_SUCCESS;
}


int
mca_pcmclient_singleton_close(void)
{
  return OMPI_SUCCESS;
}


struct mca_pcmclient_base_module_1_0_0_t *
mca_pcmclient_singleton_init(int *priority, 
                             bool *allow_multiple_user_threads, 
                             bool *have_hidden_threads)
{
    *priority = 0;
    *allow_multiple_user_threads = true;
    *have_hidden_threads = false;
    
    return &mca_pcmclient_singleton_1_0_0;
}


int
mca_pcmclient_singleton_finalize(void)
{
    if (NULL != mca_pcmclient_singleton_procs) {
        free(mca_pcmclient_singleton_procs);
    }

  return OMPI_SUCCESS;
}


