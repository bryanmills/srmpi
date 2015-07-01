//#include "mpi.h"
//#include <stdio.h>

/** Was using this to test performance of send, commented out for now. */

//static int totalBytes = 0; 
//static double totalTime = 0.0; 
// 
//int MPI_Send(void* buffer, int count, MPI_Datatype datatype, 
//             int dest, int tag, MPI_Comm comm) 
//{ 
//
//    FILE *fp = fopen("/tmp/mysend.out", "a");
//    fprintf(fp, "Testing from send\n");
//
//
//    double tstart = MPI_Wtime();       /* Pass on all the arguments */ 
//    int extent; 
//    int result    = PMPI_Send(buffer,count,datatype,dest,tag,comm);    
// 
//    MPI_Type_size(datatype, &extent);  /* Compute size */ 
//    totalBytes += count*extent; 
// 
//    totalTime  += MPI_Wtime() - tstart;         /* and time          */ 
//
//    fprintf(fp, "It took %f\n", totalTime);
//    fclose(fp); 
//    return result;                        
//} 
