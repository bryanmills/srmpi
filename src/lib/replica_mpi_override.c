#include "mpi.h"
#include <stdio.h>
#include <unistd.h>
#include "mpi_optimized_collectives.h"
#include "constants.h"
#include "checks.h"
#include "bit_ops.h"
#include "list.h"
#include "replica_mpi_override.h"
#include "shared_replica_mpi.h"
#include "parallel_replica_mpi.h"
#include "mirror_replica_mpi.h"

/** Print at rank 0 the fact that we are actually using this library,
    not sure if there is a better way of doing this? */
int MPI_Init(int *argc, char ***argv) {
    PMPI_Init(argc, argv);
    
    int rank, err;
	err = MPI_Comm_rank( MPI_COMM_WORLD, &rank );
    // ignore error
    if (rank == 0) {
        printf("We are running replica library - bnm\n");
        if(IS_PARALLEL == 1) {
            printf("We are running in parallel mode\n");
        } else {
            printf("We are running in mirror mode\n");
        }
    }
    
    private_comm = malloc(sizeof(MPI_Comm));
    err = PMPI_Comm_dup(MPI_COMM_WORLD, &private_comm);
    has_been_killed = 0;

    int local_size;
    err = PMPI_Comm_size(MPI_COMM_WORLD, &local_size);

    char *hostname = malloc(30*sizeof(char));
    gethostname(hostname, 30);
    printf("Replica-Mode Rank %d of %d running on host %s\n", rank, local_size, hostname);

    INIT_LIST_HEAD(&irequest_list.list);
    INIT_LIST_HEAD(&isend_list.list);
    INIT_LIST_HEAD(&pending_requests.list);

    INIT_LIST_HEAD(&topo_list.list);

    failed_nodes = malloc(sizeof(int)*local_size);
    int i;
    for(i=0;i<local_size;i++) {
        failed_nodes[i] = 0;
    }

//    failed_nodes[3] = 0;
//    failed_nodes[0] = 0;

    return SC_Build_process_map(rank, local_size);
}

int MPI_Finalize( ) {
    sigkill_replica();
    cancel_pending_requests();

    PMPI_Finalize();
}

int MPI_Comm_size( MPI_Comm comm, int *size ) {
    int local_size;
    PMPI_Comm_size(comm, &local_size);

    local_size = local_size / 2;

    *size = local_size;
    return MPI_SUCCESS;
}


int MPI_Comm_rank( MPI_Comm comm, int *rank ) {
    int local_rank;
    PMPI_Comm_rank(comm, &local_rank);
    
    if(is_replica == 0) {
        *rank = local_rank;
    } else {
        //        printf("Request rank for replica %d returned %d\n", local_rank, map[local_rank]);
        fflush(NULL);
        *rank = map[local_rank];
    }

    return MPI_SUCCESS;
}

int MPI_Wait(MPI_Request *request, MPI_Status *status) {
    check_sigkill();
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Unable to determine how to override MPI_Wait");
    } else {
        return SC_Mirror_Wait(request, status);
    }
}

int MPI_Waitall(int count, MPI_Request array_of_requests[], 
                MPI_Status array_of_statuses[]) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Unable to determine how to override MPI_Waitall for parallel");
    } else {
        int i, err;
        for(i=0; i<count; i++) {
            MPI_Status *status;
            if(array_of_statuses == MPI_STATUSES_IGNORE) {
                status = MPI_STATUSES_IGNORE;
            } else {
                status = &array_of_statuses[i];
            }
            err = SC_Mirror_Wait(&array_of_requests[i], status);
        }
        return err;
    }
}

int MPI_Test(MPI_Request *request, int *flag, MPI_Status *status) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Unable to determine how to override MPI_Wait");
    } else {
        return SC_Mirror_Test(request, flag, status);
    }
}

int MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {

    if(IS_PARALLEL == 1) {
        return SC_Parallel_Send(buf, count, datatype, dest, tag, comm);
    } else {
        return SC_Mirror_Send(buf, count, datatype, dest, tag, comm);
        //        return SC_Fatal_failure("Unable to determine how to override MPI_Send");
    }
}

