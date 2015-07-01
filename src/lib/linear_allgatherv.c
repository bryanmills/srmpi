#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "bit_ops.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

int
MPI_Allgatherv_simple( void *sbuf, int scnt, MPI_Datatype sdt,
                       void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                       MPI_Comm comm )
{
	int rc = MPI_SUCCESS, size, rank, extent, total_size = 0, i;

	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Type_size( rdt, &extent ) );

	MPI_CHECK( rc = MPI_Gatherv( sbuf, scnt, sdt, rbuf, rcnts,
                                 displs, rdt, 0, comm ) );

	for( i = 0 ; i < size ; i++ )
		total_size += rcnts[ i ];

	MPI_CHECK( rc = MPI_Bcast( rbuf, total_size, rdt, 0, comm ) );

	return rc;
}

