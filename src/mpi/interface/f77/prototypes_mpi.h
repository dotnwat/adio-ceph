#ifndef LAM_F77_PROTOTYPES_MPI_H
#define LAM_F77_PROTOTYPES_MPI_H

/*
 * $HEADER$
 */

/*
 * This file prototypes all MPI fortran functions in all four fortran
 * symbol conventions as well as all the internal "real" LAM wrapper
 * functions (different from any of the four fortran symbol
 * conventions for clarity, at the cost of more typing for me...).
 * This file is included in the top-level build ONLY. The prototyping
 * is done ONLY for MPI_* bindings
 */

/*
 * Zeroth, the LAM wrapper functions, with a "_f" suffix.
 */
/* This is needed ONLY if the "lower-level prototypes_pmpi.h" has not
 * already been included
 */
#ifndef LAM_F77_PROTOTYPES_PMPI_H
void mpi_alloc_mem_f(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void mpi_comm_get_name_f(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                         MPI_Fint charlen);
void mpi_comm_set_name_f(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void mpi_init_f(MPI_Fint *ierror);
void mpi_finalize_f(MPI_Fint *ierror);
void mpi_free_mem_f(char *baseptr, MPI_Fint *ierr);
#endif

/*
 * First, all caps.
 */
void MPI_ALLOC_MEM(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void MPI_COMM_GET_NAME(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                       MPI_Fint charlen);
void MPI_COMM_SET_NAME(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void MPI_INIT(MPI_Fint *ierror);
void MPI_FINALIZE(MPI_Fint *ierror);
void MPI_FREE_MEM(char *baseptr, MPI_Fint *ierr);

/*
 * Second, all lower case.
 */
void mpi_alloc_mem(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void mpi_comm_get_name(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                       MPI_Fint charlen);
void mpi_comm_set_name(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void mpi_init(MPI_Fint *ierror);
void mpi_finalize(MPI_Fint *ierror);
void mpi_free_mem(char *baseptr, MPI_Fint *ierr);

/*
 * Third, one trailing underscore.
 */
void mpi_alloc_mem_(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void mpi_comm_get_name_(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                        MPI_Fint charlen);
void mpi_comm_set_name_(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void mpi_init_(MPI_Fint *ierror);
void mpi_finalize_(MPI_Fint *ierror);
void mpi_free_mem_(char *baseptr, MPI_Fint *ierr);

/*
 * Fourth, two trailing underscores.
 */
void mpi_alloc_mem__(MPI_Fint *size, MPI_Fint *info, char *baseptr, MPI_Fint *ierr);
void mpi_comm_get_name__(MPI_Fint *comm, char *name, MPI_Fint *l, MPI_Fint *ierror, 
                         MPI_Fint charlen);
void mpi_comm_set_name__(MPI_Fint *comm, char *name, MPI_Fint *ierror, MPI_Fint charlen);
void mpi_init__(MPI_Fint *ierror);
void mpi_finalize__(MPI_Fint *ierror);
void mpi_free_mem__(char *baseptr, MPI_Fint *ierr);

#endif /* PROTOTYPE_H */
