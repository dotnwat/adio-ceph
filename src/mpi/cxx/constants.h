// -*- c++ -*-
// 
// Copyright (c) 2004-2005 The Trustees of Indiana University.
//                         All rights reserved.
// Copyright (c) 2004-2005 The Trustees of the University of Tennessee.
//                         All rights reserved.
// Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
//                         University of Stuttgart.  All rights reserved.
// Copyright (c) 2004-2005 The Regents of the University of California.
//                         All rights reserved.
// $COPYRIGHT$
// 
// Additional copyrights may follow
// 
// $HEADER$
//


// return  codes
OMPI_DECLSPEC extern const int SUCCESS;
OMPI_DECLSPEC extern const int ERR_BUFFER;
OMPI_DECLSPEC extern const int ERR_COUNT;
OMPI_DECLSPEC extern const int ERR_TYPE;
OMPI_DECLSPEC extern const int ERR_TAG ;
OMPI_DECLSPEC extern const int ERR_COMM;
OMPI_DECLSPEC extern const int ERR_RANK;
OMPI_DECLSPEC extern const int ERR_REQUEST;
OMPI_DECLSPEC extern const int ERR_ROOT;
OMPI_DECLSPEC extern const int ERR_GROUP;
OMPI_DECLSPEC extern const int ERR_OP;
OMPI_DECLSPEC extern const int ERR_TOPOLOGY;
OMPI_DECLSPEC extern const int ERR_DIMS;
OMPI_DECLSPEC extern const int ERR_ARG;
OMPI_DECLSPEC extern const int ERR_UNKNOWN;
OMPI_DECLSPEC extern const int ERR_TRUNCATE;
OMPI_DECLSPEC extern const int ERR_OTHER;
OMPI_DECLSPEC extern const int ERR_INTERN;
OMPI_DECLSPEC extern const int ERR_PENDING;
OMPI_DECLSPEC extern const int ERR_IN_STATUS;
OMPI_DECLSPEC extern const int ERR_LASTCODE;

OMPI_DECLSPEC extern const int ERR_BASE;
OMPI_DECLSPEC extern const int ERR_INFO_VALUE;
OMPI_DECLSPEC extern const int ERR_INFO_KEY;
OMPI_DECLSPEC extern const int ERR_INFO_NOKEY;
OMPI_DECLSPEC extern const int ERR_KEYVAL;
OMPI_DECLSPEC extern const int ERR_NAME;
OMPI_DECLSPEC extern const int ERR_NO_MEM;
OMPI_DECLSPEC extern const int ERR_SERVICE;
OMPI_DECLSPEC extern const int ERR_SPAWN;
OMPI_DECLSPEC extern const int ERR_WIN;


// assorted constants
OMPI_DECLSPEC extern const void* BOTTOM;
OMPI_DECLSPEC extern const int PROC_NULL;
OMPI_DECLSPEC extern const int ANY_SOURCE;
OMPI_DECLSPEC extern const int ANY_TAG;
OMPI_DECLSPEC extern const int UNDEFINED;
OMPI_DECLSPEC extern const int BSEND_OVERHEAD;
OMPI_DECLSPEC extern const int KEYVAL_INVALID;

// error-handling specifiers
OMPI_DECLSPEC extern const Errhandler  ERRORS_ARE_FATAL;
OMPI_DECLSPEC extern const Errhandler  ERRORS_RETURN;
OMPI_DECLSPEC extern const Errhandler  ERRORS_THROW_EXCEPTIONS;

// maximum sizes for strings
OMPI_DECLSPEC extern const int MAX_PROCESSOR_NAME;
OMPI_DECLSPEC extern const int MAX_ERROR_STRING;
OMPI_DECLSPEC extern const int MAX_INFO_KEY;
OMPI_DECLSPEC extern const int MAX_INFO_VAL;
OMPI_DECLSPEC extern const int MAX_PORT_NAME;
OMPI_DECLSPEC extern const int MAX_OBJECT_NAME;

