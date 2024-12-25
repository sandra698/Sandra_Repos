#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

void generate_matrix_and_vector(float *A, float *b, int N, int local_rows, int cols_per_process) {
        for (int i = 0; i < local_rows * cols_per_process; i++) {
            A[i] = (float)(rand() % 10);
        }
        for (int i = 0; i < N; i++) {
            b[i] = (float)(rand() % 10);
        }
}

int main(int argc, char **argv) {
    MPI_Init(&argc, &argv);

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    
    int N = 10000; // фиксированное большое значение
    int P = size;

    // 2D процессная решётка
    int dims[2] = {0, 0};
    MPI_Dims_create(P, 2, dims); // определяем размеры решётки
    int rows_per_process = N / dims[0];
    int cols_per_process = N / dims[1];

    // локальные данные
    float *local_A = (float *)malloc(rows_per_process * cols_per_process * sizeof(float));
    float *b = (float *)malloc(N * sizeof(float));
    float *local_c = (float *)malloc(rows_per_process * sizeof(float));

    //отладочное сообщение
//printf("Rank %d: Memory allocated successfully\n", rank);

    // создание окна для RMA
    float *shared_b;
    MPI_Win win;
    MPI_Win_allocate(N * sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &shared_b, &win);
 

    if (rank == 0) {
        generate_matrix_and_vector(local_A, shared_b, N, rows_per_process, cols_per_process);
	//printf("Rank %d: Matrix and vector generated\n", rank);
    }

    MPI_Win_fence(0, win); // cинхронизация для передачи вектора b
    //printf("Rank %d: First MPI_Win_fence completed\n", rank);

    //замер времени стартует 
   double start_time = MPI_Wtime();


    if (rank != 0) {
        if (b == NULL) 
	   {
           fprintf(stderr, "Rank %d: Failed to allocate memory for vector b\n", rank);
		   MPI_Abort(MPI_COMM_WORLD, 1);
	   }

	//printf("Rank %d: Before MPI_Get\n", rank);

        MPI_Get(b, N, MPI_FLOAT, 0, 0, N, MPI_FLOAT, win); // чтение вектора b
        
	//printf("Rank %d: After MPI_Get\n", rank);
    }

    MPI_Win_fence(0, win); // синхронизация после получения данных
    //printf("Rank %d: Second MPI_Win_fence completed\n", rank);

  

    // локальные вычисления
    for (int i = 0; i < rows_per_process; i++) {
        local_c[i] = 0.0;
        for (int j = 0; j < cols_per_process; j++) {
            local_c[i] += local_A[i * cols_per_process + j] * b[j];
        }
    }

//завершаем замер времени 
double end_time = MPI_Wtime();

//сделала так, чтобы выводился только на 0 процессе
if (rank == 0) {
     printf ("Rank %d: Computation completed in  %f second \n", rank,  end_time - start_time); 
    }

 
  



    // сбор результата на процессе 0
    float *result = (rank == 0) ? (float *)malloc(N * sizeof(float)) : NULL;
    MPI_Gather(local_c, rows_per_process, MPI_FLOAT, result, rows_per_process, MPI_FLOAT, 0, MPI_COMM_WORLD);
//printf("Rank %d: MPI_Gather completed\n", rank);

    //if (rank == 0) {
    //    printf("Result vector c computed successfully.\n");
    //    free(result);
   // }

    // освобождаем ресурсы
    MPI_Win_free(&win);
    free(local_A);
    free(local_c);
    if (b) free(b);

    MPI_Finalize();
    return 0;
}
