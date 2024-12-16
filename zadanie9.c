#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <math.h>
#include <time.h>

#define MATRIX_SIZE 600      
#define BLOCK_SIZE 10        

int main(int argc, char **argv)
    {
    int rank, size;               
    int rank_in_row, rank_in_col; // Ранги в строковых и столбцовых группах
    int grid_size;                
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
    //создание продгруп с помощью split
    MPI_Comm_split(MPI_COMM_WORLD, row_id, rank, &row_communicator);
    MPI_Comm_split(MPI_COMM_WORLD, col_id, rank, &col_communicator);
    //получили ранги новых в новых  коммуникаторах
    MPI_Comm_rank(row_communicator, &rank_in_row);
    MPI_Comm_rank(col_communicator, &rank_in_col);

    // Размер части матрицы для каждого процесса
    int size_part_matrix = MATRIX_SIZE * MATRIX_SIZE / size;//size_part_matrix - количество элементов

    // Выделение памяти для матриц
    int *matrix_A = (int*)malloc(size_part_matrix * sizeof(int));
    int *matrix_B = (int*)malloc(size_part_matrix * sizeof(int));
    int *matrix_C = (int*)calloc(size_part_matrix, sizeof(int)); // Инициализация нулями

    // Массивы для передачи строк и столбцов
    int *current_block_row = (int*)malloc(MATRIX_SIZE * BLOCK_SIZE / grid_size * sizeof(int));
    int *current_block_col = (int*)malloc(MATRIX_SIZE * BLOCK_SIZE / grid_size * sizeof(int));

//current_block_row и current_block_col- массивы использ для хранения блоков строк их матрицы B и блоков столбцов из матрицы A которые будут передаваться между собой
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
	//rank_in_row- проверяет, что текущий процесс ответственный за подготовку строки для передачи
	//current_block_row- копирует блок строки из матрицы В
	//MPI_Bcast- передает блоки строки current_block_row  всем проуессам в той же строке
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

	//ank_in_col- проверяет что текущий процесс ответственный за подготовку столбца
        MPI_Bcast(current_block_col, MATRIX_SIZE * BLOCK_SIZE / grid_size, MPI_INT, block_index / (MATRIX_SIZE / grid_size), col_communicator);
//MPI_Bcast- передает блок столбца current_block_col всем процессам в том же столбце 
	// Вычисление части матрицы C
        for (int x = 0; x < MATRIX_SIZE / grid_size; x++)//двойной цикл перебирает все элементы локального блока матрицы С
           {
            for (int y = 0; y < MATRIX_SIZE / grid_size; y++)
                {
                int chast_sum = 0;
                for (int h = 0; h < BLOCK_SIZE; h++)//выполняет суммирование произведений элементов блоков строки и столбца
                    {
                    chast_sum += current_block_col[y * BLOCK_SIZE + h] * current_block_row[h * (MATRIX_SIZE / grid_size) + x];
                    }
                matrix_C[x * (MATRIX_SIZE / grid_size) + y] += chast_sum;
                }
            }
    }
//matrix_C- к результату добавляется вычисленная сумма
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
