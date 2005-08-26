/*
 * Copyright (c) 2004-2005 The Trustees of Indiana University.
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


#include "ompi_config.h"

#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_LIBGEN_H
#include <libgen.h>
#endif
#include <stdlib.h>

#include "opal/util/os_create_dirpath.h"
#include "opal/util/argv.h"
#include "ompi/include/constants.h"

#ifdef WIN32
#define PATH_SEP "\\"
#else
#define PATH_SEP "/"
#endif

static const char *path_sep = PATH_SEP;


int opal_os_create_dirpath(const char *path, const mode_t mode)
{
    struct stat buf;
    char **parts, *tmp;
    int i, len;

    if (NULL == path) { /* protect ourselves from errors */
	return(OMPI_ERROR);
    }

    if (0 == stat(path, &buf)) { /* already exists */
	if (mode == (mode & buf.st_mode)) { /* has correct mode */
	    return(OMPI_SUCCESS);
	}
	if (0 == chmod(path, (buf.st_mode | mode))) { /* successfully change mode */
	    return(OMPI_SUCCESS);
	}
	return(OMPI_ERROR); /* can't set correct mode */
    }

    /* quick -- try to make directory */
    if (0 == mkdir(path, mode)) {
	return(OMPI_SUCCESS);
    }

    /* didnt work, so now have to build our way down the tree */
    /* Split the requested path up into its individual parts */

    parts = opal_argv_split(path, path_sep[0]);

    /* Ensure to allocate enough space for tmp: the strlen of the
       incoming path + 1 (for \0) */

    tmp = malloc(strlen(path) + 1);
    tmp[0] = '\0';

    /* Iterate through all the subdirectory names in the path,
       building up a directory name.  Check to see if that dirname
       exists.  If it doesn't, create it. */

    /* Notes about stat(): Windows has funny definitions of what will
       return 0 from stat().  "C:" will return failure, while "C:\"
       will return success.  Similarly, "C:\foo" will return success,
       while "C:\foo\" will return failure (assuming that a folder
       named "foo" exists under C:\).

       POSIX implementations of stat() are generally a bit more
       forgiving; most will return true for "/foo" and "/foo/"
       (assuming /foo exists).  But we might as well abide by the same
       rules as Windows and generally disallow checking for names
       ending with path_sep (the only possible allowable one is
       checking for "/", which is the root directory, and is
       guaranteed to exist on valid POSIX filesystems, and is
       therefore not worth checking for). */

    len = opal_argv_count(parts);
    for (i = 0; i < len; ++i) {
        if (i == 0) {

#ifdef WIN32         
            /* In the Windows case, check for "<drive>:" case (i.e.,
               an absolute pathname).  If this is the case, ensure
               that it ends in a path_sep. */

            if (2 == strlen(parts[0]) && isalpha(parts[0][0]) &&
                ':' == parts[0][1]) {
                strcat(tmp, parts[i]);
                strcat(tmp, path_sep);
            }
            
            /* Otherwise, it's a relative path.  Per the comment
               above, we don't want a '\' at the end, so just append
               this part. */

            else {
                strcat(tmp, parts[i]);
            }
#else
            /* If in POSIX-land, ensure that we never end a directory
               name with path_sep */

            if ('/' == path[0]) {
                strcat(tmp, path_sep);
            }
            strcat(tmp, parts[i]);
#endif
        }

        /* If it's not the first part, ensure that there's a
           preceeding path_sep and then append this part */

        else {
            if (path_sep[0] != tmp[strlen(tmp) - 1]) {
                strcat(tmp, path_sep);
            }
            strcat(tmp, parts[i]);
        }

        /* Now that we finally have the name to check, check it.
           Create it if it doesn't exist. */

        if (0 != stat(tmp, &buf)) {
            if (0 != mkdir(tmp, mode) && 0 != stat(tmp, &buf)) { 
                opal_argv_free(parts);
                free(tmp);
                return OMPI_ERROR;
            }
        }
    }

    /* All done */

    opal_argv_free(parts);
    free(tmp);
    return OMPI_SUCCESS;
}
