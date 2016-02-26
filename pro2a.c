#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include "mpi.h"
#include <string.h>

typedef struct complex{
  double real;
  double imag;
} Complex;

int cal_pixel(Complex c){
  int count, max_iter;
  Complex z;
  double temp, lengthsq;
  
  max_iter = 256;
  z.real = 0;
  z.imag = 0;
  count = 0;

  do{
    temp = z.real * z.real - z.imag * z.imag + c.real;
    z.imag = 2 * z.real * z.imag + c.imag;
    z.real = temp;
    lengthsq = z.real * z.real + z.imag * z.imag;
    count ++;
  }
  while ((lengthsq < 4.0) && (count < max_iter));
  return(count);
}

#define MASTERPE 0

int main(int argc, char **argv){
  FILE *file;
  int i, j, k = 0, x, y;
  Complex c;
  int tmp;
  char *data_l, *data_l_tmp;
  int nx=10000, ny=10000;
  int nrows_l, nums_l, masterow_l; //to determine size of chunk
  int mystrt, myend;
  int nprocs, mype;

  double start_time, end_time;

  MPI_Status status;

  /* regular MPI initialization stuff */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &mype);

  if (argc != 3){
    int err = 0;
    printf("argc %d\n", argc);
    if (mype == MASTERPE){
      printf("usage: mandelbrot nx ny");
      MPI_Abort(MPI_COMM_WORLD,err );
    }
  }

  /* calculate chunk of each node */
  nrows_l = nx / nprocs; 
  masterow_l = nx - (nprocs - 1) * nrows_l;
  if(mype == MASTERPE){
  	nums_l = masterow_l;
    mystrt = 0;
    myend = masterow_l;
  }else{
  	nums_l = nrows_l;
    mystrt = masterow_l + (mype -1) * nrows_l;
    myend = mystrt + nrows_l;
  }

  /* create buffer for local work only */
  data_l = (char *) malloc(2 * nums_l * ny * sizeof(char));
  data_l_tmp = data_l;

  start_time = MPI_Wtime();
  
  /* calc each procs coordinates and call local mandelbrot set function */
  for (i = mystrt; i < myend; ++i){
    c.real = i/((double) nx) * 4. - 2. ; 
    for (j = 0; j < ny; ++j){
      c.imag = j/((double) ny) * 4. - 2. ; 
      tmp = cal_pixel(c);
      if(tmp == 256) {
      	*data_l++ = '1'; 
        *data_l++ = ',';
      }
      else {
      	*data_l++ = '0'; 
        *data_l++ = ',';
      } 
    }
    *(--data_l) = '\n';
    data_l++;
  }

  data_l = data_l_tmp;

  if (mype == MASTERPE){ 

	  file = fopen("mandelbrot.txt", "w");
    fwrite(data_l, 2 * masterow_l * ny, sizeof(char), file);
    fclose(file);  

    for (i = 1; i < nprocs; ++i){
      MPI_Recv(data_l, 2 * nrows_l * ny, MPI_CHAR, i, 0, MPI_COMM_WORLD, &status); 
      file = fopen("mandelbrot.txt", "a");
      fwrite(data_l, 2 * nrows_l * ny, sizeof(char), file);
      fclose(file);  
    }

	  end_time = MPI_Wtime();
    printf("running time: %f\n", end_time - start_time);

  }
  else{
    MPI_Send(data_l, 2 * nrows_l * ny, MPI_CHAR, MASTERPE, 0, MPI_COMM_WORLD);
  } 

  MPI_Finalize();
}
