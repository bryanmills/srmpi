#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

int do_something(int rank, int root);

int do_something(int rank, int root) {
    return (2*rank) + root + 1;
}

int main(int argc,char *argv[])
{
    int rank, num_of_processes;
    int i;
    int root = 0;

    MPI_Comm comm = MPI_COMM_WORLD;

    MPI_Init(&argc,&argv);
    MPI_Comm_size( comm, &num_of_processes);
    MPI_Comm_rank( comm, &rank);

    int localsum = 0;
    int globalsum = 0;
    int expectedsum = 0;
    
    if(rank == 0) {
        printf("Checking mpi_allreduce(sum)... (if you see no output then you are good)\n");
    }

    localsum = do_something(rank, root);
    globalsum = 0;
    MPI_Allreduce(&localsum,&globalsum,1,MPI_INT,MPI_SUM,MPI_COMM_WORLD);
    
    // check on all nodes!
    expectedsum = 0;
    for(i=0; i<num_of_processes; i++) {
        expectedsum = expectedsum + do_something(i, root);
    }
    
    if (globalsum != expectedsum) {
        printf("ERROR: Expected %d got %d [root:%d]\n", expectedsum, globalsum, root);
    }


    MPI_Comm mycomm;
    MPI_Comm_dup(MPI_COMM_WORLD, &mycomm);

    double *dlocalsum    = malloc(sizeof(double)*2);
    double *dglobalsum   = malloc(sizeof(double)*2);
    double *dexpectedsum = malloc(sizeof(double)*2);

    dlocalsum[0] = do_something(rank, root) + 0.25;
    dlocalsum[1] = do_something(rank, root) + 0.99;
    MPI_Allreduce(dlocalsum,dglobalsum,2,MPI_DOUBLE,MPI_SUM,mycomm);
    
    // check on all nodes!
    dexpectedsum[0] = 0;
    dexpectedsum[1] = 0;
    for(i=0; i<num_of_processes; i++) {
        dexpectedsum[0] = dexpectedsum[0] + do_something(i, root) + 0.25;
        dexpectedsum[1] = dexpectedsum[1] + do_something(i, root) + 0.99;
    }
    
    if (dglobalsum[0] != dexpectedsum[0]) {
        printf("ERROR: Expected %f got %f [root:%d]\n", dexpectedsum[0], dglobalsum[0], root);
    }

    MPI_Finalize();
}
