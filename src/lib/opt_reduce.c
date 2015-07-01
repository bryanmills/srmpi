#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "bit_ops.h"
#include "constants.h"
#include <stdbool.h>
#include "mpi_optimized_collectives.h"

/** In some cases the cubic reduce doesn't work, use this as a
    fall-back **/
int
MPI_Reduce_linear( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                   MPI_Op op, int root, MPI_Comm comm )
{
	int i, rank, rc, size;
	MPI_Aint lb, extent;
	char *free_buf = NULL, *pml_buf = NULL, *inplace_temp = NULL;
	char *inbuf = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );

	if( root == MPI_PROC_NULL ){
		return MPI_SUCCESS;
	} else if( root != rank ){

		MPI_CHECK( rc = MPI_Send( sbuf, cnt, dt, root, 
                                  REDUCE_TAG, comm ) );
		return rc;
	}

	MPI_CHECK( rc = MPI_Type_get_extent( dt, &lb, &extent ) );

	if( sbuf == MPI_IN_PLACE ){
		sbuf = rbuf;
		NULL_CHECK( inplace_temp = malloc( cnt * extent ) );
		rbuf = inplace_temp - lb;
	}

	if( size > 1 ){
		NULL_CHECK( free_buf = malloc( extent * cnt ) );
		pml_buf = free_buf - lb;
	}

	if( rank == ( size - 1 ) ){
		memcpy( rbuf, sbuf, extent * cnt );
	} else {
		MPI_CHECK( rc = MPI_Recv( rbuf, cnt, dt, size - 1, 
                                  REDUCE_TAG, comm,
                                  MPI_STATUS_IGNORE ) );
	}
	
	/*
	 * Loop receiving and reducing only subset of OPs may be supported
	 */	
	for( i = size - 2 ; i >= 0 ; i-- ){

		if( rank == i ){
			inbuf = sbuf;
		} else {
			MPI_CHECK( rc = MPI_Recv( pml_buf, cnt, dt, i, 
                                      REDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE ) 
                       );
			inbuf = pml_buf;
		}
		MPI_CHECK( rc = MPI_Reduce_local( inbuf, rbuf, cnt, 
                                          dt, op ) );
	}

	if( inplace_temp ){
		memcpy( sbuf, inplace_temp, extent * cnt );
		free( inplace_temp );
	}

	if( free_buf != NULL )
		free( free_buf );

	return rc;
}

int op_is_commute(MPI_Op op) {

    //    return 0;
    //    return MPI_OP_FLAGS_COMMUTE;
    // XXX - assume false, this is the pessimistic approach, not sure how to determine this
    return 1;
    int is_commute = 1;
    MPI_Op_commutative(op, &is_commute);
    //MPI_Op_commutative(op, *is_commute);
//    
    return is_commute;
    //    return (bool) (0 != (op->o_flags & OMPI_OP_FLAGS_COMMUTE));
}


/*
 *	reduce_log_intra
 *
 *	Function:	- reduction using O(log N) algorithm
 *	Accepts:	- same as MPI_Reduce()
 *	Returns:	- MPI_SUCCESS or error code
 *
 * 
 *      Performing reduction on each dimension of the hypercube.
 *	An example for 8 procs (dimensions = 3):
 *
 *      Stage 1, reduce on X dimension,  1 -> 0, 3 -> 2, 5 -> 4, 7 -> 6
 *
 *          6----<---7		proc_0: 0+1
 *         /|       /|		proc_1: 1
 *        / |      / |		proc_2: 2+3
 *       /  |     /  |		proc_3: 3
 *      4----<---5   |		proc_4: 4+5
 *      |   2--< |---3		proc_5: 5
 *      |  /     |  /		proc_6: 6+7
 *      | /      | /		proc_7: 7
 *      |/       |/
 *      0----<---1
 *
 *	Stage 2, reduce on Y dimension, 2 -> 0, 6 -> 4
 *
 *          6--------7		proc_0: 0+1+2+3
 *         /|       /|		proc_1: 1
 *        v |      / |		proc_2: 2+3
 *       /  |     /  |		proc_3: 3
 *      4--------5   |		proc_4: 4+5+6+7
 *      |   2--- |---3		proc_5: 5
 *      |  /     |  /		proc_6: 6+7
 *      | v      | /		proc_7: 7
 *      |/       |/
 *      0--------1
 *
 *	Stage 3, reduce on Z dimension, 4 -> 0
 *
 *          6--------7		proc_0: 0+1+2+3+4+5+6+7
 *         /|       /|		proc_1: 1
 *        / |      / |		proc_2: 2+3
 *       /  |     /  |		proc_3: 3
 *      4--------5   |		proc_4: 4+5+6+7
 *      |   2--- |---3		proc_5: 5
 *      v  /     |  /		proc_6: 6+7
 *      | /      | /		proc_7: 7
 *      |/       |/
 *      0--------1
 *
 *
 */
