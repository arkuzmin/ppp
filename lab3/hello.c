#include <stdio.h>
#include <mpi.h>

int main( int argc, char **argv ){
                                   	
int size, rank, lnm, pack_size;
pack_size = 40;

double t0, t1, dt;

char nm[MPI_MAX_PROCESSOR_NAME+1];
char *mess;

mess = malloc(pack_size);

MPI_Init(&argc, &argv);

MPI_Get_processor_name(nm, &lnm);

MPI_Comm_size( MPI_COMM_WORLD, &size);
MPI_Comm_rank( MPI_COMM_WORLD, &rank);

t0 = MPI_Wtime();
MPI_Request sreq, rreq;

int sender, receiver;

sender = (rank + size - 1) % size;
receiver = (rank + 1) % size;

MPI_Irecv( mess, pack_size, MPI_CHAR, sender, MPI_ANY_TAG, MPI_COMM_WORLD, &rreq);
MPI_Send( mess, pack_size, MPI_CHAR, receiver, 0, MPI_COMM_WORLD );
MPI_Wait( &rreq, MPI_STATUS_IGNORE);
t1 = MPI_Wtime();

printf("I node %d\nSend message %s to %d. \nWaited for %10.4f seconds.\n", rank, mess, rank+1, t1-t0);
MPI_Finalize();
return 0;
} 	            	