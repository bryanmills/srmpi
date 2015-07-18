#include "mpi.h"
#include "shared_replica_mpi.h"
#include "mirror_replica_mpi.h"
#include "constants.h"
#include "checks.h"
#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>


/**
 * Builds a map which points one rank to his cooresponding replica
 * rank. Also sets the global variable is_replica to true or false.
 */
int SC_Mirror_build_process_map(int rank, int size) {

    map = malloc(sizeof(int) * size);
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



int SC_Mirror_Isend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request) {
    return SC_Mirror_Isend_generic(buf, count, datatype, dest, 
                                   tag, comm, main_request, SC_ISEND);
}

int SC_Mirror_Issend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request) {
    return SC_Mirror_Isend_generic(buf, count, datatype, dest, 
                                   tag, comm, main_request, SC_ISSEND);
}

int SC_Mirror_Ibsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request) {
    return SC_Mirror_Isend_generic(buf, count, datatype, dest, 
                                   tag, comm, main_request, SC_IBSEND);
}

int SC_Mirror_Irsend(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm, MPI_Request *main_request) {
    return SC_Mirror_Isend_generic(buf, count, datatype, dest, 
                                   tag, comm, main_request, SC_IRSEND);
}

/**
 * All the isend functions do basically the same thing from a replica
 * standpoint of view. This function calls the appropriate underlying
 * function and then registers the two requests into the internal
 * isend_list struct. Then in the wait/test function we look for the
 * requests in this list and once one of the two requests are
 * completed then they return. They do NOT wait on both requests to
 * complete, one is enough in all cases.
 */
int SC_Mirror_Isend_generic(void *buf, int count, MPI_Datatype datatype, int dest, 
                            int tag, MPI_Comm comm, MPI_Request *main_request, int isend_type) {
    //        int local_rank;
    //        PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //        printf("Isend FROM internal rank [%d] TO external rank [%d]\n", local_rank, dest);
    int err;

    // if I'm dead do nothing
    if(am_i_failed() == 1) {
        return do_failed_death();
        // should exit from above call
    }

    struct request_list *list_elem;
    list_elem = (struct request_list *)malloc(sizeof(struct request_list));
    MPI_Request *dup_request = NULL;
    MPI_Request *actual_main_request = NULL;

    // store pointers to both the main request and the duplicate
    *(&list_elem->request) = main_request;
    *(&list_elem->actual_main_request) = NULL;
    *(&list_elem->dup_request) = NULL;
    *(&list_elem->type_of_request) = SC_ISEND_REQUEST_TYPE;
    *(&list_elem->source_or_dest) = dest;
    *(&list_elem->sent_main) = 0;
    *(&list_elem->sent_dup) = 0;

    int replica_tag = tag | SC_REPLICA_TAG_MASK;
    if(is_replica == 1) {
        tag = replica_tag;
    }
    if(isend_type == SC_ISEND) {
        if(is_failed(rank_map[dest]) == 0) {
            actual_main_request = malloc(sizeof(MPI_Request));
            *(&list_elem->actual_main_request) = actual_main_request;

            // XXX - this seems really bad but trying to get experiment to work
            int size = 0, lb=0;
            MPI_CHECK( err = MPI_Type_get_extent( datatype, &lb, &size ) );
            //            MPI_CHECK( err = MPI_Type_size( datatype, &size ) );
            void *main_buf = malloc(count*size);
            memcpy(main_buf, buf, count*size);

            err = PMPI_Isend(main_buf, count, datatype, rank_map[dest], tag, comm, actual_main_request);
            *(&list_elem->sent_main) = 1;
        }
        if(is_failed(map[rank_map[dest]]) == 0) {
            dup_request = malloc(sizeof(MPI_Request));
            *(&list_elem->dup_request) = dup_request;

            // XXX - this seems really bad but trying to get experiment to work
            int size = 0, lb=0;
            MPI_CHECK( err = MPI_Type_get_extent( datatype, &lb, &size ) );
            //            MPI_CHECK( err = MPI_Type_size( datatype, &size ) );
            void *dup_buf = malloc(count*size);
            memcpy(dup_buf, buf, count*size);

            err = PMPI_Isend(dup_buf, count, datatype, map[rank_map[dest]], tag, comm, dup_request);
            *(&list_elem->sent_dup) = 1;
        }
    } else {
        // push this in here for now
        actual_main_request = malloc(sizeof(MPI_Request));
        *(&list_elem->actual_main_request) = actual_main_request;
        dup_request = malloc(sizeof(MPI_Request));
        *(&list_elem->dup_request) = dup_request;
        if(isend_type == SC_ISSEND) {
            err = PMPI_Issend(buf, count, datatype, rank_map[dest], tag, comm, actual_main_request);
            err = PMPI_Issend(buf, count, datatype, map[rank_map[dest]], tag, comm, dup_request);
        } else if(isend_type == SC_IBSEND) {
            err = PMPI_Ibsend(buf, count, datatype, rank_map[dest], tag, comm, actual_main_request);
            err = PMPI_Ibsend(buf, count, datatype, map[rank_map[dest]], tag, comm, dup_request);
        } else if(isend_type == SC_IRSEND) {
            err = PMPI_Irsend(buf, count, datatype, rank_map[dest], tag, comm, actual_main_request);
            err = PMPI_Irsend(buf, count, datatype, map[rank_map[dest]], tag, comm, dup_request);
        } else {
            char *mesg = malloc(sizeof(char)*100);
            sprintf(mesg, "Unable to determine type of isend, %d", isend_type);
            SC_Fatal_failure(mesg); // we abort after this, no worries about free
        }
    }

    // add the request_list struct to the list
    // XXXX change isend to irequest - TESTING
    list_add(&(list_elem->list), &(irequest_list.list));

    return err;
}

