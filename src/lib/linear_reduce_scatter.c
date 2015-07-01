#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

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
