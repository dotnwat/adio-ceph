/*
 * $HEADER$
 */

#ifndef OMPI_MPI_H
#define OMPI_MPI_H

#include "ompi_config.h"

/*
 * Just in case you need it.  :-)
 */
#define OMPI_MPI 1

/*
 * MPI version
 */
#define MPI_VERSION 2
#define MPI_SUBVERSION 0

/*
 * To accomodate programs written for MPI implementations that use a
 * straight ROMIO import
 */
#if !defined(OMPI_BUILDING) || !OMPI_BUILDING
#define MPIO_Request MPI_Request
#define MPIO_Test MPI_Test
#define MPIO_Wait MPI_Wait
#endif

/*
 * Typedefs
 */
typedef long MPI_Aint;
typedef struct ompi_communicator_t *MPI_Comm;
typedef struct ompi_datatype_t *MPI_Datatype;
typedef struct ompi_errhandler_t *MPI_Errhandler;
typedef struct ompi_file_t *MPI_File;
typedef struct ompi_group_t *MPI_Group;
typedef struct ompi_info_t *MPI_Info;
typedef struct ompi_op_t *MPI_Op;
typedef struct ompi_request_t *MPI_Request;
typedef struct ompi_status_public_t MPI_Status;
typedef struct ompi_win_t *MPI_Win;

/*
 * MPI_Status
 */
struct ompi_status_public_t { 
  int MPI_SOURCE;
  int MPI_TAG;
  int MPI_ERROR;
  int _count;
};
typedef struct ompi_status_public_t ompi_status_public_t;

/*
 * User typedefs
 */
#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
  typedef int (MPI_Copy_function)(MPI_Comm, int, void *,
                                  void *, void *, int *);
  typedef int (MPI_Delete_function)(MPI_Comm, int, void *, void *);
  typedef int (MPI_Datarep_extent_function)(MPI_Datatype, MPI_Aint *,
                                            void *);
  typedef int (MPI_Datarep_conversion_function)(void *, MPI_Datatype, 
                                                int, void *, MPI_Offset,
                                                void *);
  typedef void (MPI_Comm_errhandler_fn)(MPI_Comm *, int *, ...);
  typedef void (MPI_File_errhandler_fn)(MPI_File *, int *, ...);
  typedef void (MPI_Win_errhandler_fn)(MPI_Win *, int *, ...);
  typedef void (MPI_Handler_function)(MPI_Comm *, int *, ...);
  typedef void (MPI_User_function)(void *, void *, int *, MPI_Datatype *);
  typedef int (MPI_Comm_copy_attr_function)(MPI_Comm, int, void *,
                                            void *, void *, int *);
  typedef int (MPI_Comm_delete_attr_function)(MPI_Comm, int, void *, void *);
  typedef int (MPI_Type_copy_attr_function)(MPI_Datatype, int, void *,
                                            void *, void *, int *);
  typedef int (MPI_Type_delete_attr_function)(MPI_Datatype, int,
                                              void *, void *);
  typedef int (MPI_Win_copy_attr_function)(MPI_Win, int, void *,
                                           void *, void *, int *);
  typedef int (MPI_Win_delete_attr_function)(MPI_Win, int, void *, void *);
  typedef int (MPI_Grequest_query_function)(void *, MPI_Status *);
  typedef int (MPI_Grequest_free_function)(void *);
  typedef int (MPI_Grequest_cancel_function)(void *, int); 
#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

/*
 * Miscellaneous constants
 * JMS: Some of these may be LAM-specific and should be removed...
 */
#define MPI_ANY_SOURCE         -1      /* match any source rank */
#define MPI_PROC_NULL          -2      /* rank of null process */
#define MPI_ROOT               -4
#define MPI_CANCEL_SOURCE      -3      /* successful cancel */
#define MPI_ANY_TAG            -1      /* match any message tag */
#define MPI_GER_TAG            -2      /* used for GER protocol */
#define MPI_MERGE_TAG          -3      /* used to merge inter-comm */
#define MPI_MAX_PROCESSOR_NAME 256     /* max proc. name length */
#define MPI_MAX_ERROR_STRING   256     /* max error message length */
#define MPI_MAX_OBJECT_NAME    64      /* max object name length */
#define MPI_UNDEFINED          -32766  /* undefined stuff */
#define MPI_CART               1       /* cartesian topology */
#define MPI_GRAPH              2       /* graph topology */
#define MPI_KEYVAL_INVALID     -1      /* invalid key value */

/*
 * More constants
 * JMS: Copied straight from LAM -- feel free to change
 * JMS: Some of these are probably LAM-specific and should be deleted
 */
#define MPI_BOTTOM               ((void *) 0)    /* base reference address */
#define MPI_BSEND_OVERHEAD       128     /* size of bsend header + ptr */
#define MPI_MAX_INFO_KEY         36      /* max info key length */
#define MPI_MAX_INFO_VAL         256     /* max info value length */
#define MPI_ARGV_NULL            ((char **) 0)   /* NULL argument vector */
#define MPI_ARGVS_NULL           ((char ***) 0)  /* NULL argument vectors */
#define MPI_ERRCODES_IGNORE      ((void *) 0)    /* don't return error codes */
#define MPI_MAX_PORT_NAME        36      /* max port name length */
#define MPI_MAX_NAME_LEN         MPI_MAX_PORT_NAME /* max port name length */
#define MPI_ORDER_C              0       /* C row major order */
#define MPI_ORDER_FORTRAN        1       /* Fortran column major order */
#define MPI_DISTRIBUTE_BLOCK     0       /* block distribution */
#define MPI_DISTRIBUTE_CYCLIC    1       /* cyclic distribution */
#define MPI_DISTRIBUTE_NONE      2       /* not distributed */
#define MPI_DISTRIBUTE_DFLT_DARG (-1)    /* default distribution arg */
#define MPI_TAG_UB_VALUE (0x7fffffff)

/*
 * Since these values are arbitrary to Open MPI, we might as well make
 * them the same as ROMIO for ease of mapping.  These values taken
 * from ROMIO's mpio.h file.
 */
#define MPI_MODE_CREATE              1  /* ADIO_CREATE */ 
#define MPI_MODE_RDONLY              2  /* ADIO_RDONLY */
#define MPI_MODE_WRONLY              4  /* ADIO_WRONLY  */
#define MPI_MODE_RDWR                8  /* ADIO_RDWR  */
#define MPI_MODE_DELETE_ON_CLOSE    16  /* ADIO_DELETE_ON_CLOSE */
#define MPI_MODE_UNIQUE_OPEN        32  /* ADIO_UNIQUE_OPEN */
#define MPI_MODE_EXCL               64  /* ADIO_EXCL */
#define MPI_MODE_APPEND            128  /* ADIO_APPEND */
#define MPI_MODE_SEQUENTIAL        256  /* ADIO_SEQUENTIAL */

#define MPI_DISPLACEMENT_CURRENT   -54278278

#define MPI_SEEK_SET            600
#define MPI_SEEK_CUR            602
#define MPI_SEEK_END            604

#define MPI_MAX_DATAREP_STRING  128


/*
 * Predefined attribute keyvals
 *
 * DO NOT CHANGE THE ORDER WITHOUT ALSO CHANGING THE ORDER IN
 * src/attribute/attribute_predefined.c.
 */
enum {
    /* MPI-1 */
    MPI_TAG_UB,
    MPI_HOST,
    MPI_IO,
    MPI_WTIME_IS_GLOBAL,

    /* MPI-2 */
    MPI_APPNUM,
    MPI_LASTUSEDCODE,
    MPI_UNIVERSE_SIZE,
    MPI_WIN_BASE,
    MPI_WIN_SIZE,
    MPI_WIN_DISP_UNIT,

    /* Even though these four are IMPI attributes, they need to be there
       for all MPI jobs */
    IMPI_CLIENT_SIZE,
    IMPI_CLIENT_COLOR,
    IMPI_HOST_SIZE,
    IMPI_HOST_COLOR
};

/*
 * Error classes and codes
 */
#define MPI_SUCCESS                   0  
#define MPI_ERR_BUFFER                1
#define MPI_ERR_COUNT                 2
#define MPI_ERR_TYPE                  3
#define MPI_ERR_TAG                   4
#define MPI_ERR_COMM                  5
#define MPI_ERR_RANK                  6
#define MPI_ERR_REQUEST               7
#define MPI_ERR_ROOT                  8
#define MPI_ERR_GROUP                 9
#define MPI_ERR_OP                    10
#define MPI_ERR_TOPOLOGY              11
#define MPI_ERR_DIMS                  12
#define MPI_ERR_ARG                   13
#define MPI_ERR_UNKNOWN               14
#define MPI_ERR_TRUNCATE              15
#define MPI_ERR_OTHER                 16
#define MPI_ERR_INTERN                17
#define MPI_ERR_IN_STATUS             18
#define MPI_ERR_PENDING               19
#define MPI_ERR_ACCESS                20
#define MPI_ERR_AMODE                 21
#define MPI_ERR_ASSERT                22
#define MPI_ERR_BAD_FILE              23
#define MPI_ERR_BASE                  24
#define MPI_ERR_CONVERSION            25
#define MPI_ERR_DISP                  26
#define MPI_ERR_DUP_DATAREP           27
#define MPI_ERR_FILE_EXISTS           28
#define MPI_ERR_FILE_IN_USE           29
#define MPI_ERR_FILE                  30
#define MPI_ERR_INFO_KEY              31
#define MPI_ERR_INFO_NOKEY            32
#define MPI_ERR_INFO_VALUE            33
#define MPI_ERR_INFO                  34
#define MPI_ERR_IO                    35
#define MPI_ERR_KEYVAL                36
#define MPI_ERR_LOCKTYPE              37
#define MPI_ERR_NAME                  38
#define MPI_ERR_NO_MEM                39
#define MPI_ERR_NOT_SAME              40
#define MPI_ERR_NO_SPACE              41
#define MPI_ERR_NO_SUCH_FILE          42
#define MPI_ERR_PORT                  43
#define MPI_ERR_QUOTA                 44
#define MPI_ERR_READ_ONLY             45
#define MPI_ERR_RMA_CONFLICT          46
#define MPI_ERR_RMA_SYNC              47
#define MPI_ERR_SERVICE               48
#define MPI_ERR_SIZE                  49
#define MPI_ERR_SPAWN                 50
#define MPI_ERR_UNSUPPORTED_DATAREP   51
#define MPI_ERR_UNSUPPORTED_OPERATION 52
#define MPI_ERR_WIN                   53
#define MPI_ERR_LASTCODE              54