int MPI_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    if(IS_PARALLEL == 1) {
        return SC_Parallel_Isend(buf, count, datatype, dest, tag, comm, request);
    } else {
        return SC_Mirror_Isend(buf, count, datatype, dest, tag, comm, request);
        //        return SC_Fatal_failure("Unable to determine how to override MPI_Isend");
    }
}

int MPI_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *request) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Unable to determine how to override MPI_Isend");
        //return SC_Parallel_Issend(buf, count, datatype, dest, tag, comm, request);
    } else {
        return SC_Mirror_Issend(buf, count, datatype, dest, tag, comm, request);
    }
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
             MPI_Comm comm, MPI_Status *status) {

    if(IS_PARALLEL == 1) {
        return SC_Parallel_Recv(buf, count, datatype, source, tag, comm, status);
    } else {
        return SC_Mirror_Recv(buf, count, datatype, source, tag, comm, status);
        //        return SC_Fatal_failure("Unable to determine how to override MPI_Recv");
    }
}

int MPI_Irecv(void *buf, int count, MPI_Datatype datatype, int source,
              int tag, MPI_Comm comm, MPI_Request *request) {

    if(IS_PARALLEL == 1) {
        return SC_Parallel_Irecv(buf, count, datatype, source, tag, comm, request);
    } else {
        return SC_Mirror_Irecv(buf, count, datatype, source, tag, comm, request);
    }
}

/**
 * Make sendrecv simply use built-in send/recv functions which already
 * understand the replication. In this case perform isend/irecv and
 * then wait for both to complete.
 */
int MPI_Sendrecv(void *sendbuf, int sendcount, MPI_Datatype sendtype,
                 int dest, int sendtag, void *recvbuf, int recvcount,
                 MPI_Datatype recvtype, int source, int recvtag,
                 MPI_Comm comm, MPI_Status *status) {

    MPI_Request *send_request = malloc(sizeof(MPI_Request));
    MPI_Request *recv_request = malloc(sizeof(MPI_Request));

    MPI_Isend( sendbuf, sendcount, sendtype, dest, sendtag, comm, send_request);
    MPI_Irecv( recvbuf, recvcount, recvtype, source, recvtag, comm, recv_request);

    //    printf("Waiting for send...\n");
    // fflush(NULL);
    MPI_Wait(send_request, status);
//    printf("Waiting for recv...\n");
//    fflush(NULL);
    MPI_Wait(recv_request, status);

//    free(send_request);
//    free(recv_request);

    return MPI_SUCCESS;
}

int MPI_Cart_create( MPI_Comm comm, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart )
{
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Cart create not implemented in parallel");
    } else {
        return SC_Mirror_Cart_create(comm, ndims, dims, periods, reorder, comm_cart);
    }
}


int MPI_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods,int *coords ) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Cart coords not implemented in parallel");
    } else {
        return SC_Mirror_Cart_get(comm, maxdims, dims, periods, coords);
    }    
}
int MPI_Cart_coords ( MPI_Comm comm, int rank, int maxdims, int *coords ) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Cart coords not implemented in parallel");
    } else {
        return SC_Mirror_Cart_coords(comm, rank, maxdims, coords);
    }
}


int MPI_Cart_shift ( MPI_Comm comm, int direction, int displ,
                     int *source, int *dest ) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Cart shift not implemented in parallel");
    } else {
        return SC_Mirror_Cart_shift(comm, direction, displ, source, dest);
    }
}

int MPI_Cart_rank (MPI_Comm comm, int *coords, int *rank ) {
    if(IS_PARALLEL == 1) {
        return SC_Fatal_failure("Cart shift not implemented in parallel");
    } else {
        return SC_Mirror_Cart_rank(comm, coords, rank);
    }
}


/** This might not necessarily be linear, if the underlying gather and
    bcast are not linear, at the very least its simple **/