// elementary datatypes (C / C++)
OMPI_DECLSPEC extern const Datatype CHAR;
OMPI_DECLSPEC extern const Datatype SHORT;          
OMPI_DECLSPEC extern const Datatype INT;            
OMPI_DECLSPEC extern const Datatype LONG;
OMPI_DECLSPEC extern const Datatype SIGNED_CHAR;
OMPI_DECLSPEC extern const Datatype UNSIGNED_CHAR;
OMPI_DECLSPEC extern const Datatype UNSIGNED_SHORT; 
OMPI_DECLSPEC extern const Datatype UNSIGNED;       
OMPI_DECLSPEC extern const Datatype UNSIGNED_LONG;  
OMPI_DECLSPEC extern const Datatype FLOAT;
OMPI_DECLSPEC extern const Datatype DOUBLE;
OMPI_DECLSPEC extern const Datatype LONG_DOUBLE;
OMPI_DECLSPEC extern const Datatype BYTE;
OMPI_DECLSPEC extern const Datatype PACKED;
OMPI_DECLSPEC extern const Datatype WCHAR;

// datatypes for reductions functions (C / C++)
OMPI_DECLSPEC extern const Datatype FLOAT_INT;
OMPI_DECLSPEC extern const Datatype DOUBLE_INT;
OMPI_DECLSPEC extern const Datatype LONG_INT;
OMPI_DECLSPEC extern const Datatype TWOINT;
OMPI_DECLSPEC extern const Datatype SHORT_INT;
OMPI_DECLSPEC extern const Datatype LONG_DOUBLE_INT;

// elementary datatype (Fortran)
OMPI_DECLSPEC extern const Datatype INTEGER;
OMPI_DECLSPEC extern const Datatype REAL;
OMPI_DECLSPEC extern const Datatype DOUBLE_PRECISION;
OMPI_DECLSPEC extern const Datatype F_COMPLEX;
OMPI_DECLSPEC extern const Datatype LOGICAL;
OMPI_DECLSPEC extern const Datatype CHARACTER;

// datatype for reduction functions (Fortran)
OMPI_DECLSPEC extern const Datatype TWOREAL;
OMPI_DECLSPEC extern const Datatype TWODOUBLE_PRECISION;
OMPI_DECLSPEC extern const Datatype TWOINTEGER;

// optional datatypes (Fortran)
OMPI_DECLSPEC extern const Datatype INTEGER1;
OMPI_DECLSPEC extern const Datatype INTEGER2;
OMPI_DECLSPEC extern const Datatype INTEGER4;
OMPI_DECLSPEC extern const Datatype REAL2;
OMPI_DECLSPEC extern const Datatype REAL4;
OMPI_DECLSPEC extern const Datatype REAL8;

// optional datatype (C / C++)
OMPI_DECLSPEC extern const Datatype LONG_LONG;
OMPI_DECLSPEC extern const Datatype UNSIGNED_LONG_LONG;

// c++ types
OMPI_DECLSPEC extern const Datatype BOOL;
OMPI_DECLSPEC extern const Datatype COMPLEX;
OMPI_DECLSPEC extern const Datatype DOUBLE_COMPLEX;
OMPI_DECLSPEC extern const Datatype LONG_DOUBLE_COMPLEX;

// special datatypes for contstruction of derived datatypes
OMPI_DECLSPEC extern const Datatype UB;
OMPI_DECLSPEC extern const Datatype LB;

