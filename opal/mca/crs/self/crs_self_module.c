/*
 * Copyright (c) 2004-2007 The Trustees of Indiana University.
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

#include "opal_config.h"

#include <sys/types.h>
#if HAVE_UNISTD_H
#include <unistd.h>
#endif  /* HAVE_UNISTD_H */

#include "opal/libltdl/ltdl.h"

#include "opal/util/output.h"
#include "opal/util/show_help.h"
#include "opal/util/argv.h"
#include "opal/util/opal_environ.h"

#include "opal/constants.h"
#include "opal/mca/base/mca_base_param.h"

#include "opal/mca/crs/crs.h"
#include "opal/mca/crs/base/base.h"
#include "opal/runtime/opal_cr.h"

#include "crs_self.h"

/*
 * Self module
 */
static opal_crs_base_module_t loc_module = {
    /** Initialization Function */
    opal_crs_self_module_init,
    /** Finalization Function */
    opal_crs_self_module_finalize,

    /** Checkpoint interface */
    opal_crs_self_checkpoint,

    /** Restart Command Access */
    opal_crs_self_restart,

    /** Disable checkpoints */
    opal_crs_self_disable_checkpoint,
    /** Enable checkpoints */
    opal_crs_self_enable_checkpoint
};

/*
 * Snapshot structure
 */
OBJ_CLASS_DECLARATION(opal_crs_self_snapshot_t);

struct opal_crs_self_snapshot_t {
    /** Base CRS snapshot type */
    opal_crs_base_snapshot_t super;
    /** Command Line used to restart the app */
    char * cmd_line;
};
typedef struct opal_crs_self_snapshot_t opal_crs_self_snapshot_t;

static void opal_crs_self_construct(opal_crs_self_snapshot_t *obj);
static void opal_crs_self_destruct( opal_crs_self_snapshot_t *obj);

OBJ_CLASS_INSTANCE(opal_crs_self_snapshot_t,
                   opal_crs_base_snapshot_t,
                   opal_crs_self_construct,
                   opal_crs_self_destruct);


/************************************
 * Locally Global vars & functions :)
 ************************************/
static lt_ptr 
crs_self_find_function(lt_dlhandle handle, char *prefix, char *suffix);

static int self_update_snapshot_metadata(opal_crs_self_snapshot_t *snapshot);

static int opal_crs_self_restart_cmd(opal_crs_self_snapshot_t *snapshot, char **cmd);
static int self_cold_start(opal_crs_self_snapshot_t *snapshot);

void opal_crs_self_construct(opal_crs_self_snapshot_t *snapshot)
{
    snapshot->cmd_line = NULL;
}

void opal_crs_self_destruct( opal_crs_self_snapshot_t *snapshot)
{
    if(NULL != snapshot->cmd_line)
        free(snapshot->cmd_line);
}

static int opal_crs_self_extract_callbacks(void);

/*
 * MCA Functions
 */
opal_crs_base_module_1_0_0_t *
opal_crs_self_component_query(int *priority)
{
    int ret;

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: component_query()");

    /*
     * Extract the user level callbacks if they exist
     */
    ret = opal_crs_self_extract_callbacks();

    if( OPAL_SUCCESS != ret || 
        !mca_crs_self_component.can_checkpoint ) {
        *priority = -1;
        return NULL;
    }
    else {
        *priority = mca_crs_self_component.super.priority;
        return &loc_module;
    }
}

