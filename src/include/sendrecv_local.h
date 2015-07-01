#ifndef _SENDRECV_LOCAL_H
#define _SENDRECV_LOCAL_H 

#include "mpi.h"

int sndrcv_local(void *sbuf, int scount, MPI_Datatype *sdtype,
                 void *rbuf, int rcount, MPI_Datatype *rdtype,
                 MPI_Comm *comm);


#endif /* _SENDRECV_LOCAL_H */

