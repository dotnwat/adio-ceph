/*
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
 */

/** @file:
 *
 * Populates global structure with system-specific information.
 *
 * Notes: add limits.h, compute size of integer and other types via sizeof(type)*CHAR_BIT
 *
 */

#include "ompi_config.h"

#ifdef HAVE_SYS_UTSNAME_H
#include <sys/utsname.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef _OMPI_SYS_INFO_H_
#define _OMPI_SYS_INFO_H_

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

/**
 * System information structure
 *
 * The ompi_sys_info() function fills the sysname, nodename, release, version, machine,
 * path_sep, and user fields
 *
 */
struct ompi_sys_info_t {
    bool init;             /**< Certifies that values have been filled.
			    * Certifies that the ompi_sys_info() function has been
			    * called at least once so fields have valid values
			    */
    char *sysname;         /**< Name of OS in use on this node. */
    char *nodename;        /**< Fully qualified host name on the network. */
    char *release;	   /**< Release level of the operating system. */
    char *version;	   /**< Version of the operating system release. */
    char *machine;	   /**< Type of hardware composing this node. */
    char *path_sep;        /**< Path separation char, saved as string.
			    * The character used to separate directories in the path - 
			    * a value that is usually either a '\' or '/', depending
			    * upon the operating system
			    */
    char *user;            /**< User id on this system. */
    char *enviro;          /**< Computing environment employed on this system.
			    * Indicates the local computing environment for managing
			    * and scheduling resources - e.g., SLURM, PBS, LSF, or BProc
			    */
    char *suffix;          /**< Automatic suffix added to file names.
			    * Some computing environments automatically "tag" files
			    * created by applications with a computer-generated suffix
			    * to ensure uniqueness of the file name. This field records
			    * that value for future use.
			    */
};
typedef struct ompi_sys_info_t ompi_sys_info_t;

OMPI_DECLSPEC extern ompi_sys_info_t ompi_system_info;


/**
 * \internal
 *
 * Discover and record a wide range of information about the system
 * upon which this code is executing. ompi_sys_info populates a global
 * variable with information about the system upon which the process
 * is executing.
 *
 * Called from \c ompi_init.
 *
 * @retval OMPI_SUCCESS If values are successfully determined.
 * @retval OMPI_ERROR If the system does not provide the requested information.
 */
OMPI_DECLSPEC int ompi_sys_info(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif
#endif