/**
 * This function searches the saved isend and irequest lists for the request.
 */
struct request_list* SC_Find_and_Remove_Request(MPI_Request *request) {
    struct request_list *stored_request = NULL;
    struct list_head *pos, *q;
    struct request_list *tmp;

    list_for_each_safe(pos, q, &irequest_list.list){
        // get the pointer to the actual request_list struct
        tmp = list_entry(pos, struct request_list, list);
        // look for the request_list struct for this request
        if (tmp->request == request) {
            if(stored_request != NULL) {
                char *mesg = malloc(sizeof(char)*100);
                sprintf(mesg, "We got duplicate requests inside the irequest list request[%d]", &request); 
                SC_Fatal_failure(mesg);
            }
            stored_request = tmp;
            list_del(pos);
        }
    }

    return stored_request;
}

int SC_Mirror_Wait(MPI_Request *request, MPI_Status *status) {
    int err;
    // XXX - I have no idea what to do to the status object if its actuall null... let test do it for now!
//    if(*request == MPI_REQUEST_NULL) {
//        return MPI_SUCCESS;
//    }
    // just keep testing...
    /** XXX - this could be more efficent because the test keeps
        looking up the request in the irequest list which is slow. If
        you blocked it would be faster but then we have repeated
        code. Maybe look at refactoring test to take a parameter for a
        blocking test? */

    // XXX - fake a slow down in the replica ... comment out later
//    if(is_replica == 1) {
//        sleep(10);
//    }

    int iflag = 0;
    do {
        err = SC_Mirror_Test(request, &iflag, status);
    } while(iflag == 0);
    
    return err;
}


