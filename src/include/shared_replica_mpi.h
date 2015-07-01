#ifndef SHARED_REPLICA_MPI_H
#define SHARED_REPLICA_MPI_H

#include "list.h"
#include "mpi.h"


struct request_list{
	struct list_head list;
    MPI_Request *request;
    MPI_Request *dup_request;
    MPI_Request *actual_main_request;
    int type_of_request;
    int status_of_request;
    // stash the source and/or dest
    int source_or_dest;
    int sent_main;
    int sent_dup;
    // used to remember the status to return on leader
    MPI_Status *saved_status;
    // these are for passing the any source from replica
    void *orig_buf;
    void *dup_buf;
    void *main_buf;
    int orig_count;
    MPI_Datatype orig_datatype;
    int orig_tag;
    MPI_Comm orig_comm;
    int *chosen_source;
};

struct sc_topology_list{
	struct list_head list;
    int topo_type;
    int ndims;
    int total_ranks;
    int *dims;
    int *periods;
    int *ranks;
    int *dim_offsets;
    MPI_Comm *comm;
};

/** map variable contains the mapping between a node its replica **/
extern int* map;
/** rank map contains a map from one node to the rank that the
    application thinks it is **/
extern int* rank_map;

/** Am I a replica? */
extern int is_replica;
extern int has_been_killed;

extern int* failed_nodes;

/** These two lists store irecv and isend requests. The reason is that
    on a wait/test we need to know about the status of those messages
    and their duplicate requests. **/
extern struct request_list irequest_list;
extern struct request_list isend_list;

extern struct request_list pending_requests;

/** This stores all the communicators associated with a topology,
    currently we only support cart **/
extern struct sc_topology_list topo_list;

extern MPI_Comm private_comm;


#define SC_IRECV_REQUEST_TYPE            0
#define SC_IRECV_ANY_SOURCE_REQUEST_TYPE 1
#define SC_ISEND_REQUEST_TYPE            2

// this means nothing has been done past irecv
#define SC_IRECV_ANY_SOURCE_NONE_STATUS        0
// this means the leader has chosen a source and told replicas
#define SC_IRECV_ANY_SOURCE_CHOSEN_STATUS      1
// this means the replica has received the chosen source
#define SC_IRECV_ANY_SOURCE_RECEIVED_STATUS    2
// after this is completed we remove the request so we do not need a
// 'completed' statas

#define SC_ISEND    0
#define SC_ISSEND   1
#define SC_IBSEND   2
#define SC_IRSEND   3
#define SC_IRECV    4

#define SC_REPLICA_TAG_MASK 0x128
#define SC_SIGKILL_TAG 0x130

#define SC_TOPO_TYPE_CART 1
#define SC_TOPO_TYPE_GRAPH 2

int SC_Build_process_map(int rank, int size);
int SC_Fatal_failure(char* msg);

int SC_Recv_any_source(void *buf, int count, MPI_Datatype datatype, int source, int tag,
                       MPI_Comm comm, MPI_Status *status_out);

int am_i_failed();
int is_failed(int rank);
int do_failed_death();

int test_or_fail_gracefully(MPI_Request *request, int *flag, MPI_Status *status, int source_or_dest_rank);

#endif
