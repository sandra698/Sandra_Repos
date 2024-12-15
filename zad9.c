#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

#define MATRIX_SIZE 600      // Размер матрицы
#define BLOCK_SIZE 10        // Размер блока

int main(int argc, char **argv)
    {
    int rank, size;               // Ранг процесса и общее количество процессов
    int rank_in_row, rank_in_col; // Ранги в строковых и столбцовых группах
    int grid_size;                // Размер решётки процессов
    MPI_Comm row_communicator, col_communicator; // Коммуникаторы для строк и столбцов

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

// Определяем размер сетки процессов
    grid_size = (int)sqrt(size);
    if (grid_size * grid_size != size) 
        {
        if (rank == 0) 
            {
            printf("Error: Number of processes must be a perfect square.\n");
            }
        MPI_Finalize();
        return 1;
        }

    // Разделяем процессы на строковые и столбцовые группы
    int row_id = rank / grid_size;
    int col_id = rank % grid_size;
    MPI_Comm_split(MPI_COMM_WORLD, row_id, rank, &row_communicator);
    MPI_Comm_split(MPI_COMM_WORLD, col_id, rank, &col_communicator);

    MPI_Comm_rank(row_communicator, &rank_in_row);
    MPI_Comm_rank(col_communicator, &rank_in_col);

    // Размер части матрицы для каждого процесса
    int size_part_matrix = MATRIX_SIZE * MATRIX_SIZE / size;

    // Выделение памяти для матриц
    int *matrix_A = (int*)malloc(size_part_matrix * sizeof(int));
    int *matrix_B = (int*)malloc(size_part_matrix * sizeof(int));
    int *matrix_C = (int*)calloc(size_part_matrix, sizeof(int)); // Инициализация нулями

    // Массивы для передачи строк и столбцов
    int *current_block_row = (int*)malloc(MATRIX_SIZE * BLOCK_SIZE / grid_size * sizeof(int));
    int *current_block_col = (int*)malloc(MATRIX_SIZE * BLOCK_SIZE / grid_size * sizeof(int));

    // Инициализация матриц A и B случайными значениями
    srand(time(NULL) + rank);
    for (int i = 0; i < size_part_matrix; i++)
        {
        matrix_A[i] = rand() % 10;
        matrix_B[i] = rand() % 10;
        }

    // Замер времени выполнения
    double start_time = MPI_Wtime();

    // Основной цикл алгоритма
    for (int block_index = 0; block_index < MATRIX_SIZE / BLOCK_SIZE; block_index++)
       {
        // Подготовка строки матрицы B для передачи
        if (rank_in_row == block_index / (MATRIX_SIZE / BLOCK_SIZE / grid_size))
            {
            for (int l = 0; l < MATRIX_SIZE * BLOCK_SIZE / grid_size; l++)
                {
                current_block_row[l] = matrix_B[(block_index % (MATRIX_SIZE / grid_size / BLOCK_SIZE)) * (MATRIX_SIZE / grid_size) + l];
                }
        }
        MPI_Bcast(current_block_row, MATRIX_SIZE * BLOCK_SIZE / grid_size, MPI_INT, block_index / (MATRIX_SIZE / grid_size), row_communicator);

	// Подготовка столбца матрицы A для передачи
        if (rank_in_col == block_index / (MATRIX_SIZE / BLOCK_SIZE / grid_size))
            {
            for (int l = 0; l < MATRIX_SIZE * BLOCK_SIZE / grid_size; l++)
                {
                current_block_col[l] = matrix_A[(block_index % (MATRIX_SIZE / grid_size / BLOCK_SIZE)) * BLOCK_SIZE +
                                                (l / BLOCK_SIZE) * (MATRIX_SIZE / grid_size) + l % BLOCK_SIZE];
                }
            }
        MPI_Bcast(current_block_col, MATRIX_SIZE * BLOCK_SIZE / grid_size, MPI_INT, block_index / (MATRIX_SIZE / grid_size), col_communicator);

	// Вычисление части матрицы C
        for (int x = 0; x < MATRIX_SIZE / grid_size; x++)
           {
            for (int y = 0; y < MATRIX_SIZE / grid_size; y++)
                {
                int chast_sum = 0;
                for (int h = 0; h < BLOCK_SIZE; h++)
                    {
                    chast_sum += current_block_col[y * BLOCK_SIZE + h] * current_block_row[h * (MATRIX_SIZE / grid_size) + x];
                    }
                matrix_C[x * (MATRIX_SIZE / grid_size) + y] += chast_sum;
                }
            }
    }

    if (rank == 0)
        {
        printf("Execution time: %lf seconds\n", MPI_Wtime() - start_time);
        }

    // Освобождение памяти
    free(matrix_A);
    free(matrix_B);
    free(matrix_C);
    free(current_block_row);
    free(current_block_col);

    MPI_Finalize();
    return 0;
} 
