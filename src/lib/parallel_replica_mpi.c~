#include "shared_replica_mpi.h"
#include "parallel_replica_mpi.h"
#include "constants.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>

/** Begin Parallel functions **/

/**
 * Builds a map which points one rank to his cooresponding replica
 * rank. Also sets the global variable is_replica to true or false.
 */
int SC_Parallel_build_process_map(int rank, int size) {

    map      = malloc(sizeof(int) * size);
    rank_map = malloc(sizeof(int) * size);
    int i;
    int half_size = size / 2;
    for(i=0;i<half_size;i++) {
        map[i] = half_size+i;
        map[half_size+i] = i;
        rank_map[i] = i;
        rank_map[half_size+i] = i;
    }

    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    if (local_rank < half_size) {
        is_replica = 0;
    } else {
        is_replica = 1;
    }
    return MPI_SUCCESS;
}

int SC_Parallel_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
    //    int local_rank;
    //    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Send FROM [%d] TO [%d]\n", local_rank, dest);

    int err;
    int actual_dest;

    if(is_replica == 0) {
        actual_dest = dest;
    } else {
        actual_dest = map[dest];
    }

    err = PMPI_Send(buf, count, datatype, actual_dest, tag, comm);

    return err;
}

int SC_Parallel_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request) {
    //    int local_rank;
    //    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Isend FROM [%d] TO [%d]\n", local_rank, dest);
    int err;
    int actual_dest;

    if(is_replica == 0) {
        actual_dest = dest;
    } else {
        actual_dest = map[dest];
    }

    err = PMPI_Isend(buf, count, datatype, actual_dest, tag, comm, main_request);

    return err;
}

int SC_Parallel_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                     MPI_Comm comm, MPI_Status *status) {
    //    int local_rank;
    //    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Recv FROM [%d] TO [%d]\n", source, local_rank);

    int actual_source;
    int err;

    if(source == MPI_ANY_SOURCE) {
        /* crap... lots of work to do now, bury it in a function,
           really just passing all args down */

        int is_irecv = 0;
        // this will block until the leader has received the message
        // and told his duplicate
        err = SC_Recv_any_source(buf, count, datatype, source, tag, comm, status);

    } else {
        // easy, just post recvs against the right rank
        if(is_replica == 0) {
            actual_source = source;
        } else {
            actual_source = map[source];
        }
        err = PMPI_Recv(buf, count, datatype, actual_source, tag, comm, status);
    }
    return err;
}

int SC_Parallel_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                      MPI_Comm comm, MPI_Request *request) {
    //    int local_rank;
    //    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Recv FROM [%d] TO [%d]\n", source, local_rank);

    int actual_source;
    int err;

    if(source == MPI_ANY_SOURCE) {
        /* crap... lots of work to do now, bury it in a function,
           really just passing all args down */

        int is_irecv = 1;
        MPI_Status *junk_stat = malloc(sizeof(MPI_Status));
        /// XXX - any source is broken to shit and back
        err = SC_Recv_any_source(buf, count, datatype, source, tag, comm, junk_stat);
        //        free(junk_stat);
    } else {

        if(is_replica == 0) {
            actual_source = source;
        } else {
            actual_source = map[source];
        }
        err = PMPI_Irecv(buf, count, datatype, actual_source, tag, comm, request);
    }
    return err;
}
