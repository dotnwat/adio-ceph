/* -*- C -*-
 * 
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
 *                         University of Stuttgart.  All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 *
 * BWB: COMPONENT TODO
 *
 *  - add process reaping code
 *  - trigger status change events on process death
 *
 */

#include "ompi_config.h"

#include "mca/pcm/pcm.h"
#include "mca/pcm/base/base_job_track.h"
#include "include/types.h"
#include "class/ompi_list.h"

#include <sys/types.h>

#ifndef MCA_PCM_RMS_H_
#define MCA_PCM_RMS_H_

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

    /*
     * Module open / close
     */
    int mca_pcm_rms_component_open(void);
    int mca_pcm_rms_component_close(void);

    /*
     * Startup / Shutdown
     */
    struct mca_pcm_base_module_1_0_0_t* mca_pcm_rms_init(int *priority, 
                                                         bool have_threads,
                                                         int constraints);
    int mca_pcm_rms_finalize(struct mca_pcm_base_module_1_0_0_t* me);

    /*
     * Interface
     */
    ompi_list_t* mca_pcm_rms_allocate_resources(struct mca_pcm_base_module_1_0_0_t* me,
                                                mca_ns_base_jobid_t jobid,
                                                int nodes, int procs);
    int mca_pcm_rms_spawn_procs(struct mca_pcm_base_module_1_0_0_t* me,
                                mca_ns_base_jobid_t jobid, ompi_list_t *schedule_list);
    int mca_pcm_rms_kill_proc(struct mca_pcm_base_module_1_0_0_t* me,
                              ompi_process_name_t *name, int flags);
    int mca_pcm_rms_kill_job(struct mca_pcm_base_module_1_0_0_t* me,
                             mca_ns_base_jobid_t jobid, int flags);
    int mca_pcm_rms_deallocate_resources(struct mca_pcm_base_module_1_0_0_t* me,
                                         mca_ns_base_jobid_t jobid,
                                         ompi_list_t *nodelist);

    struct mca_pcm_rms_module_t {
        mca_pcm_base_module_t super;

        mca_pcm_base_job_list_t *jobs;

        char *partition;
        char *prun_args;
        int constraints;
    };
    typedef struct mca_pcm_rms_module_t mca_pcm_rms_module_t;

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

#endif /* MCA_PCM_RMS_H_ */
