#include "mpi.h"
#include <stdio.h>
#include "checks.h"
#include "string.h"
#include "constants.h"
#include <stdlib.h>
#include "mpi_linear_collectives.h"


int
MPI_Scatter_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                    void *rbuf, int rcnt, MPI_Datatype rdt, int root,
                    MPI_Comm comm )
{
	int rc = MPI_SUCCESS, rank, size, extent, reqcnt  = 0, dst;
	MPI_Request *reqs = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );

	if( rank != root ){

		MPI_CHECK( rc = MPI_Recv( rbuf, rcnt, rdt, root, SCATTER_TAG,
                                  comm, MPI_STATUS_IGNORE ) );
	} else {

		MPI_CHECK( rc = MPI_Type_size( sdt, &extent ) );

		NULL_CHECK( reqs = malloc( sizeof( MPI_Request ) *
                                   ( size - 1 ) ) );

		memmove( rbuf, sbuf + ( rank * scnt * extent ),
                 scnt * extent );

		for( dst = 0 ; dst < size ; dst++ ){

			if( dst == root )
				continue;

			MPI_CHECK( rc = MPI_Isend( sbuf +
                                       ( dst * scnt * extent ),
                                       scnt, sdt, dst,
                                       SCATTER_TAG, comm,
                                       &reqs[ reqcnt++ ] ) );
		}

		MPI_CHECK( rc = MPI_Waitall( reqcnt, reqs,
                                     MPI_STATUSES_IGNORE ) );

		free( reqs );
	}

	return rc;
}

