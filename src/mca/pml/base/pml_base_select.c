/*
 * $HEADER$
 */

#include "lam_config.h"

#include "lfc/lam_list.h"
#include "runtime/runtime.h"
#include "mca/mca.h"
#include "mca/base/base.h"
#include "mca/pml/pml.h"
#include "mca/pml/base/base.h"


typedef struct opened_module_t {
  lam_list_item_t super;

  mca_pml_base_module_t *om_module;
} opened_module_t;


/**
 * Function for selecting one module from all those that are
 * available.
 *
 * Call the init function on all available modules and get their
 * priorities.  Select the module with the highest priority.  All
 * other modules will be closed and unloaded.  The selected module
 * will have all of its function pointers saved and returned to the
 * caller.
 */
int mca_pml_base_select(mca_pml_t *selected, bool *allow_multi_user_threads,
                        bool *have_hidden_threads)
{
  int priority, best_priority;
  bool user_threads, hidden_threads;
  bool best_user_threads, best_hidden_threads;
  lam_list_item_t *item;
  mca_base_module_list_item_t *mli;
  mca_pml_base_module_t *module, *best_module;
  mca_pml_t *actions;
  lam_list_t opened;
  opened_module_t *om;  

  /* Traverse the list of available modules; call their init
     functions. */

  best_priority = -1;
  best_module = NULL;
  OBJ_CONSTRUCT(&opened, lam_list_t);
  for (item = lam_list_get_first(&mca_pml_base_modules_available);
       lam_list_get_end(&mca_pml_base_modules_available) != item;
       item = lam_list_get_next(item)) {
    mli = (mca_base_module_list_item_t *) item;
    module = (mca_pml_base_module_t *) mli->mli_module;

    lam_output_verbose(10, mca_pml_base_output, 
                       "select: initializing %s module %s",
                       module->pmlm_version.mca_type_name,
                       module->pmlm_version.mca_module_name);
    if (NULL == module->pmlm_init) {
      lam_output_verbose(10, mca_pml_base_output,
                         "select: no init function; ignoring module");
    } else {
      actions = module->pmlm_init(&priority, &user_threads,
                                  &hidden_threads);
      if (NULL == actions) {
        lam_output_verbose(10, mca_pml_base_output,
                           "select: init returned failure");
      } else {
        lam_output_verbose(10, mca_pml_base_output,
                           "select: init returned priority %d", priority);
        if (priority > best_priority) {
          best_priority = priority;
          best_user_threads = user_threads;
          best_hidden_threads = hidden_threads;
          best_module = module;
        }

        om = malloc(sizeof(opened_module_t));
        if (NULL == om) {
          return LAM_ERR_OUT_OF_RESOURCE;
        }
        OBJ_CONSTRUCT(om, lam_list_item_t);
        om->om_module = module;
        lam_list_append(&opened, (lam_list_item_t*) om);
      }
    }
  }

  /* Finished querying all modules.  Check for the bozo case. */

  if (NULL == best_module) {
    /* JMS Replace with show_help */
    lam_abort(1, "No pml module available.  This shouldn't happen.");
  } 

  /* Finalize all non-selected modules */

  for (item = lam_list_remove_first(&opened);
       NULL != item;
       item = lam_list_remove_first(&opened)) {
    om = (opened_module_t *) item;
    if (om->om_module != best_module) {

      /* Finalize */

      if (NULL != om->om_module->pmlm_finalize) {

        /* Blatently ignore the return code (what would we do to
           recover, anyway?  This module is going away, so errors
           don't matter anymore) */

        om->om_module->pmlm_finalize();
        lam_output_verbose(10, mca_pml_base_output, 
                           "select: module %s not selected / finalized",
                           module->pmlm_version.mca_module_name);
      }
    }
    free(om);
  }

  /* This base function closes, unloads, and removes from the
     available list all unselected modules.  The available list will
     contain only the selected module. */

  mca_base_modules_close(mca_pml_base_output, &mca_pml_base_modules_available, 
                         (mca_base_module_t *) best_module);

  /* Save the winner */

  mca_pml_base_selected_module = *best_module;
  mca_pml = *actions;
  *selected = *actions;
  *allow_multi_user_threads = best_user_threads;
  *have_hidden_threads = best_hidden_threads;
  lam_output_verbose(10, mca_pml_base_output, 
                     "select: module %s selected",
                     module->pmlm_version.mca_module_name);

  /* All done */

  return LAM_SUCCESS;
}
