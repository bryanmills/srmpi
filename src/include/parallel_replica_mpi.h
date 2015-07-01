#ifndef PARALLEL_REPLICA_MPI_H
#define PARALLEL_REPLICA_MPI_H

#include "mpi.h"

int SC_Parallel_build_process_map(int rank, int size);

int SC_Parallel_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm);
int SC_Parallel_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request);

int SC_Parallel_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                     MPI_Comm comm, MPI_Status *status);
int SC_Parallel_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                      MPI_Comm comm, MPI_Request *request);
int SC_Parallel_Recv_any_source(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                                MPI_Comm comm, MPI_Status *status, int is_irecv, MPI_Request *request_out);


#endif