int SC_Mirror_Test_ANY_SOURCE_leader(MPI_Request *request, int *flag, MPI_Status *status, struct request_list *stored_request) {
    int err;

    MPI_Status *junk_status = malloc(sizeof(MPI_Status));
    int local_rank;
    int internal_flag = 0;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

    if(stored_request->status_of_request == SC_IRECV_ANY_SOURCE_NONE_STATUS) {
        // The real status is what the user really wants.
        MPI_Status *real_status = malloc(sizeof(MPI_Status));
        MPI_CHECK( err = PMPI_Test(request, &internal_flag, real_status) );
        if(internal_flag == 1) {
            int *chosen_source = malloc(sizeof(int)*1);
            chosen_source[0] = real_status->MPI_SOURCE;
            // stash the real status for return later
            *(&stored_request->saved_status) = real_status;
            *(&stored_request->status_of_request) = SC_IRECV_ANY_SOURCE_CHOSEN_STATUS;
            MPI_Request *replica_request = malloc(sizeof(MPI_Request));
            MPI_CHECK( err = PMPI_Isend(chosen_source, 1, MPI_INT, map[local_rank],
                                        REPLICA_SEND_TAG, MPI_COMM_WORLD, replica_request) );
            // update the dup request to be the now pending 
            *(&stored_request->dup_request) = replica_request;
        } else {
            // free the real status, it wasn't done yet.
            free(real_status);
            // just add the original request back to the list
            list_add(&(stored_request->list), &(irequest_list.list));
            *flag = 0;
        }
    }

    /** note the fall thru here, if we just made the isend request
        above this will now execute because the status of the stored
        request was updated. */
    if (stored_request->status_of_request == SC_IRECV_ANY_SOURCE_CHOSEN_STATUS){
        // else we have chosen status so test the dup_request
        MPI_CHECK( err = PMPI_Test(stored_request->dup_request, &internal_flag, junk_status) );
        // flag will be set by last call
        if(internal_flag == 0) {
            // add the request back to the list because next time we need to find
            list_add(&(stored_request->list), &(irequest_list.list));
            *flag = 0;
        } else {
            *flag = 1;
        }
    }

    // do some cleanup if we finally got done
    if(*flag == 1) {
        // copy the status correctly from the previously saved status
        if(status != MPI_STATUS_IGNORE) {
            memcpy(status, stored_request->saved_status, sizeof(MPI_Status));
        }
        // just clean up after we got a 1
        free(stored_request->saved_status);
        free(stored_request->dup_request);
        free(stored_request);
    }
    free(junk_status);
    return err;

}

int SC_Mirror_Test_ANY_SOURCE_follower(MPI_Request *request, int *flag, MPI_Status *status, struct request_list *stored_request) {
    int err;
    int local_rank;
    int internal_flag = 0;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //                printf("Ready to wait for last request... rank:%d\n", local_rank);
    //                fflush(NULL);
    MPI_Status *junk_status = malloc(sizeof(MPI_Status));
    // wait to hear from main to know source
            
    if(stored_request->status_of_request == SC_IRECV_ANY_SOURCE_NONE_STATUS) {
        // this means we are waiting for the leader to send us the chosen source
        MPI_CHECK( err = PMPI_Test(request, &internal_flag, junk_status) );
        if(internal_flag == 1) {
            // we got the chosen source, post the orig recv to her
            MPI_Request *replica_request = malloc(sizeof(MPI_Request));

            int replica_tag = stored_request->orig_tag | SC_REPLICA_TAG_MASK;
            // post appropriate non-blocking recv
            err = PMPI_Irecv(stored_request->orig_buf, 
                             stored_request->orig_count, 
                             stored_request->orig_datatype,
                             map[stored_request->chosen_source[0]], // this is the 'other' node
                             replica_tag, 
                             stored_request->orig_comm, 
                             replica_request);
            // update the status and set the dup_request correctly
            *(&stored_request->status_of_request) = SC_IRECV_ANY_SOURCE_RECEIVED_STATUS;
            *(&stored_request->dup_request) = replica_request;
        } else {
            // just add the original request back to the list
            list_add(&(stored_request->list), &(irequest_list.list));
            *flag = 0;
        }
    } 

    // note the fall-thru here, status is updated in the above if
    if(stored_request->status_of_request == SC_IRECV_ANY_SOURCE_RECEIVED_STATUS) {
        // else we have chosen status so test the dup_request
        MPI_CHECK( err = PMPI_Test(stored_request->dup_request, &internal_flag, status) );
        // flag will be set by last call
        if(internal_flag == 0) {
            // add the request back to the list because next time we need to find
            list_add(&(stored_request->list), &(irequest_list.list));
            *flag = 0;
        } else {
            *flag = 1;

            if(status != MPI_STATUS_IGNORE) {
                // remap the source, this will actually be the replica rank
                status->MPI_SOURCE = rank_map[stored_request->chosen_source[0]];
            }
            free(stored_request->dup_request);
            free(stored_request);
        }
    }
    free(junk_status);
    return err;
}

