/*
 * Copyright (c) 2011      Sandia National Laboratories. All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "ompi_config.h"

#include "opal/class/opal_object.h"
#include "ompi/message/message.h"
#include "ompi/constants.h"

OBJ_CLASS_INSTANCE(ompi_message_t,
                   opal_free_list_item_t,
                   NULL, NULL);

opal_free_list_t ompi_message_free_list;
opal_pointer_array_t  ompi_message_f_to_c_table;

ompi_predefined_message_t ompi_message_null;

int
ompi_message_init(void)
{
    int rc;

    OBJ_CONSTRUCT(&ompi_message_free_list, opal_free_list_t);
    rc = opal_free_list_init(&ompi_message_free_list,
                             sizeof(ompi_message_t),
                             OBJ_CLASS(ompi_message_t),
                             8, -1, 8);

    OBJ_CONSTRUCT(&ompi_message_f_to_c_table, opal_pointer_array_t);

    ompi_message_null.message.req_ptr = NULL;
    ompi_message_null.message.count = 0;
    ompi_message_null.message.m_f_to_c_index = 
        opal_pointer_array_add(&ompi_message_f_to_c_table, &ompi_message_null);

    return rc;
}

int
ompi_message_finalize(void)
{
    OBJ_DESTRUCT(&ompi_message_free_list);
    OBJ_DESTRUCT(&ompi_message_f_to_c_table);

    return OMPI_SUCCESS;
}
