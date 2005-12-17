/*
 Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
                         University Research and Technology
                         Corporation.  All rights reserved.
 Copyright (c) 2004-2005 The University of Tennessee and The University
                         of Tennessee Research Foundation.  All rights
                         reserved.
 Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
                         University of Stuttgart.  All rights reserved.
 Copyright (c) 2004-2005 The Regents of the University of California.
                         All rights reserved.
 $COPYRIGHT$
 
 Additional copyrights may follow
 
 $HEADER$
 */

#include "ompi_config.h"
#include "opal/win32/ompi_utsname.h"

/*
    This has to fill in the following information

    1. sysname: name of the operating system -- 
    2. nodename: GetComputerName
    3. release: GetVersionEx
    4. version: GetVersionEx
    5. machine: GetSystemInfo
*/
    
OMPI_DECLSPEC int
uname( struct utsname *un )
{

    /* 1. get the OS name */
    TCHAR env_variable[] = "OS=%OS%";
    DWORD info_buf_count;
    OSVERSIONINFO version_info;
    SYSTEM_INFO sys_info;
    TCHAR info_buf[OMPI_UTSNAME_LEN];

    info_buf_count = ExpandEnvironmentStrings( env_variable, info_buf, OMPI_UTSNAME_LEN); 
    if (0 == info_buf_count) {
        return 1;
    }

    /* unfortunately, we need to trim the first three characters from un->sysname */
    sprintf (un->sysname,"%s", info_buf+3);

    info_buf_count = OMPI_UTSNAME_LEN;
    if (!GetComputerName( un->nodename, &info_buf_count)) {
        return 1;
    }
    
    version_info.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    if (!GetVersionEx(&version_info)) {
        return 1;
    } else {
        /* fill in both release and version information */
        sprintf (un->release, "%d.%d.%d", version_info.dwMajorVersion,
                                          version_info.dwMinorVersion,
                                          version_info.dwBuildNumber);
    }

    /* get machine information */
    GetSystemInfo(&sys_info);
    sprintf(un->machine, "%u", sys_info.dwProcessorType);

    /* version : need to ask Jeff */
    sprintf(un->version, "undefined");
    
    return 0;
}
