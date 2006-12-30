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

// do not include ompi_config.h because it kills the free/malloc defines
#include "mpi.h"
#include "ompi/mpi/cxx/mpicxx.h"
#include "opal/threads/mutex.h"

//
// These functions are all not inlined because they need to use locks to
// protect the handle maps and it would be bad to have those in headers
// because that would require that we always install the lock headers.
// Instead we take the function call hit (we're locking - who cares about
// a function call.  And these aren't exactly the performance critical
// functions) and make everyone's life easier.
//


// construction
MPI::Comm::Comm()
{
}

// copy
MPI::Comm::Comm(const Comm_Null& data) : Comm_Null(data) 
{
}


void
MPI::Comm::Free(void) 
{
    MPI_Comm save = mpi_comm;
    (void)MPI_Comm_free(&mpi_comm);
    
    OPAL_THREAD_LOCK(MPI::mpi_map_mutex);
    if (MPI::Comm::mpi_comm_map[save] != 0)
        delete MPI::Comm::mpi_comm_map[save];
    MPI::Comm::mpi_comm_map.erase(save);
    OPAL_THREAD_UNLOCK(MPI::mpi_map_mutex);
}


void
MPI::Comm::Set_errhandler(const MPI::Errhandler& errhandler)
{
    my_errhandler = (MPI::Errhandler *)&errhandler;
    OPAL_THREAD_LOCK(MPI::mpi_map_mutex);
    MPI::Comm::mpi_comm_err_map[mpi_comm] = this;
    OPAL_THREAD_UNLOCK(MPI::mpi_map_mutex);
    (void)MPI_Errhandler_set(mpi_comm, errhandler);
}


//JGS I took the const out because it causes problems when trying to
//call this function with the predefined NULL_COPY_FN etc.
int
MPI::Comm::Create_keyval(MPI::Comm::_MPI2CPP_COPYATTRFN_* comm_copy_attr_fn,
                         MPI::Comm::_MPI2CPP_DELETEATTRFN_* comm_delete_attr_fn,
                         void* extra_state)
{
    int keyval;
    (void)MPI_Keyval_create(ompi_mpi_cxx_comm_copy_attr_intercept, 
                            ompi_mpi_cxx_comm_delete_attr_intercept,
                            &keyval, extra_state);
    key_pair_t* copy_and_delete = 
        new key_pair_t(comm_copy_attr_fn, comm_delete_attr_fn); 
    OPAL_THREAD_LOCK(MPI::mpi_map_mutex);
    MPI::Comm::mpi_comm_key_fn_map[keyval] = copy_and_delete;
    OPAL_THREAD_UNLOCK(MPI::mpi_map_mutex);
    return keyval;
}


void
MPI::Comm::Free_keyval(int& comm_keyval)
{
    int save = comm_keyval;
    (void)MPI_Keyval_free(&comm_keyval);
    OPAL_THREAD_LOCK(MPI::mpi_map_mutex);
    if (MPI::Comm::mpi_comm_key_fn_map[save] != 0)
        delete MPI::Comm::mpi_comm_key_fn_map[save];
    MPI::Comm::mpi_comm_key_fn_map.erase(save);
    OPAL_THREAD_UNLOCK(MPI::mpi_map_mutex);
}


void
MPI::Comm::Set_attr(int comm_keyval, const void* attribute_val) const
{
    CommType type;
    int status;
    
    (void)MPI_Comm_test_inter(mpi_comm, &status);
    if (status) {
        type = eIntercomm;
    }
    else {
        (void)MPI_Topo_test(mpi_comm, &status);    
        if (status == MPI_CART)
            type = eCartcomm;
        else if (status == MPI_GRAPH)
            type = eGraphcomm;
        else
            type = eIntracomm;
    }
    OPAL_THREAD_LOCK(MPI::mpi_map_mutex);
    if (MPI::Comm::mpi_comm_map[mpi_comm] == 0) {
        comm_pair_t* comm_type = new comm_pair_t((Comm*) this, type);
        MPI::Comm::mpi_comm_map[mpi_comm] = comm_type;
    }
    OPAL_THREAD_UNLOCK(MPI::mpi_map_mutex);
    (void)MPI_Attr_put(mpi_comm, comm_keyval, (void*) attribute_val);
}
