/*
 * Copyright (c) 2004-2007 The Trustees of the University of Tennessee.
 *                         All rights reserved.
 * $COPYRIGHT$
 * 
 * Additional copyrights may follow
 * 
 * $HEADER$
 */

#include "opal_config.h"

#include <stdlib.h>
#include <string.h>

#include "opal/mca/installdirs/installdirs.h"

static int installdirs_windows_open(void);

opal_installdirs_base_component_t mca_installdirs_windows_component = {
    /* First, the mca_component_t struct containing meta information
       about the component itself */
    {
        /* Indicate that we are a backtrace v1.0.0 component (which also
           implies a specific MCA version) */
        OPAL_INSTALLDIRS_BASE_VERSION_1_0_0,

        /* Component name and version */
        "windows",
        OPAL_MAJOR_VERSION,
        OPAL_MINOR_VERSION,
        OPAL_RELEASE_VERSION,

        /* Component open and close functions */
        installdirs_windows_open,
        NULL
    },

    /* Next the MCA v1.0.0 component meta data */
    {
        /* Whether the component is checkpointable or not */
        true
    },
};


#define SET_FIELD(KEY, FIELD, ENVNAME)                                                           \
    do {                                                                                         \
        int i;                                                                                   \
        DWORD cbData, valueLength, keyType;                                                      \
        char valueName[1024], vData[1024];                                                       \
        for( i = 0; true; i++) {                                                                 \
            valueLength = 1024;                                                                  \
            valueName[0] = '\0';                                                                 \
            cbData = 1024;                                                                       \
            valueLength = 1024;                                                                  \
            if( ERROR_SUCCESS == RegEnumValue( (KEY), i, valueName, &valueLength,                \
                                               NULL, &keyType, vData, &cbData ) ) {              \
                if( ((REG_EXPAND_SZ == keyType) || (REG_SZ == keyType)) &&                       \
                    (0 == strncasecmp( valueName, (ENVNAME), strlen((ENVNAME)) )) ) {            \
                    mca_installdirs_windows_component.install_dirs_data.FIELD = strdup(vData);   \
                    break;                                                                       \
                }                                                                                \
            } else                                                                               \
                break;                                                                           \
        }                                                                                        \
    } while (0)


static int
installdirs_windows_open(void)
{
    HKEY ompi_key, hKey = HKEY_CURRENT_USER;

    /* The OPAL_PREFIX is the only one which is required to be in the registry.
     * All others can be composed starting from OPAL_PREFIX.
     */
    if( ERROR_SUCCESS != RegOpenKeyEx( hKey, "Software\\Open MPI", 0, KEY_READ, &ompi_key) )
        return 0;

    SET_FIELD(ompi_key, prefix, "OPAL_PREFIX");
    SET_FIELD(ompi_key, exec_prefix, "OPAL_EXEC_PREFIX");
    SET_FIELD(ompi_key, bindir, "OPAL_BINDIR");
    SET_FIELD(ompi_key, sbindir, "OPAL_SBINDIR");
    SET_FIELD(ompi_key, libexecdir, "OPAL_LIBEXECDIR");
    SET_FIELD(ompi_key, datarootdir, "OPAL_DATAROOTDIR");
    SET_FIELD(ompi_key, datadir, "OPAL_DATADIR");
    SET_FIELD(ompi_key, sysconfdir, "OPAL_SYSCONFDIR");
    SET_FIELD(ompi_key, sharedstatedir, "OPAL_SHAREDSTATEDIR");
    SET_FIELD(ompi_key, localstatedir, "OPAL_LOCALSTATEDIR");
    SET_FIELD(ompi_key, libdir, "OPAL_LIBDIR");
    SET_FIELD(ompi_key, includedir, "OPAL_INCLUDEDIR");
    SET_FIELD(ompi_key, infodir, "OPAL_INFODIR");
    SET_FIELD(ompi_key, mandir, "OPAL_MANDIR");
    SET_FIELD(ompi_key, pkgdatadir, "OPAL_PKGDATADIR");
    SET_FIELD(ompi_key, pkglibdir, "OPAL_PKGLIBDIR");
    SET_FIELD(ompi_key, pkgincludedir, "OPAL_PKGINCLUDEDIR");

    RegCloseKey(ompi_key);

    return OPAL_SUCCESS;
}
