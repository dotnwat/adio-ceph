/*
 * $HEADER$
 */

#include "lam_config.h"

#include <stdio.h>

#include "include/constants.h"
#include "lfc/lam_list.h"
#include "mca/base/base.h"
#include "mca/coll/coll.h"
#include "mca/coll/base/base.h"
#include "mca/ptl/ptl.h"
#include "mca/ptl/base/base.h"
#include "mca/pml/pml.h"
#include "mca/pml/base/base.h"


/*
 * Look at available pml, ptl, and coll modules and find a set that
 * works nicely.  Also set the final MPI thread level.  There are many
 * factors involved here, and this first implementation is rather
 * simplistic.  
 *
 * The contents of this function will likely be replaced 
 */
int mca_base_init_select_modules(int requested, 
                                bool allow_multi_user_threads,
                                bool have_hidden_threads, int *provided)
{
  lam_list_t colls;
  bool user_threads, hidden_threads;

  /* Make final lists of available modules (i.e., call the query/init
     functions and see if they return happiness).  For pml, there will
     only be one (because there's only one for the whole process), but
     for ptl and coll, we'll get lists back. */
  /* JMS: At some point, we'll need to feed it the thread level to
     ensure to pick one high enough (e.g., if we need CR) */

  if (LAM_SUCCESS != mca_pml_base_select(&mca_pml, 
                                         &user_threads, &hidden_threads)) {
    return LAM_ERROR;
  }
  allow_multi_user_threads |= user_threads;
  have_hidden_threads |= hidden_threads;

  if (LAM_SUCCESS != mca_ptl_base_select(&user_threads, &hidden_threads)) {
    return LAM_ERROR;
  }
  allow_multi_user_threads |= user_threads;
  have_hidden_threads |= hidden_threads;

  OBJ_CONSTRUCT(&colls, lam_list_t);
  if (LAM_SUCCESS != mca_coll_base_select(&colls, &user_threads, 
                                          &hidden_threads)) {
    return LAM_ERROR;
  }
  allow_multi_user_threads |= user_threads;
  have_hidden_threads |= hidden_threads;

  /* Now that we have a final list of all available modules, do the
     selection.  pml is already selected. */

  /* JMS ...Do more here with the thread level, etc.... */
  *provided = requested;
  if(have_hidden_threads)
      lam_set_using_threads(true);

  /* Tell the selected pml module about all the selected ptl
     modules */

  mca_pml.pml_add_ptls(&mca_ptl_base_modules_initialized);

  /* All done */

  return LAM_SUCCESS;
}