#define MPI_ERR_SYSRESOURCE          -2


/*
 * Comparison results.  Don't change the order of these, the group
 * comparison functions rely on it.
 */
enum {
  MPI_IDENT,
  MPI_CONGRUENT,
  MPI_SIMILAR,
  MPI_UNEQUAL
};

/*
 * MPI_Init_thread constants
 */
enum {
  MPI_THREAD_SINGLE,
  MPI_THREAD_FUNNELED,
  MPI_THREAD_SERIALIZED,
  MPI_THREAD_MULTIPLE
};

/*
 * Datatype combiners.
 */
enum {
  MPI_COMBINER_NAMED,
  MPI_COMBINER_DUP,
  MPI_COMBINER_CONTIGUOUS,
  MPI_COMBINER_VECTOR,
  MPI_COMBINER_HVECTOR_INTEGER,
  MPI_COMBINER_HVECTOR,
  MPI_COMBINER_INDEXED,
  MPI_COMBINER_HINDEXED_INTEGER,
  MPI_COMBINER_HINDEXED,
  MPI_COMBINER_INDEXED_BLOCK,
  MPI_COMBINER_STRUCT_INTEGER,
  MPI_COMBINER_STRUCT,
  MPI_COMBINER_SUBARRAY,
  MPI_COMBINER_DARRAY,
  MPI_COMBINER_F90_REAL,
  MPI_COMBINER_F90_COMPLEX,
  MPI_COMBINER_F90_INTEGER,
  MPI_COMBINER_RESIZED
};

/*
 * NULL handles
 */
#define MPI_GROUP_NULL (&ompi_mpi_group_null)
#define MPI_COMM_NULL (&ompi_mpi_comm_null)
#define MPI_REQUEST_NULL (&ompi_request_null)
#define MPI_OP_NULL (&ompi_mpi_op_null)
#define MPI_ERRHANDLER_NULL (&ompi_mpi_errhandler_null)
#define MPI_INFO_NULL (&ompi_mpi_info_null)
#define MPI_WIN_NULL ((MPI_Win) 0)
#define MPI_FILE_NULL (&ompi_mpi_file_null)

#define MPI_STATUS_IGNORE ((MPI_Status *) 0)
#define MPI_STATUSES_IGNORE ((MPI_Status *) 0)

/* MPI-2 specifies that the name "MPI_TYPE_NULL_DELETE_FN" (and all
   related friends) must be accessible in C, C++, and Fortran. This is
   unworkable if the back-end Fortran compiler uses all caps for its
   linker symbol convention -- it results in two functions with
   different signatures that have the same name (i.e., both C and
   Fortran use the symbol MPI_TYPE_NULL_DELETE_FN).  So we have to
   #define the C names to be something else, so that they names are
   *accessed* through MPI_TYPE_NULL_DELETE_FN, but their actual symbol
   name is different.

   However, this file is included when the fortran wrapper functions
   are compiled in Open MPI, so we do *not* want these #defines in
   this case (i.e., we need the Fortran wrapper function to be
   compiled as MPI_TYPE_NULL_DELETE_FN).  So add some #if kinds of
   protection for this case. */

#if !defined(OMPI_COMPILING_F77_WRAPPERS)
#define MPI_NULL_DELETE_FN OMPI_C_MPI_NULL_DELETE_FN
#define MPI_NULL_COPY_FN OMPI_C_MPI_NULL_COPY_FN
#define MPI_DUP_FN OMPI_C_MPI_DUP_FN

#define MPI_TYPE_NULL_DELETE_FN OMPI_C_MPI_TYPE_NULL_DELETE_FN
#define MPI_TYPE_NULL_COPY_FN OMPI_C_MPI_TYPE_NULL_COPY_FN
#define MPI_TYPE_DUP_FN OMPI_C_MPI_TYPE_DUP_FN

#define MPI_COMM_NULL_DELETE_FN OMPI_C_MPI_COMM_NULL_DELETE_FN
#define MPI_COMM_NULL_COPY_FN OMPI_C_MPI_COMM_NULL_COPY_FN
#define MPI_COMM_DUP_FN OMPI_C_MPI_COMM_DUP_FN

#define MPI_WIN_NULL_DELETE_FN OMPI_C_MPI_WIN_NULL_DELETE_FN
#define MPI_WIN_NULL_COPY_FN OMPI_C_MPI_WIN_NULL_COPY_FN
#define MPI_WIN_DUP_FN OMPI_C_MPI_WIN_DUP_FN
#endif

#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif
int OMPI_C_MPI_TYPE_NULL_DELETE_FN( MPI_Datatype datatype, int type_keyval,
                                    void* attribute_val_out, void* flag );
int OMPI_C_MPI_TYPE_NULL_COPY_FN( MPI_Datatype datatype, int type_keyval, 
                                  void* extra_state,
                                  void* attribute_val_in, 
                                  void* attribute_val_out, int* flag );
int OMPI_C_MPI_TYPE_DUP_FN( MPI_Datatype datatype, int type_keyval, 
                            void* extra_state, void* attribute_val_in, 
                            void* attribute_val_out, int* flag );
int OMPI_C_MPI_COMM_NULL_DELETE_FN( MPI_Comm comm, int comm_keyval,
                                    void* attribute_val_out, void* flag );
int OMPI_C_MPI_COMM_NULL_COPY_FN( MPI_Comm comm, int comm_keyval, 
                                  void* extra_state, void* attribute_val_in,
                                  void* attribute_val_out, int* flag );
int OMPI_C_MPI_COMM_DUP_FN( MPI_Comm comm, int comm_keyval, void* extra_state,
                            void* attribute_val_in, void* attribute_val_out,
                            int* flag );
int OMPI_C_MPI_NULL_DELETE_FN( MPI_Comm comm, int comm_keyval,
                               void* attribute_val_out, void* flag );
int OMPI_C_MPI_NULL_COPY_FN( MPI_Comm comm, int comm_keyval, void* extra_state,
                             void* attribute_val_in, void* attribute_val_out,
                             int* flag );
int OMPI_C_MPI_DUP_FN( MPI_Comm comm, int comm_keyval, void* extra_state,
                       void* attribute_val_in, void* attribute_val_out,
                       int* flag );
int OMPI_C_MPI_WIN_NULL_DELETE_FN( MPI_Win window, int win_keyval,
                                   void* attribute_val_out, void* flag );
int OMPI_C_MPI_WIN_NULL_COPY_FN( MPI_Win window, int win_keyval, 
                                 void* extra_state, void* attribute_val_in,
                                 void* attribute_val_out, int* flag );
int OMPI_C_MPI_WIN_DUP_FN( MPI_Win window, int win_keyval, void* extra_state,
                           void* attribute_val_in, void* attribute_val_out,
                           int* flag );

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

/*
 * External variables
 */
extern struct ompi_communicator_t ompi_mpi_comm_world;
extern struct ompi_communicator_t ompi_mpi_comm_self;
extern struct ompi_communicator_t ompi_mpi_comm_null;

extern struct ompi_group_t ompi_mpi_group_empty;
extern struct ompi_group_t ompi_mpi_group_null;

extern struct ompi_request_t ompi_request_null;

extern struct ompi_op_t ompi_mpi_op_null;
extern struct ompi_op_t ompi_mpi_op_max, ompi_mpi_op_min;
extern struct ompi_op_t ompi_mpi_op_sum, ompi_mpi_op_prod;
extern struct ompi_op_t ompi_mpi_op_land, ompi_mpi_op_band;
extern struct ompi_op_t ompi_mpi_op_lor, ompi_mpi_op_bor;
extern struct ompi_op_t ompi_mpi_op_lxor, ompi_mpi_op_bxor;
extern struct ompi_op_t ompi_mpi_op_maxloc, ompi_mpi_op_minloc;
extern struct ompi_op_t ompi_mpi_op_replace;

extern struct ompi_datatype_t ompi_mpi_char, ompi_mpi_byte;
extern struct ompi_datatype_t ompi_mpi_int, ompi_mpi_logic;
extern struct ompi_datatype_t ompi_mpi_short, ompi_mpi_long;
extern struct ompi_datatype_t ompi_mpi_float, ompi_mpi_double;
extern struct ompi_datatype_t ompi_mpi_long_double;
extern struct ompi_datatype_t ompi_mpi_cplex, ompi_mpi_packed;
extern struct ompi_datatype_t ompi_mpi_unsigned_char;
extern struct ompi_datatype_t ompi_mpi_unsigned_short;
extern struct ompi_datatype_t ompi_mpi_unsigned, ompi_mpi_datatype_null;
extern struct ompi_datatype_t ompi_mpi_unsigned_long;
extern struct ompi_datatype_t ompi_mpi_ub, ompi_mpi_lb;
extern struct ompi_datatype_t ompi_mpi_float_int, ompi_mpi_double_int;
extern struct ompi_datatype_t ompi_mpi_long_int, ompi_mpi_2int;
extern struct ompi_datatype_t ompi_mpi_short_int, ompi_mpi_dblcplex;
extern struct ompi_datatype_t ompi_mpi_integer, ompi_mpi_real;
extern struct ompi_datatype_t ompi_mpi_dblprec, ompi_mpi_character;
extern struct ompi_datatype_t ompi_mpi_2real, ompi_mpi_2dblprec;
extern struct ompi_datatype_t ompi_mpi_2integer, ompi_mpi_longdbl_int;
extern struct ompi_datatype_t ompi_mpi_wchar, ompi_mpi_long_long_int;
extern struct ompi_datatype_t ompi_mpi_long_long, ompi_mpi_unsigned_long_long;
extern struct ompi_datatype_t ompi_mpi_cxx_cplex, ompi_mpi_cxx_dblcplex;
extern struct ompi_datatype_t ompi_mpi_cxx_ldblcplex;
extern struct ompi_datatype_t ompi_mpi_cxx_bool;
extern struct ompi_datatype_t ompi_mpi_2cplex, ompi_mpi_2dblcplex;
/* other MPI2 datatype */
extern struct ompi_datatype_t ompi_mpi_real4, ompi_mpi_real8;
extern struct ompi_datatype_t ompi_mpi_real16;
extern struct ompi_datatype_t ompi_mpi_complex8, ompi_mpi_complex16;
extern struct ompi_datatype_t ompi_mpi_real32;
extern struct ompi_datatype_t ompi_mpi_integer1, ompi_mpi_integer2, ompi_mpi_integer4;
extern struct ompi_datatype_t ompi_mpi_integer16;

