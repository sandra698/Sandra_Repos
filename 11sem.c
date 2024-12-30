#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// Функция для генерации локальной части матрицы на каждом процессе
void generate_matrix(float *A, int local_rows, int cols_per_process)
    {
    for (int i = 0; i < local_rows * cols_per_process; i++) 
        {
        A[i] = (float)(rand() % 10);
        }
}

// Функция для генерации вектора b только на процессе 0
void generate_vector(float *b, int N) 
    {
    for (int i = 0; i < N; i++) 
        {
        b[i] = (float)(rand() % 10);
        }
    } 

int main(int argc, char **argv) 
    {
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

    // Локальные данные
    float *local_A = (float *)malloc(rows_per_process * cols_per_process * sizeof(float));
    float *b = (float *)malloc(N * sizeof(float));
    float *local_c = (float *)malloc(rows_per_process * sizeof(float));

    // Создание окна для RMA
    float *shared_b;
    MPI_Win win;
    MPI_Win_allocate(N * sizeof(float), sizeof(float), MPI_INFO_NULL, MPI_COMM_WORLD, &shared_b, &win);

    // Генерация матрицы A локально на каждом процессе
    generate_matrix(local_A, rows_per_process, cols_per_process);

    // Генерация вектора b только на процессе 0
    if (rank == 0) 
        {
        generate_vector(shared_b, N);
        }

    MPI_Win_fence(0, win); // Синхронизация для передачи вектора b

    // Замер времени
    double start_time = MPI_Wtime();

    // Получение вектора b процессами, отличными от 0
    if (rank != 0) 
        {
        MPI_Get(b, N, MPI_FLOAT, 0, 0, N, MPI_FLOAT, win); // Чтение вектора b
        }  

    MPI_Win_fence(0, win); // Синхронизация после получения данных

    // Локальные вычисления
    for (int i = 0; i < rows_per_process; i++) 
        {
        local_c[i] = 0.0;
        for (int j = 0; j < cols_per_process; j++) 
	    {
            local_c[i] += local_A[i * cols_per_process + j] * b[j];
            } 
        } 

    // Завершение замера времени
    double end_time = MPI_Wtime();
    if (rank == 0) 
        {
        printf("Rank %d: Computation completed in %f seconds\n", rank, end_time - start_time);
        }

    // Сбор результата на процессе 0
    float *result = (rank == 0) ? (float *)malloc(N * sizeof(float)) : NULL;
    MPI_Gather(local_c, rows_per_process, MPI_FLOAT, result, rows_per_process, MPI_FLOAT, 0, MPI_COMM_WORLD);

    // Освобождение ресурсов
    MPI_Win_free(&win);
    free(local_A);
    free(local_c);
    if (b) free(b);

    if (rank == 0) 
        {
        free(result);
        }

    MPI_Finalize();
    return 0;
}
