#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "bit_ops.h"
#include "constants.h"
#include "mpi_optimized_collectives.h"

/** I didn't find a non-linear version of the scan function in openmpi
    in either basic, tuned or hierarchy. Is it even possible given the
    collective? **/
int
MPI_Scan_linear( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                 MPI_Op op, MPI_Comm comm )
{
	int rc = MPI_SUCCESS, extent, rank, size;

	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Type_size( dt, &extent ) );

	if( rank == 0 ){
		memmove( rbuf, sbuf, cnt * extent );
		if( size > 1 ){
			MPI_CHECK( rc = MPI_Send( rbuf, cnt, dt, 1,
                                      SCAN_TAG, comm ) );
		}
	} else {
		MPI_CHECK( rc = MPI_Recv( rbuf, cnt, dt, rank - 1,
                                  SCAN_TAG, comm,
                                  MPI_STATUS_IGNORE ) );

		MPI_CHECK( rc = MPI_Reduce_local( sbuf, rbuf, cnt, dt, op ) );

		if( rank != ( size - 1 ) ){
			MPI_CHECK( rc = MPI_Send( rbuf, cnt, dt, rank + 1,
                                      SCAN_TAG, comm ) );
		}
	}

	return rc;
}