// datatype decoding constants
OMPI_DECLSPEC extern const int COMBINER_NAMED;
OMPI_DECLSPEC extern const int COMBINER_DUP;
OMPI_DECLSPEC extern const int COMBINER_CONTIGUOUS;
OMPI_DECLSPEC extern const int COMBINER_VECTOR;
OMPI_DECLSPEC extern const int COMBINER_HVECTOR_INTEGER;
OMPI_DECLSPEC extern const int COMBINER_HVECTOR;
OMPI_DECLSPEC extern const int COMBINER_INDEXED;
OMPI_DECLSPEC extern const int COMBINER_HINDEXED_INTEGER;
OMPI_DECLSPEC extern const int COMBINER_HINDEXED;
OMPI_DECLSPEC extern const int COMBINER_INDEXED_BLOCK;
OMPI_DECLSPEC extern const int COMBINER_STRUCT_INTEGER;
OMPI_DECLSPEC extern const int COMBINER_STRUCT;
OMPI_DECLSPEC extern const int COMBINER_SUBARRAY;
OMPI_DECLSPEC extern const int COMBINER_DARRAY;
OMPI_DECLSPEC extern const int COMBINER_F90_REAL;
OMPI_DECLSPEC extern const int COMBINER_F90_COMPLEX;
OMPI_DECLSPEC extern const int COMBINER_F90_INTEGER;
OMPI_DECLSPEC extern const int COMBINER_RESIZED;

// thread constants
OMPI_DECLSPEC extern const int THREAD_SINGLE;
OMPI_DECLSPEC extern const int THREAD_FUNNELED;
OMPI_DECLSPEC extern const int THREAD_SERIALIZED;
OMPI_DECLSPEC extern const int THREAD_MULTIPLE;

// reserved communicators
// JGS these can not be const because Set_errhandler is not const
OMPI_DECLSPEC extern Intracomm COMM_WORLD;
OMPI_DECLSPEC extern Intracomm COMM_SELF;

// results of communicator and group comparisons
OMPI_DECLSPEC extern const int IDENT;
OMPI_DECLSPEC extern const int CONGRUENT;
OMPI_DECLSPEC extern const int SIMILAR;
OMPI_DECLSPEC extern const int UNEQUAL;

// environmental inquiry keys
OMPI_DECLSPEC extern const int TAG_UB;
OMPI_DECLSPEC extern const int IO;
OMPI_DECLSPEC extern const int HOST;
OMPI_DECLSPEC extern const int WTIME_IS_GLOBAL;
OMPI_DECLSPEC extern const int UNIVERSE_SIZE;
OMPI_DECLSPEC extern const int APPNUM;
OMPI_DECLSPEC extern const int WIN_BASE;
OMPI_DECLSPEC extern const int WIN_SIZE;
OMPI_DECLSPEC extern const int WIN_DISP_UNIT;

// collective operations
OMPI_DECLSPEC extern const Op MAX;
OMPI_DECLSPEC extern const Op MIN;
OMPI_DECLSPEC extern const Op SUM;
OMPI_DECLSPEC extern const Op PROD;
OMPI_DECLSPEC extern const Op MAXLOC;
OMPI_DECLSPEC extern const Op MINLOC;
OMPI_DECLSPEC extern const Op BAND;
OMPI_DECLSPEC extern const Op BOR;
OMPI_DECLSPEC extern const Op BXOR;
OMPI_DECLSPEC extern const Op LAND;
OMPI_DECLSPEC extern const Op LOR;
OMPI_DECLSPEC extern const Op LXOR;
OMPI_DECLSPEC extern const Op REPLACE;

// null handles
OMPI_DECLSPEC extern const Group        GROUP_NULL;
OMPI_DECLSPEC extern const Win          WIN_NULL;
OMPI_DECLSPEC extern const Info         INFO_NULL;
//OMPI_DECLSPEC extern const Comm         COMM_NULL;
//OMPI_DECLSPEC extern const MPI_Comm     COMM_NULL;
OMPI_DECLSPEC extern Comm_Null          COMM_NULL;
OMPI_DECLSPEC extern const Datatype     DATATYPE_NULL;
OMPI_DECLSPEC extern Request            REQUEST_NULL;
OMPI_DECLSPEC extern const Op           OP_NULL;
OMPI_DECLSPEC extern const Errhandler   ERRHANDLER_NULL;  

// constants specifying empty or ignored input
OMPI_DECLSPEC extern const char**       ARGV_NULL;
OMPI_DECLSPEC extern const char***      ARGVS_NULL;

// empty group
OMPI_DECLSPEC extern const Group  GROUP_EMPTY;

// topologies
OMPI_DECLSPEC extern const int GRAPH;
OMPI_DECLSPEC extern const int CART;