static int opal_crs_self_extract_callbacks(void)
{
    bool callback_matched = true;
    lt_dlhandle executable;

    if( opal_cr_is_tool ) {
        return OPAL_SUCCESS;
    }
    
    /*
     * Open the executable so that we can lookup the necessary symbols
     */
    executable = lt_dlopen(NULL);
    if ( NULL == executable) {
        opal_show_help("help-opal-crs-self.txt", "self:lt_dlopen",
                       true);
        return OPAL_ERROR;
    }

    /*
     * Find the function names
     */
    mca_crs_self_component.ucb_checkpoint_fn = (opal_crs_self_checkpoint_callback_fn_t)
        crs_self_find_function(executable, 
                               mca_crs_self_component.prefix,
                               SUFFIX_CHECKPOINT);

    mca_crs_self_component.ucb_continue_fn = (opal_crs_self_continue_callback_fn_t)
        crs_self_find_function(executable, 
                               mca_crs_self_component.prefix,
                               SUFFIX_CONTINUE);

    mca_crs_self_component.ucb_restart_fn = (opal_crs_self_restart_callback_fn_t)
        crs_self_find_function(executable, 
                               mca_crs_self_component.prefix,
                               SUFFIX_RESTART);

    /*
     * Done with executable, close it
     */
    lt_dlclose(executable);

    /*
     * Sanity check
     */
    mca_crs_self_component.can_checkpoint = true;

    if(NULL == mca_crs_self_component.ucb_checkpoint_fn) {
        callback_matched = false;
        mca_crs_self_component.can_checkpoint = false;
    }
    if(NULL == mca_crs_self_component.ucb_continue_fn) {
        callback_matched = false;
    }
    if(NULL == mca_crs_self_component.ucb_restart_fn) {
        callback_matched = false;
    }

    return OPAL_SUCCESS;
}

int opal_crs_self_module_init(void)
{
    bool callback_matched = true;

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: module_init()");

    if( opal_cr_is_tool ) {
        return OPAL_SUCCESS;
    }
    
    /*
     * Sanity check
     */
    if(NULL == mca_crs_self_component.ucb_checkpoint_fn) {
        callback_matched = false;
        mca_crs_self_component.can_checkpoint = false;
    }
    if(NULL == mca_crs_self_component.ucb_continue_fn) {
        callback_matched = false;
    }
    if(NULL == mca_crs_self_component.ucb_restart_fn) {
        callback_matched = false;
    }
    if( !callback_matched ) {
        if( 1 <= mca_crs_self_component.super.verbose ) {
            opal_show_help("help-opal-crs-self.txt", "self:no_callback", false,
                           "checkpoint", mca_crs_self_component.prefix, SUFFIX_CHECKPOINT,
                           "continue  ", mca_crs_self_component.prefix, SUFFIX_CONTINUE,
                           "restart   ", mca_crs_self_component.prefix, SUFFIX_RESTART,
                           PREFIX_DEFAULT);
        }
    }

    /*
     * If the user requested that we do_restart, then call their callback
     */
    if(mca_crs_self_component.do_restart) {
        opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                            "crs:self: module_init: Call their restart function");
        if( NULL != mca_crs_self_component.ucb_restart_fn) 
            mca_crs_self_component.ucb_restart_fn();
    }

    return OPAL_SUCCESS;
}

int opal_crs_self_module_finalize(void)
{
    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: module_finalize()");

    return OPAL_SUCCESS;
}