int SC_Mirror_Test_KNOWN_SOURCE(MPI_Request *request, int *flag, MPI_Status *status, struct request_list *stored_request) {
    int err;
    int iflag = 0, iflag2 = 0;

    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    

//    printf("We believe we sent to the main %d from rank %d request_type %d\n", rank_map[stored_request->source_or_dest], local_rank, stored_request->type_of_request);
//    fflush(NULL);
    
    err = test_or_fail_gracefully(stored_request->actual_main_request, &iflag, status, rank_map[stored_request->source_or_dest]);

//    printf("We believe we sent to the dup %d from rank %d\n", map[rank_map[stored_request->source_or_dest]], local_rank);
//    fflush(NULL);
    err = test_or_fail_gracefully(stored_request->dup_request, &iflag2, status, map[rank_map[stored_request->source_or_dest]]); 

    if(iflag == 1) {
        *flag = 1;
        if(stored_request->type_of_request == SC_IRECV_REQUEST_TYPE) {
            int size = 0, lb=0;
            MPI_CHECK( err = MPI_Type_get_extent( stored_request->orig_datatype, &lb, &size ) );
            //            MPI_CHECK( err = MPI_Type_size( stored_request->orig_datatype, &size ) );
            memcpy(stored_request->orig_buf, stored_request->main_buf, stored_request->orig_count*size);
        }
    } else if(iflag2 == 1) {
        *flag = 1;
        if(stored_request->type_of_request == SC_IRECV_REQUEST_TYPE) {
            int size = 0, lb=0;
            MPI_CHECK( err = MPI_Type_get_extent( stored_request->orig_datatype, &lb, &size ) );
            //            MPI_CHECK( err = MPI_Type_size( stored_request->orig_datatype, &size ) );
            memcpy(stored_request->orig_buf, stored_request->dup_buf, stored_request->orig_count*size);
        }
    } else {
        *flag = 0;
        list_add(&(stored_request->list), &(irequest_list.list));
    }

    if(flag == 1) {
        if(status != MPI_STATUS_IGNORE) {    
            status->MPI_SOURCE = rank_map[status->MPI_SOURCE];
        }
    }
//    MPI_CHECK( err = PMPI_Test(request, &iflag, status) );
//    MPI_CHECK( err = PMPI_Test(dup_request, &iflag2, status) );
//    if(iflag == 0 || iflag2 == 0) {
//        *flag = 0;
//        // must add back to the list
//        list_add(&(stored_request->list), &(irequest_list.list));
//    } else {
//        *flag = 1;
//        if(status != MPI_STATUS_IGNORE) {    
//            status->MPI_SOURCE = rank_map[status->MPI_SOURCE];
//        }
//    }
    return err;
}

int SC_Mirror_Test(MPI_Request *request, int *flag, MPI_Status *status) {
    // obvious case.
    // XXX - I have no idea what to do to the status object if its actuall null... let test do it for now!
//    if(*request == MPI_REQUEST_NULL) {
//        return MPI_SUCCESS;
//    }

    int err;
    struct request_list *stored_request;

    /** note we remove the stored request from the irequest list in
        this call, if you need it to be there later you will need to
        add it back to the list */
    stored_request = SC_Find_and_Remove_Request(request);
    if(stored_request == NULL) {
        char *mesg;
        sprintf(mesg, "Request without a stored request, request[%d]", request);
        return SC_Fatal_failure(mesg);
    }

    if(stored_request->type_of_request == SC_IRECV_ANY_SOURCE_REQUEST_TYPE) {
        // XXX deal with failure and promotion to leader ... hard
        if(is_replica == 0) {
            err = SC_Mirror_Test_ANY_SOURCE_leader(stored_request->actual_main_request, flag, status, stored_request);
        } else {
            err = SC_Mirror_Test_ANY_SOURCE_follower(stored_request->actual_main_request, flag, status, stored_request);
        }
    } else {
        err = SC_Mirror_Test_KNOWN_SOURCE(stored_request->actual_main_request, flag, status, stored_request);
    }
    return err;
}


/**
 * Posts recvs to both the main the and the replica but returns as
 * soon as one of them return.
 */ 
