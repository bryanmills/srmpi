#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

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
            //            printf("right before receive with pml_buf\n");
			MPI_CHECK( rc = MPI_Recv( pml_buf, cnt, dt, i, 
                                      REDUCE_TAG, comm,
                                      MPI_STATUS_IGNORE ) 
                       );
			inbuf = pml_buf;
		}
        //        printf("right before reduce local\n");
		MPI_CHECK( rc = MPI_Reduce_local( inbuf, rbuf, cnt, 
                                          dt, op ) );
	}

	if( inplace_temp ){
        //        printf("right before memcpy");
		memcpy( sbuf, inplace_temp, extent * cnt );
		free( inplace_temp );
	}

	if( free_buf != NULL )
		free( free_buf );
    //    printf("Return rc\n");
	return rc;
}
