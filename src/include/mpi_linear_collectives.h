#ifndef MPI_LINEAR_COLLECTIVES
#define MPI_LINEAR_COLLECTIVES

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


int MPI_Bcast_linear( void *buf, int cnt, MPI_Datatype dt, int root, 
                      MPI_Comm comm );

int MPI_Barrier_linear( MPI_Comm comm );

int MPI_Scatter_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                        void *rbuf, int rcnt, MPI_Datatype rdt, int root,
                        MPI_Comm comm );

int MPI_Scatterv_linear( void *sbuf, int *scnts, int *displs, MPI_Datatype sdt,
                         void *rbuf, int rcnt, MPI_Datatype rdt, int root,
                         MPI_Comm comm );

int MPI_Reduce_linear( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                       MPI_Op op, int root, MPI_Comm comm );

int MPI_Gather_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                       void *rbuf, int rcnt, MPI_Datatype rdt,
                       int root, MPI_Comm comm );

int MPI_Allgather_simple( void *sbuf, int scnt, MPI_Datatype sdt, 
                          void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm);

int MPI_Scan_linear( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                     MPI_Op op, MPI_Comm comm );

int MPI_Gatherv_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                        void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                        int root, MPI_Comm comm );

int MPI_Reduce_scatter_simple( void *sbuf, void *rbuf, int *rcnts,
                               MPI_Datatype dt, MPI_Op op, MPI_Comm comm );

int MPI_Alltoall_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                         void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm );

int MPI_Alltoallv_linear( void *sbuf, int *scnts, int *sdispls, MPI_Datatype sdt,
                          void *rbuf, int *rcnts, int *rdispls, MPI_Datatype rdt,
                          MPI_Comm comm );

int MPI_Allreduce_simple( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                          MPI_Op op, MPI_Comm comm );

int MPI_Allgatherv_simple( void *sbuf, int scnt, MPI_Datatype sdt,
                           void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                           MPI_Comm comm );

#endif

