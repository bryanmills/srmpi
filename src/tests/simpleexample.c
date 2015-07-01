#include <stdio.h>
#include "mpi.h"
#include <unistd.h>
#include <string.h>

int main(int argc, char **argv)
{
    int rank;
    int dest;
    int num_of_processes;
    int tag = 0;
    char message[100];

    MPI_Status status;

    MPI_Init(&argc, &argv);

    MPI_Comm_size(MPI_COMM_WORLD, &num_of_processes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (rank != 0) {
        sprintf(message, "Greetings from process %d", rank);
        dest = 0;
        MPI_Send(message, strlen(message)+1, MPI_CHAR, dest, tag, MPI_COMM_WORLD);
    } else { // rank is 0

        FILE *fp = fopen("/tmp/myjunk.out", "w");
        fprintf(fp, "Test");
        fclose(fp);

        int source = 0;
        for (source = 1; source < num_of_processes; source++) {
            MPI_Recv(message, 100, MPI_CHAR, source, tag, MPI_COMM_WORLD, &status);
            printf("%s\n", message);
        }
    }
    MPI_Finalize();
    return 0;
}

//
//#include <stdio.h>
//#include "mpi.h"
//#include <unistd.h>
//
//int main(int argc, char **argv)
//{
//  int rank, size;
//  MPI_Init(&argc, &argv);
//  MPI_Comm_size(MPI_COMM_WORLD, &size);
//  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
//  printf("Hellow world from proces %d of %d\n",rank,size);
//  MPI_Finalize();
//  return 0;
//}
