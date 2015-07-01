#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "mpi_optimized_collectives.h"

/** adapted from openmpi see,
    ~/openmpi-1.6.4/ompi/mca/coll/basic/coll_basic_barrier.c 

    This function has all leaf nodes wait for a receive, then each
    interior node sends to their children and then waits for them to
    receive it, then waits for their parent to send a message. The
    last block of code is the parent sending the message.  */
int MPI_Barrier_log( MPI_Comm comm )
{

    int i;
    int err;
    int peer;
    int dim;
    int hibit;
    int mask;
    int size ;
    int rank ;

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );	
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );	

    
    /* Send null-messages up and down the tree.  Synchronization at the
     * root (rank 0). */
    
    dim = opal_cube_dim(size);
    hibit = opal_hibit(rank, dim);
    --dim;
    
    /* Receive from children. */
    
    for (i = dim, mask = 1 << i; i > hibit; --i, mask >>= 1) {
        peer = rank | mask;
        if (peer < size) {

            MPI_CHECK( err = MPI_Recv( NULL, 0, MPI_BYTE, peer, 
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

        MPI_CHECK( err = MPI_Send( NULL, 0, MPI_BYTE, peer, 
                                   BARRIER_TAG, comm ) );

        //        err =
        //            MCA_PML_CALL(send
        //                         (NULL, 0, MPI_BYTE, peer,
        //                          MCA_COLL_BASE_TAG_BARRIER,
        //                          MCA_PML_BASE_SEND_STANDARD, comm));
        if (MPI_SUCCESS != err) {
            return err;
        }

        MPI_CHECK( err = MPI_Recv( NULL, 0, MPI_BYTE, peer, 
                                   BARRIER_TAG, comm, MPI_STATUS_IGNORE ) );
    
        //        err = MCA_PML_CALL(recv(NULL, 0, MPI_BYTE, peer,
        //                                MCA_COLL_BASE_TAG_BARRIER,
        //                                comm, MPI_STATUS_IGNORE));
    }
    
    /* Send to children. */
    
    for (i = hibit + 1, mask = 1 << i; i <= dim; ++i, mask <<= 1) {
        peer = rank | mask;
        if (peer < size) {

            MPI_CHECK( err = MPI_Send( NULL, 0, MPI_BYTE, peer, 
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
