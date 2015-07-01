#ifndef MIRROR_REPLICA_MPI_H
#define MIRROR_REPLICA_MPI_H

#include "mpi.h"
#include "shared_replica_mpi.h"

int SC_Mirror_Isend_generic(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                            MPI_Comm comm, MPI_Request *main_request, int isend_type);
int SC_Mirror_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                    MPI_Comm comm, MPI_Request *main_request);
int SC_Mirror_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                     MPI_Comm comm, MPI_Request *main_request);
int SC_Mirror_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                     MPI_Comm comm, MPI_Request *main_request);
int SC_Mirror_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, 
                     MPI_Comm comm, MPI_Request *main_request);

int SC_Mirror_Wait(MPI_Request *request, MPI_Status *status);

int SC_Mirror_Test(MPI_Request *request, int *flag, MPI_Status *status);

int SC_Mirror_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);

int SC_Mirror_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                    MPI_Comm comm, MPI_Request *main_request);
int SC_Mirror_Irecv_generic(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                            MPI_Comm comm, MPI_Request *main_request, int irecv_type);

struct sc_topology_list* SC_Find_Topology(MPI_Comm comm);
int SC_Mirror_Cart_create( MPI_Comm comm, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart );
int SC_Mirror_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords );
int SC_Mirror_Cart_shift ( MPI_Comm comm, int direction, int displ,
                           int *source, int *dest );
int SC_Mirror_Cart_rank (MPI_Comm comm, int *coords, int *rank );
int SC_Mirror_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords );
int SC_Mirror_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                   MPI_Comm comm, MPI_Status *status);

#endif

