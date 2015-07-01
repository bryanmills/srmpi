#include "mpi.h"
#include "constants.h"
#include "checks.h"
#include "string.h"

/** */
int sndrcv_local(void *sbuf, int scount, MPI_Datatype *sdtype,
                 void *rbuf, int rcount, MPI_Datatype *rdtype,
                 MPI_Comm *comm) {

    int err;
    int rank;
    MPI_Aint sextent=0;
    MPI_Aint slb=0;

	MPI_CHECK( err = MPI_Type_get_extent( sdtype, &slb, &sextent ) );

    if (sdtype == rdtype) {
        memcpy(rbuf, sbuf, sextent*scount);
    } else {
        MPI_CHECK( err = MPI_Comm_rank( comm, &rank ) );

        /* XXX - we can be much smarter - on same machine do direct
           copies, problem is how to translate between send and
           receive types!  Although its unclear how to handle
           differences in sdtype and rdtype
        */
        MPI_CHECK( err = MPI_Sendrecv( sbuf, scount, sdtype, rank, GATHER_TAG,
                                       rbuf, rcount, rdtype, rank, GATHER_TAG,
                                       comm, MPI_STATUS_IGNORE ) );
    }

    return err;
}

