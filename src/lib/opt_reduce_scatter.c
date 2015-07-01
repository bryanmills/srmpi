#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "sendrecv_local.h"
#include "mpi_optimized_collectives.h"

int
MPI_Reduce_scatter_simple( void *sbuf, void *rbuf, int *rcnts,
                           MPI_Datatype dt, MPI_Op op, MPI_Comm comm )
{
	int rank, size, rc = MPI_SUCCESS, *displs = NULL, extent, i;
	int count;
	void *tbuf = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Type_size( dt, &extent ) );

	NULL_CHECK( displs = malloc( sizeof( int ) * size ) );

	displs[ 0 ] = 0;
	for( i = 0 ; i < ( size - 1 ) ; ++i ){
		displs[ i + 1 ] = displs[ i ] + rcnts[ i ];
	}

	count = displs[ size - 1 ] + rcnts[ size - 1 ];

	if( sbuf == MPI_IN_PLACE )
		sbuf = rbuf;

	if( rank == 0 ){
		NULL_CHECK( tbuf = malloc( count * extent ) );
	}

	MPI_CHECK( rc = MPI_Reduce( sbuf, tbuf, count, dt, op,
                                0, comm ) );
	
	MPI_CHECK( rc = MPI_Scatterv( tbuf, rcnts, displs, dt, rbuf,
                                  rcnts[ rank ], dt, 0, comm ) );

	free( displs );

	if( tbuf != NULL )
		free( tbuf );

	return rc;
}

