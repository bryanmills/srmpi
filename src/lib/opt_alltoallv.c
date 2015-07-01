#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "topo.h"
#include "sendrecv_local.h"
#include "mpi_optimized_collectives.h"

int
MPI_Alltoallv_pairwise(void *sbuf, int *scounts, int *sdisps,
                       MPI_Datatype *sdtype,
                       void* rbuf, int *rcounts, int *rdisps,
                       MPI_Datatype *rdtype,
                       MPI_Comm *comm)
{
    int line = -1, err = 0;
    int rank, size, step;
    int sendto, recvfrom;
    void *psnd, *prcv;
    MPI_Aint sext, rext, lb;

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    err = MPI_Type_get_extent( sdtype, &lb, &sext );
    err = MPI_Type_get_extent( rdtype, &lb, &rext );

//	MPI_CHECK( err = MPI_Type_size( sdtype, &sext ) ); 
//	MPI_CHECK( err = MPI_Type_size( rdtype, &rext ) ); 

    psnd = ((char *) sbuf) + (sdisps[rank] * sext);
    prcv = ((char *) rbuf) + (rdisps[rank] * rext);

    if (0 != scounts[rank]) {
        err = sndrcv_local(psnd, scounts[rank], sdtype,
                           prcv, rcounts[rank], rdtype, comm);

        if (MPI_SUCCESS != err) {
            return err;
        }
    }

    /* If only one process, we're done. */
    if (1 == size) {
        return MPI_SUCCESS;
    }

    /* Perform pairwise exchange starting from 1 since local exhange is done */
    for (step = 1; step < size + 1; step++) {

        /* Determine sender and receiver for this step. */
        sendto  = (rank + step) % size;
        recvfrom = (rank + size - step) % size;

        /* Determine sending and receiving locations */
        psnd = (char*)sbuf + sdisps[sendto] * sext;
        prcv = (char*)rbuf + rdisps[recvfrom] * rext;

        /* send and receive */
		MPI_CHECK( err = MPI_Sendrecv( psnd, scounts[sendto], sdtype, sendto, 
                                       ALLTOALLV_TAG,
                                       prcv, rcounts[recvfrom], rdtype, recvfrom, 
                                       ALLTOALLV_TAG,
                                       comm, MPI_STATUS_IGNORE) );

        if (err != MPI_SUCCESS) { line = __LINE__; goto err_hndl;  }
    }

    return MPI_SUCCESS;
 
 err_hndl:
    return err;
}