int SC_Mirror_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                   MPI_Comm comm, MPI_Status *status) {
    int local_rank;
    PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Recv FROM [%d] TO [%d]\n", source, local_rank);


    int err = MPI_SUCCESS;
    if(source == MPI_ANY_SOURCE) {
        MPI_CHECK( err = SC_Recv_any_source(buf, count, datatype, source, tag, comm, status) );
        return err;
    }

    if(am_i_failed() == 1) {
        PMPI_Finalize();
        exit(EXIT_SUCCESS);
        return MPI_SUCCESS;
    }


    // XXX - this seems really bad but trying to get experiment to work
    int size = 0, lb=0;
	MPI_CHECK( err = MPI_Type_get_extent( datatype, &lb, &size ) );
    //    MPI_CHECK( err = MPI_Type_size( datatype, &size ) );
    void *dup_buf = malloc(count*size);
    void *main_buf = malloc(count*size);

    MPI_Request *main_request = NULL;
    MPI_Request *dup_request = NULL;

    int replica_tag = tag | SC_REPLICA_TAG_MASK;
    if(is_failed(rank_map[source]) == 0) {
        main_request = malloc(sizeof(MPI_Request));
        MPI_CHECK( err = PMPI_Irecv(main_buf, count, datatype, rank_map[source], tag, comm, main_request) );
        add_pending_request(main_request);
    }
    if(is_failed(map[rank_map[source]]) == 0) {
        dup_request = malloc(sizeof(MPI_Request));
        MPI_CHECK( err = PMPI_Irecv(dup_buf, count, datatype, map[rank_map[source]], replica_tag, comm, dup_request) );
        add_pending_request(dup_request);
    }
    // wait for both of them to complete
    int flag = 0, flag2 = 0;
    do {
        if(flag == 0) {
            err = test_or_fail_gracefully(main_request, &flag, status, rank_map[source]);
            if(flag == 1) {
                remove_pending_request(main_request);
                memcpy(buf, main_buf, count*size);
            }
        }
        if(flag2 == 0) {
            err = test_or_fail_gracefully(dup_request, &flag2, status, map[rank_map[source]]);
            if(flag2 == 1) {
                //                printf("Replica completed first!!!\n");
                remove_pending_request(dup_request);
                memcpy(buf, dup_buf, count*size);
            }
        }
//
//        if(flag == 0 && is_failed(rank_map[source]) == 0) {
//            MPI_CHECK( err = PMPI_Test(main_request, &flag, status) );
//        }
//        if(flag2 == 0 && is_failed(map[rank_map[source]]) == 0) {
//            MPI_CHECK( err = PMPI_Test(dup_request, &flag2, status) );
//        }
    } while(flag == 0 && flag2 == 0);

    if(status != MPI_STATUS_IGNORE) {
        // fix up status if came from dup request
        status->MPI_SOURCE = rank_map[status->MPI_SOURCE];
    }

//    free(main_request);
//    free(dup_request);

    return err;
}

/**
 * Blocking send, wait for both to receive or one to fail.
 */
int SC_Mirror_Send(void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) {
        int local_rank;
        PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //    printf("Send FROM [%d] TO [%d]\n", local_rank, dest);

    int err;

    if(am_i_failed() == 1) {
        PMPI_Finalize();
        exit(EXIT_SUCCESS);
        return MPI_SUCCESS;
    }

    MPI_Status *junk_stat = malloc(sizeof(MPI_Status));

    MPI_Request *main_request = NULL;
    MPI_Request *dup_request = NULL;
    int replica_tag = tag | SC_REPLICA_TAG_MASK;

    // if you are replica send out with replica tag
    int send_tag;
    if(is_replica == 0) {
        send_tag = tag;
    } else {
        send_tag = replica_tag;
    }

    int size = 0, lb=0;
    MPI_CHECK( err = MPI_Type_get_extent( datatype, &lb, &size ) );
    //        MPI_CHECK( err = MPI_Type_size( datatype, &size ) );
    void *dup_buf = malloc(count*size);
    void *main_buf = malloc(count*size);

    if(is_failed(rank_map[dest]) == 0) {
        main_request = malloc(sizeof(MPI_Request));
        memcpy(main_buf, buf, count*size);
        MPI_CHECK( err = PMPI_Isend(main_buf, count, datatype, rank_map[dest], send_tag, comm, main_request) );
        add_pending_request(main_request);
    }
    if(is_failed(map[rank_map[dest]]) == 0) {
        dup_request = malloc(sizeof(MPI_Request));
        // XXX - this seems really bad but trying to get experiment to work
        memcpy(dup_buf, buf, count*size);
        MPI_CHECK( err = PMPI_Isend(dup_buf, count, datatype, map[rank_map[dest]], send_tag, comm, dup_request) );
        add_pending_request(dup_request);
    }

    int flag = 0, flag2 = 0;
    do {
        if(flag == 0) {
            err = test_or_fail_gracefully(main_request, &flag, junk_stat, rank_map[dest]);
            if(flag == 1) {
                remove_pending_request(main_request);
            }
        }
        if(flag2 == 0) {
            err = test_or_fail_gracefully(dup_request, &flag2, junk_stat, map[rank_map[dest]]);
            if(flag2 == 1) {
                remove_pending_request(dup_request);
            }
        }
    } while(flag == 0 && flag2 == 0);

//    free(main_request);
//    free(dup_request);
    free(junk_stat);

    return err;
}

