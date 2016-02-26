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
#define WORK_TAG 1
#define FREE_TAG 2
#define STOP_TAG 3

int main(int argc, char **argv){
  FILE *file;
  int i, j, x, y;
  Complex c;
  int tmp;
  char *data_l, *data_l_tmp;
  int nx=10000, ny=10000;
  int nprocs, mype;

  int chunk = atoi(argv[1]); //row base
  int tasks_num  = 0;
  int curow = 0, myrow; //for master and slaver seperately

  double start_time, end_time;

  MPI_Status status;

  /* regular MPI initialization stuff */
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &nprocs);
  MPI_Comm_rank(MPI_COMM_WORLD, &mype);

  if (argc != 4){
    int err = 0;
    printf("argc %d\n", argc);
    if (mype == MASTERPE){
      printf("usage: mandelbrot nx ny");
      MPI_Abort(MPI_COMM_WORLD,err );
    }
  }

  data_l = (char *) malloc(2 * chunk * ny  * sizeof(char));
  data_l_tmp = data_l;

  if(mype == MASTERPE){
    int source;

    start_time = MPI_Wtime();

    /*First round of sending tasks to each node*/
    for(i = 1; i < nprocs; i++){
      MPI_Send(&curow, 1, MPI_INT, i, WORK_TAG, MPI_COMM_WORLD);
      tasks_num += chunk;
      curow += chunk;
    }

    while(tasks_num > 0){
      MPI_Recv(data_l, 2 * chunk * ny, MPI_CHAR, MPI_ANY_SOURCE, FREE_TAG, MPI_COMM_WORLD, &status);
      source = status.MPI_SOURCE;
      tasks_num -= chunk;


      if(curow < nx){
        MPI_Send(&curow, 1, MPI_INT, source, WORK_TAG, MPI_COMM_WORLD);
        tasks_num += chunk;
        curow += chunk;
      }
      else{
        MPI_Send(&curow, 1, MPI_INT, source, STOP_TAG, MPI_COMM_WORLD);
      }
    }

    end_time = MPI_Wtime();
    printf("running time: %f\n", end_time - start_time);

  }
  else{
    while(MPI_Recv(&myrow, 1, MPI_INT, 0, MPI_ANY_TAG, MPI_COMM_WORLD, &status) == MPI_SUCCESS
      && status.MPI_TAG == WORK_TAG){

      /*Calculation of Mandelbrot set*/
      for(i = myrow; i < myrow + chunk; i++){
        c.real = i / ((double) nx) * 4. - 2. ; 
        for(j = 0; j < ny; j++){
          c.imag = j / ((double) ny) * 4. - 2. ;
          tmp = cal_pixel(c);
          if(tmp == 256){
            *data_l++ = '1'; 
            *data_l++ = ',';
          }
          else{
            *data_l++ = '0'; 
            *data_l++ = ',';
          }
        }
        *(--data_l) = '\n';
        data_l++;
      }

      data_l = data_l_tmp;

      if(myrow==300) {printf("%c\n", *(data_l+20000));}

      /*writing to file*/
      file = fopen("data2.txt", "r+");
      fseek(file, myrow * 2 * ny, SEEK_SET);
      fputs(data_l, file);
      //fwrite(data_l, 2 * chunk * ny, sizeof(char), file);
      fclose(file); 

      MPI_Send(data_l, 2 * chunk * ny, MPI_CHAR, 0, FREE_TAG, MPI_COMM_WORLD);
    }
  }

  MPI_Finalize();
}


