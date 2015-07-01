#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"


int
MPI_Scatterv_linear( void *sbuf, int *scnts, int *displs, MPI_Datatype sdt,
                     void *rbuf, int rcnt, MPI_Datatype rdt, int root,
                     MPI_Comm comm )
{
	int rc = MPI_SUCCESS, rank, size, extent, dst, reqcnt = 0;
	MPI_Request *reqs = NULL;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );


	if( rank != root ){

		MPI_CHECK( rc = MPI_Recv( rbuf, rcnt, rdt, root, SCATTERV_TAG,
                                  comm, MPI_STATUS_IGNORE ) );
	} else {

		MPI_CHECK( rc = MPI_Type_size( sdt, &extent ) );

		NULL_CHECK( reqs = malloc( sizeof( MPI_Request ) *
                                   ( size - 1 ) ) );

		memmove( rbuf, sbuf + ( displs[ rank ] * extent ),
                 extent * scnts[ rank ] );

		for( dst = 0 ; dst < size ; dst++ ){

			if( dst == root )
				continue;

			MPI_CHECK( rc = MPI_Isend( sbuf +
                                       ( displs[ dst ] * extent ),
                                       scnts[ dst ],
                                       sdt, dst, SCATTERV_TAG,
                                       comm,
                                       &reqs[ reqcnt++ ] ) );
		}

		MPI_CHECK( rc = MPI_Waitall( reqcnt, reqs,
                                     MPI_STATUSES_IGNORE ) );

		free( reqs );
	}

	return rc;
}