int SC_Mirror_Irecv(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                    MPI_Comm comm, MPI_Request *main_request) {
    return SC_Mirror_Irecv_generic(buf, count, datatype, source, tag,
                                   comm,main_request, SC_IRECV);
}

int SC_Mirror_Irecv_generic_ANY_SOURCE(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                                       MPI_Comm comm, MPI_Request *main_request, int irequest_type) {
    int err = MPI_SUCCESS;
    if(source != MPI_ANY_SOURCE) {
        SC_Fatal_failure("Attempt to call Internal function Irecv ANY_SOURCE and source is not ANY_SOURCE, fail.");
        err = MPI_ERR_OTHER;
        return err;
    }

    // init the request list item
    struct request_list *list_elem;
    list_elem = (struct request_list *)malloc(sizeof(struct request_list));
    // store pointers to both the main request and the duplicate
    *(&list_elem->request) = main_request;
    *(&list_elem->actual_main_request) = main_request;
    //    *(&list_elem->dup_request) = NULL;
    *(&list_elem->source_or_dest) = source;
    *(&list_elem->type_of_request) = SC_IRECV_ANY_SOURCE_REQUEST_TYPE;
    *(&list_elem->status_of_request) = SC_IRECV_ANY_SOURCE_NONE_STATUS;

    if(is_replica == 0) {
        /** If you are not the replica then you are leader by default
            so simply post the any source receive. The rest of this
            magic is located in the test function.  **/
        *(&list_elem->orig_comm) = comm;
        MPI_CHECK( err = PMPI_Irecv(buf, count, datatype, source, 
                                    tag, comm, main_request) );
    } else {
        /** you are the replica, post to the leader to get the choosen
            source. The magic for this is in the test function **/
        int local_rank;
        PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);

        // receive buffer for getting the chosen rank from leader
        int *chosen_source = malloc(sizeof(int)*1);
        chosen_source[0] = -1;
        *(&list_elem->chosen_source) = chosen_source;
        // save off all the original information 
        *(&list_elem->orig_buf) = buf;
        *(&list_elem->orig_count) = count;
        *(&list_elem->orig_datatype) = datatype;
        *(&list_elem->orig_tag) = tag;
        *(&list_elem->orig_comm) = comm;
       
        MPI_CHECK( err = PMPI_Irecv(chosen_source, 1, MPI_INT, rank_map[local_rank], 
                                    REPLICA_SEND_TAG, MPI_COMM_WORLD, main_request) );
    }
    // add the request item to the irequest list
    list_add(&(list_elem->list), &(irequest_list.list));
    return err;
}

/**
 * XXX - this might change when failures are involved ?
 * All the isend functions do the same thing from a replica standpoint
 * of view. This function calls the appropriate underlying function
 * and then registers the two requests into the internal isend_list
 * struct. Then in the wait/test function we look for the requests in
 * this list and once one of the two requests are completed then they
 * return. They do NOT wait on both requests to complete, one is
 * enough in all cases.
 */
