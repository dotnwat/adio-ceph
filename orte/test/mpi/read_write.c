/*
 * A simple MPI test that reads lines from standard input and writes them
 * to both standard output and a file
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <mpi.h>

int main(int argc, char *argv[])
{
    int self;
    int size;
    int value;
    char line[1024];
    FILE *file;
    unsigned int bytes = 0;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(MPI_COMM_WORLD, &self);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    printf("Hello from process %d of %d\n", self, size);
    MPI_Barrier(MPI_COMM_WORLD);
    if (0 == self) {
        unlink("./junk");
        file = fopen("./junk", "w+");
        if (NULL == file) {
            fprintf(stderr, "Couldn't open ./junk!");
            MPI_Abort(MPI_COMM_WORLD, 1);
        }
        while (NULL != fgets(line, sizeof(line), stdin)) {
           /* fprintf(stderr, line); */
            fprintf(file, line);
            bytes += strlen(line) + 1;
        }
        fclose(file);
        fprintf(stderr, "\nWrote %d bytes to ./junk\n", bytes);
    }
    MPI_Finalize();

    return 0;
}
