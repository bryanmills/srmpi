#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "mpi_optimized_collectives.h"

/** adapted from openmpi see,
    ~/openmpi-1.6.4/ompi/mca/coll/basic/coll_basic_bcast.c
    This does a binomial tree based broadcast.
*/
int MPI_Bcast_log( void *buff, int count, MPI_Datatype datatype, int root,
                   MPI_Comm comm )
{

    int i;
    int size;
    int rank;
    int vrank;
    int peer;
    int dim;
    int hibit;
    int mask;
    int err;
	int rcnt=0;
	MPI_Request *areqs = NULL;
    

	MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( err = MPI_Comm_size( comm, &size ) );

    vrank = (rank + size - root) % size;
    
    dim = opal_cube_dim(size);
    hibit = opal_hibit(vrank, dim);
    --dim;
 
    /* malloc the maximum possible send request, this is the dimension
       minus the depth then one more the leaf, this might allocate a
       more slots than necessary if the number of nodes do not evenly
       divide into the tree.
    */
    NULL_CHECK( areqs = malloc( sizeof( MPI_Request ) * ( dim-(hibit+1)+1 ) ) );

    /* Receive data from parent in the tree. */
    
    if (vrank > 0) {
        peer = ((vrank & ~(1 << hibit)) + root) % size;
    
        //        printf("Getting ready to receive from %d at %d\n", peer, rank);
        MPI_CHECK( err = MPI_Recv( buff, count, datatype, peer, BCAST_TAG, comm, MPI_STATUS_IGNORE ) );

        if (MPI_SUCCESS != err) {
            return err;
        }
     }

    /* Send data to the children. */
   
    // just in case there are no children, return should be success
    err = MPI_SUCCESS; 
    for (i = hibit + 1, mask = 1 << i; i <= dim; ++i, mask <<= 1) {
        peer = vrank | mask;
        if (peer < size) {
            peer = (peer + root) % size;
            MPI_CHECK( err = MPI_Isend( buff, count, datatype, peer, 
                                        BCAST_TAG, comm,
                                        &areqs[ rcnt ] ) );
            rcnt++;
            if (MPI_SUCCESS != err) {
                free(areqs);
                return err;
            }
        }
     }
    
    /* Wait on all requests. */

    if (rcnt > 0) {
        MPI_CHECK( err = MPI_Waitall( rcnt, areqs, MPI_STATUSES_IGNORE ) );
        /* Free the reqs */    
        free(areqs);
     }
    
    /* All done */
    
    return err;
}

