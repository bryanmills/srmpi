#include "replica_mpi_override.h"
#include "shared_replica_mpi.h"
#include "parallel_replica_mpi.h"
#include "constants.h"
#include "checks.h"
#include "mpi.h"

int* failed_nodes;

/** map variable contains the mapping between a node its replica **/
int* map;
/** rank map contains a map from one node to the rank that the
    application thinks it is **/
int* rank_map;

/** Am I a replica? */
int is_replica;
int has_been_killed;

/** These two lists store irecv and isend requests. The reason is that
    on a wait/test we need to know about the status of those messages
    and their duplicate requests. **/
struct request_list irequest_list;
struct request_list isend_list;
struct request_list pending_requests;

/** This stores all the communicators associated with a topology,
    currently we only support cart **/
struct sc_topology_list topo_list;

MPI_Comm private_comm;

int SC_Build_process_map(int rank, int size) {

    if(size % 2 == 1) {
        // we could do something like not use the last node but...
        char* msg = malloc(sizeof(char)*200);
        sprintf(msg, "The number of nodes must be evenly divided by 2 in order to set node assignment for replication. Size [%d]", size);
        return SC_Fatal_failure(msg);
        // we are dying, no free
    }

    if(IS_PARALLEL == 1) {
        return SC_Parallel_build_process_map(rank, size);
    } else {
        return SC_Mirror_build_process_map(rank, size);
        //return SC_Fatal_failure("Unable to determine how to build process map");
    }
}

int SC_Fatal_failure(char* msg) {
    printf("FATAL : ");
    printf(msg);
    printf("\n");
    fflush(NULL);
    assert( 1 == 0);
    return MPI_Abort( MPI_COMM_WORLD, -1);
}


/**
 * This elects the main process as the leader and blocks until that
 * task has received a message. Once they have received a message then
 * we inform the replica which rank was the source and then the
 * replica posts a receive to that rank explicitly.
 */
int SC_Recv_any_source(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, MPI_Status *status_out) {
    int err;
    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank); 

    MPI_Status *status = malloc(sizeof(MPI_Status));
    MPI_Request *request = malloc(sizeof(MPI_Request));
    /* need to do the following, one of the replicas will actually
       grab a message and once that happens need to communicate to all
       other replicas which source was picked.
    */
    // at the end this variable will hold the real source
    int any_source_real = -1;
    // by default the non-replica is elected the leader
    if(is_replica == 0) {
        //        printf("Getting read to post recv\n");
        err = PMPI_Irecv(buf, count, datatype, source, /* ANY_SOURCE */
                         tag, comm, request);
        // wait for this guy to get picked up
        PMPI_Wait(request, status_out);
        //        printf("Main returned ... with source %d\n", status_out->MPI_SOURCE);
        //        fflush(NULL);
        any_source_real = status_out->MPI_SOURCE;
        /* XXX - request_out isn't really want you need to send back
         * to the caller, because after its waited on the request
         * appears to forget stuff. Tried be below Test call, need to
         * look at probe? Currently kinda broken for irecv.
         */
        //        do {
        //            err = PMPI_Test(request_out, &flag, status_out);
        //            if(flag == 1) {
        //                // we have a message figure out from who
        //                any_source_real = status_out->MPI_SOURCE;
        //                //printf("Found source to be %d\n", any_source_real);
        //                //fflush(NULL);
        //            }
        //        } while(flag == 0);
        /** We now need to send the actual source to our replica. The
            problem here is that the replica needs to both receive the
            actual source and then send the message to the right
            place. We need to not return from this function until both
            of those steps are done. Hacked it having the replica call
            me back and tell me she is done, hence the irecv and wait */
        int *junk = malloc(sizeof(int));
        PMPI_Irecv(junk, 1, MPI_INT, map[local_rank], REPLICA_SEND_TAG, comm, request);

        PMPI_Send(&any_source_real, 1, MPI_INT, map[local_rank], REPLICA_SEND_TAG, comm);
        err = PMPI_Wait(request, status);
    } else {
        // we are one of the replicas, listen for the actual source
        err = PMPI_Recv(&any_source_real, 1, MPI_INT, map[local_rank], REPLICA_SEND_TAG, comm, status);
        // post the real recv from the same source

        //        printf("Found source on replica to be %d\n", any_source_real);
        //        fflush(NULL);
        // Note that this the status you want to passed back to the calling method because it will have the proper MPI_SOURCE set
        // XXX - make sure this tag thing works in parallel
        int replica_tag = tag | SC_REPLICA_TAG_MASK;
        err = PMPI_Recv(buf, count, datatype, map[any_source_real], replica_tag, comm, status_out);
        // overwrite the MPI_SOURCE on the status object
        status_out->MPI_SOURCE = rank_map[status_out->MPI_SOURCE];

        int junk = 1;
        /**
         * ??? I kinda want to do an isend here, however noone knows to
         * wait on the send to complete there. ???
         */
        err = PMPI_Send(&junk, 1, MPI_INT, map[local_rank], REPLICA_SEND_TAG, comm);
            
    }
    free(request);
    free(status);
    return err;
}


