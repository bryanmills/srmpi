#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "topo.h"
#include "sendrecv_local.h"
#include "mpi_optimized_collectives.h"

int MPI_Alltoall_pairwise(void *sbuf, int scount, 
                          MPI_Datatype *sdtype,
                          void* rbuf, int rcount,
                          MPI_Datatype *rdtype,
                          MPI_Comm *comm)
{
    int line = -1, err = 0;
    int size  =0;
    int rank  =0;
    MPI_Aint lb;
    MPI_Aint sext;
    MPI_Aint rext;
    int step;
    int sendto, recvfrom;
    void * tmpsend, *tmprecv;

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    err = MPI_Type_get_extent( sdtype, &lb, &sext );
    err = MPI_Type_get_extent( rdtype, &lb, &rext );

//
//    err = ompi_datatype_get_extent (sdtype, &lb, &sext);
//    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl; }
//    err = ompi_datatype_get_extent (rdtype, &lb, &rext);
//    if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl; }

    
    /* Perform pairwise exchange - starting from 1 so the local copy is last */
    for (step = 1; step < size + 1; step++) {

        /* Determine sender and receiver for this step. */
        sendto  = (rank + step) % size;
        recvfrom = (rank + size - step) % size;

        /* Determine sending and receiving locations */
        tmpsend = (char*)sbuf + sendto * sext * scount;
        tmprecv = (char*)rbuf + recvfrom * rext * rcount;
        printf("Getting ready to send/recv\n");
        fflush(NULL);
		MPI_CHECK( err = MPI_Sendrecv( tmpsend, scount, sdtype, sendto,
                                       ALLTOALL_TAG,
                                       tmprecv, rcount, rdtype, recvfrom,
                                       ALLTOALL_TAG,
                                       comm, MPI_STATUS_IGNORE) );
        printf("After ready to send/recv\n");
        fflush(NULL);

//        /* send and receive */
//        err = ompi_coll_tuned_sendrecv( tmpsend, scount, sdtype, sendto, 
//                                        MCA_COLL_BASE_TAG_ALLTOALL,
//                                        tmprecv, rcount, rdtype, recvfrom, 
//                                        MCA_COLL_BASE_TAG_ALLTOALL,
//                                        comm, MPI_STATUS_IGNORE, rank);
        if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }
    }

    return MPI_SUCCESS;
 
 err_hndl:
    return err;
}
