#include <stdio.h>
#include <mpi.h>

int main( int argc, char **argv ){
  
  int size, rank, lnm;
  
  struct {
    double value;
    int rank;
    } num, max, nums;
  
  char nm[MPI_MAX_PROCESSOR_NAME+1];
   
  MPI_Init( &argc, &argv );
  
  MPI_Get_processor_name( nm, &lnm );
  
  MPI_Comm_size( MPI_COMM_WORLD, &size );
  MPI_Comm_rank( MPI_COMM_WORLD, &rank );
  
  //printf("Hello World!!\nI am %d of %d.\nI am on %s.\n", rank, size, nm );
  
  double sTime, eTime;
  
  num.value = size-rank;
  num.rank = rank;
  
  //printf("%d value is %d\n", rank, r);
  
  sTime = MPI_Wtime();
  
  MPI_Reduce(&num, &max, 1, MPI_DOUBLE_INT, MPI_MAXLOC, 0, MPI_COMM_WORLD);
  
  eTime = MPI_Wtime();
  
  if (rank == 0)
    printf("Maximum is %2.0f on %d, counting took %6.3f seconds\n", max.value, max.rank, eTime - sTime);
  
  if (rank != 0)
  {
    //MPI_Request req;
    //MPI_Issend(&num, 1, MPI_DOUBLE_INT, 1, 0, MPI_COMM_WORLD, &req);
    MPI_Send(&num, 1, MPI_DOUBLE_INT, 0, 777, MPI_COMM_WORLD);
    //MPI_Wait(&req, MPI_STATUS_IGNORE);
    //printf("-----%d sent value\n", rank);
  }
  else
  {
    sTime = MPI_Wtime();
    max.value = num.value;
    max.rank = num.rank;
    //printf("-----working with 0\n");
    int i;
    for(i = 1; i < size; i++)
    {
	MPI_Recv(&nums, 1, MPI_DOUBLE_INT, i, 777, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
	if (nums.value > max.value)
	{
	    max.value = nums.value;
	    max.rank = nums.rank;
	}    
	//printf("-----working with %d\n", i);    
    }
    eTime = MPI_Wtime();
    printf("Maximum is %2.0f on %d, counting took %6.3f seconds\n", max.value, max.rank, eTime - sTime);
  }
    
  MPI_Finalize();
  return 0;
}