int SC_Mirror_Irecv_generic(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                            MPI_Comm comm, MPI_Request *main_request, int irequest_type) {

    //            int local_rank;
    //            PMPI_Comm_rank(MPI_COMM_WORLD, &local_rank);    
    //            printf("Irecv FROM internal rank [%d] TO external rank [%d]\n", local_rank, source);
    int err = MPI_SUCCESS;

    if(source == MPI_ANY_SOURCE) {  
        err = SC_Mirror_Irecv_generic_ANY_SOURCE(buf, count, datatype, source, tag,
                                                 comm, main_request, irequest_type);
        return err;
    }

    // if I'm dead do nothing
    if(am_i_failed() == 1) {
        return do_failed_death();
        // should exit from above call
    }

    struct request_list *list_elem;
    list_elem = (struct request_list *)malloc(sizeof(struct request_list));
    // store pointers to both the main request and the duplicate
    *(&list_elem->request) = main_request;
    *(&list_elem->source_or_dest) = source;

    MPI_Request *actual_main_request = NULL;
    MPI_Request *dup_request = NULL;
    *(&list_elem->type_of_request) = SC_IRECV_REQUEST_TYPE;

    // store pointers to both the main request and the duplicate
    *(&list_elem->request) = main_request;
    *(&list_elem->actual_main_request) = NULL;
    *(&list_elem->dup_request) = NULL;
    *(&list_elem->sent_main) = 0;
    *(&list_elem->sent_dup) = 0;

    // XXX - this seems really bad but trying to get experiment to work
    int size = 0, lb=0;
    MPI_CHECK( err = MPI_Type_get_extent( datatype, &lb, &size ) );
    //    MPI_CHECK( err = MPI_Type_size( datatype, &size ) );
    void *dup_buf = malloc(count*size);
    void *main_buf = malloc(count*size);

    int replica_tag = tag | SC_REPLICA_TAG_MASK;
    if(irequest_type == SC_IRECV) {
        if(is_failed(rank_map[source]) == 0) {
            actual_main_request = malloc(sizeof(MPI_Request));
            *(&list_elem->actual_main_request) = actual_main_request;
            err = PMPI_Irecv(main_buf, count, datatype, rank_map[source], tag, comm, actual_main_request);
            *(&list_elem->sent_main) = 1;

            // need this to copy it after completion (done in test)
            *(&list_elem->orig_buf) = buf;
            *(&list_elem->orig_datatype) = datatype;
            *(&list_elem->orig_count) = count;
            *(&list_elem->main_buf) = main_buf;
        }
        if(is_failed(map[rank_map[source]]) == 0) {
            dup_request = malloc(sizeof(MPI_Request));
            *(&list_elem->dup_request) = dup_request;

            // need this to copy it after completion (done in test)
            *(&list_elem->orig_buf) = buf;
            *(&list_elem->orig_datatype) = datatype;
            *(&list_elem->orig_count) = count;
            *(&list_elem->dup_buf) = dup_buf;

            err = PMPI_Irecv(dup_buf, count, datatype, map[rank_map[source]], replica_tag, comm, dup_request);
            *(&list_elem->sent_dup) = 1;
        }
    } else {
        char *mesg = malloc(sizeof(char)*100);
        sprintf(mesg, "Unable to determine type of irequest, %d", irequest_type);
        SC_Fatal_failure(mesg); // we abort after this, no worries about free
    }

    // add the request_list struct to the list
    list_add(&(list_elem->list), &(irequest_list.list));

    //    printf("Main request %d\n", main_request);

    return err;
}

/**
 * This function searches the topology list for the one associated with the comm
 */
struct sc_topology_list* SC_Find_Topology(MPI_Comm comm) {
    struct sc_topology_list *stored_topo= NULL;
    struct list_head *pos, *q;
    struct sc_topology_list *tmp;

    list_for_each_safe(pos, q, &topo_list.list){
        // get the pointer to the actual request_list struct
        tmp = list_entry(pos, struct sc_topology_list, list);
        int result;
        MPI_Comm_compare(*tmp->comm, comm, &result);
        if (result == MPI_IDENT) {
            if(stored_topo != NULL) {
                char *mesg = malloc(sizeof(char)*100);
                sprintf(mesg, "We got duplicate topos for the same communication comm[%d]", comm); 
                SC_Fatal_failure(mesg);
            }
            stored_topo = tmp;
        }
	}
    return stored_topo;
}

