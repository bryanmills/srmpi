#include "mpi.h"
#include "config.h"
#include <stdio.h>
#include "mpi_linear_collectives.h"

#ifndef SIMPLE_REPLICA_NODES
/** Print at rank 0 the fact that we are actually using this library,
    not sure if there is a better way of doing this? */
int MPI_Init(int *argc, char ***argv) {
    PMPI_Init(argc, argv);
    
    int rank, err;
	err = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    // ignore error
    if (rank == 0) {
        printf("We are running linear collective library - bnm\n");
    }
}
#endif

/** Wrapper function to make it easier to switch out different
    versions of the broadcast during debug */
int MPI_Bcast( void *buf, int cnt, MPI_Datatype dt, int root,
               MPI_Comm comm ) {

    return MPI_Bcast_linear(buf, cnt, dt, root, comm);
}

#ifndef SIMPLE_REPLICA_NODES
int MPI_Barrier( MPI_Comm comm ) {

    return MPI_Barrier_linear(comm);
}
#endif

int MPI_Reduce( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                MPI_Op op, int root, MPI_Comm comm )
{

    return MPI_Reduce_linear(sbuf, rbuf, cnt, dt, op, root, comm);
}

int MPI_Gather( void *sbuf, int scnt, MPI_Datatype sdt,
                void *rbuf, int rcnt, MPI_Datatype rdt,
                int root, MPI_Comm comm ) {

    return MPI_Gather_linear(sbuf, scnt, sdt, rbuf, rcnt, rdt, root, comm);
}

int MPI_Allgather( void *sbuf, int scnt, MPI_Datatype sdt, 
                   void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm)
{

    return MPI_Allgather_simple(sbuf, scnt, sdt, rbuf, rcnt, rdt, comm);
}

int MPI_Scan( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
              MPI_Op op, MPI_Comm comm )
{
    return MPI_Scan_linear(sbuf, rbuf, cnt, dt, op, comm);
}

int MPI_Gatherv( void *sbuf, int scnt, MPI_Datatype sdt,
                 void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                 int root, MPI_Comm comm ) {

    return MPI_Gatherv_linear(sbuf, scnt, sdt, rbuf, rcnts, displs, rdt, root, comm);
}

int MPI_Scatter( void *sbuf, int scnt, MPI_Datatype sdt,
                 void *rbuf, int rcnt, MPI_Datatype rdt, int root, 
                 MPI_Comm comm ) {

    return MPI_Scatter_linear(sbuf, scnt, sdt, rbuf, rcnt, rdt, root, comm);
}

int
MPI_Scatterv( void *sbuf, int *scnts, int *displs, MPI_Datatype sdt,
              void *rbuf, int rcnt, MPI_Datatype rdt, int root,
              MPI_Comm comm )
{
    return MPI_Scatterv_linear(sbuf, scnts, displs, sdt,
                               rbuf, rcnt, rdt, root, comm);
}

int
MPI_Reduce_scatter( void *sbuf, void *rbuf, int *rcnts,
                    MPI_Datatype dt, MPI_Op op, MPI_Comm comm )
{
    return MPI_Reduce_scatter_simple(sbuf, rbuf, rcnts,
                                     dt, op, comm);
}

int MPI_Alltoall( void *sbuf, int scnt, MPI_Datatype sdt,
                  void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm ) {

    return MPI_Alltoall_linear(sbuf, scnt, sdt, rbuf, rcnt, rdt, comm);
}

int
MPI_Alltoallv( void *sbuf, int *scnts, int *sdispls, MPI_Datatype sdt,
               void *rbuf, int *rcnts, int *rdispls, MPI_Datatype rdt,
               MPI_Comm comm )
{
    return MPI_Alltoallv_linear(sbuf, scnts, sdispls, sdt,
                                rbuf, rcnts, rdispls, rdt, comm);
}

int MPI_Allreduce( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                   MPI_Op op, MPI_Comm comm )
{
    return MPI_Allreduce_simple(sbuf, rbuf, cnt, dt, op, comm);
}

int MPI_Allgatherv( void *sbuf, int scnt, MPI_Datatype sdt, 
                    void *rbuf, int *rcnts, int *displs, 
                    MPI_Datatype rdt, MPI_Comm comm)
{

    return MPI_Allgatherv_simple(sbuf, scnt, sdt, rbuf, rcnts, displs, rdt, comm);
}

