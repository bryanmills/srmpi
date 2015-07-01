#ifndef _COLLECTIVE_CONSTANTS_H
#define _COLLECTIVE_CONSTANTS_H

#define REDUCE_TAG	( 0xdead )
#define BCAST_TAG	( 0xdead + 0x1 )
#define GATHER_TAG	( 0xdead + 0x2 )
#define GATHERV_TAG	( 0xdead + 0x3 )
#define SCATTER_TAG	( 0xdead + 0x4 )
#define SCATTERV_TAG	( 0xdead + 0x5 )
#define ALLTOALL_TAG	( 0xdead + 0x6 )
#define ALLTOALLV_TAG	( 0xdead + 0x7 )
#define ALLTOALLW_TAG	( 0xdead + 0x8 )
#define SCAN_TAG	( 0xdead + 0x9 )
#define BARRIER_TAG	( 0xdead + 0x10 )
#define REDUCE_SCATTER_TAG	( 0xdead + 0x11 )

#define REPLICA_SEND_TAG	( 0xdead + 0x12 )
#define REPLICA_SIGKILL_TAG	( 0xdead + 0x16 )

#endif