extern struct ompi_errhandler_t ompi_mpi_errhandler_null;
extern struct ompi_errhandler_t ompi_mpi_errors_are_fatal;
extern struct ompi_errhandler_t ompi_mpi_errors_return;

extern struct ompi_file_t ompi_mpi_file_null;

extern struct ompi_info_t ompi_mpi_info_null;

extern MPI_Fint *MPI_F_STATUS_IGNORE;
extern MPI_Fint *MPI_F_STATUSES_IGNORE;

/*
 * MPI predefined handles
 */
#define MPI_COMM_WORLD (&ompi_mpi_comm_world)
#define MPI_COMM_SELF (&ompi_mpi_comm_self)

#define MPI_GROUP_EMPTY (&ompi_mpi_group_empty)

#define MPI_MAX (&ompi_mpi_op_max)
#define MPI_MIN (&ompi_mpi_op_min)
#define MPI_SUM (&ompi_mpi_op_sum)
#define MPI_PROD (&ompi_mpi_op_prod)
#define MPI_LAND (&ompi_mpi_op_land)
#define MPI_BAND (&ompi_mpi_op_band)
#define MPI_LOR (&ompi_mpi_op_lor)
#define MPI_BOR (&ompi_mpi_op_bor)
#define MPI_LXOR (&ompi_mpi_op_lxor)
#define MPI_BXOR (&ompi_mpi_op_bxor)
#define MPI_MAXLOC (&ompi_mpi_op_maxloc)
#define MPI_MINLOC (&ompi_mpi_op_minloc)
#define MPI_REPLACE (&ompi_mpi_op_replace)

#define MPI_DATATYPE_NULL (&ompi_mpi_datatype_null)
#define MPI_BYTE (&ompi_mpi_byte)
#define MPI_PACKED (&ompi_mpi_packed)
#define MPI_CHAR (&ompi_mpi_char)
#define MPI_SHORT (&ompi_mpi_short)
#define MPI_INT (&ompi_mpi_int)
#define MPI_LONG (&ompi_mpi_long)
#define MPI_FLOAT (&ompi_mpi_float)
#define MPI_DOUBLE (&ompi_mpi_double)
#define MPI_LONG_DOUBLE (&ompi_mpi_long_double)
#define MPI_UNSIGNED_CHAR (&ompi_mpi_unsigned_char)
#define MPI_UNSIGNED_SHORT (&ompi_mpi_unsigned_short)
#define MPI_UNSIGNED_LONG (&ompi_mpi_unsigned_long)
#define MPI_UNSIGNED (&ompi_mpi_unsigned)
#define MPI_FLOAT_INT (&ompi_mpi_float_int)
#define MPI_DOUBLE_INT (&ompi_mpi_double_int)
#define MPI_LONG_DOUBLE_INT (&ompi_mpi_longdbl_int)
#define MPI_LONG_INT (&ompi_mpi_long_int)
#define MPI_SHORT_INT (&ompi_mpi_short_int)
#define MPI_2INT (&ompi_mpi_2int)
#define MPI_UB (&ompi_mpi_ub)
#define MPI_LB (&ompi_mpi_lb)
#define MPI_WCHAR (&ompi_mpi_wchar)
#if HAVE_LONG_LONG
#define MPI_LONG_LONG_INT (&ompi_mpi_long_long_int)
#define MPI_LONG_LONG (&ompi_mpi_long_long)
#define MPI_UNSIGNED_LONG_LONG (&ompi_mpi_unsigned_long_long)
#endif  /* HAVE_LONG_LONG */

/* Fortran datatype bindings */
#define MPI_INTEGER1 (&ompi_mpi_integer1)
#define MPI_INTEGER2 (&ompi_mpi_integer2)
#define MPI_INTEGER4 (&ompi_mpi_integer4)
#if HAVE_LONG_LONG
#define MPI_INTEGER8 (&ompi_mpi_integr8)
#endif  /* HAVE_LONG_LONG */
#define MPI_REAL (&ompi_mpi_real)
#define MPI_REAL4 (&ompi_mpi_real4)
#define MPI_REAL8 (&ompi_mpi_real8)
#if HAVE_LONG_DOUBLE
#define MPI_REAL16 (&ompi_mpi_real16)
#endif  /* HAVE_LONG_DOUBLE */
#define MPI_COMPLEX8 (&ompi_mpi_complex8)
#define MPI_COMPLEX16 (&ompi_mpi_complex16)
#if HAVE_LONG_DOUBLE
#define MPI_COMPLEX32 (&ompi_mpi_complex32)
#endif  /* HAVE_LONG_DOUBLE */

#define MPI_ERRORS_ARE_FATAL (&ompi_mpi_errors_are_fatal)
#define MPI_ERRORS_RETURN (&ompi_mpi_errors_return)


