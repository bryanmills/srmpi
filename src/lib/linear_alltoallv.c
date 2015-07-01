#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

int
MPI_Alltoallv_linear( void *sbuf, int *scnts, int *sdispls, MPI_Datatype sdt,
                      void *rbuf, int *rcnts, int *rdispls, MPI_Datatype rdt,
                      MPI_Comm comm )
{
	int rc = MPI_SUCCESS, size, rank, sndextent, rcvextent, reqcnt = 0;
	int i;
	MPI_Request *reqs = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Type_size( rdt, &rcvextent ) );
	MPI_CHECK( rc = MPI_Type_size( sdt, &sndextent ) );

	NULL_CHECK( reqs = malloc( sizeof( MPI_Request ) *
                               ( size - 1 ) * 2 ) );

	memmove( rbuf + ( rdispls[ rank ] * rcvextent ),
             sbuf + ( sdispls[ rank ] * sndextent ),
             rcnts[ rank ] * rcvextent );

	for( i = 0 ; i < size ; i++ ){

		if( i == rank )
			continue;

		MPI_CHECK( rc = MPI_Isend( sbuf + ( sdispls[ i ] * sndextent ),
                                   scnts[ i ], sdt, i, ALLTOALL_TAG,
                                   comm, &reqs[ reqcnt++ ] ) );

		MPI_CHECK( rc = MPI_Irecv( rbuf + ( rdispls[ i ] * rcvextent ),
                                   rcnts[ i ], rdt, i, ALLTOALL_TAG,
                                   comm, &reqs[ reqcnt++ ] ) );
	}

	MPI_CHECK( rc = MPI_Waitall( reqcnt, reqs, MPI_STATUSES_IGNORE ) );

	free( reqs );

	return rc;
}

