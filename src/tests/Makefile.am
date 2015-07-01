CC                      = ${MPICC}
AM_CFLAGS               = ${HOST_CFLAGS} -g


if OPTIMIZED_COLLECTIVES
AM_CFLAGS              += -L../lib -loptcollectives
endif

if LINEAR_COLLECTIVES
AM_CFLAGS              += -L../lib -llinearcollectives
endif
#AM_CFLAGS              += -L../lib -lreplicampi
testdir          = ${datadir}/${TEST_PACKAGE}/test

test_PROGRAMS                     = bcast_test 
bcast_test_SOURCES                = bcast_test.c

test_PROGRAMS                    += barrier_test 
barrier_test_SOURCES              = barrier_test.c

test_PROGRAMS                    += reduce_test 
reduce_test_SOURCES               = reduce_test.c
test_PROGRAMS                    += reduce_scatter_test 
reduce_scatter_test_SOURCES       = reduce_scatter_test.c
test_PROGRAMS                    += allreduce_test 
allreduce_test_SOURCES            = allreduce_test.c

test_PROGRAMS                    += gather_test
gather_test_SOURCES               = gather_test.c
test_PROGRAMS                    += gatherv_test
gatherv_test_SOURCES              = gatherv_test.c
test_PROGRAMS                    += allgather_test
allgather_test_SOURCES            = allgather_test.c
test_PROGRAMS                    += allgatherv_test
allgatherv_test_SOURCES           = allgatherv_test.c

test_PROGRAMS                    += scan_test
scan_test_SOURCES                 = scan_test.c

test_PROGRAMS                    += scatter_test
scatter_test_SOURCES              = scatter_test.c
test_PROGRAMS                    += scatterv_test
scatterv_test_SOURCES             = scatterv_test.c

test_PROGRAMS                    += alltoall_test
alltoall_test_SOURCES             = alltoall_test.c
test_PROGRAMS                    += alltoallv_test
alltoallv_test_SOURCES            = alltoallv_test.c

test_PROGRAMS                    += anysource_test
anysource_test_SOURCES            = anysource_test.c

test_PROGRAMS                    += isend_test
isend_test_SOURCES                = isend_test.c

test_PROGRAMS                    += irecv_test
irecv_test_SOURCES                = irecv_test.c

test_PROGRAMS                    += cart_test
cart_test_SOURCES                 = cart_test.c

test_PROGRAMS                   += send_test
send_test_SOURCES                = send_test.c