int SC_Mirror_Cart_create( MPI_Comm comm, int ndims, int *dims, int *periods, int reorder, MPI_Comm *comm_cart )
{
    int err;
    int i, j, k;
    struct sc_topology_list *topo_elem;
    topo_elem = (struct sc_topology_list *)malloc(sizeof(struct sc_topology_list));
    
    *(&topo_elem->topo_type) = SC_TOPO_TYPE_CART;
    *(&topo_elem->ndims)     = ndims;
    *(&topo_elem->dims)      = dims;
    *(&topo_elem->periods)   = periods;
    
    int *dim_offsets = malloc(sizeof(int)*ndims);
    int total_needed = 1;
    for(i=0;i<ndims;i++) {
        dim_offsets[i] = 1;
        for(j=i+1;j<ndims;j++) {
            dim_offsets[i] *= dims[j];
        }
        total_needed *= dims[i];
    }
    int *ranks = malloc(sizeof(int)*total_needed);

    for(i=0,j=0;i<total_needed;i++,j++) {
        ranks[j] = i;
    }

    *(&topo_elem->ranks) = ranks;
    *(&topo_elem->dim_offsets) = dim_offsets;
    *(&topo_elem->total_ranks) = total_needed;

    list_add(&(topo_elem->list), &(topo_list.list));

    MPI_Group orig_group;
    MPI_Comm_group(comm, &orig_group); 
    err = PMPI_Comm_create ( comm, orig_group, comm_cart );

    *(&topo_elem->comm)   = comm_cart;
    return MPI_SUCCESS;


}

int SC_Mirror_Cart_coords(MPI_Comm comm, int rank, int maxdims, int *coords ) {

    struct sc_topology_list *stored_topo= SC_Find_Topology(comm);

    if(stored_topo == NULL) {
        return SC_Fatal_failure("Could not find stored topology information for communicator");
    }
    int i, j;
    for(i=0;i<stored_topo->total_ranks;i++) {
        int i_part = i;
        if(stored_topo->ranks[i] == rank) {
            for(j=0;j<stored_topo->ndims;j++) {
                coords[j] = i_part / stored_topo->dim_offsets[j];
                i_part = i_part - (stored_topo->dim_offsets[j]*coords[j]);
            }
        }
    }

    return MPI_SUCCESS;

}

int SC_Mirror_Cart_shift ( MPI_Comm comm, int direction, int displ,
                           int *source, int *dest ) {

    int local_rank;
    PMPI_Comm_rank(comm, &local_rank);

    int replica_rank = rank_map[local_rank];

    struct sc_topology_list *stored_topo= SC_Find_Topology(comm);

    if(stored_topo == NULL) {
        return SC_Fatal_failure("Could not find stored topology information for communicator");
    }

    int *coords = malloc(sizeof(int)*stored_topo->ndims);
    MPI_Cart_coords(comm, replica_rank, stored_topo->ndims, coords);
    
    int orig_loc = coords[direction];
    // source first!
    coords[direction] -= displ;
    if(coords[direction] < 0) {
        coords[direction] = stored_topo->dims[direction] - 1;
    }
    if(coords[direction] >= stored_topo->dims[direction]) {
        coords[direction] = 0;
    }
    MPI_Cart_rank(comm, coords, source);

    coords[direction] = orig_loc;

    coords[direction] += displ;
    if(coords[direction] < 0) {
        coords[direction] = stored_topo->dims[direction] - 1;
    }
    if(coords[direction] >= stored_topo->dims[direction]) {
        coords[direction] = 0;
    }
    MPI_Cart_rank(comm, coords, dest);
    
    return MPI_SUCCESS;
}

int SC_Mirror_Cart_rank (MPI_Comm comm, int *coords, int *rank ) {
    struct sc_topology_list *stored_topo= SC_Find_Topology(comm);

    if(stored_topo == NULL) {
        return SC_Fatal_failure("Could not find stored topology information for communicator");
    }

    int loc = 0, i;
    for(i=0;i<stored_topo->ndims;i++) {
        loc += coords[i]*stored_topo->dim_offsets[i];
    }
    *rank = stored_topo->ranks[loc];
    return MPI_SUCCESS;
}

int SC_Mirror_Cart_get (MPI_Comm comm, int maxdims, int *dims, int *periods, int *coords ) {
    struct sc_topology_list *stored_topo= SC_Find_Topology(comm);

    if(stored_topo == NULL) {
        return SC_Fatal_failure("Could not find stored topology information for communicator");
    }

    int local_rank;
    PMPI_Comm_rank(comm, &local_rank);

    int replica_rank = rank_map[local_rank];

    dims = stored_topo->dims;
    periods = stored_topo->periods;
    SC_Mirror_Cart_coords(comm, replica_rank, maxdims, coords);
    return MPI_SUCCESS;
}