int MPI_Reduce_scatter_halving(void *sbuf, void *rbuf, int *rcounts,
                               MPI_Datatype *dtype,
                               MPI_Op *op,
                               MPI_Comm *comm)
{
    int i, rank, size, count, err = MPI_SUCCESS;
    int true_lb, true_extent, lb, extent, buf_size;
    int *disps = NULL;
    char *recv_buf = NULL, *recv_buf_free = NULL;
    char *result_buf = NULL, *result_buf_free = NULL;
    int zerocounts = 0;

    /* get datatype information */
	MPI_CHECK( err = MPI_Type_get_extent( dtype, &lb, &extent ) );
	MPI_CHECK( err = MPI_Type_get_true_extent( dtype, &true_lb, &true_extent ) );

    /* Initialize */
	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    /* Find displacements and the like */
    NULL_CHECK( disps = (int*) malloc(sizeof(int) * size) );

    disps[0] = 0;
    for (i = 0; i < (size - 1); ++i) {
        disps[i + 1] = disps[i] + rcounts[i];
        if (0 == rcounts[i]) {
            zerocounts = 1;
        }
    }
    count = disps[size - 1] + rcounts[size - 1];
    if (0 == rcounts[size - 1]) {
        zerocounts = 1;
    }

    /* short cut the trivial case */
    if (0 == count) {
        free(disps);
        return MPI_SUCCESS;
    }

    buf_size = true_extent + (count - 1) * extent;

    /* Handle MPI_IN_PLACE */
    if (MPI_IN_PLACE == sbuf) {
        sbuf = rbuf;
    }
    
//    printf("Is commute %d\n", op_is_commute(op));
//    fflush(NULL);
    if (op_is_commute(op) == 1 && (zerocounts == 0)) {
        // ignore the big message case!! -bnm
        int tmp_size = 1, remain = 0, tmp_rank;

        /* temporary receive buffer.  See coll_basic_reduce.c for details on sizing */
        NULL_CHECK( recv_buf_free = (char*) malloc(buf_size) );
        recv_buf = recv_buf_free - lb;
        if (NULL == recv_buf_free) {
            goto cleanup;
        }

        /* allocate temporary buffer for results */
        NULL_CHECK(result_buf_free = (char*) malloc(buf_size));
        result_buf = result_buf_free - lb;

        /* copy local buffer into the temporary results */
        err = sndrcv_local(sbuf, count, dtype,
                           result_buf, count, dtype, comm);

        if (MPI_SUCCESS != err) goto cleanup;

        /* figure out power of two mapping: grow until larger than
           comm size, then go back one, to get the largest power of
           two less than comm size */
        while (tmp_size <= size) tmp_size <<= 1;
        tmp_size >>= 1;
        remain = size - tmp_size;

        /* If comm size is not a power of two, have the first "remain"
           procs with an even rank send to rank + 1, leaving a power of
           two procs to do the rest of the algorithm */
        if (rank < 2 * remain) {
            if ((rank & 1) == 0) {
//                printf("Send to rank %d\n", rank+1);
//                fflush(NULL);

                MPI_CHECK( err = MPI_Send( result_buf, count, dtype,
                                           rank+1, REDUCE_SCATTER_TAG,
                                           comm) );

//                err = MCA_PML_CALL(send(result_buf, count, dtype, rank + 1, 
//                                        MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                        MCA_PML_BASE_SEND_STANDARD,
//                                        comm));
                if (MPI_SUCCESS != err) goto cleanup;

                /* we don't participate from here on out */
                tmp_rank = -1;
            } else {
//                printf("Recv to rank %d\n", rank-1);
//                fflush(NULL);

                MPI_CHECK( err = MPI_Recv( recv_buf, count, dtype, rank - 1,
                                           REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE) );

//                err = MCA_PML_CALL(recv(recv_buf, count, dtype, rank - 1,
//                                        MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                        comm, MPI_STATUS_IGNORE));

                MPI_CHECK( err = MPI_Reduce_local( recv_buf, result_buf, count, 
                                                   dtype, op ) );

                /* integrate their results into our temp results */
                //                ompi_op_reduce(op, recv_buf, result_buf, count, dtype);

                /* adjust rank to be the bottom "remain" ranks */
                tmp_rank = rank / 2;
            }
        } else {
            /* just need to adjust rank to show that the bottom "even
               remain" ranks dropped out */
            tmp_rank = rank - remain;
        }

        /* For ranks not kicked out by the above code, perform the
           recursive halving */
        if (tmp_rank >= 0) {
            int *tmp_disps = NULL, *tmp_rcounts = NULL;
            int mask, send_index, recv_index, last_index;

            /* recalculate disps and rcounts to account for the
               special "remainder" processes that are no longer doing
               anything */
            NULL_CHECK( tmp_rcounts = (int*) malloc(tmp_size * sizeof(int)) );
            if (NULL == tmp_rcounts) {
                goto cleanup;
            }
            NULL_CHECK( tmp_disps = (int*) malloc(tmp_size * sizeof(int)) );
            if (NULL == tmp_disps) {
                goto cleanup;
            }

            for (i = 0 ; i < tmp_size ; ++i) {
                if (i < remain) {
                    /* need to include old neighbor as well */
                    tmp_rcounts[i] = rcounts[i * 2 + 1] + rcounts[i * 2];
                } else {
                    tmp_rcounts[i] = rcounts[i + remain];
                }
            }

            tmp_disps[0] = 0;
            for (i = 0; i < tmp_size - 1; ++i) {
                tmp_disps[i + 1] = tmp_disps[i] + tmp_rcounts[i];
            }

            /* do the recursive halving communication.  Don't use the
               dimension information on the communicator because I
               think the information is invalidated by our "shrinking"
               of the communicator */
            mask = tmp_size >> 1;
            send_index = recv_index = 0;
            last_index = tmp_size;
            while (mask > 0) {
                int tmp_peer, peer, send_count, recv_count;
                MPI_Request *request = malloc(sizeof(MPI_Request));

                tmp_peer = tmp_rank ^ mask;
                peer = (tmp_peer < remain) ? tmp_peer * 2 + 1 : tmp_peer + remain;

                /* figure out if we're sending, receiving, or both */
                send_count = recv_count = 0;
                if (tmp_rank < tmp_peer) {
                    send_index = recv_index + mask;
                    for (i = send_index ; i < last_index ; ++i) {
                        send_count += tmp_rcounts[i];
                    }
                    for (i = recv_index ; i < send_index ; ++i) {
                        recv_count += tmp_rcounts[i];
                    }
                } else {
                    recv_index = send_index + mask;
                    for (i = send_index ; i < recv_index ; ++i) {
                        send_count += tmp_rcounts[i];
                    }
                    for (i = recv_index ; i < last_index ; ++i) {
                        recv_count += tmp_rcounts[i];
                    }
                }

                /* actual data transfer.  Send from result_buf,
                   receive into recv_buf */
                if (send_count > 0 && recv_count != 0) {
//                    printf("Irecv rank %d from %d \n", rank, peer);
//                    fflush(NULL);
                    MPI_CHECK( err = MPI_Irecv( recv_buf + tmp_disps[recv_index] * extent,
                                                recv_count, dtype, peer, REDUCE_SCATTER_TAG,
                                                comm, request ) );

//                    err = MCA_PML_CALL(irecv(recv_buf + tmp_disps[recv_index] * extent,
//                                             recv_count, dtype, peer,
//                                             MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                             comm, &request));
                    if (MPI_SUCCESS != err) {
                        free(tmp_rcounts);
                        free(tmp_disps);
                        goto cleanup;
                    }                                             
                }
                if (recv_count > 0 && send_count != 0) {
//                    printf("Sending from rank %d to rank %d\n", rank, peer);
//                    fflush(NULL);

                    MPI_CHECK( err = MPI_Send( result_buf + tmp_disps[send_index] * extent,
                                               send_count, dtype, peer,
                                               REDUCE_SCATTER_TAG, comm) );

//                    err = MCA_PML_CALL(send(result_buf + tmp_disps[send_index] * extent,
//                                            send_count, dtype, peer, 
//                                            MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                            MCA_PML_BASE_SEND_STANDARD,
//                                            comm));
                    if (MPI_SUCCESS != err) {
                        free(tmp_rcounts);
                        free(tmp_disps);
                        goto cleanup;
                    }                                             
                }
                if (send_count > 0 && recv_count != 0) {
                    //                    printf("waiting at rank %d\n", rank);
                    MPI_CHECK( err = MPI_Wait( request, MPI_STATUSES_IGNORE ) );
                    //                    err = ompi_request_wait(&request, MPI_STATUS_IGNORE);
                    if (MPI_SUCCESS != err) {
                        free(tmp_rcounts);
                        free(tmp_disps);
                        goto cleanup;
                    }                                             
                }
                //                printf("done waiting at rank %d\n", rank);

                /* if we received something on this step, push it into
                   the results buffer */
                if (recv_count > 0) {
                    MPI_CHECK( err = MPI_Reduce_local( recv_buf + tmp_disps[recv_index] * extent, 
                                                       result_buf + tmp_disps[recv_index] * extent, 
                                                       recv_count, dtype, op ) );

//                    ompi_op_reduce(op, 
//                                   recv_buf + tmp_disps[recv_index] * extent, 
//                                   result_buf + tmp_disps[recv_index] * extent,
//                                   recv_count, dtype);
                }

                /* update for next iteration */
                send_index = recv_index;
                last_index = recv_index + mask;
                mask >>= 1;
            }

            /* copy local results from results buffer into real receive buffer */
            if (0 != rcounts[rank]) {
                err = sndrcv_local(result_buf + disps[rank] * extent, 
                                   rcounts[rank], dtype,
                                   rbuf, rcounts[rank], dtype, comm);

//                err = ompi_datatype_sndrcv(result_buf + disps[rank] * extent,
//                                           rcounts[rank], dtype, 
//                                           rbuf, rcounts[rank], dtype);
                if (MPI_SUCCESS != err) {
                    free(tmp_rcounts);
                    free(tmp_disps);
                    goto cleanup;
                }                                             
            }

            free(tmp_rcounts);
            free(tmp_disps);
        }

        /* Now fix up the non-power of two case, by having the odd
           procs send the even procs the proper results */
        if (rank < 2 * remain) {
            if ((rank & 1) == 0) {
                if (rcounts[rank]) {
                    MPI_CHECK( err = MPI_Recv( rbuf, rcounts[rank], dtype, rank + 1,
                                               REDUCE_SCATTER_TAG, comm, MPI_STATUS_IGNORE ) );

//                    err = MCA_PML_CALL(recv(rbuf, rcounts[rank], dtype, rank + 1,
//                                            MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                            comm, MPI_STATUS_IGNORE));
                    if (MPI_SUCCESS != err) goto cleanup;
                }
            } else {
                if (rcounts[rank - 1]) {
                    MPI_CHECK( err = MPI_Send( result_buf + disps[rank - 1] * extent,
                                               rcounts[rank - 1], dtype, rank - 1,
                                               REDUCE_SCATTER_TAG, comm ) );

//                    err = MCA_PML_CALL(send(result_buf + disps[rank - 1] * extent,
//                                            rcounts[rank - 1], dtype, rank - 1,
//                                            MCA_COLL_BASE_TAG_REDUCE_SCATTER,
//                                            MCA_PML_BASE_SEND_STANDARD,
//                                            comm));
                    if (MPI_SUCCESS != err) goto cleanup;
                }
            }            
        }

    } else {
        if (0 == rank) {
            /* temporary receive buffer.  See coll_basic_reduce.c for
               details on sizing */
            NULL_CHECK( recv_buf_free = (char*) malloc(buf_size) );
            recv_buf = recv_buf_free - lb;
            if (NULL == recv_buf_free) {
                //                err = OMPI_ERR_OUT_OF_RESOURCE;
                goto cleanup;
            }
        }

        // revert to simple one
        MPI_Reduce_scatter_simple( sbuf, rbuf, rcounts, dtype, op, comm);
//        /* reduction */
//        err =
//            comm->c_coll.coll_reduce(sbuf, recv_buf, count, dtype, op, 0,
//                                     comm, comm->c_coll.coll_reduce_module);
//
//        /* scatter */
//        if (MPI_SUCCESS == err) {
//            err = comm->c_coll.coll_scatterv(recv_buf, rcounts, disps, dtype,
//                                             rbuf, rcounts[rank], dtype, 0,
//                                             comm, comm->c_coll.coll_scatterv_module);
//        }
    }

 cleanup:
    if (NULL != disps) free(disps);
    if (NULL != recv_buf_free) free(recv_buf_free);
    if (NULL != result_buf_free) free(result_buf_free);

    return err;
}