int opal_crs_self_checkpoint(pid_t pid, opal_crs_base_snapshot_t *base_snapshot, opal_crs_state_type_t *state)
{
    opal_crs_self_snapshot_t *snapshot = OBJ_NEW(opal_crs_self_snapshot_t);
    int ret, exit_status = OPAL_SUCCESS;
    char * restart_cmd = NULL;

    /*
     * Setup for snapshot directory creation
     */
    if(NULL != snapshot->super.reference_name)
        free(snapshot->super.reference_name);
    snapshot->super.reference_name = strdup(base_snapshot->reference_name);

    if(NULL != snapshot->super.local_location)
        free(snapshot->super.local_location);
    snapshot->super.local_location  = strdup(base_snapshot->local_location);

    if(NULL != snapshot->super.remote_location)
        free(snapshot->super.remote_location);
    snapshot->super.remote_location  = strdup(base_snapshot->remote_location);

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: checkpoint(%d, ---)", pid);

    if(!mca_crs_self_component.can_checkpoint) {
        opal_show_help("help-opal-crs-self.txt", "self:ckpt_disabled", false);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /*
     * Create the snapshot directory
     */
    snapshot->super.component_name = strdup(mca_crs_self_component.super.crs_version.mca_component_name);
    if( OPAL_SUCCESS != (ret = opal_crs_base_init_snapshot_directory(&snapshot->super) )) {
        *state = OPAL_CRS_ERROR;
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: checkpoint(): Error: Unable to initialize the directory for (%s).",
                    snapshot->super.reference_name);
        exit_status = ret;
        goto cleanup;
    }

    /*
     * Call the user callback function
     */
    if(NULL != mca_crs_self_component.ucb_checkpoint_fn) {
        mca_crs_self_component.ucb_checkpoint_fn(&restart_cmd);
    }

    /*
     * Save the restart command
     */
    if( NULL == restart_cmd) {
        *state = OPAL_CRS_ERROR;
        opal_show_help("help-opal-crs-self.txt", "self:no-restart-cmd",
                       true);
        exit_status = OPAL_ERROR;
        goto cleanup;
    }
    else {
        snapshot->cmd_line = strdup(restart_cmd);

        opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                            "crs:self: checkpoint: Restart Command (%s)", snapshot->cmd_line);
    }

    /*
     * The best we can do is update the metadata file with the
     * application argv and argc we started with.
     */
    if( OPAL_SUCCESS != (ret = self_update_snapshot_metadata(snapshot)) ) {
        *state = OPAL_CRS_ERROR;
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: checkpoint(): Error: Unable to update metadata for snapshot (%s).",
                    snapshot->super.reference_name);
        exit_status = ret;
        goto cleanup;
    }


    *state = OPAL_CRS_CONTINUE;
    
    /*
     * Call their continue routine for completeness
     */
    if(NULL != mca_crs_self_component.ucb_continue_fn) {
        mca_crs_self_component.ucb_continue_fn();
    }

    base_snapshot = &(snapshot->super);

 cleanup:
    if( NULL != restart_cmd) {
        free(restart_cmd);
        restart_cmd = NULL;
    }

    return exit_status;
}

/*
 * Notice that the user restart callback is not called here, but always from 
 *  opal_init for the self module.
 */
int opal_crs_self_restart(opal_crs_base_snapshot_t *base_snapshot, bool spawn_child, pid_t *child_pid)
{
    opal_crs_self_snapshot_t *snapshot = OBJ_NEW(opal_crs_self_snapshot_t);
    char **cr_argv = NULL;
    char * cr_cmd = NULL;
    int ret;
    int exit_status = OPAL_SUCCESS;
    int status;

    snapshot->super = *base_snapshot;

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: restart(%s, %d)", snapshot->super.reference_name, spawn_child);

    /*
     * If we need to reconstruct the snapshot
     */
    if(snapshot->super.cold_start) {
        if( OPAL_SUCCESS != (ret = self_cold_start(snapshot)) ){
            exit_status = ret;
            opal_output(mca_crs_self_component.super.output_handle,
                        "crs:blcr: blcr_restart: Unable to reconstruct the snapshot.");
            goto cleanup;
        }
    }

    /*
     * Get the restart command
     */
    if ( OPAL_SUCCESS != (ret = opal_crs_self_restart_cmd(snapshot, &cr_cmd)) ) {
        exit_status = ret;
        goto cleanup;
    }
    if ( NULL == (cr_argv = opal_argv_split(cr_cmd, ' ')) ) {
        exit_status = OPAL_ERROR;
        goto cleanup;
    }


    if (!spawn_child) {
        opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                            "crs:self: self_restart: SELF: exec :(%s, %s):",
                            strdup(cr_argv[0]),
                            opal_argv_join(cr_argv, ' '));

        status = execvp(strdup(cr_argv[0]), cr_argv);

        if(status < 0) {
            opal_output(mca_crs_self_component.super.output_handle,
                        "crs:self: self_restart: SELF: Child failed to execute :(%d):", status);
        }
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: self_restart: SELF: execvp returned %d", status);
        exit_status = status;
        goto cleanup;
    }
    else {
        *child_pid = fork();
        if( *child_pid == 0) {
            /* Child Process */
            opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                                "crs:self: self_restart: CHILD: exec :(%s, %s):",
                                strdup(cr_argv[0]),
                                opal_argv_join(cr_argv, ' '));

            status = execvp(strdup(cr_argv[0]), cr_argv);

            if(status < 0) {
                opal_output(mca_crs_self_component.super.output_handle,
                            "crs:self: self_restart: CHILD: Child failed to execute :(%d):", status);
            }
            opal_output(mca_crs_self_component.super.output_handle,
                        "crs:self: self_restart: CHILD: execvp returned %d", status);
            exit_status = status;
            goto cleanup;
        }
        else if(*child_pid > 0) {
            /* Parent is done once it is started. */
            ;
        }
        else {
            opal_output(mca_crs_self_component.super.output_handle,
                        "crs:self: self_restart: CHILD: fork failed :(%d):", *child_pid);
        }
    }

 cleanup:
    if( NULL != cr_cmd)
        free(cr_cmd);
    if( NULL != cr_argv) 
        opal_argv_free(cr_argv);

    return exit_status;
}

