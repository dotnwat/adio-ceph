/* -*- C -*-
 *
 * $HEADER$
 *
 * A program that just spins - provides mechanism for testing user-driven
 * abnormal program termination
 */

#include <stdio.h>

#include "orte/util/proc_info.h"

int main(int argc, char* argv[])
{

    int i, rc;
    double pi;
    pid_t pid;

    if (0 > (rc = orte_init())) {
        fprintf(stderr, "spin: couldn't init orte - error code %d\n", rc);
        return rc;
    }
    pid = getpid();

    printf("spin: Name [%lu,%lu,%lu] Pid %ld\n", ORTE_NAME_ARGS(orte_process_info.my_name), (long)pid);
    
    i = 0;
    while (1) {
        i++;
        pi = i / 3.14159256;
        if (i > 100) i = 0;
    }

    return 0;
}
