#ifndef MPI_OPTIMIZED_COLLECTIVES
#define MPI_OPTIMIZED_COLLECTIVES
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>


/** These are the ones without log/optimized implementation **/

int MPI_Scan_linear( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                     MPI_Op op, MPI_Comm comm );

int MPI_Gatherv_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                        void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                        int root, MPI_Comm comm );

int MPI_Scatterv_linear( void *sbuf, int *scnts, int *displs, MPI_Datatype sdt,
                         void *rbuf, int rcnt, MPI_Datatype rdt, int root,
                         MPI_Comm comm );

// allreduce makes used of reduce (optimal) and bcast (optimal) but
//  there might be a better way.
int MPI_Allreduce_simple( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                          MPI_Op op, MPI_Comm comm );

// allgatherv simple makes use of gatherv (not optimal) and bcast (optimal)
int MPI_Allgatherv_simple( void *sbuf, int scnt, MPI_Datatype sdt,
                           void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                           MPI_Comm comm );

/** From here below they are all at least log-based **/

int MPI_Bcast_log( void *buff, int count, MPI_Datatype datatype, int root,
                   MPI_Comm comm );

//int MPI_Barrier_log( MPI_Comm comm );

// in some case the optimized version falls back to linear
int MPI_Reduce_linear( void *sbuf, void *rbuf, int count, MPI_Datatype dtype,
                    MPI_Op op, int root, MPI_Comm comm );
int MPI_Reduce_log( void *sbuf, void *rbuf, int count, MPI_Datatype dtype,
                    MPI_Op op, int root, MPI_Comm comm );

int MPI_Scatter_binomial( void *sbuf, int scount, MPI_Datatype sdtype,
                          void *rbuf, int rcount, MPI_Datatype rdtype, int root,
                          MPI_Comm comm );


int MPI_Alltoall_pairwise(void *sbuf, int scount, 
                          MPI_Datatype *sdtype,
                          void* rbuf, int rcount,
                          MPI_Datatype *rdtype,
                          MPI_Comm *comm);

int MPI_Gather_binomial(void *sbuf, int scount,
                        MPI_Datatype *sdtype,
                        void *rbuf, int rcount,
                        MPI_Datatype *rdtype,
                        int root,
                        MPI_Comm *comm);

int MPI_Allgather_simple( void *sbuf, int scnt, MPI_Datatype sdt, 
                          void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm);

/** in some cases reduce scatter also uses the simple one */
int MPI_Reduce_scatter_simple( void *sbuf, void *rbuf, int *rcnts,
                               MPI_Datatype dt, MPI_Op op, MPI_Comm comm );
int MPI_Reduce_scatter_halving(void *sbuf, void *rbuf, int *rcounts,
                               MPI_Datatype *dtype,
                               MPI_Op *op,
                               MPI_Comm *comm);
int MPI_Alltoallv_pairwise(void *sbuf, int *scounts, int *sdisps,
                           MPI_Datatype *sdtype,
                           void* rbuf, int *rcounts, int *rdisps,
                           MPI_Datatype *rdtype,
                           MPI_Comm *comm);
#endif