int am_i_failed() {
    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);
    return failed_nodes[local_rank];
}

int is_failed(int rank) {
    return failed_nodes[rank];
}

int do_failed_death() {
    // XXX - need to cancel/wait for all pending async requests
    PMPI_Finalize();
    exit(EXIT_SUCCESS);
    return MPI_SUCCESS;
}

/** tests a request or check to see if the rank has failed, if dead
    return as if it completed but cancel request if dead **/
int test_or_fail_gracefully(MPI_Request *request, int *flag, MPI_Status *status, int source_or_dest_rank) {
    int err = MPI_SUCCESS;
    if(is_failed(source_or_dest_rank) == 1) {
        // well crap, our source or dest is dead for this request.
        if(request != NULL) {
            MPI_CHECK( err = MPI_Cancel(request) ); // XXX - is this blocking?
        }
        *flag = 1;
    } else {
        if(request == NULL) {
            *flag = 1;
        } else {
            MPI_CHECK( err = PMPI_Test(request, flag, status) );
        }
    }
    return err;
}

int add_pending_request(MPI_Request *request) {
    struct request_list *my_request;
    my_request = (struct request_list *)malloc(sizeof(struct request_list));
    *(&my_request->request) = request;
    list_add(&(my_request->list), &(pending_requests.list));
}

int remove_pending_request(MPI_Request *request) {
    struct list_head *pos, *q;
    struct request_list *tmp;

    list_for_each_safe(pos, q, &pending_requests.list){
        // get the pointer to the actual request_list struct
        tmp = list_entry(pos, struct request_list, list);
        // look for the request_list struct for this request
        if (tmp->request == request) {
            list_del(pos);
        }
    }
}

int cancel_pending_requests() {
    struct list_head *pos, *q;
    struct request_list *tmp;
    int err = MPI_SUCCESS;
    int flag = 0;

    list_for_each_safe(pos, q, &pending_requests.list){
        // get the pointer to the actual request_list struct
        tmp = list_entry(pos, struct request_list, list);
        MPI_CHECK( err = PMPI_Test(tmp->request, &flag, MPI_STATUS_IGNORE) );
        if(flag == 0) {
            //            printf("Cancel stuff\n");
            MPI_CHECK( err = MPI_Cancel(tmp->request) ); // XXX - is this blocking?
        }
        list_del(pos);
    }
    return err;
}

int sigkill_replica() {
//    if(has_been_killed == 1) {
//        return 0;
//    }
    // XXX hack 
    if(is_replica == 1) {
        return 0;
    }
    int local_rank;
    int err;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank); 

    // at first I was just aborting... still can if this doesn't work
//    if(local_rank == 0) {
//        printf("Killing everyone because rank 0 got the end\n");
//        fflush(NULL);
//        MPI_Abort(MPI_COMM_WORLD, 0);
//    }
//    return 0;
    if(is_replica == 0) {
        int *buf = malloc(sizeof(int));
        buf[0] = 1;
        int count = 1;
        MPI_CHECK( err = PMPI_Send(&buf, count, MPI_INT, map[local_rank], 8, private_comm) );
        return 0;
    }
    return 0;
}

MPI_Request *sigkill_request = NULL;

int check_sigkill() {
    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank); 
    int flag = 1;
    int err;
    if(sigkill_request == NULL) {
        int *buf = malloc(sizeof(int));
        sigkill_request = malloc(sizeof(MPI_Request));
        MPI_CHECK( err = PMPI_Irecv(buf, 1, MPI_INT, map[local_rank], 8, private_comm, sigkill_request));
    }
    //MPI_CHECK( err = PMPI_Iprobe(map[local_rank], REPLICA_SIGKILL_TAG, private_comm, &flag, MPI_STATUS_IGNORE) );
    //    printf("Buffer back from recv is %d\n", buf[0]);
    
    MPI_CHECK( err = PMPI_Test(sigkill_request, &flag, MPI_STATUS_IGNORE) );
    //    printf("checking to see if sigkill was sent to me at rank:%d from:%d flag:%d tag:%d\n", local_rank, map[local_rank], flag, 8);
    if(flag == 1) {
        printf("Found I should die... rank:%d\n", local_rank);
        has_been_killed = 1;
        // kill myself
        //cancel_pending_requests();
        do_failed_death();
    }
    return 0;
}

