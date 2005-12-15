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
// $COPYRIGHT$
// 
// Additional copyrights may follow
// 
// $HEADER$
//

class Comm_Null {
#if 0 /* OMPI_ENABLE_MPI_PROFILING */
  //  friend class PMPI::Comm_Null;
#endif
public:

#if 0 /* OMPI_ENABLE_MPI_PROFILING */

  // construction
  inline Comm_Null() { }
  // copy
  inline Comm_Null(const Comm_Null& data) : pmpi_comm(data.pmpi_comm) { }
  // inter-language operability  
  inline Comm_Null(const MPI_Comm& data) : pmpi_comm(data) { }

  inline Comm_Null(const PMPI::Comm_Null& data) : pmpi_comm(data) { }

  // destruction
  virtual inline ~Comm_Null() { }

  inline Comm_Null& operator=(const Comm_Null& data) {
    pmpi_comm = data.pmpi_comm; 
    return *this;
  }

  // comparison
  inline bool operator==(const Comm_Null& data) const {
    return (bool) (pmpi_comm == data.pmpi_comm); }

  inline bool operator!=(const Comm_Null& data) const {
    return (bool) (pmpi_comm != data.pmpi_comm);}

  // inter-language operability (conversion operators)
  inline operator MPI_Comm() const { return pmpi_comm; }
  //  inline operator MPI_Comm*() /*const JGS*/ { return pmpi_comm; }
  inline operator const PMPI::Comm_Null&() const { return pmpi_comm; }

#else

  // construction
  inline Comm_Null() : mpi_comm(MPI_COMM_NULL) { }
  // copy
  inline Comm_Null(const Comm_Null& data) : mpi_comm(data.mpi_comm) { }
  // inter-language operability  
  inline Comm_Null(const MPI_Comm& data) : mpi_comm(data) { }

  // destruction
  virtual inline ~Comm_Null() { }

 // comparison
  // JGS make sure this is right (in other classes too)
  inline bool operator==(const Comm_Null& data) const {
    return (bool) (mpi_comm == data.mpi_comm); }

  inline bool operator!=(const Comm_Null& data) const {
    return (bool) !(*this == data);}

  // inter-language operability (conversion operators)
  inline operator MPI_Comm() const { return mpi_comm; }

#endif

  
protected:

#if 0 /* OMPI_ENABLE_MPI_PROFILING */
  PMPI::Comm_Null pmpi_comm;
#else
  MPI_Comm mpi_comm;
#endif
  

};


class Comm : public Comm_Null {
public:

  typedef void Errhandler_fn(Comm&, int*, ...);
  typedef int Copy_attr_function(const Comm& oldcomm, int comm_keyval,
				 void* extra_state, void* attribute_val_in,
				 void* attribute_val_out, 
				 bool& flag);
  typedef int Delete_attr_function(Comm& comm, int comm_keyval, 
				   void* attribute_val,
				   void* extra_state);
#if !0 /* OMPI_ENABLE_MPI_PROFILING */
#define _MPI2CPP_ERRHANDLERFN_ Errhandler_fn
#define _MPI2CPP_COPYATTRFN_ Copy_attr_function
#define _MPI2CPP_DELETEATTRFN_ Delete_attr_function
#endif

  // construction
  Comm();

  // copy
  Comm(const Comm_Null& data);

#if 0 /* OMPI_ENABLE_MPI_PROFILING */
  Comm(const Comm& data) : 
    Comm_Null(data),
    pmpi_comm((const PMPI::Comm&) data) { }

  // inter-language operability
  Comm(const MPI_Comm& data) : Comm_Null(data), pmpi_comm(data) { }

  Comm(const PMPI::Comm& data) :
    Comm_Null((const PMPI::Comm_Null&)data),
    pmpi_comm(data) { }

  operator const PMPI::Comm&() const { return pmpi_comm; }

  // assignment
  Comm& operator=(const Comm& data) {
    this->Comm_Null::operator=(data);
    pmpi_comm = data.pmpi_comm; 
    return *this;
  }
  Comm& operator=(const Comm_Null& data) {
    this->Comm_Null::operator=(data);
    MPI_Comm tmp = data;
    pmpi_comm = tmp; 
    return *this;
  }
  // inter-language operability
  Comm& operator=(const MPI_Comm& data) {
    this->Comm_Null::operator=(data);
    pmpi_comm = data;
    return *this;
  }

#else
  Comm(const Comm& data) : Comm_Null(data.mpi_comm) { }
  // inter-language operability
  Comm(const MPI_Comm& data) : Comm_Null(data) { }
#endif


  //
  // Point-to-Point
  //

  virtual void Send(const void *buf, int count, 
		    const Datatype & datatype, int dest, int tag) const;

  virtual void Recv(void *buf, int count, const Datatype & datatype,
		    int source, int tag, Status & status) const;


  virtual void Recv(void *buf, int count, const Datatype & datatype,
		    int source, int tag) const;
  
  virtual void Bsend(const void *buf, int count,
		     const Datatype & datatype, int dest, int tag) const;
  
  virtual void Ssend(const void *buf, int count, 
		     const Datatype & datatype, int dest, int tag) const ;

  virtual void Rsend(const void *buf, int count,
		     const Datatype & datatype, int dest, int tag) const;
  
  virtual Request Isend(const void *buf, int count,
			const Datatype & datatype, int dest, int tag) const;
  
  virtual Request Ibsend(const void *buf, int count, const
			 Datatype & datatype, int dest, int tag) const;
  
  virtual Request Issend(const void *buf, int count,
			 const Datatype & datatype, int dest, int tag) const;
  
