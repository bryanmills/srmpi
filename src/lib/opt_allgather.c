#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "bit_ops.h"
#include "constants.h"

int
MPI_Allgather_simple( void *sbuf, int scnt, MPI_Datatype sdt, 
                      void *rbuf, int rcnt, MPI_Datatype rdt, MPI_Comm comm)
{
	int rc = MPI_SUCCESS, size;

	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );

	MPI_CHECK( rc = MPI_Gather( sbuf, scnt, sdt, rbuf, rcnt,
                                rdt, 0, comm ) );

	MPI_CHECK( rc = MPI_Bcast( rbuf, rcnt * size, rdt, 0,
                               comm ) );	
	return rc;
}
