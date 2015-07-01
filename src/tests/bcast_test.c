#include <stdio.h>
#include <string.h>
#include "mpi.h"
#include <unistd.h>

int main(int argc, char **argv)
{
    int rank;
    int num_of_processes;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int check_num = 100;
    int check_num2 = 150;
    int num = 0;
    int num2 = 0;

    int num_errors = 0;
    int my_root;

    for(my_root=0; my_root<num_of_processes; my_root++) {
        if(rank == 0) {
            num = (check_num);
            num2 = check_num2;
            printf("Checking bcast at root %d... (if no output then good)\n", my_root);
            fflush(stdout);
        }
        
        MPI_Bcast(&num, 1, MPI_INT, 0, MPI_COMM_WORLD);
        MPI_Bcast(&num2, 1, MPI_INT, 0, MPI_COMM_WORLD);

        
        if(num != check_num) {
            printf("ERROR - first number expected %d got %d at rank %d\n", check_num, num, rank);
            num_errors = num_errors + 1;
        }

        if(num2 != check_num2) {
            printf("ERROR - second number expected %d got %d at rank %d\n", check_num2, num2, rank);
            num_errors = num_errors + 1;
        }
    }

    MPI_Finalize();
    return 0;
}