#if defined(c_plusplus) || defined(__cplusplus)
extern "C" {
#endif

  /*
   * MPI API
   */
  /* JMS: Look for missing functions (e.g., MPI_File_*) */

  int MPI_Abort(MPI_Comm comm, int errorcode);
  int MPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                   int target_rank, MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win); 
  int MPI_Add_error_class(int *errorclass);
  int MPI_Add_error_code(int errorclass, int *errorcode);
  int MPI_Add_error_string(int errorcode, char *string);
  int MPI_Address(void *location, MPI_Aint *address);
  int MPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                    void *recvbuf, int recvcount, 
                    MPI_Datatype recvtype, MPI_Comm comm);
  int MPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                     void *recvbuf, int *recvcounts, 
                     int *displs, MPI_Datatype recvtype, MPI_Comm comm);
  int MPI_Alloc_mem(MPI_Aint size, MPI_Info info, 
                    void *baseptr);
  int MPI_Allreduce(void *sendbuf, void *recvbuf, int count, 
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm); 
  int MPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   void *recvbuf, int recvcount, 
                   MPI_Datatype recvtype, MPI_Comm comm);
  int MPI_Alltoallv(void *sendbuf, int *sendcounts, int *sdispls, 
                    MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                    int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
  int MPI_Alltoallw(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtypes, 
                    void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtypes,
                    MPI_Comm comm);
  int MPI_Attr_delete(MPI_Comm comm, int keyval);
  int MPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
  int MPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
  int MPI_Barrier(MPI_Comm comm);
  int MPI_Bcast(void *buffer, int count, MPI_Datatype datatype, 
                int root, MPI_Comm comm);
  int MPI_Bsend(void *buf, int count, MPI_Datatype datatype, 
                int dest, int tag, MPI_Comm comm);
  int MPI_Bsend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, MPI_Request *request); 
  int MPI_Buffer_attach(void *buffer, int size);
  int MPI_Buffer_detach(void *buffer, int *size);
  int MPI_Cancel(MPI_Request *request);
  int MPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords);
  int MPI_Cart_create(MPI_Comm old_comm, int ndims, int *dims, 
                      int *periods, int reorder, MPI_Comm *comm_cart);
  int MPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, 
                   int *periods, int *coords);
  int MPI_Cart_map(MPI_Comm comm, int ndims, int *dims, 
                   int *periods, int *newrank);
  int MPI_Cart_rank(MPI_Comm comm, int *coords, int *rank);
  int MPI_Cart_shift(MPI_Comm comm, int direction, int disp, 
                     int *rank_source, int *rank_dest);
  int MPI_Cart_sub(MPI_Comm comm, int *remain_dims, MPI_Comm *new_comm);
  int MPI_Cartdim_get(MPI_Comm comm, int *ndims);
  int MPI_Close_port(char *port_name);
  int MPI_Comm_accept(char *port_name, MPI_Info info, int root, 
                      MPI_Comm comm, MPI_Comm *newcomm);
  MPI_Fint MPI_Comm_c2f(MPI_Comm comm);
  int MPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
  int MPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
  int MPI_Comm_connect(char *port_name, MPI_Info info, int root, 
                       MPI_Comm comm, MPI_Comm *newcomm);
  int MPI_Comm_create_errhandler(MPI_Comm_errhandler_fn *function, 
                                 MPI_Errhandler *errhandler);
  int MPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
                             MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
                             int *comm_keyval, void *extra_state);
  int MPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
  int MPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);
  int MPI_Comm_disconnect(MPI_Comm *comm);
  int MPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
  MPI_Comm MPI_Comm_f2c(MPI_Fint comm);
  int MPI_Comm_free_keyval(int *comm_keyval);
  int MPI_Comm_free(MPI_Comm *comm);
  int MPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, 
                        void *attribute_val, int *flag);
  int MPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *erhandler);
  int MPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
  int MPI_Comm_get_parent(MPI_Comm *parent);
  int MPI_Comm_group(MPI_Comm comm, MPI_Group *group);
  int MPI_Comm_join(int fd, MPI_Comm *intercomm);
  int MPI_Comm_rank(MPI_Comm comm, int *rank);
  int MPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);
  int MPI_Comm_remote_size(MPI_Comm comm, int *size);
  int MPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
  int MPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
  int MPI_Comm_set_name(MPI_Comm comm, char *comm_name);
  int MPI_Comm_size(MPI_Comm comm, int *size);
  int MPI_Comm_spawn(char *command, char **argv, int maxprocs, MPI_Info info, 
                     int root, MPI_Comm comm, MPI_Comm *intercomm, 
                     int *array_of_errcodes);
  int MPI_Comm_spawn_multiple(int count, char **array_of_commands, char ***array_of_argv, 
                              int *array_of_maxprocs, MPI_Info *array_of_info, 
                              int root, MPI_Comm comm, MPI_Comm *intercomm, 
                              int *array_of_errcodes);
  int MPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
  int MPI_Comm_test_inter(MPI_Comm comm, int *flag);
  int MPI_Dims_create(int nnodes, int ndims, int *dims);
  MPI_Fint MPI_Errhandler_c2f(MPI_Errhandler errhandler);
  int MPI_Errhandler_create(MPI_Handler_function *function, 
                                MPI_Errhandler *errhandler);
  MPI_Errhandler MPI_Errhandler_f2c(MPI_Fint errhandler);
  int MPI_Errhandler_free(MPI_Errhandler *errhandler);
  int MPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
  int MPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
  int MPI_Error_class(int errorcode, int *errorclass);
  int MPI_Error_string(int errorcode, char *string, int *resultlen);
  int MPI_Exscan(void *sendbuf, void *recvbuf, int count, 
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  MPI_Fint MPI_File_c2f(MPI_File file);
  MPI_File MPI_File_f2c(MPI_Fint file);
  int MPI_File_call_errhandler(MPI_File fh, int errorcode);
  int MPI_File_create_errhandler(MPI_File_errhandler_fn *function,
                        MPI_Errhandler *errhandler);
  int MPI_File_set_errhandler( MPI_File file, MPI_Errhandler errhandler);
  int MPI_File_get_errhandler( MPI_File file, MPI_Errhandler *errhandler);
  int MPI_File_open(MPI_Comm comm, char *filename, int amode,
                                    MPI_Info info, MPI_File *fh);
  int MPI_File_close(MPI_File *fh);
  int MPI_File_delete(char *filename, MPI_Info info);
  int MPI_File_set_size(MPI_File fh, MPI_Offset size);
  int MPI_File_preallocate(MPI_File fh, MPI_Offset size);
  int MPI_File_get_size(MPI_File fh, MPI_Offset *size);
  int MPI_File_get_group(MPI_File fh, MPI_Group *group);
  int MPI_File_get_amode(MPI_File fh, int *amode);
  int MPI_File_set_info(MPI_File fh, MPI_Info info);
  int MPI_File_get_info(MPI_File fh, MPI_Info *info_used);
  int MPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                       MPI_Datatype filetype, char *datarep, MPI_Info info);
  int MPI_File_get_view(MPI_File fh, MPI_Offset *disp,
                           MPI_Datatype *etype, 
                           MPI_Datatype *filetype, char *datarep);
  int MPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf,
                        int count, MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_write_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_write_at_all(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Request *request);
  int MPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Request *request);
  int MPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Status *status);
  int MPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Status *status);
  int MPI_File_write(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Status *status);
  int MPI_File_write_all(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Status *status);
  int MPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Request *request);
  int MPI_File_iwrite(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Request *request);
  int MPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);
  int MPI_File_get_position(MPI_File fh, MPI_Offset *offset);
  int MPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset,
                            MPI_Offset *disp);
  int MPI_File_read_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_write_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_iread_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Request *request);
  int MPI_File_iwrite_shared(MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype, MPI_Request *request);
  int MPI_File_read_ordered(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_write_ordered(MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype, MPI_Status *status);
  int MPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence);
  int MPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset);
  int MPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                                         int count, MPI_Datatype datatype);
  int MPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                                          int count, MPI_Datatype datatype);
  int MPI_File_write_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_read_all_begin(MPI_File fh, void *buf, int count,
                                      MPI_Datatype datatype);
  int MPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_write_all_begin(MPI_File fh, void *buf, int count,
                                       MPI_Datatype datatype);
  int MPI_File_write_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_read_ordered_begin(MPI_File fh, void *buf, int count,
                                          MPI_Datatype datatype);
  int MPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_write_ordered_begin(MPI_File fh, void *buf, int count,
                                           MPI_Datatype datatype);
  int MPI_File_write_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
  int MPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype,
                                               MPI_Aint *extent);
  int MPI_File_set_atomicity(MPI_File fh, int flag);
  int MPI_File_get_atomicity(MPI_File fh, int *flag);
  int MPI_File_sync(MPI_File fh);
  /*
   * file functions end
   */
  int MPI_Finalize(void);
  int MPI_Finalized(int *flag);
  int MPI_Free_mem(void *base);
  int MPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                 int root, MPI_Comm comm);
  int MPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int *recvcounts, int *displs, 
                  MPI_Datatype recvtype, int root, MPI_Comm comm);
  int MPI_Get_address(void *location, MPI_Aint *address);
  int MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count);
  int MPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, 
                       int *count);
  int MPI_Get(void *origin_addr, int origin_count, 
              MPI_Datatype origin_datatype, int target_rank, 
              MPI_Aint target_disp, int target_count, 
              MPI_Datatype target_datatype, MPI_Win win);
  int MPI_Get_processor_name(char *name, int *resultlen);
  int MPI_Get_version(int *version, int *subversion);
  int MPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, 
                      int *edges, int reorder, MPI_Comm *comm_graph);
  int MPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, 
                    int *index, int *edges);
  int MPI_Graph_map(MPI_Comm comm, int nnodes, int *index, int *edges, 
                    int *newrank);
  int MPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
  int MPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, 
                          int *neighbors);
  int MPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
  int MPI_Grequest_complete(MPI_Request request);
  int MPI_Grequest_start(MPI_Grequest_query_function *query_fn,
                         MPI_Grequest_free_function *free_fn,
                         MPI_Grequest_cancel_function *cancel_fn,
                         void *extra_state, MPI_Request *request);
  MPI_Fint MPI_Group_c2f(MPI_Group group);
  int MPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
  int MPI_Group_difference(MPI_Group group1, MPI_Group group2, 
                           MPI_Group *newgroup);
  int MPI_Group_excl(MPI_Group group, int n, int *ranks, 
                          MPI_Group *newgroup);
  MPI_Group MPI_Group_f2c(MPI_Fint group);
  int MPI_Group_free(MPI_Group *group);
  int MPI_Group_incl(MPI_Group group, int n, int *ranks, 
                          MPI_Group *newgroup);
  int MPI_Group_intersection(MPI_Group group1, MPI_Group group2, 
                             MPI_Group *newgroup);
  int MPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
                           MPI_Group *newgroup);
  int MPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], 
                           MPI_Group *newgroup);
  int MPI_Group_rank(MPI_Group group, int *rank);
  int MPI_Group_size(MPI_Group group, int *size);
  int MPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, 
                                MPI_Group group2, int *ranks2);
  int MPI_Group_union(MPI_Group group1, MPI_Group group2, 
                      MPI_Group *newgroup);
  int MPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  MPI_Fint MPI_Info_c2f(MPI_Info info);
  int MPI_Info_create(MPI_Info *info);
  int MPI_Info_delete(MPI_Info info, char *key);
  int MPI_Info_dup(MPI_Info info, MPI_Info *newinfo);
  MPI_Info MPI_Info_f2c(MPI_Fint info);
  int MPI_Info_free(MPI_Info *info);
  int MPI_Info_get(MPI_Info info, char *key, int valuelen, 
                   char *value, int *flag);
  int MPI_Info_get_nkeys(MPI_Info info, int *nkeys);
  int MPI_Info_get_nthkey(MPI_Info info, int n, char *key);
  int MPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, 
                            int *flag);
  int MPI_Info_set(MPI_Info info, char *key, char *value);
  int MPI_Init(int *argc, char ***argv);
  int MPI_Initialized(int *flag);
  int MPI_Init_thread(int *argc, char ***argv, int required, 
                      int *provided);
  int MPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
                           MPI_Comm bridge_comm, int remote_leader, 
                           int tag, MPI_Comm *newintercomm);
  int MPI_Intercomm_merge(MPI_Comm intercomm, int high, 
                          MPI_Comm *newintercomm);
  int MPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
                 MPI_Status *status);
  int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, 
                int tag, MPI_Comm comm, MPI_Request *request);
  int MPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int MPI_Is_thread_main(int *flag);
  int MPI_Keyval_create(MPI_Copy_function *copy_fn, 
                        MPI_Delete_function *delete_fn, 
                        int *keyval, void *extra_state);
  int MPI_Keyval_free(int *keyval);
  int MPI_Lookup_name(char *service_name, MPI_Info info, char *port_name);
  MPI_Fint MPI_Op_c2f(MPI_Op op); 
  int MPI_Op_create(MPI_User_function *function, int commute, 
                    MPI_Op *op);
  int MPI_Open_port(MPI_Info info, char *port_name);
  MPI_Op MPI_Op_f2c(MPI_Fint op);
  int MPI_Op_free(MPI_Op *op);
  int MPI_Pack_external(char *datarep, void *inbuf, int incount,
                        MPI_Datatype datatype, void *outbuf,
                        MPI_Aint outsize, MPI_Aint *position);
  int MPI_Pack_external_size(char *datarep, int incount, 
                             MPI_Datatype datatype, MPI_Aint *size);
  int MPI_Pack(void *inbuf, int incount, MPI_Datatype datatype, 
               void *outbuf, int outsize, int *position, MPI_Comm comm);
  int MPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, 
                    int *size);
  int MPI_Pcontrol(const int level, ...);
  int MPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
  int MPI_Publish_name(char *service_name, MPI_Info info, 
                       char *port_name);
  int MPI_Put(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, 
              int target_rank, MPI_Aint target_disp, int target_count, 
              MPI_Datatype target_datatype, MPI_Win win);
  int MPI_Query_thread(int *provided);
  int MPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
                    int tag, MPI_Comm comm, MPI_Request *request);
  int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, 
               int tag, MPI_Comm comm, MPI_Status *status);
  int MPI_Reduce(void *sendbuf, void *recvbuf, int count, 
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
  int MPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, 
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  int MPI_Register_datarep(char *datarep, 
                       MPI_Datarep_conversion_function *read_conversion_fn,
                       MPI_Datarep_conversion_function *write_conversion_fn,
                       MPI_Datarep_extent_function *dtype_file_extent_fn,
                       void *extra_state);
  MPI_Fint MPI_Request_c2f(MPI_Request request);
  MPI_Request MPI_Request_f2c(MPI_Fint request);
  int MPI_Request_free(MPI_Request *request);
  int MPI_Request_get_status(MPI_Request request, int *flag, 
                             MPI_Status *status);
  int MPI_Rsend(void *ibuf, int count, MPI_Datatype datatype, int dest, 
                int tag, MPI_Comm comm);
  int MPI_Rsend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, 
                     MPI_Request *request);
  int MPI_Scan(void *sendbuf, void *recvbuf, int count, 
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  int MPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                  int root, MPI_Comm comm);
  int MPI_Scatterv(void *sendbuf, int *sendcounts, int *displs, 
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, 
                   MPI_Datatype recvtype, int root, MPI_Comm comm);
  int MPI_Send_init(void *buf, int count, MPI_Datatype datatype, 
                    int dest, int tag, MPI_Comm comm, 
                    MPI_Request *request);
  int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
               int tag, MPI_Comm comm);
  int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   int dest, int sendtag, void *recvbuf, int recvcount,
                   MPI_Datatype recvtype, int source, int recvtag, 
                   MPI_Comm comm,  MPI_Status *status);
  int MPI_Sendrecv_replace(void * buf, int count, MPI_Datatype datatype, 
                           int dest, int sendtag, int source, int recvtag,
                           MPI_Comm comm, MPI_Status *status);
  int MPI_Ssend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, 
                     MPI_Request *request);
  int MPI_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, 
                int tag, MPI_Comm comm);
  int MPI_Start(MPI_Request *request);
  int MPI_Startall(int count, MPI_Request *array_of_requests);
  int MPI_Status_c2f(MPI_Status *c_status, MPI_Fint *f_status);
  int MPI_Status_f2c(MPI_Fint *f_status, MPI_Status *c_status);
  int MPI_Status_set_cancelled(MPI_Status *status, int flag);
  int MPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype,
                              int count);
  int MPI_Testall(int count, MPI_Request array_of_requests[], int *flag, 
                  MPI_Status array_of_statuses[]);
  int MPI_Testany(int count, MPI_Request array_of_requests[], int *index, 
                  int *flag, MPI_Status *status);
  int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
  int MPI_Test_cancelled(MPI_Status *status, int *flag);
  int MPI_Testsome(int incount, MPI_Request array_of_requests[], 
                   int *outcount, int array_of_indices[], 
                   MPI_Status array_of_statuses[]);
  int MPI_Topo_test(MPI_Comm comm, int *status);
  MPI_Fint MPI_Type_c2f(MPI_Datatype datatype);
  int MPI_Type_commit(MPI_Datatype *type);
  int MPI_Type_contiguous(int count, MPI_Datatype oldtype, 
                          MPI_Datatype *newtype);
  int MPI_Type_create_darray(int size, int rank, int ndims, 
                             int gsize_array[], int distrib_array[], 
                             int darg_array[], int psize_array[],
                             int order, MPI_Datatype oldtype, 
                             MPI_Datatype *newtype);
  int MPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);
  int MPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);
  int MPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);
  int MPI_Type_create_hindexed(int count, int array_of_blocklengths[], 
                               MPI_Aint array_of_displacements[], 
                               MPI_Datatype oldtype, 
                               MPI_Datatype *newtype);
  int MPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, 
                              MPI_Datatype oldtype, 
                              MPI_Datatype *newtype);
  int MPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, 
                             MPI_Type_delete_attr_function *type_delete_attr_fn, 
                             int *type_keyval, void *extra_state);
  int MPI_Type_create_indexed_block(int count, int blocklength,
                                  int array_of_displacements[],
                                  MPI_Datatype oldtype,
                                  MPI_Datatype *newtype);
  int MPI_Type_create_struct(int count, int array_of_block_lengths[], 
                             MPI_Aint array_of_displacements[], 
                             MPI_Datatype array_of_types[], 
                             MPI_Datatype *newtype);
  int MPI_Type_create_subarray(int ndims, int size_array[], int subsize_array[], 
                               int start_array[], int order, 
                               MPI_Datatype oldtype, MPI_Datatype *newtype);
  int MPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, 
                               MPI_Aint extent, MPI_Datatype *newtype); 
  int MPI_Type_delete_attr(MPI_Datatype type, int type_keyval);
  int MPI_Type_dup(MPI_Datatype type, MPI_Datatype *newtype);
  int MPI_Type_extent(MPI_Datatype type, MPI_Aint *extent);
  int MPI_Type_free(MPI_Datatype *type);
  int MPI_Type_free_keyval(int *type_keyval);
  MPI_Datatype MPI_Type_f2c(MPI_Fint datatype);
  int MPI_Type_get_attr(MPI_Datatype type, int type_keyval, 
                        void *attribute_val, int *flag);
  int MPI_Type_get_contents(MPI_Datatype mtype, int max_integers, 
                            int max_addresses, int max_datatypes, 
                            int array_of_integers[], 
                            MPI_Aint array_of_addresses[], 
                            MPI_Datatype array_of_datatypes[]);
  int MPI_Type_get_envelope(MPI_Datatype type, int *num_integers, 
                            int *num_addresses, int *num_datatypes, 
                            int *combiner);
  int MPI_Type_get_extent(MPI_Datatype type, MPI_Aint *lb, 
                          MPI_Aint *extent);
  int MPI_Type_get_name(MPI_Datatype type, char *type_name, 
                        int *resultlen);
  int MPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, 
                               MPI_Aint *true_extent);
  int MPI_Type_hindexed(int count, int array_of_blocklengths[], 
                        MPI_Aint array_of_displacements[], 
                        MPI_Datatype oldtype, MPI_Datatype *newtype);
  int MPI_Type_hvector(int count, int blocklength, MPI_Aint stride, 
                       MPI_Datatype oldtype, MPI_Datatype *newtype);
  int MPI_Type_indexed(int count, int array_of_blocklengths[], 
                       int array_of_displacements[], 
                       MPI_Datatype oldtype, MPI_Datatype *newtype);
  int MPI_Type_lb(MPI_Datatype type, MPI_Aint *lb);
  int MPI_Type_match_size(int typeclass, int size, MPI_Datatype *type);
  int MPI_Type_set_attr(MPI_Datatype type, int type_keyval, 
                        void *attr_val);
  int MPI_Type_set_name(MPI_Datatype type, char *type_name);
  int MPI_Type_size(MPI_Datatype type, int *size);
  int MPI_Type_struct(int count, int array_of_blocklengths[], 
                      MPI_Aint array_of_displacements[], 
                      MPI_Datatype array_of_types[], 
                      MPI_Datatype *newtype);
  int MPI_Type_ub(MPI_Datatype mtype, MPI_Aint *ub);
  int MPI_Type_vector(int count, int blocklength, int stride, 
                      MPI_Datatype oldtype, MPI_Datatype *newtype);
  int MPI_Unpack(void *inbuf, int insize, int *position, 
                 void *outbuf, int outcount, MPI_Datatype datatype, 
                 MPI_Comm comm);
  int MPI_Unpublish_name(char *service_name, MPI_Info info, 
                         char *port_name);
  int MPI_Unpack_external (char *datarep, void *inbuf, MPI_Aint insize,
                           MPI_Aint *position, void *outbuf, int outcount,
                           MPI_Datatype datatype);
  int MPI_Waitall(int count, MPI_Request *array_of_requests, 
                  MPI_Status *array_of_statuses);
  int MPI_Waitany(int count, MPI_Request *array_of_requests, 
                  int *index, MPI_Status *status);
  int MPI_Wait(MPI_Request *request, MPI_Status *status);
  int MPI_Waitsome(int incount, MPI_Request *array_of_requests, 
                   int *outcount, int *array_of_indices, 
                   MPI_Status *array_of_statuses);
  MPI_Fint MPI_Win_c2f(MPI_Win win);
  int MPI_Win_call_errhandler(MPI_Win win, int errorcode);
  int MPI_Win_complete(MPI_Win win);
  int MPI_Win_create(void *base, MPI_Aint size, int disp_unit, 
                     MPI_Info info, MPI_Comm comm, MPI_Win *win);
  int MPI_Win_create_errhandler(MPI_Win_errhandler_fn *function, 
                                MPI_Errhandler *errhandler);
  int MPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, 
                          MPI_Win_delete_attr_function *win_delete_attr_fn, 
                          int *win_keyval, void *extra_state);
  int MPI_Win_delete_attr(MPI_Win win, int win_keyval);
  MPI_Win MPI_Win_f2c(MPI_Fint win);
  int MPI_Win_fence(int assert, MPI_Win win);
  int MPI_Win_free(MPI_Win *win);
  int MPI_Win_free_keyval(int *win_keyval);
  int MPI_Win_get_attr(MPI_Win win, int win_keyval, 
                       void *attribute_val, int *flag);
  int MPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
  int MPI_Win_get_group(MPI_Win win, MPI_Group *group);
  int MPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);
  int MPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);
  int MPI_Win_post(MPI_Group group, int assert, MPI_Win win);
  int MPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
  int MPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);
  int MPI_Win_set_name(MPI_Win win, char *win_name);
  int MPI_Win_start(MPI_Group group, int assert, MPI_Win win);
  int MPI_Win_test(MPI_Win win, int *flag);
  int MPI_Win_unlock(int rank, MPI_Win win);
  int MPI_Win_wait(MPI_Win win);
  double MPI_Wtick(void);
  double MPI_Wtime(void);

  /*
   * Profiling MPI API
   */
  int PMPI_Abort(MPI_Comm comm, int errorcode);
  int PMPI_Accumulate(void *origin_addr, int origin_count, MPI_Datatype origin_datatype,
                   int target_rank, MPI_Aint target_disp, int target_count,
                   MPI_Datatype target_datatype, MPI_Op op, MPI_Win win); 
  int PMPI_Add_error_class(int *errorclass);
  int PMPI_Add_error_code(int errorclass, int *errorcode);
  int PMPI_Add_error_string(int errorcode, char *string);
  int PMPI_Address(void *location, MPI_Aint *address);
  int PMPI_Allgather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                    void *recvbuf, int recvcount, 
                    MPI_Datatype recvtype, MPI_Comm comm);
  int PMPI_Allgatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                     void *recvbuf, int *recvcounts, 
                     int *displs, MPI_Datatype recvtype, MPI_Comm comm);
  int PMPI_Alloc_mem(MPI_Aint size, MPI_Info info, 
                    void *baseptr);
  int PMPI_Allreduce(void *sendbuf, void *recvbuf, int count, 
                    MPI_Datatype datatype, MPI_Op op, MPI_Comm comm); 
  int PMPI_Alltoall(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   void *recvbuf, int recvcount, 
                   MPI_Datatype recvtype, MPI_Comm comm);
  int PMPI_Alltoallv(void *sendbuf, int *sendcounts, int *sdispls, 
                    MPI_Datatype sendtype, void *recvbuf, int *recvcounts,
                    int *rdispls, MPI_Datatype recvtype, MPI_Comm comm);
  int PMPI_Alltoallw(void *sendbuf, int *sendcounts, int *sdispls, MPI_Datatype *sendtypes, 
                    void *recvbuf, int *recvcounts, int *rdispls, MPI_Datatype *recvtypes,
                    MPI_Comm comm);
  int PMPI_Attr_delete(MPI_Comm comm, int keyval);
  int PMPI_Attr_get(MPI_Comm comm, int keyval, void *attribute_val, int *flag);
  int PMPI_Attr_put(MPI_Comm comm, int keyval, void *attribute_val);
  int PMPI_Barrier(MPI_Comm comm);
  int PMPI_Bcast(void *buffer, int count, MPI_Datatype datatype, 
                int root, MPI_Comm comm);
  int PMPI_Bsend(void *buf, int count, MPI_Datatype datatype, 
                int dest, int tag, MPI_Comm comm);
  int PMPI_Bsend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, MPI_Request *request); 
  int PMPI_Buffer_attach(void *buffer, int size);
  int PMPI_Buffer_detach(void *buffer, int *size);
  int PMPI_Cancel(MPI_Request *request);
  int PMPI_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords);
  int PMPI_Cart_create(MPI_Comm old_comm, int ndims, int *dims, 
                      int *periods, int reorder, MPI_Comm *comm_cart);
  int PMPI_Cart_get(MPI_Comm comm, int maxdims, int *dims, 
                   int *periods, int *coords);
  int PMPI_Cart_map(MPI_Comm comm, int ndims, int *dims, 
                   int *periods, int *newrank);
  int PMPI_Cart_rank(MPI_Comm comm, int *coords, int *rank);
  int PMPI_Cart_shift(MPI_Comm comm, int direction, int disp, 
                     int *rank_source, int *rank_dest);
  int PMPI_Cart_sub(MPI_Comm comm, int *remain_dims, MPI_Comm *new_comm);
  int PMPI_Cartdim_get(MPI_Comm comm, int *ndims);
  int PMPI_Close_port(char *port_name);
  int PMPI_Comm_accept(char *port_name, MPI_Info info, int root, 
                      MPI_Comm comm, MPI_Comm *newcomm);
  MPI_Fint PMPI_Comm_c2f(MPI_Comm comm);
  int PMPI_Comm_call_errhandler(MPI_Comm comm, int errorcode);
  int PMPI_Comm_compare(MPI_Comm comm1, MPI_Comm comm2, int *result);
  int PMPI_Comm_connect(char *port_name, MPI_Info info, int root, 
                       MPI_Comm comm, MPI_Comm *newcomm);
  int PMPI_Comm_create_errhandler(MPI_Comm_errhandler_fn *function, 
                                 MPI_Errhandler *errhandler);
  int PMPI_Comm_create_keyval(MPI_Comm_copy_attr_function *comm_copy_attr_fn, 
                             MPI_Comm_delete_attr_function *comm_delete_attr_fn, 
                             int *comm_keyval, void *extra_state);
  int PMPI_Comm_create(MPI_Comm comm, MPI_Group group, MPI_Comm *newcomm);
  int PMPI_Comm_delete_attr(MPI_Comm comm, int comm_keyval);
  int PMPI_Comm_disconnect(MPI_Comm *comm);
  int PMPI_Comm_dup(MPI_Comm comm, MPI_Comm *newcomm);
  MPI_Comm PMPI_Comm_f2c(MPI_Fint comm);
  int PMPI_Comm_free_keyval(int *comm_keyval);
  int PMPI_Comm_free(MPI_Comm *comm);
  int PMPI_Comm_get_attr(MPI_Comm comm, int comm_keyval, 
                        void *attribute_val, int *flag);
  int PMPI_Comm_get_errhandler(MPI_Comm comm, MPI_Errhandler *erhandler);
  int PMPI_Comm_get_name(MPI_Comm comm, char *comm_name, int *resultlen);
  int PMPI_Comm_get_parent(MPI_Comm *parent);
  int PMPI_Comm_group(MPI_Comm comm, MPI_Group *group);
  int PMPI_Comm_join(int fd, MPI_Comm *intercomm);
  int PMPI_Comm_rank(MPI_Comm comm, int *rank);
  int PMPI_Comm_remote_group(MPI_Comm comm, MPI_Group *group);
  int PMPI_Comm_remote_size(MPI_Comm comm, int *size);
  int PMPI_Comm_set_attr(MPI_Comm comm, int comm_keyval, void *attribute_val);
  int PMPI_Comm_set_errhandler(MPI_Comm comm, MPI_Errhandler errhandler);
  int PMPI_Comm_set_name(MPI_Comm comm, char *comm_name);
  int PMPI_Comm_size(MPI_Comm comm, int *size);
  int PMPI_Comm_spawn(char *command, char **argv, int maxprocs, MPI_Info info, 
                     int root, MPI_Comm comm, MPI_Comm *intercomm, 
                     int *array_of_errcodes);
  int PMPI_Comm_spawn_multiple(int count, char **array_of_commands, char ***array_of_argv, 
                              int *array_of_maxprocs, MPI_Info *array_of_info, 
                              int root, MPI_Comm comm, MPI_Comm *intercomm, 
                              int *array_of_errcodes);
  int PMPI_Comm_split(MPI_Comm comm, int color, int key, MPI_Comm *newcomm);
  int PMPI_Comm_test_inter(MPI_Comm comm, int *flag);
  int PMPI_Dims_create(int nnodes, int ndims, int *dims);
  MPI_Fint PMPI_Errhandler_c2f(MPI_Errhandler errhandler);
  int PMPI_Errhandler_create(MPI_Handler_function *function,
                    MPI_Errhandler *errhandler);
  MPI_Errhandler PMPI_Errhandler_f2c(MPI_Fint errhandler);
  int PMPI_Errhandler_free(MPI_Errhandler *errhandler);
  int PMPI_Errhandler_get(MPI_Comm comm, MPI_Errhandler *errhandler);
  int PMPI_Errhandler_set(MPI_Comm comm, MPI_Errhandler errhandler);
  int PMPI_Error_class(int errorcode, int *errorclass);
  int PMPI_Error_string(int errorcode, char *string, int *resultlen);
  int PMPI_Exscan(void *sendbuf, void *recvbuf, int count,
                 MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  MPI_Fint PMPI_File_c2f(MPI_File file);
  MPI_File PMPI_File_f2c(MPI_Fint file);
  int PMPI_File_call_errhandler(MPI_File fh, int errorcode);
  int PMPI_File_create_errhandler(MPI_File_errhandler_fn *function,
                        MPI_Errhandler *errhandler);
  int PMPI_File_set_errhandler( MPI_File file, MPI_Errhandler errhandler);
  int PMPI_File_get_errhandler( MPI_File file, MPI_Errhandler *errhandler);
  int PMPI_File_open(MPI_Comm comm, char *filename, int amode,
                                    MPI_Info info, MPI_File *fh);
  int PMPI_File_close(MPI_File *fh);
  int PMPI_File_delete(char *filename, MPI_Info info);
  int PMPI_File_set_size(MPI_File fh, MPI_Offset size);
  int PMPI_File_preallocate(MPI_File fh, MPI_Offset size);
  int PMPI_File_get_size(MPI_File fh, MPI_Offset *size);
  int PMPI_File_get_group(MPI_File fh, MPI_Group *group);
  int PMPI_File_get_amode(MPI_File fh, int *amode);
  int PMPI_File_set_info(MPI_File fh, MPI_Info info);
  int PMPI_File_get_info(MPI_File fh, MPI_Info *info_used);
  int PMPI_File_set_view(MPI_File fh, MPI_Offset disp, MPI_Datatype etype,
                       MPI_Datatype filetype, char *datarep, MPI_Info info);
  int PMPI_File_get_view(MPI_File fh, MPI_Offset *disp,
                           MPI_Datatype *etype, 
                           MPI_Datatype *filetype, char *datarep);
  int PMPI_File_read_at(MPI_File fh, MPI_Offset offset, void *buf,
                        int count, MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_read_at_all(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_write_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_write_at_all(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_iread_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Request *request);
  int PMPI_File_iwrite_at(MPI_File fh, MPI_Offset offset, void *buf,
                    int count, MPI_Datatype datatype, MPI_Request *request);
  int PMPI_File_read(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Status *status);
  int PMPI_File_read_all(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Status *status);
  int PMPI_File_write(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Status *status);
  int PMPI_File_write_all(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Status *status);
  int PMPI_File_iread(MPI_File fh, void *buf, int count, MPI_Datatype
                   datatype, MPI_Request *request);
  int PMPI_File_iwrite(MPI_File fh, void *buf, int count, MPI_Datatype
                    datatype, MPI_Request *request);
  int PMPI_File_seek(MPI_File fh, MPI_Offset offset, int whence);
  int PMPI_File_get_position(MPI_File fh, MPI_Offset *offset);
  int PMPI_File_get_byte_offset(MPI_File fh, MPI_Offset offset,
                            MPI_Offset *disp);
  int PMPI_File_read_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_write_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_iread_shared(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Request *request);
  int PMPI_File_iwrite_shared(MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype, MPI_Request *request);
  int PMPI_File_read_ordered(MPI_File fh, void *buf, int count,
                            MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_write_ordered(MPI_File fh, void *buf, int count,
                             MPI_Datatype datatype, MPI_Status *status);
  int PMPI_File_seek_shared(MPI_File fh, MPI_Offset offset, int whence);
  int PMPI_File_get_position_shared(MPI_File fh, MPI_Offset *offset);
  int PMPI_File_read_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                                         int count, MPI_Datatype datatype);
  int PMPI_File_read_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_write_at_all_begin(MPI_File fh, MPI_Offset offset, void *buf,
                                          int count, MPI_Datatype datatype);
  int PMPI_File_write_at_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_read_all_begin(MPI_File fh, void *buf, int count,
                                      MPI_Datatype datatype);
  int PMPI_File_read_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_write_all_begin(MPI_File fh, void *buf, int count,
                                       MPI_Datatype datatype);
  int PMPI_File_write_all_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_read_ordered_begin(MPI_File fh, void *buf, int count,
                                          MPI_Datatype datatype);
  int PMPI_File_read_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_write_ordered_begin(MPI_File fh, void *buf, int count,
                                           MPI_Datatype datatype);
  int PMPI_File_write_ordered_end(MPI_File fh, void *buf, MPI_Status *status);
  int PMPI_File_get_type_extent(MPI_File fh, MPI_Datatype datatype,
                                               MPI_Aint *extent);
  int PMPI_File_set_atomicity(MPI_File fh, int flag);
  int PMPI_File_get_atomicity(MPI_File fh, int *flag);
  int PMPI_File_sync(MPI_File fh);
  int PMPI_Finalize(void);
  int PMPI_Finalized(int *flag);
  int PMPI_Free_mem(void *base);
  int PMPI_Gather(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                 void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                 int root, MPI_Comm comm);
  int PMPI_Gatherv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int *recvcounts, int *displs, 
                  MPI_Datatype recvtype, int root, MPI_Comm comm);
  int PMPI_Get_address(void *location, MPI_Aint *address);
  int PMPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count);
  int PMPI_Get_elements(MPI_Status *status, MPI_Datatype datatype, 
                       int *count);
  int PMPI_Get(void *origin_addr, int origin_count, 
              MPI_Datatype origin_datatype, int target_rank, 
              MPI_Aint target_disp, int target_count, 
              MPI_Datatype target_datatype, MPI_Win win);
  int PMPI_Get_processor_name(char *name, int *resultlen);
  int PMPI_Get_version(int *version, int *subversion);
  int PMPI_Graph_create(MPI_Comm comm_old, int nnodes, int *index, 
                      int *edges, int reorder, MPI_Comm *comm_graph);
  int PMPI_Graph_get(MPI_Comm comm, int maxindex, int maxedges, 
                    int *index, int *edges);
  int PMPI_Graph_map(MPI_Comm comm, int nnodes, int *index, int *edges, 
                    int *newrank);
  int PMPI_Graph_neighbors_count(MPI_Comm comm, int rank, int *nneighbors);
  int PMPI_Graph_neighbors(MPI_Comm comm, int rank, int maxneighbors, 
                          int *neighbors);
  int PMPI_Graphdims_get(MPI_Comm comm, int *nnodes, int *nedges);
  int PMPI_Grequest_complete(MPI_Request request);
  int PMPI_Grequest_start(MPI_Grequest_query_function *query_fn,
                         MPI_Grequest_free_function *free_fn,
                         MPI_Grequest_cancel_function *cancel_fn,
                         void *extra_state, MPI_Request *request);
  MPI_Fint PMPI_Group_c2f(MPI_Group group);
  int PMPI_Group_compare(MPI_Group group1, MPI_Group group2, int *result);
  int PMPI_Group_difference(MPI_Group group1, MPI_Group group2, 
                           MPI_Group *newgroup);
  int PMPI_Group_excl(MPI_Group group, int n, int *ranks, 
                          MPI_Group *newgroup);
  MPI_Group PMPI_Group_f2c(MPI_Fint group);
  int PMPI_Group_free(MPI_Group *group);
  int PMPI_Group_incl(MPI_Group group, int n, int *ranks, 
                          MPI_Group *newgroup);
  int PMPI_Group_intersection(MPI_Group group1, MPI_Group group2, 
                             MPI_Group *newgroup);
  int PMPI_Group_range_excl(MPI_Group group, int n, int ranges[][3], 
                           MPI_Group *newgroup);
  int PMPI_Group_range_incl(MPI_Group group, int n, int ranges[][3], 
                           MPI_Group *newgroup);
  int PMPI_Group_rank(MPI_Group group, int *rank);
  int PMPI_Group_size(MPI_Group group, int *size);
  int PMPI_Group_translate_ranks(MPI_Group group1, int n, int *ranks1, 
                                MPI_Group group2, int *ranks2);
  int PMPI_Group_union(MPI_Group group1, MPI_Group group2, 
                      MPI_Group *newgroup);
  int PMPI_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  MPI_Fint PMPI_Info_c2f(MPI_Info info);
  int PMPI_Info_create(MPI_Info *info);
  int PMPI_Info_delete(MPI_Info info, char *key);
  int PMPI_Info_dup(MPI_Info info, MPI_Info *newinfo);
  MPI_Info PMPI_Info_f2c(MPI_Fint info);
  int PMPI_Info_free(MPI_Info *info);
  int PMPI_Info_get(MPI_Info info, char *key, int valuelen, 
                   char *value, int *flag);
  int PMPI_Info_get_nkeys(MPI_Info info, int *nkeys);
  int PMPI_Info_get_nthkey(MPI_Info info, int n, char *key);
  int PMPI_Info_get_valuelen(MPI_Info info, char *key, int *valuelen, 
                            int *flag);
  int PMPI_Info_set(MPI_Info info, char *key, char *value);
  int PMPI_Init(int *argc, char ***argv);
  int PMPI_Initialized(int *flag);
  int PMPI_Init_thread(int *argc, char ***argv, int required, 
                      int *provided);
  int PMPI_Intercomm_create(MPI_Comm local_comm, int local_leader, 
                           MPI_Comm bridge_comm, int remote_leader, 
                           int tag, MPI_Comm *newintercomm);
  int PMPI_Intercomm_merge(MPI_Comm intercomm, int high, 
                          MPI_Comm *newintercomm);
  int PMPI_Iprobe(int source, int tag, MPI_Comm comm, int *flag, 
                 MPI_Status *status);
  int PMPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source, 
                int tag, MPI_Comm comm, MPI_Request *request);
  int PMPI_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int PMPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int PMPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, 
                 int tag, MPI_Comm comm, MPI_Request *request);
  int PMPI_Is_thread_main(int *flag);
  int PMPI_Keyval_create(MPI_Copy_function *copy_fn, 
                        MPI_Delete_function *delete_fn, 
                        int *keyval, void *extra_state);
  int PMPI_Keyval_free(int *keyval);
  int PMPI_Lookup_name(char *service_name, MPI_Info info, char *port_name);
  MPI_Fint PMPI_Op_c2f(MPI_Op op); 
  int PMPI_Op_create(MPI_User_function *function, int commute, 
                    MPI_Op *op);
  int PMPI_Open_port(MPI_Info info, char *port_name);
  MPI_Op PMPI_Op_f2c(MPI_Fint op);
  int PMPI_Op_free(MPI_Op *op);
  int PMPI_Pack_external(char *datarep, void *inbuf, int incount,
                        MPI_Datatype datatype, void *outbuf,
                        MPI_Aint outsize, MPI_Aint *position);
  int PMPI_Pack_external_size(char *datarep, int incount, 
                             MPI_Datatype datatype, MPI_Aint *size);
  int PMPI_Pack(void *inbuf, int incount, MPI_Datatype datatype, 
               void *outbuf, int outsize, int *position, MPI_Comm comm);
  int PMPI_Pack_size(int incount, MPI_Datatype datatype, MPI_Comm comm, 
                    int *size);
  int PMPI_Pcontrol(const int level, ...);
  int PMPI_Probe(int source, int tag, MPI_Comm comm, MPI_Status *status);
  int PMPI_Publish_name(char *service_name, MPI_Info info, 
                       char *port_name);
  int PMPI_Put(void *origin_addr, int origin_count, MPI_Datatype origin_datatype, 
              int target_rank, MPI_Aint target_disp, int target_count, 
              MPI_Datatype target_datatype, MPI_Win win);
  int PMPI_Query_thread(int *provided);
  int PMPI_Recv_init(void *buf, int count, MPI_Datatype datatype, int source,
                    int tag, MPI_Comm comm, MPI_Request *request);
  int PMPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, 
               int tag, MPI_Comm comm, MPI_Status *status);
  int PMPI_Reduce(void *sendbuf, void *recvbuf, int count, 
                 MPI_Datatype datatype, MPI_Op op, int root, MPI_Comm comm);
  int PMPI_Reduce_scatter(void *sendbuf, void *recvbuf, int *recvcounts, 
                         MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  int PMPI_Register_datarep(char *datarep, 
                       MPI_Datarep_conversion_function *read_conversion_fn,
                       MPI_Datarep_conversion_function *write_conversion_fn,
                       MPI_Datarep_extent_function *dtype_file_extent_fn,
                       void *extra_state);
  MPI_Fint PMPI_Request_c2f(MPI_Request request);
  MPI_Request PMPI_Request_f2c(MPI_Fint request);
  int PMPI_Request_free(MPI_Request *request);
  int PMPI_Request_get_status(MPI_Request request, int *flag, 
                             MPI_Status *status);
  int PMPI_Rsend(void *ibuf, int count, MPI_Datatype datatype, int dest, 
                int tag, MPI_Comm comm);
  int PMPI_Rsend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, 
                     MPI_Request *request);
  int PMPI_Scan(void *sendbuf, void *recvbuf, int count, 
               MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);
  int PMPI_Scatter(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                  void *recvbuf, int recvcount, MPI_Datatype recvtype, 
                  int root, MPI_Comm comm);
  int PMPI_Scatterv(void *sendbuf, int *sendcounts, int *displs, 
                   MPI_Datatype sendtype, void *recvbuf, int recvcount, 
                   MPI_Datatype recvtype, int root, MPI_Comm comm);
  int PMPI_Send_init(void *buf, int count, MPI_Datatype datatype, 
                    int dest, int tag, MPI_Comm comm, 
                    MPI_Request *request);
  int PMPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, 
               int tag, MPI_Comm comm);
  int PMPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype, 
                   int dest, int sendtag, void *recvbuf, int recvcount,
                   MPI_Datatype recvtype, int source, int recvtag, 
                   MPI_Comm comm,  MPI_Status *status);
  int PMPI_Sendrecv_replace(void * buf, int count, MPI_Datatype datatype, 
                           int dest, int sendtag, int source, int recvtag,
                           MPI_Comm comm, MPI_Status *status);
  int PMPI_Ssend_init(void *buf, int count, MPI_Datatype datatype, 
                     int dest, int tag, MPI_Comm comm, 
                     MPI_Request *request);
  int PMPI_Ssend(void *buf, int count, MPI_Datatype datatype, int dest, 
                int tag, MPI_Comm comm);
  int PMPI_Start(MPI_Request *request);
  int PMPI_Startall(int count, MPI_Request *array_of_requests);
  int PMPI_Status_c2f(MPI_Status *c_status, MPI_Fint *f_status);
  int PMPI_Status_f2c(MPI_Fint *f_status, MPI_Status *c_status);
  int PMPI_Status_set_cancelled(MPI_Status *status, int flag);
  int PMPI_Status_set_elements(MPI_Status *status, MPI_Datatype datatype,
                              int count);
  int PMPI_Testall(int count, MPI_Request array_of_requests[], int *flag, 
                  MPI_Status array_of_statuses[]);
  int PMPI_Testany(int count, MPI_Request array_of_requests[], int *index, int *flag, MPI_Status *status);
  int PMPI_Test(MPI_Request *request, int *flag, MPI_Status *status);
  int PMPI_Test_cancelled(MPI_Status *status, int *flag);
  int PMPI_Testsome(int incount, MPI_Request array_of_requests[], 
                   int *outcount, int array_of_indices[], 
                   MPI_Status array_of_statuses[]);
  int PMPI_Topo_test(MPI_Comm comm, int *status);
  MPI_Fint PMPI_Type_c2f(MPI_Datatype datatype);
  int PMPI_Type_commit(MPI_Datatype *type);
  int PMPI_Type_contiguous(int count, MPI_Datatype oldtype, 
                          MPI_Datatype *newtype);
  int PMPI_Type_create_darray(int size, int rank, int ndims, 
                             int gsize_array[], int distrib_array[], 
                             int darg_array[], int psize_array[],
                             int order, MPI_Datatype oldtype, 
                             MPI_Datatype *newtype);
  int PMPI_Type_create_f90_complex(int p, int r, MPI_Datatype *newtype);
  int PMPI_Type_create_f90_integer(int r, MPI_Datatype *newtype);
  int PMPI_Type_create_f90_real(int p, int r, MPI_Datatype *newtype);
  int PMPI_Type_create_hindexed(int count, int array_of_blocklengths[], 
                               MPI_Aint array_of_displacements[], 
                               MPI_Datatype oldtype, 
                               MPI_Datatype *newtype);
  int PMPI_Type_create_hvector(int count, int blocklength, MPI_Aint stride, 
                              MPI_Datatype oldtype, 
                              MPI_Datatype *newtype);
  int PMPI_Type_create_keyval(MPI_Type_copy_attr_function *type_copy_attr_fn, 
                             MPI_Type_delete_attr_function *type_delete_attr_fn, 
                             int *type_keyval, void *extra_state);
  int PMPI_Type_create_indexed_block(int count, int blocklength,
                                  int array_of_displacements[],
                                  MPI_Datatype oldtype,
                                  MPI_Datatype *newtype);
  int PMPI_Type_create_struct(int count, int array_of_block_lengths[], 
                             MPI_Aint array_of_displacements[], 
                             MPI_Datatype array_of_types[], 
                             MPI_Datatype *newtype);
  int PMPI_Type_create_subarray(int ndims, int size_array[], int subsize_array[], 
                               int start_array[], int order, 
                               MPI_Datatype oldtype, MPI_Datatype *newtype);
  int PMPI_Type_create_resized(MPI_Datatype oldtype, MPI_Aint lb, 
                               MPI_Aint extent, MPI_Datatype *newtype); 
  int PMPI_Type_delete_attr(MPI_Datatype type, int type_keyval);
  int PMPI_Type_dup(MPI_Datatype type, MPI_Datatype *newtype);
  int PMPI_Type_extent(MPI_Datatype type, MPI_Aint *extent);
  int PMPI_Type_free(MPI_Datatype *type);
  int PMPI_Type_free_keyval(int *type_keyval);
  MPI_Datatype PMPI_Type_f2c(MPI_Fint datatype);
  int PMPI_Type_get_attr(MPI_Datatype type, int type_keyval, 
                        void *attribute_val, int *flag);
  int PMPI_Type_get_contents(MPI_Datatype mtype, int max_integers, 
                            int max_addresses, int max_datatypes, 
                            int array_of_integers[], 
                            MPI_Aint array_of_addresses[], 
                            MPI_Datatype array_of_datatypes[]);
  int PMPI_Type_get_envelope(MPI_Datatype type, int *num_integers, 
                            int *num_addresses, int *num_datatypes, 
                            int *combiner);
  int PMPI_Type_get_extent(MPI_Datatype type, MPI_Aint *lb, 
                          MPI_Aint *extent);
  int PMPI_Type_get_name(MPI_Datatype type, char *type_name, 
                        int *resultlen);
  int PMPI_Type_get_true_extent(MPI_Datatype datatype, MPI_Aint *true_lb, 
                               MPI_Aint *true_extent);
  int PMPI_Type_hindexed(int count, int array_of_blocklengths[], 
                        MPI_Aint array_of_displacements[], 
                        MPI_Datatype oldtype, MPI_Datatype *newtype);
  int PMPI_Type_hvector(int count, int blocklength, MPI_Aint stride, 
                       MPI_Datatype oldtype, MPI_Datatype *newtype);
  int PMPI_Type_indexed(int count, int array_of_blocklengths[], 
                       int array_of_displacements[], 
                       MPI_Datatype oldtype, MPI_Datatype *newtype);
  int PMPI_Type_lb(MPI_Datatype type, MPI_Aint *lb);
  int PMPI_Type_match_size(int typeclass, int size, MPI_Datatype *type);
  int PMPI_Type_set_attr(MPI_Datatype type, int type_keyval, 
                        void *attr_val);
  int PMPI_Type_set_name(MPI_Datatype type, char *type_name);
  int PMPI_Type_size(MPI_Datatype type, int *size);
  int PMPI_Type_struct(int count, int array_of_blocklengths[], 
                      MPI_Aint array_of_displacements[], 
                      MPI_Datatype array_of_types[], 
                      MPI_Datatype *newtype);
  int PMPI_Type_ub(MPI_Datatype mtype, MPI_Aint *ub);
  int PMPI_Type_vector(int count, int blocklength, int stride, 
                      MPI_Datatype oldtype, MPI_Datatype *newtype);
  int PMPI_Unpack(void *inbuf, int insize, int *position, 
                 void *outbuf, int outcount, MPI_Datatype datatype, 
                 MPI_Comm comm);
  int PMPI_Unpublish_name(char *service_name, MPI_Info info, 
                         char *port_name);
  int PMPI_Unpack_external (char *datarep, void *inbuf, MPI_Aint insize,
                           MPI_Aint *position, void *outbuf, int outcount,
                           MPI_Datatype datatype);
  int PMPI_Waitall(int count, MPI_Request *array_of_requests, 
                  MPI_Status *array_of_statuses);
  int PMPI_Waitany(int count, MPI_Request *array_of_requests, 
                  int *index, MPI_Status *status);
  int PMPI_Wait(MPI_Request *request, MPI_Status *status);
  int PMPI_Waitsome(int incount, MPI_Request *array_of_requests, 
                   int *outcount, int *array_of_indices, 
                   MPI_Status *array_of_statuses);
  MPI_Fint PMPI_Win_c2f(MPI_Win win);
  int PMPI_Win_call_errhandler(MPI_Win win, int errorcode);
  int PMPI_Win_complete(MPI_Win win);
  int PMPI_Win_create(void *base, MPI_Aint size, int disp_unit, 
                     MPI_Info info, MPI_Comm comm, MPI_Win *win);
  int PMPI_Win_create_errhandler(MPI_Win_errhandler_fn *function, 
                                MPI_Errhandler *errhandler);
  int PMPI_Win_create_keyval(MPI_Win_copy_attr_function *win_copy_attr_fn, 
                          MPI_Win_delete_attr_function *win_delete_attr_fn, 
                          int *win_keyval, void *extra_state);
  int PMPI_Win_delete_attr(MPI_Win win, int win_keyval);
  MPI_Win PMPI_Win_f2c(MPI_Fint win);
  int PMPI_Win_fence(int assert, MPI_Win win);
  int PMPI_Win_free(MPI_Win *win);
  int PMPI_Win_free_keyval(int *win_keyval);
  int PMPI_Win_get_attr(MPI_Win win, int win_keyval, 
                       void *attribute_val, int *flag);
  int PMPI_Win_get_errhandler(MPI_Win win, MPI_Errhandler *errhandler);
  int PMPI_Win_get_group(MPI_Win win, MPI_Group *group);
  int PMPI_Win_get_name(MPI_Win win, char *win_name, int *resultlen);
  int PMPI_Win_lock(int lock_type, int rank, int assert, MPI_Win win);
  int PMPI_Win_post(MPI_Group group, int assert, MPI_Win win);
  int PMPI_Win_set_attr(MPI_Win win, int win_keyval, void *attribute_val);
  int PMPI_Win_set_errhandler(MPI_Win win, MPI_Errhandler errhandler);
  int PMPI_Win_set_name(MPI_Win win, char *win_name);
  int PMPI_Win_start(MPI_Group group, int assert, MPI_Win win);
  int PMPI_Win_test(MPI_Win win, int *flag);
  int PMPI_Win_unlock(int rank, MPI_Win win);
  int PMPI_Win_wait(MPI_Win win);
  double PMPI_Wtick(void);
  double PMPI_Wtime(void);

#if defined(c_plusplus) || defined(__cplusplus)
}
#endif

/*                                                                             
 * Conditional MPI 2 C++ bindings support.  Include if:
 *   - We want C++ bindings support
 *   - We are not building OMPI itself
 *   - We are using a C++ compiler
 */

#if defined(OMPI_WANT_CXX_BINDINGS)
#if !defined(OMPI_BUILDING) || !OMPI_BUILDING
#if defined(__cplusplus) || defined(c_plusplus) 
#include "mpi/cxx/mpicxx.h"
#endif
#endif
#endif

#endif /* OMPI_MPI_H */