  virtual Request Irsend(const void *buf, int count,
			 const Datatype & datatype, int dest, int tag) const;

  virtual Request Irecv(void *buf, int count,
			const Datatype & datatype, int source, int tag) const;

  virtual bool Iprobe(int source, int tag, Status & status) const;

  virtual bool Iprobe(int source, int tag) const;

  virtual void Probe(int source, int tag, Status & status) const;
  
  virtual void Probe(int source, int tag) const;
  
  virtual Prequest Send_init(const void *buf, int count,
			     const Datatype & datatype, int dest, 
			     int tag) const;
  
  virtual Prequest Bsend_init(const void *buf, int count,
			      const Datatype & datatype, int dest, 
			      int tag) const;
  
  virtual Prequest Ssend_init(const void *buf, int count,
			      const Datatype & datatype, int dest, 
			      int tag) const;
  
  virtual Prequest Rsend_init(const void *buf, int count,
			      const Datatype & datatype, int dest, 
			      int tag) const;
  
  virtual Prequest Recv_init(void *buf, int count,
			     const Datatype & datatype, int source, 
			     int tag) const;
  
  virtual void Sendrecv(const void *sendbuf, int sendcount,
			const Datatype & sendtype, int dest, int sendtag, 
			void *recvbuf, int recvcount, 
			const Datatype & recvtype, int source,
			int recvtag, Status & status) const;
  
  virtual void Sendrecv(const void *sendbuf, int sendcount,
			const Datatype & sendtype, int dest, int sendtag, 
			void *recvbuf, int recvcount, 
			const Datatype & recvtype, int source,
			int recvtag) const;

  virtual void Sendrecv_replace(void *buf, int count,
				const Datatype & datatype, int dest, 
				int sendtag, int source,
				int recvtag, Status & status) const;

  virtual void Sendrecv_replace(void *buf, int count,
				const Datatype & datatype, int dest, 
				int sendtag, int source,
				int recvtag) const;
  
  //
  // Groups, Contexts, and Communicators
  //

  virtual Group Get_group() const;
  
  virtual int Get_size() const;

  virtual int Get_rank() const;
  
  static int Compare(const Comm & comm1, const Comm & comm2);
  
  virtual Comm& Clone() const = 0;

  virtual void Free(void);
  
  virtual bool Is_inter() const;


  // 
  // Process Creation
  //

  virtual void Disconnect();

  static Intercomm Get_parent();
  
  static Intercomm Join(const int fd);

  //
  //External Interfaces
  //
  
  virtual void Get_name(char * comm_name, int& resultlen) const;

  virtual void Set_name(const char* comm_name);
  
  //
  //Process Topologies
  //
  
  virtual int Get_topology() const;
  
  //
  // Environmental Inquiry
  //
  
  virtual void Abort(int errorcode);

  //
  // Errhandler
  //

  virtual void Set_errhandler(const Errhandler& errhandler);

  virtual Errhandler Get_errhandler() const;

  //JGS took out const below from fn arg
  static Errhandler Create_errhandler(Comm::Errhandler_fn* function);

  //
  // Keys and Attributes
  //

//JGS I took the const out because it causes problems when trying to
//call this function with the predefined NULL_COPY_FN etc.
  static int Create_keyval(Copy_attr_function* comm_copy_attr_fn,
			   Delete_attr_function* comm_delete_attr_fn,
			   void* extra_state);
  
  static void Free_keyval(int& comm_keyval);

  virtual void Set_attr(int comm_keyval, const void* attribute_val) const;

  virtual bool Get_attr(int comm_keyval, void* attribute_val) const;
  
  virtual void Delete_attr(int comm_keyval);

  static int NULL_COPY_FN(const Comm& oldcomm, int comm_keyval,
			  void* extra_state, void* attribute_val_in,
			  void* attribute_val_out, bool& flag);
  
  static int DUP_FN(const Comm& oldcomm, int comm_keyval,
		    void* extra_state, void* attribute_val_in,
		    void* attribute_val_out, bool& flag);
  
  static int NULL_DELETE_FN(Comm& comm, int comm_keyval, void* attribute_val,
			    void* extra_state);


  //#if 0 /* OMPI_ENABLE_MPI_PROFILING */
  //  virtual const PMPI::Comm& get_pmpi_comm() const { return pmpi_comm; }
  //#endif

private:
#if 0 /* OMPI_ENABLE_MPI_PROFILING */
  PMPI::Comm pmpi_comm;
#endif

#if ! 0 /* OMPI_ENABLE_MPI_PROFILING */
public: // JGS hmmm, these used by errhandler_intercept
        // should make it a friend

  Errhandler* my_errhandler;

  typedef ::std::pair<Comm*, CommType> comm_pair_t;
  typedef ::std::map<MPI_Comm, comm_pair_t*> mpi_comm_map_t;
  static mpi_comm_map_t mpi_comm_map;
  static opal_mutex_t *mpi_comm_map_mutex;

  typedef ::std::map<MPI_Comm, Comm*> mpi_err_map_t;
  static mpi_err_map_t mpi_err_map;
  static opal_mutex_t *mpi_err_map_mutex;
  
  typedef ::std::pair<Comm::_MPI2CPP_COPYATTRFN_*, Comm::_MPI2CPP_DELETEATTRFN_*> key_pair_t;
  typedef ::std::map<int, key_pair_t*> key_fn_map_t;
  static key_fn_map_t key_fn_map;
  static opal_mutex_t *key_fn_map_mutex;
  
  void init() {
    my_errhandler = (Errhandler*)0;
  }

  static Op* current_op;

#endif

};