int
MPI_Reduce_log( void *sbuf, void *rbuf, int count, MPI_Datatype dtype,
                MPI_Op op, int root, MPI_Comm comm )
{
    int i, size, rank, vrank;
    int err, peer, dim, mask;
    MPI_Aint true_lb, true_extent, lb, extent;
    char *free_buffer = NULL;
    char *free_rbuf = NULL;
    char *pml_buffer = NULL;
    char *snd_buffer = NULL;
    char *rcv_buffer = (char*)rbuf;
    char *inplace_temp = NULL;

    /* JMS Codearound for now -- if the operations is not communative,
     * just call the linear algorithm.  Need to talk to Edgar / George
     * about fixing this algorithm here to work with non-communative
     * operations. */

    if (op_is_commute(op) == 0) {
        return MPI_Reduce_linear(sbuf, rbuf, count, dtype, op, root, comm);
    }

	MPI_CHECK( err = MPI_Type_get_extent( dtype, &lb, &extent ) );
    // This might not be available outside of OpenMPI? - bnm
	MPI_CHECK( err = MPI_Type_get_true_extent( dtype, &true_lb, &true_extent ) );

    /* Some variables */
	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    vrank = op_is_commute(op) ? (rank - root + size) % size : rank;
    dim = opal_cube_dim(size);
    //    dim = comm->c_cube_dim;

    /* Allocate the incoming and resulting message buffers.  See lengthy
     * rationale above. */
    
    free_buffer = (char*)malloc(true_extent + (count - 1) * extent);
//    if (NULL == free_buffer) {
//        return OMPI_ERR_OUT_OF_RESOURCE;
//    }
    
    pml_buffer = free_buffer - lb;
    /* read the comment about commutative operations (few lines down
     * the page) */
    if (op_is_commute(op)) {
        rcv_buffer = pml_buffer;
    }
    
    /* Allocate sendbuf in case the MPI_IN_PLACE option has been used. See lengthy
     * rationale above. */
    if (MPI_IN_PLACE == sbuf) {
        inplace_temp = (char*)malloc(true_extent + (count - 1) * extent);
        if (NULL == inplace_temp) {
            //            err = OMPI_ERR_OUT_OF_RESOURCE;
            goto cleanup_and_return;
        }
        sbuf = inplace_temp - lb;

		memcpy( sbuf, rbuf, extent * count );
        // maybe I can replace this with a memcpy ? not sure? - bnm
        //        err = ompi_datatype_copy_content_same_ddt(dtype, count, (char*)sbuf, (char*)rbuf);
    }
    snd_buffer = (char*)sbuf;

    if (rank != root && 0 == (vrank & 1)) {
        /* root is the only one required to provide a valid rbuf.
         * Assume rbuf is invalid for all other ranks, so fix it up
         * here to be valid on all non-leaf ranks */
        free_rbuf = (char*)malloc(true_extent + (count - 1) * extent);
        if (NULL == free_rbuf) {
            //            err = OMPI_ERR_OUT_OF_RESOURCE;
            goto cleanup_and_return;
        }
        rbuf = free_rbuf - lb;
    }

    /* Loop over cube dimensions. High processes send to low ones in the
     * dimension. */
    for (i = 0, mask = 1; i < dim; ++i, mask <<= 1) {

        /* A high-proc sends to low-proc and stops. */
        if (vrank & mask) {
            peer = vrank & ~mask;
            if (op_is_commute(op)) {
                peer = (peer + root) % size;
            }
            MPI_CHECK( err = MPI_Send( snd_buffer, count, dtype, peer,
                                       REDUCE_TAG, comm ) );

//            err = MCA_PML_CALL(send(snd_buffer, count,
//                                    dtype, peer, MCA_COLL_BASE_TAG_REDUCE,
//                                    MCA_PML_BASE_SEND_STANDARD, comm));
            if (MPI_SUCCESS != err) {
                goto cleanup_and_return;
            }
            snd_buffer = (char*)rbuf;
            break;
        }

        /* A low-proc receives, reduces, and moves to a higher
         * dimension. */

        else {
            peer = vrank | mask;
            if (peer >= size) {
                continue;
            }
            if (op_is_commute(op)) {
                peer = (peer + root) % size;
            }

            /* Most of the time (all except the first one for commutative
             * operations) we receive in the user provided buffer
             * (rbuf). But the exception is here to allow us to dont have
             * to copy from the sbuf to a temporary location. If the
             * operation is commutative we dont care in which order we
             * apply the operation, so for the first time we can receive
             * the data in the pml_buffer and then apply to operation
             * between this buffer and the user provided data. */


            MPI_CHECK( err = MPI_Recv( rcv_buffer, count, dtype, peer, 
                                       REDUCE_TAG, comm, MPI_STATUS_IGNORE ) );

//            err = MCA_PML_CALL(recv(rcv_buffer, count, dtype, peer,
//                                    MCA_COLL_BASE_TAG_REDUCE, comm,
//                                    MPI_STATUS_IGNORE));
            if (MPI_SUCCESS != err) {
                goto cleanup_and_return;
            }
            /* Perform the operation. The target is always the user
             * provided buffer We do the operation only if we receive it
             * not in the user buffer */
            if (snd_buffer != sbuf) {
                /* the target buffer is the locally allocated one */
                // note: order of inputs is source buffer then target buffer
                //       looks strange here because source is rcv_buffer

                MPI_CHECK( err = MPI_Reduce_local( rcv_buffer, pml_buffer, count, 
                                                   dtype, op ) );

                //                ompi_op_reduce(op, rcv_buffer, pml_buffer, count, dtype);
            } else {
                /* If we're commutative, we don't care about the order of
                 * operations and we can just reduce the operations now.
                 * If we are not commutative, we have to copy the send
                 * buffer into a temp buffer (pml_buffer) and then reduce
                 * what we just received against it. */
                if (op_is_commute(op) == 0) {

                    // this might not be right? -bnm
                    memcpy( pml_buffer, (char*)sbuf, extent * count );
//                    ompi_datatype_copy_content_same_ddt(dtype, count, pml_buffer,
//                                                        (char*)sbuf);

                    MPI_CHECK( err = MPI_Reduce_local( rbuf, pml_buffer, count, 
                                                      dtype, op ) );
                    //                    ompi_op_reduce(op, rbuf, pml_buffer, count, dtype);
                } else {
                    MPI_CHECK( err = MPI_Reduce_local( sbuf, pml_buffer, count, 
                                                      dtype, op ) );
                    //                    ompi_op_reduce(op, sbuf, pml_buffer, count, dtype);
                }
                /* now we have to send the buffer containing the computed data */
                snd_buffer = pml_buffer;
                /* starting from now we always receive in the user
                 * provided buffer */
                rcv_buffer = (char*)rbuf;
            }
        }
    }

    /* Get the result to the root if needed. */
    err = MPI_SUCCESS;
    if (0 == vrank) {
        if (root == rank) {
            memcpy( (char*)rbuf, snd_buffer, extent * count );
            //            ompi_datatype_copy_content_same_ddt(dtype, count, (char*)rbuf, snd_buffer);
        } else {
            MPI_CHECK( err = MPI_Send( snd_buffer, count, dtype, root,
                                       REDUCE_TAG, comm ) );
//            err = MCA_PML_CALL(send(snd_buffer, count,
//                                    dtype, root, MCA_COLL_BASE_TAG_REDUCE,
//                                    MCA_PML_BASE_SEND_STANDARD, comm));
        }
    } else if (rank == root) {

        MPI_CHECK( err = MPI_Recv( rcv_buffer, count, dtype, 0, 
                                   REDUCE_TAG, comm, MPI_STATUS_IGNORE ) );

//        err = MCA_PML_CALL(recv(rcv_buffer, count, dtype, 0,
//                                MCA_COLL_BASE_TAG_REDUCE,
//                                comm, MPI_STATUS_IGNORE));
        if (rcv_buffer != rbuf) {
            MPI_CHECK( err = MPI_Reduce_local( rcv_buffer, rbuf, count, 
                                              dtype, op ) );
            //            ompi_op_reduce(op, rcv_buffer, rbuf, count, dtype);
        }
    }

 cleanup_and_return:
    if (NULL != inplace_temp) {
        free(inplace_temp);
    }
    if (NULL != free_buffer) {
        free(free_buffer);
    }
    if (NULL != free_rbuf) {
        free(free_rbuf);
    }

    /* All done */

    return err;
}