int LINEAR_MPI_Barrier( MPI_Comm comm )
{
	int rc = MPI_SUCCESS;

	MPI_CHECK( rc = MPI_Gather( NULL, 0, MPI_BYTE, NULL, 0,
                                MPI_BYTE, 0, comm ) );

    MPI_CHECK( rc = MPI_Bcast( NULL, 0, MPI_BYTE, 0, comm ) );

	return rc;
}


/** adapted from openmpi see,
    ~/openmpi-1.6.4/ompi/mca/coll/basic/coll_basic_barrier.c 

    This function has all leaf nodes wait for a receive, then each
    interior node sends to their children and then waits for them to
    receive it, then waits for their parent to send a message. The
    last block of code is the parent sending the message.  */
int MPI_Barrier( MPI_Comm comm )
{
    int i;
    int err;
    int peer;
    int dim;
    int hibit;
    int mask;
    int size ;
    int rank ;

	MPI_CHECK( err = PMPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = PMPI_Comm_size( comm, &size ) );

//    // hack
	int extra = 0;
//	  if(is_replica == 1) {
//	    extra = (size / 2)-1;
//	  } else {
//	    extra = 0;
//	      }
//	if(is_replica == 1) {
//	  return MPI_SUCCESS;
//	}
    size = size / 2;

    
    /* Send null-messages up and down the tree.  Synchronization at the
     * root (rank 0). */
    
    dim = opal_cube_dim(size);
    hibit = opal_hibit(rank, dim);
    --dim;
    
    /* Receive from children. */
    
    for (i = dim, mask = 1 << i; i > hibit; --i, mask >>= 1) {
        peer = rank | mask;
        if (peer < size) {

            MPI_CHECK( err = SC_Mirror_Recv( NULL, 0, MPI_BYTE, peer+extra, 
                                       BARRIER_TAG, comm, MPI_STATUS_IGNORE ) );

            //            err = MCA_PML_CALL(recv(NULL, 0, MPI_BYTE, peer,
            //                                    MCA_COLL_BASE_TAG_BARRIER,
            //                                    comm, MPI_STATUS_IGNORE));
            if (MPI_SUCCESS != err) {
                return err;
            }
        }
    }
    
    /* Send to and receive from parent. */
    
    if (rank > 0) {
        peer = rank & ~(1 << hibit);

        MPI_CHECK( err = SC_Mirror_Send( NULL, 0, MPI_BYTE, peer+extra, 
                                   BARRIER_TAG, comm ) );

        //        err =
        //            MCA_PML_CALL(send
        //                         (NULL, 0, MPI_BYTE, peer,
        //                          MCA_COLL_BASE_TAG_BARRIER,
        //                          MCA_PML_BASE_SEND_STANDARD, comm));
        if (MPI_SUCCESS != err) {
            return err;
        }

        MPI_CHECK( err = SC_Mirror_Recv( NULL, 0, MPI_BYTE, peer+extra, 
                                   BARRIER_TAG, comm, MPI_STATUS_IGNORE ) );
    
        //        err = MCA_PML_CALL(recv(NULL, 0, MPI_BYTE, peer,
        //                                MCA_COLL_BASE_TAG_BARRIER,
        //                                comm, MPI_STATUS_IGNORE));
    }
    
    /* Send to children. */
    
    for (i = hibit + 1, mask = 1 << i; i <= dim; ++i, mask <<= 1) {
        peer = rank | mask;
        if (peer < size) {

            MPI_CHECK( err = SC_Mirror_Send( NULL, 0, MPI_BYTE, peer+extra, 
                                       BARRIER_TAG, comm ) );

            //            err = MCA_PML_CALL(send(NULL, 0, MPI_BYTE, peer,
            //                                    MCA_COLL_BASE_TAG_BARRIER,
            //                                    MCA_PML_BASE_SEND_STANDARD, comm));
            if (MPI_SUCCESS != err) {
                return err;
            }
        }
    }
    
    /* All done */
    
    return MPI_SUCCESS;
}