int opal_crs_self_disable_checkpoint(void)
{
    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: disable_checkpoint()");

    mca_crs_self_component.can_checkpoint = false;

    return OPAL_SUCCESS;
}

int opal_crs_self_enable_checkpoint(void)
{
    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: enable_checkpoint()");

    mca_crs_self_component.can_checkpoint = true;

    return OPAL_SUCCESS;
}

/******************
 * Local functions
 ******************/
static lt_ptr
crs_self_find_function(lt_dlhandle handle, char *prefix, char *suffix){
    char *func_to_find = NULL;
    lt_ptr ptr;

    if( NULL == prefix || 0 >= strlen(prefix) ) {
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: crs_self_find_function: Error: prefix is NULL or empty string!");
        return NULL;
    }
    if( NULL == suffix || 0 >= strlen(suffix) ) {
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: crs_self_find_function: Error: suffix is NULL or empty string!");
        return NULL;
    }

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: crs_self_find_function(--, %s, %s)", 
                        prefix, suffix);

    asprintf(&func_to_find, "%s_%s", prefix, suffix);

    ptr = lt_dlsym(handle, func_to_find);
    if( NULL == ptr) {
        opal_output_verbose(12, mca_crs_self_component.super.output_handle,
                            "crs:self: crs_self_find_function: WARNING: Function \"%s\" not found",
                            func_to_find);
    }
    else {
        opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                            "crs:self: crs_self_find_function: Found function \"%s\"",
                            func_to_find);
    }

    if( NULL == func_to_find)
        free(func_to_find);

    return ptr;
}

/*
 * Self is a special case. The 'fname' here is the command line that the user
 * wishes to execute. This function takes this command line and adds
 *   -mca crs_self_do_restart 1
 * Which will trigger the restart callback once the program has been run.
 *
 * For example, The user starts their program with:
 *   $ my_prog arg1 arg2
 *
 * They checkpoint it:
 *   $ opal_checkpoint -mca crs self 1234
 *
 * They restart it:
 *   $ opal_restart -mca crs self my_prog arg1 arg2
 *
 * fname is then:
 *   fname = "my_prog arg1 arg2"
 *
 * This funciton translates that to the command:
 *   cmd = "my_prog arg1 arg2 -mca crs self -mca crs_self_do_restart 1"
 *
 * Which will cause the program "my_prog" to call their restart function 
 * upon opal_init time.
 *
 * Note: The user could bypass the opal_restart routine safely by simply calling
 *   $ my_prog arg1 arg2 -mca crs self -mca crs_self_do_restart 1
 * However, for consistency sake, we should not encourage this as it won't work for 
 * all of the other checkpointers.
 */
