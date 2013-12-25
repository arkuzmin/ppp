#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <omp.h>

// swap entries in array v at positions i and j; used by quicksort
void swap(int * v, int i, int j)
{
  int t = v[i];
  v[i] = v[j];
  v[j] = t;
}

// (quick) sort slice of array v; slice starts at s and is of length n
int quicksort(int * v, int s, int n, int level, int rank)
{
  int x, p, i, parallel_count = 0;
  
  /*if (rank == 0) {
    if (omp_in_parallel())
    {
      #pragma omp single
      printf("parallelized with %d threads\n",
               omp_get_num_threads());
    }
    else
    {
      printf("single thread\n");
    }
  }*/

  // base case?
  if (n <= 1)
    return;
  // pick pivot and swap with first element
  x = v[s + n/2];
  swap(v, s, s + n/2);
  // partition slice starting at s+1
  p = s;

  for (i = s+1; i < s+n; i++)
    if (v[i] < x) {
      p++;
      swap(v, i, p);
    }
  // swap pivot into place
  swap(v, s, p);
  // recurse into partition
  if (level < 3)
  {
    parallel_count = omp_get_num_threads();
    #pragma omp parallel sections
    {
      #pragma omp section
      parallel_count += quicksort(v, s, p-s, level+1, rank);
      #pragma omp section
      parallel_count += quicksort(v, p+1, s+n-p-1, level+1, rank);
    }
  } else
  {
    quicksort(v, s, p-s, level+1, rank);
    quicksort(v, p+1, s+n-p-1, level+1, rank);  
  }

  return parallel_count;
}

// merge two sorted arrays v1, v2 of lengths n1, n2, respectively
int * merge(int * v1, int n1, int * v2, int n2)
{
  int * result = (int *)malloc((n1 + n2) * sizeof(int));
  int i = 0;
  int j = 0;
  int k;

  for (k = 0; k < n1 + n2; k++) {
    if (i >= n1) {
      result[k] = v2[j];
      j++;
    }
    else if (j >= n2) {
      result[k] = v1[i];
      i++;
    }
    else if (v1[i] < v2[j]) { // indices in bounds as i < n1 && j < n2
      result[k] = v1[i];
      i++;
    }
    else { // v2[j] <= v1[i]
      result[k] = v2[j];
      j++;
    }
  }
  return result;
}

int main(int argc, char ** argv)
{
  int n;
  int * data = NULL;
  int c, s;
  int * chunk;
  int o;
  int * other;
  int step;
  int p, id;
  MPI_Status status;
  double elapsed_time, sort_time, merge_time;
  FILE * file = NULL;
  int i;
  int openmp_set = 0;
  int parallel_count = 0;

  MPI_Init(&argc, &argv);
  
  MPI_Pcontrol(TRACEFILES, NULL, "trace", 0);
  MPI_Pcontrol(TRACELEVEL, 1, 1, 1);
  MPI_Pcontrol(TRACENODE, 1000000, 1, 1);
  
  MPI_Comm_size(MPI_COMM_WORLD, &p);
  MPI_Comm_rank(MPI_COMM_WORLD, &id);

  #ifdef _OPENMP
    omp_set_nested(1);
    omp_set_num_threads(2);
  #endif

  if (id == 0) {
    // read size of data
    file = fopen(argv[1], "r");
    fscanf(file, "%d", &n);
    // compute chunk size
    c = n/p; if (n%p) c++;
    // read data from file
    data = (int *)malloc(p*c * sizeof(int));
    for (i = 0; i < n; i++)
      fscanf(file, "%d", &(data[i]));
    fclose(file);
    // pad data with 0 -- doesn't matter
    for (i = n; i < p*c; i++)
      data[i] = 0;
  }

  // start the timer
  MPI_Barrier(MPI_COMM_WORLD);
  elapsed_time = - MPI_Wtime();

  // broadcast size
  MPI_Bcast(&n, 1, MPI_INT, 0, MPI_COMM_WORLD);

  // compute chunk size
  c = n/p; if (n%p) c++;

  // scatter data
  chunk = (int *)malloc(c * sizeof(int));
  MPI_Scatter(data, c, MPI_INT, chunk, c, MPI_INT, 0, MPI_COMM_WORLD);
  free(data);
  data = NULL;

  // compute size of own chunk and sort it
  if (n >= c * (id+1))
    s = c;
  else
    s = n - c * id;

  sort_time = - MPI_Wtime();
  MPI_Pcontrol(TRACEEVENT, "entry", 1, 0, NULL);
  parallel_count = quicksort(chunk, 0, s, 1, id);
  MPI_Pcontrol(TRACEEVENT, "exit", 1, 0, NULL);
  sort_time += MPI_Wtime();

  merge_time = - MPI_Wtime();
  // up to log_2 p merge steps
  MPI_Pcontrol(TRACEEVENT, "entry", 2, 0, NULL);
  for (step = 1; step < p; step = 2*step) {
    if (id % (2*step)) {
      // id is no multiple of 2*step: send chunk to id-step and exit loop
      MPI_Send(chunk, s, MPI_INT, id-step, 0, MPI_COMM_WORLD);
      break;
    }
    // id is multiple of 2*step: merge in chunk from id+step (if it exists)
    if (id+step < p) {
      // compute size of chunk to be received
      if (n >= c * (id+2*step))
        o = c * step;
      else
        o = n - c * (id+step);
      // receive other chunk
      other = (int *)malloc(o * sizeof(int));
      MPI_Recv(other, o, MPI_INT, id+step, 0, MPI_COMM_WORLD, &status);
      // merge and free memory
      data = merge(chunk, s, other, o);
      free(chunk);
      free(other);
      chunk = data;
      s = s + o;
    }
  }
  merge_time += MPI_Wtime();
  MPI_Pcontrol(TRACEEVENT, "exit", 2, 0, NULL);
  // stop the timer
  elapsed_time += MPI_Wtime();

  // write sorted data to out file and print out timer
  if (id == 0) {
    file = fopen(argv[2], "w");
    fprintf(file, "%d\n", s);   // assert (s == n)
    for (i = 0; i < s; i++)
      fprintf(file, "%d\n", chunk[i]);
    fclose(file);
    printf("size: %d proc: %d sec: %f\n", n, p, elapsed_time);
    printf("Sort time: %f Merge time: %f Parallel count: %d \n",
      sort_time, merge_time, parallel_count);
    //printf("%d %2d %f\n", n, p, elapsed_time);
  }

  MPI_Finalize();
  return 0;
}
