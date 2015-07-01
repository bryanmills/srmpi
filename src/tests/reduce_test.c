#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>
#include <stdlib.h>

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
        printf("Checking mpi_reduce(sum)... (if you see no output then you are good)\n");
    }

    for(root=0; root<num_of_processes; root++) {
        localsum = do_something(rank, root);
        globalsum = 0;
        MPI_Reduce(&localsum,&globalsum,1,MPI_INT,MPI_SUM,root,MPI_COMM_WORLD);
        if(rank == root) {
            expectedsum = 0;
            for(i=0; i<num_of_processes; i++) {
                expectedsum = expectedsum + do_something(i, root);
            }
    
            if (globalsum != expectedsum) {
                printf("ERROR: Expected %d got %d [root:%d]\n", expectedsum, globalsum, root);
            }
    
        }
    }

    MPI_Finalize();
}