static int opal_crs_self_restart_cmd(opal_crs_self_snapshot_t *snapshot, char **cmd)
{
    extern char **environ;
    
    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: restart_cmd(%s, ---)", snapshot->cmd_line);

    opal_setenv(mca_base_param_env_var("crs"), 
                "self", 
                true, &environ);
    opal_setenv(mca_base_param_env_var("crs_self_do_restart"), 
                "1", 
                true, &environ);
    opal_setenv(mca_base_param_env_var("crs_self_prefix"), 
                mca_crs_self_component.prefix, 
                true, &environ);


    /* Instead of adding it to the command line, we should use the environment
     * to pass the values. This allow sthe OPAL application to be braindead 
     * WRT MCA parameters
     *   add_args = strdup("-mca crs self -mca crs_self_do_restart 1");
     */

    asprintf(cmd, "%s", snapshot->cmd_line);

    return OPAL_SUCCESS;
}

static int self_cold_start(opal_crs_self_snapshot_t *snapshot) {
    char * content = NULL;
    char * component_name = NULL;
    int prev_pid;
    int len = 0;
    FILE * meta_data = NULL;
    int exit_status = OPAL_SUCCESS;

    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: cold_start(%s)", snapshot->super.reference_name);

    /*
     * Find the snapshot directory, read the metadata file
     */
    if( NULL == (meta_data = opal_crs_base_open_read_metadata(snapshot->super.local_location,
                                                              &component_name, &prev_pid) ) ) {
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    snapshot->super.component_name = strdup(component_name);

    /* Compare the strings to make sure this is our snapshot before going further */
    if ( 0 != strncmp(mca_crs_self_component.super.crs_version.mca_component_name, 
                      component_name, strlen(component_name)) ) {
        exit_status = OPAL_ERROR;
        opal_output(mca_crs_self_component.super.output_handle,
                    "crs:self: self_cold_start: Error: This snapshot (%s) is not intended for us (%s)\n", 
                    component_name, mca_crs_self_component.super.crs_version.mca_component_name);
        goto cleanup;
    }

    /*
     * Restart command
     * JJH: Command lines limited to 256 chars.
     */
    len = 256; /* Max size for a SELF filename */
    content = (char *) malloc(sizeof(char) * len);
    if (NULL == fgets(content, len, meta_data) ) {
        free(content);
        content = NULL;
        goto cleanup;
    }
    /* Strip of newline */
    len = strlen(content);
    content[len - 1] = '\0';

    /* save the command line in the structure */
    asprintf(&snapshot->cmd_line, "%s", content);

    /*
     * Reset the cold_start flag
     */
    snapshot->super.cold_start = false;

 cleanup:
    if(NULL != meta_data)
        fclose(meta_data);
    if(NULL != content)
        free(content);

    return exit_status;

}

static int self_update_snapshot_metadata(opal_crs_self_snapshot_t *snapshot) {
    char * dir_name = NULL;
    FILE *meta_data = NULL;
    int exit_status = OPAL_SUCCESS;
    
    opal_output_verbose(10, mca_crs_self_component.super.output_handle,
                        "crs:self: update_snapshot_metadata(%s)",
                        snapshot->super.reference_name);
    
    /*
     * Append to the metadata file:
     *  the relative path of the context filename
     */
    if( NULL == (meta_data = opal_crs_base_open_metadata(&snapshot->super, 'w') ) ) {
        exit_status = OPAL_ERROR;
        goto cleanup;
    }

    /* How user wants us to restart */
    if(NULL != snapshot->cmd_line) {
        fprintf(meta_data, "%s\n", snapshot->cmd_line);
    }
    else {
        opal_show_help("help-opal-crs-self.txt", "self:no-restart-cmd",
                       true);
        exit_status = OPAL_ERROR;
    }

 cleanup:
    if(NULL != meta_data) {
        fclose(meta_data);
    }
    if(NULL != dir_name) {
        free(dir_name);
        dir_name = NULL;
    }
    
    return exit_status;
}
