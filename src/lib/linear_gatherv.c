#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"

/** Currently the openmpi implementation doesn't have a non-linear
    implementation of gatherv. In theory we could adapt gather to do
    this? I think you could actually call gather then just re-oreint
    the memory although it would require 2x the mem?  */
MPI_Gatherv_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                    void *rbuf, int *rcnts, int *displs, MPI_Datatype rdt,
                    int root, MPI_Comm comm )
{
	int rc = MPI_SUCCESS, rank, src, size, extent, reqcnt = 0;
	MPI_Request *reqs = NULL;

	assert( ( rcnts != NULL ) && ( displs != NULL ) );

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Type_size( rdt, &extent ) );

	if( rank != root ){

		MPI_CHECK( rc = MPI_Send( sbuf, scnt, sdt, root, GATHERV_TAG,
                                  comm ) );
	} else {
		NULL_CHECK( reqs = malloc( sizeof( MPI_Request ) *
                                   ( size - 1 ) ) );

		for( src = 0 ; src < size ; src++ ){

            if( src == root ){
				memmove( rbuf + ( displs[ src ] * extent ),
                         sbuf, extent * rcnts[ src ] );
				
				continue;
			}


			MPI_CHECK( rc = MPI_Irecv( rbuf +
                                       ( displs[ src ] * extent ),	
                                       rcnts[ src ], rdt, src,
                                       GATHERV_TAG, comm,
                                       &reqs[ reqcnt++ ] ) );
		}

		MPI_CHECK( rc = MPI_Waitall( reqcnt, reqs,
                                     MPI_STATUSES_IGNORE ) );

	}

	free( reqs );

	return rc;
}
