/*
 * Copyright (c) 2010      Cisco Systems, Inc.  All rights reserved. 
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "orte_config.h"
#include "orte/constants.h"

#include "opal/mca/base/base.h"
#include "opal/util/output.h"
#include "opal/mca/base/mca_base_param.h"
#include "opal/class/opal_pointer_array.h"

#include "orte/util/proc_info.h"
#include "orte/util/show_help.h"

#include "sensor_file.h"

/*
 * Local functions
 */

static int orte_sensor_file_open(void);
static int orte_sensor_file_close(void);
static int orte_sensor_file_query(mca_base_module_t **module, int *priority);

orte_sensor_file_component_t mca_sensor_file_component = {
    {
        {
            ORTE_SENSOR_BASE_VERSION_1_0_0,
            
            "file", /* MCA component name */
            ORTE_MAJOR_VERSION,  /* MCA component major version */
            ORTE_MINOR_VERSION,  /* MCA component minor version */
            ORTE_RELEASE_VERSION,  /* MCA component release version */
            orte_sensor_file_open,  /* component open  */
            orte_sensor_file_close, /* component close */
            orte_sensor_file_query  /* component query */
        },
        {
            /* The component is checkpoint ready */
            MCA_BASE_METADATA_PARAM_CHECKPOINT
        }
    }
};


/**
  * component open/close/init function
  */
static int orte_sensor_file_open(void)
{
    mca_base_component_t *c = &mca_sensor_file_component.super.base_version;
    int tmp;

    /* lookup parameters */
    mca_base_param_reg_int(c, "sample_rate",
                           "Sample rate in seconds (default=10)",
                           false, false, 10, &mca_sensor_file_component.sample_rate);
    
    mca_base_param_reg_string(c, "filename",
                           "File to be monitored",
                           false, false, NULL, &mca_sensor_file_component.file);

    mca_base_param_reg_int(c, "check_size",
                           "Check the file size",
                           false, false, false, &tmp);
    mca_sensor_file_component.check_size = OPAL_INT_TO_BOOL(tmp);
    
    mca_base_param_reg_int(c, "check_access",
                           "Check access time",
                           false, false, false, &tmp);
    mca_sensor_file_component.check_access = OPAL_INT_TO_BOOL(tmp);

    mca_base_param_reg_int(c, "check_mod",
                           "Check modification time",
                           false, false, false, &tmp);
    mca_sensor_file_component.check_mod = OPAL_INT_TO_BOOL(tmp);

    mca_base_param_reg_int(c, "limit",
                           "Number of times the sensor can detect no motion before declaring error (default=3)",
                           false, false, 3, &mca_sensor_file_component.limit);
    
    return ORTE_SUCCESS;
}


static int orte_sensor_file_query(mca_base_module_t **module, int *priority)
{    
    *priority = 0;  /* select only if specified */
    *module = (mca_base_module_t *)&orte_sensor_file_module;
    
    return ORTE_SUCCESS;
}

/**
 *  Close all subsystems.
 */

static int orte_sensor_file_close(void)
{
    return ORTE_SUCCESS;
}
