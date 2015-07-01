#include "mpi.h"
#include <stdio.h>
#include "string.h"
#include "checks.h"
#include "constants.h"
#include "mpi_linear_collectives.h"


/** Copied from linear example file. This is actually very similar to
    the basic implementation found in openmpi, there is a tuned
    version which is more complicated but it looks like it will
    require pull more code from opemmpi ... come back to this
    later. Although after thinking about it wouldn't a gather
    operation always be best to be linear, all nodes report to
    root. It makes sense that allgather would be more tree based but
    maybe not gather... */
int MPI_Gather_linear( void *sbuf, int scnt, MPI_Datatype sdt,
                       void *rbuf, int rcnt, MPI_Datatype rdt,
                       int root, MPI_Comm comm )
{
	int rc = MPI_SUCCESS, rank, size, src, extent;
	MPI_Request *request = NULL;
	int reqcnt = 0;

	MPI_CHECK( rc = MPI_Comm_rank( comm, &rank ) );
	MPI_CHECK( rc = MPI_Comm_size( comm, &size ) );
	MPI_CHECK( rc = MPI_Type_size( rdt, &extent ) );

	if( rank != root ){

		MPI_CHECK( rc = MPI_Send( sbuf, scnt, sdt, root,
                                  GATHER_TAG, comm ) );

	} else {
		NULL_CHECK( request = malloc( sizeof( MPI_Request ) *
                                      ( size - 1 ) ) );
		/* 
		 * Copy root data
		 */
		memmove( rbuf + ( rank * rcnt * extent ), sbuf,
                 rcnt * extent );
		for( src = 0 ; src < size ; src++ ){
			
			if( src == root )
				continue;

			MPI_CHECK( rc = MPI_Irecv(
                                      rbuf + ( src * rcnt * extent ),
                                      rcnt, rdt, src,
                                      GATHER_TAG, comm,
                                      &request[ reqcnt++ ] )
                       );
		}
		
		MPI_CHECK( rc = MPI_Waitall( reqcnt, request, 
                                     MPI_STATUSES_IGNORE ) );
	}

	free( request );

	return rc;
}
