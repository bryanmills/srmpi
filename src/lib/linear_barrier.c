#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

/** This might not necessarily be linear, if the underlying gather and
    bcast are not linear, at the very least its simple **/
int MPI_Barrier_linear( MPI_Comm comm )
{
	int rc = MPI_SUCCESS;

	MPI_CHECK( rc = MPI_Gather( NULL, 0, MPI_BYTE, NULL, 0,
                                MPI_BYTE, 0, comm ) );

    MPI_CHECK( rc = MPI_Bcast( NULL, 0, MPI_BYTE, 0, comm ) );

	return rc;
}
