// -*- c++ -*-
//
// Copyright (c) 2004-2005 The Trustees of Indiana University and Indiana
//                         University Research and Technology
//                         Corporation.  All rights reserved.
// Copyright (c) 2004-2005 The University of Tennessee and The University
//                         of Tennessee Research Foundation.  All rights
//                         reserved.
// Copyright (c) 2004-2005 High Performance Computing Center Stuttgart, 
//                         University of Stuttgart.  All rights reserved.
// Copyright (c) 2004-2005 The Regents of the University of California.
//                         All rights reserved.
// Copyright (c) 2006      Cisco Systems, Inc.  All rights reserved.
// $COPYRIGHT$
// 
// Additional copyrights may follow
// 
// $HEADER$
//

class Errhandler {
public:
  // construction / destruction
  inline Errhandler()
    : mpi_errhandler(MPI_ERRHANDLER_NULL) {}

  inline virtual ~Errhandler() { }

  inline Errhandler(MPI_Errhandler i)
    : mpi_errhandler(i) {}

 // copy / assignment
  inline Errhandler(const Errhandler& e)
    : comm_handler_fn(e.comm_handler_fn), 
#if OMPI_PROVIDE_MPI_FILE_INTERFACE
      file_handler_fn(e.file_handler_fn), 
#endif
      win_handler_fn(e.win_handler_fn), 
      mpi_errhandler(e.mpi_errhandler) { }

  inline Errhandler& operator=(const Errhandler& e)
  {
    mpi_errhandler = e.mpi_errhandler;
    comm_handler_fn = e.comm_handler_fn;
#if OMPI_PROVIDE_MPI_FILE_INTERFACE
    file_handler_fn = e.file_handler_fn;
#endif
    win_handler_fn = e.win_handler_fn;
    return *this;
  }

  // comparison
  inline bool operator==(const Errhandler &a) {
    return (bool)(mpi_errhandler == a.mpi_errhandler); }
  
  inline bool operator!=(const Errhandler &a) {
    return (bool)!(*this == a); }

  // inter-language operability
  inline Errhandler& operator= (const MPI_Errhandler &i) {
    mpi_errhandler = i; return *this; }
 
  inline operator MPI_Errhandler() const { return mpi_errhandler; }
 
  //  inline operator MPI_Errhandler*() { return &mpi_errhandler; }

  //
  // Errhandler access functions
  //
  
  virtual void Free();

  Comm::Errhandler_fn* comm_handler_fn;
#if OMPI_PROVIDE_MPI_FILE_INTERFACE
  File::Errhandler_fn* file_handler_fn;
#endif
  Win::Errhandler_fn* win_handler_fn;

  MPI_Errhandler mpi_errhandler;
};
