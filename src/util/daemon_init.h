/*
 * $HEADER$
 */
/** @file **/

#include "ompi_config.h"

/*
 * Turn a process into a daemon.
 *
 * This function converts a process into a daemon in an orderly manner. It first forks a child process,
 * then the parent exits. The child continues on to become a session leader, reset the file mode creation
 * mask, and changes working directories to the one specified.
 *
 * @param working_dir Pointer to a character string containing the desired working directory. Providing
 * a value of NULL will cause the function to leave the program in the current working directory.
 *
 * @retval OMPI_SUCCESS Indicates that the conversion was successful
 * @retval OMPI_ERROR Indicates that the conversion was not successful - a fork could not be completed.
 */
OMPI_DECLSPEC int ompi_daemon_init(char *working_dir);
