/**
 * VampirTrace
 * http://www.tu-dresden.de/zih/vampirtrace
 *
 * Copyright (c) 2005-2008, ZIH, TU Dresden, Federal Republic of Germany
 *
 * Copyright (c) 1998-2005, Forschungszentrum Juelich, Juelich Supercomputing
 *                          Centre, Federal Republic of Germany
 *
 * See the file COPYING in the package base directory for details
 **/

#include "vt_sync.h"
#include "vt_iowrap.h"
#include "vt_pform.h"

#include <stdlib.h>
#include <stdio.h>

#define VT_LOOP_COUNT 10

static int64_t vt_offset_master(int64_t* ltime, int slave, MPI_Comm comm)
{
  int i, min;
  int64_t tsend[VT_LOOP_COUNT], trecv[VT_LOOP_COUNT];
  int64_t pingpong_time, sync_time;
  MPI_Status stat;

  /* exchange VT_LOOP_COUNT ping pong messages with slave */

  for (i = 0; i < VT_LOOP_COUNT; i++)
    {
      tsend[i] = vt_pform_wtime();
      PMPI_Send(NULL, 0, MPI_INT, slave, 1, comm);
      PMPI_Recv(NULL, 0, MPI_INT, slave, 2, comm, &stat);
      trecv[i] = vt_pform_wtime();
    }

  /* select ping pong with shortest transfer time */
  
  pingpong_time = trecv[0] - tsend[0];
  min = 0;

  for (i = 1; i < VT_LOOP_COUNT; i++)
    if ((trecv[i] - tsend[i]) < pingpong_time)
      {
	pingpong_time = (trecv[i] - tsend[i]);
	min = i;
      }
  
  sync_time = tsend[min] + (pingpong_time / 2);

  /* send sync_time together with corresponding measurement index to slave */
  
  PMPI_Send(&min, 1, MPI_INT, slave, 3, comm);
  PMPI_Send(&sync_time, 1, MPI_LONG_LONG_INT, slave, 4, comm);

  /* the process considered as the global clock returns 0 as offset */

  *ltime = vt_pform_wtime();
  return 0;
}


static int64_t vt_offset_slave(int64_t* ltime, int master, MPI_Comm comm)
{
  int i, min;
  int64_t tsendrecv[VT_LOOP_COUNT];
  int64_t sync_time;
  MPI_Status stat;

  for (i = 0; i < VT_LOOP_COUNT; i++)
    {
      PMPI_Recv(NULL, 0, MPI_INT, master, 1, comm, &stat);
      tsendrecv[i] = vt_pform_wtime();
      PMPI_Send(NULL, 0, MPI_INT, master, 2, comm);
    }

  /* receive corresponding time together with its index from master */

  PMPI_Recv(&min, 1, MPI_INT, master, 3, comm,  &stat);
  PMPI_Recv(&sync_time, 1, MPI_LONG_LONG_INT, master, 4, comm, &stat);

  *ltime = tsendrecv[min];
  return sync_time - *ltime; 
}

int64_t vt_offset(int64_t* ltime, MPI_Comm comm)
{
  int i;
  int myrank, myrank_host, myrank_sync;
  int numnodes;
  int64_t offset;

  MPI_Comm host_comm;
  MPI_Comm sync_comm;

  VT_SUSPEND_IO_TRACING();

  offset = 0;
  *ltime = vt_pform_wtime();

  /* barrier at entry */
  PMPI_Barrier(comm);

  PMPI_Comm_rank(comm, &myrank);

  /* create communicator containing all processes on the same node */

  PMPI_Comm_split(comm, (vt_pform_node_id() & 0x7FFFFFFF), 0, &host_comm);
  PMPI_Comm_rank(host_comm, &myrank_host);

  /* create communicator containing all processes with rank zero in the
     previously created communicators */
  
  PMPI_Comm_split(comm, myrank_host, 0, &sync_comm);
  PMPI_Comm_rank(sync_comm, &myrank_sync);
  PMPI_Comm_size(sync_comm, &numnodes);

  /* measure offsets between all nodes and the root node (rank 0 in sync_comm) */

  if (myrank_host == 0)
    {
      for (i = 1; i < numnodes; i++)
	{
	  PMPI_Barrier(sync_comm);
	  if (myrank_sync == i)
	    offset = vt_offset_slave(ltime, 0, sync_comm);
	  else if (myrank_sync == 0)
	    offset = vt_offset_master(ltime, i, sync_comm);
	}
    }

  /* distribute offset and ltime across all processes on the same node */

  PMPI_Bcast(&offset, 1, MPI_LONG_LONG_INT, 0, host_comm);
  PMPI_Bcast(ltime, 1, MPI_LONG_LONG_INT, 0, host_comm);

  /* barrier at exit */
  PMPI_Barrier(comm);

  VT_RESUME_IO_TRACING();

  return offset;
}























