#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

int main(int argc,char *argv[]){
	int *sdisp,*scounts,*rdisp,*rcounts;
	int i;
    int expected_num = 0;

    int rank;
    int num_of_processes;
	MPI_Init(&argc,&argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

	scounts=(int*)malloc(sizeof(int)*num_of_processes);
	rcounts=(int*)malloc(sizeof(int)*num_of_processes);
	sdisp=(int*)malloc(sizeof(int)*num_of_processes);
	rdisp=(int*)malloc(sizeof(int)*num_of_processes);

    if(rank == 0) {
        printf("Getting ready to test alltoall, if you see no output then you are good...\n");
    }

    /* how all-to-all works 

       All nodes send distinct i blocks of data to all i nodes. The
       receiving node will receive the data found in his rank position
       from each sending node but placed in the receiving array at the
       location of the sending rank.

       example:
       node 1 sending: 1 4 6
       node 2 sending: 6 3 9
       node 3 sending: 5 7 8

       after completion
       node 1 receiving: 1 6 5
       node 2 receiving: 4 3 7
       node 3 receiving: 6 9 8

       Note that receiving is a column of the sending matrix for each the node's rank
    */

    /* This should produce send arrays that look like this:
       node 1: 0  1  2  3  4  ...
       node 2: 0  2  4  6  8  ...
       node 3: 0  3  6  12 15 ...
    */
    for(i=0; i<num_of_processes; i++) {
        scounts[i] = rank*i;
    }

    /* send the data */
    MPI_Alltoall(scounts,1,MPI_INT,
                 rcounts,1,MPI_INT,
                 MPI_COMM_WORLD);

    for(i=0;i<num_of_processes; i++) {
        expected_num = i*rank;
        if(rcounts[i] != expected_num) {
            printf("ERROR: At rank %d expected %d got %d from node %d\n", rank, expected_num, rcounts[i], i);
        }
    }
    MPI_Finalize();
}

