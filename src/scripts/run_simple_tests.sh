#!/bin/bash
#PBS -l nodes=2:ppn=8
#PBS -l walltime=00:03:00
#PBS -j oe
#PBS -N simple_tests

cd $PBS_O_WORKDIR

export MPICH_NO_BUFFER_ALIAS_CHECK=1

TESTPATH=/home/bmills/mills_mpi_work/collectives/optimized/tests

echo "++++ TESTING BCAST"
date
aprun -B $TESTPATH/bcast_test
date

echo "++++ TESTING REDUCE"
date
aprun -B $TESTPATH/reduce_test
date

echo "++++ TESTING REDUCE_SCATTER"
date
aprun -B $TESTPATH/reduce_scatter_test
date

echo "++++ TESTING ALLREDUCE"
date
aprun -B $TESTPATH/allreduce_test
date

echo "++++ TESTING SCATTER"
date
aprun -B $TESTPATH/scatter_test
date

echo "++++ TESTING SCATTERV"
date
aprun -B $TESTPATH/scatterv_test
date

echo "++++ TESTING SCAN"
date
aprun -B $TESTPATH/scan_test
date

echo "++++ TESTING GATHER"
date
aprun -B $TESTPATH/gather_test
date

echo "++++ TESTING GATHERV"
date
aprun -B $TESTPATH/gatherv_test
date

echo "++++ TESTING ALLTOALL"
date
aprun -B $TESTPATH/alltoall_test
date

echo "++++ TESTING ALLTOALLV"
date
aprun -B $TESTPATH/alltoallv_test
date

echo "++++ TESTING ALLREDUCE"
date
aprun -B $TESTPATH/allreduce_test
date

echo "++++ TESTING ALLGATHER"
date
aprun -B $TESTPATH/allgather_test
date

echo "++++ TESTING ALLGATHERV"
date
aprun -B $TESTPATH/allgatherv_test
date
