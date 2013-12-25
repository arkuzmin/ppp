#include <mpi.h>
#include <stdio.h>


#define SIZE 7

int main( int argc, char **argv){

    int a[ SIZE ][ SIZE ],
	c[ SIZE ][ SIZE];
    int rank, i, j;
    
    MPI_Datatype
	Row, Column;
	
    MPI_Status status;
    
    MPI_Init ( &argc, &argv );
    MPI_Comm_rank ( MPI_COMM_WORLD, &rank );
    
    if( rank != 0)
	goto done;

    for ( i = 0; i < SIZE; i++ )
	for ( j = 0; j < SIZE; j++ )
	    a[i][j] = 10 * (i+1) + (j+1);
	    
    puts ("Sended: ");
    for (i = 0; i < SIZE; i++){
	for (j = 0; j < SIZE; j++)
	    printf("%4d", a[i][j]);
	puts("");
    }
    
    MPI_Type_vector (SIZE, 1, SIZE, MPI_INT, &Row);
    
    MPI_Type_hvector (SIZE, 1, sizeof(int), Row, &Column );
    
    MPI_Type_commit (&Column);
    
    MPI_Sendrecv( a, 1, Column, rank, 0, c, SIZE*SIZE, MPI_INT, rank, 0,
	MPI_COMM_WORLD, &status);
	
    puts("");
    
    puts("Received transported matrix:");
    for( i=0; i<SIZE; i++){
	for ( j=0; j<SIZE; j++ )
	    printf("%4d",c[i][j]);
	puts("");
    }
    puts("");
    
    
    MPI_Type_free (&Row);
    MPI_Type_free (&Column);
    
done:
    MPI_Finalize();
    return 0;    
}    
 	    	