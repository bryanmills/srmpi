#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "bit_ops.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

/** This appears to the standard  way this is done throughout opemmpi,
    its seem like  there should be some optimization  but not worrying
    about it right now **/
MPI_Allreduce_simple( void *sbuf, void *rbuf, int cnt, MPI_Datatype dt,
                      MPI_Op op, MPI_Comm comm ) 
{
	int rc = MPI_SUCCESS, rank;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );

	/*
	 * Reduce to 0 and broadcast
	 *
	 */
	if( sbuf == MPI_IN_PLACE ){
		if( rank == 0 ){
			MPI_CHECK( rc = MPI_Reduce( MPI_IN_PLACE, rbuf,
                                        cnt, dt, op, 0, 
                                        comm ) );
		} else {
			MPI_CHECK( rc = MPI_Reduce( rbuf, NULL, cnt, 
                                        dt, op, 0, comm ) );
		}
	} else {
		MPI_CHECK( rc = MPI_Reduce( sbuf, rbuf, cnt, dt, op,
                                    0, comm ) );
	}

	if( rc != MPI_SUCCESS )
		return rc;

	return MPI_Bcast( rbuf, cnt, dt, 0, comm );
}	
