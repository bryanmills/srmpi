#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"


/** Just moved this in here for test ... to be removed */
int MPI_Bcast_linear( void *buf, int cnt, MPI_Datatype dt, int root,
                      MPI_Comm comm )
{
	int i, size, rank, rc, rcnt=0;
	MPI_Request *areqs = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );	
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );	

	if( rank != root )
		return MPI_Recv( buf, cnt, dt, root, BCAST_TAG, comm,
                         MPI_STATUS_IGNORE );

	NULL_CHECK( areqs = malloc( sizeof( MPI_Request ) * ( size - 1 ) ) );

	for( i = 0 ; i < size ; i++ ){
		if( i == rank )
			continue;

        // XXXXXX this is purposeful bug to test linear/opt
		MPI_CHECK( rc = MPI_Isend( buf, cnt, dt, i, BCAST_TAG, comm,
                                   &areqs[ rcnt++ ] ) );
	}

	MPI_CHECK( rc = MPI_Waitall( rcnt, areqs, MPI_STATUSES_IGNORE ) );

	free( areqs );

	return rc;
}

