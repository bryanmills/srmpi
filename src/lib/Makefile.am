
SUBDIRS                 =
CC                      = ${MPICC}
AM_CFLAGS               = ${HOST_CFLAGS} -g -I../include

#lib_LTLIBRARIES = #libreplicampi.la

if OPTIMIZED_COLLECTIVES
lib_LTLIBRARIES         = liboptcollectives.la
liboptcollectives_la_SOURCES  = opt_bcast.c opt_barrier.c opt_reduce.c opt_gather.c opt_allgather.c opt_scan.c opt_gatherv.c opt_scatter.c opt_scatterv.c opt_reduce_scatter.c opt_alltoall.c opt_alltoallv.c opt_allreduce.c opt_allgatherv.c 

if SIMPLE_REPLICA
liboptcollectives_la_SOURCES += shared_replica_mpi.c parallel_replica_mpi.c mirror_replica_mpi.c replica_mpi_override.c
endif

liboptcollectives_la_SOURCES += sendrecv_local.c topo.c opt_mpi_override.c

liboptcollectives_la_LDFLAGS    	=
liboptcollectives_la_CFLAGS     	= ${AM_CFLAGS}
#liboptcollectives_la_LIBADD     	= @LTLIBOBJS@

endif

if LINEAR_COLLECTIVES
lib_LTLIBRARIES         = liblinearcollectives.la

liblinearcollectives_la_SOURCES = linear_bcast.c linear_barrier.c linear_reduce.c linear_gather.c linear_allgather.c linear_scan.c linear_gatherv.c linear_scatter.c linear_scatterv.c linear_reduce_scatter.c linear_alltoall.c linear_alltoallv.c linear_allreduce.c linear_allgatherv.c

# allreduce.c allgatherv.c

liblinearcollectives_la_SOURCES += linear_mpi_override.c

if SIMPLE_REPLICA
liblinearcollectives_la_SOURCES += shared_replica_mpi.c parallel_replica_mpi.c mirror_replica_mpi.c replica_mpi_override.c
endif


liblinearcollectives_la_LDFLAGS    	=
liblinearcollectives_la_CFLAGS     	= ${AM_CFLAGS}


endif



##libreplicampi_la_SOURCES = replica_mpi_override.c
##libreplicampi_la_CFLAGS = ${HOST_CFLAGS}
