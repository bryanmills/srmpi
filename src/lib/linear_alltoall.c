#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "sendrecv_local.h"
#include "mpi_linear_collectives.h"


int
MPI_Alltoall_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                     void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm )
{
	int rc = MPI_SUCCESS, size, rank, sndextent, rcvextent, reqcnt = 0;
	int i;
	MPI_Request *reqs = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Type_size( sdt, &sndextent ) ); 
	MPI_CHECK( rc = MPI_Type_size( rdt, &rcvextent ) ); 

	NULL_CHECK( reqs = malloc( sizeof( MPI_Request ) *
                               ( size - 1 ) * 2 ) );

	memmove( rbuf + ( rank * rcnt * rcvextent ),
             sbuf + ( rank * scnt * sndextent ),
             rcnt * rcvextent );

	for( i = 0 ; i < size ; i++ ){

		if( rank == i )
			continue;

		MPI_CHECK( rc = MPI_Isend( sbuf + ( i * scnt * sndextent ),
                                   scnt, sdt, i, ALLTOALL_TAG,
                                   comm, &reqs[ reqcnt++ ] ) );

		MPI_CHECK( rc = MPI_Irecv( rbuf + ( i * rcnt * rcvextent ),
                                   rcnt, rdt, i, ALLTOALL_TAG,
                                   comm, &reqs[ reqcnt++ ] ) );
	}

	MPI_CHECK( rc = MPI_Waitall( reqcnt, reqs, MPI_STATUSES_IGNORE ) );	
	free( reqs );
	
	return rc;
}

// I couldn't get this algorithm to work -bnm
//
//int
//MPI_Alltoall_optimized_linear(void *sbuf, int scount,
//                              MPI_Datatype *sdtype,
//                              void *rbuf, int rcount,
//                              MPI_Datatype *rdtype,
//                              MPI_Comm *comm)
//{
//    printf("********************** Alltoall\n");
//    fflush(NULL);
//    int i;
//    int rank;
//    int size = 0;
//    int size2 = 0;
//    int err;
//    int nreqs;
//    char *psnd;
//    char *prcv;
//    int lb = 0;
//    int sndinc;
//    int rcvinc;
//
//    MPI_Request **req;
//    MPI_Request **sreq;
//    MPI_Request **rreq;
//
//    /* Initialize. */
//
//    printf("Size %d\n", size2);
//    MPI_CHECK( err = MPI_Type_get_extent( sdtype, &lb, &sndinc ) );
//    printf("Size %d\n", size2);
//	MPI_CHECK( err = MPI_Type_get_extent( rdtype, &lb, &rcvinc ) );
//    printf("lb %d\n", lb);
//
//	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
//	MPI_CHECK( err = MPI_Comm_size( comm, &size2 ) );
//    if(MPI_SUCCESS != err) {
//        printf("Error getting size");
//    }
//
//	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );
//    sndinc *= scount;
//    rcvinc *= rcount;
//
//    printf("RcvInc %d\n", rcvinc);
//    printf("lb %d\n", lb);
//    printf("Size %d\n", size);
//
//    /* simple optimization */
//
//    psnd = ((char *) sbuf) + (rank * sndinc);
//    prcv = ((char *) rbuf) + (rank * rcvinc);
//
//    err = sndrcv_local(psnd, scount, sdtype, prcv, rcount, rdtype, comm);
//
//    printf("Size %d\n", size);
//    //    err = ompi_datatype_sndrcv(psnd, scount, sdtype, prcv, rcount, rdtype);
//    if (MPI_SUCCESS != err) {
//        return err;
//    }
//
//    /* If only one process, we're done. */
//
//    if (1 == size) {
//        return MPI_SUCCESS;
//    }
//
//    /* Initiate all send/recv to/from others. */
//    printf("Size %d\n", size);
//    req = rreq = (MPI_Request**) malloc(sizeof(MPI_Request) * (size*2));
//    sreq = rreq + size - 1;
//
//    printf("Size %d\n", size);
//    prcv = (char *) rbuf;
//    psnd = (char *) sbuf;
//
//    /* Post all receives first -- a simple optimization */
//
//
//    printf("********************** Alltoall before recv size %d\n", size);
//    fflush(NULL);
//
//    for (nreqs = 0, i = (rank + 1) % size; i != rank; i = (i + 1) % size, ++rreq, ++nreqs) {
//        printf("Recv on %d\n", i);
//        fflush(NULL);
//		MPI_CHECK( err = MPI_Irecv( prcv + (i * rcvinc), rcount, rdtype, i,
//                                    ALLTOALL_TAG, comm, rreq ) )
//
//            //rbuf + ( i * rcnt * rcvextent ),
//            //                                   rcnt, rdt, i, ,
//            //                                   comm, &reqs[ reqcnt++ ] ) );
//
//            //        err =
//            //            MCA_PML_CALL(irecv_init
//            //                         ());
//            //        if (MPI_SUCCESS != err) {
//            //            mca_coll_basic_free_reqs(req, nreqs);
//            //            return err;
//            //        }
//            }
//
//    /* Now post all sends */
//
//    printf("********************** Alltoall before send\n");
//    fflush(NULL);
//
//    for (nreqs = 0, i = (rank + 1) % size; i != rank; i = (i + 1) % size, ++sreq, ++nreqs) {
//
//		MPI_CHECK( err = MPI_Isend( psnd + (i * sndinc), scount, sdtype, i,
//                                    ALLTOALL_TAG, comm, sreq));
//
//        //sbuf + ( i * scnt * sndextent ),
//        //                                   scnt, sdt, i, ALLTOALL_TAG,
//        //                                   comm, &reqs[ reqcnt++ ] ) );
//        //
//        //
//        //        err =
//        //            MCA_PML_CALL(isend_init
//        //                         (psnd + (i * sndinc), scount, sdtype, i,
//        //                          MCA_COLL_BASE_TAG_ALLTOALL,
//        //                          MCA_PML_BASE_SEND_STANDARD, comm, sreq));
//        //        if (MPI_SUCCESS != err) {
//        //            mca_coll_basic_free_reqs(req, nreqs);
//        //            return err;
//        //        }
//    }
//
//    nreqs = (size - 1) * 2;
//    /* Start your engines.  This will never return an error. */
//
//    //    MCA_PML_CALL(start(nreqs, req));
//
//    /* Wait for them all.  If there's an error, note that we don't
//     * care what the error was -- just that there *was* an error.  The
//     * PML will finish all requests, even if one or more of them fail.
//     * i.e., by the end of this call, all the requests are free-able.
//     * So free them anyway -- even if there was an error, and return
//     * the error after we free everything. */
//
//    //    err = ompi_request_wait_all(nreqs, req, MPI_STATUSES_IGNORE);
//
//    /* Free the reqs */
//
//    //    mca_coll_basic_free_reqs(req, nreqs);
//
//    printf("********************** Alltoall before check\n");
//    fflush(NULL);
//
//    MPI_CHECK( err = MPI_Waitall( nreqs, req, MPI_STATUSES_IGNORE ) );	
//
//	free( rreq );
//	free( sreq );
//
//
//    /* All done */
//
//    return err;
//}


