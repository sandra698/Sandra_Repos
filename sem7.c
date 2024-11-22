#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 256  // Размер всей сетки (должен быть степенью двойки)
#define MAX_ITER 1000  // Максимальное число итераций
#define TOL 1e-6  // Точность

//initialize_grid- инициализируем сетку случайными числами
void initialize_grid(double* grid, int local_rows, int cols) 


    {
    for (int i = 0; i < local_rows; i++)
        {
        for (int j = 0; j < cols; j++)
       	    {
            grid[i * cols + j] = rand() / (double)RAND_MAX;
            }
        }
    }

int main(int argc, char** argv)
    {
    int rank, size;
    MPI_Init(&argc, &argv);

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);//определяем ранг
    MPI_Comm_size(MPI_COMM_WORLD, &size);//определяем общее кол-во процесов

    //ДОПОЛНИТЕЛЬНЫЕ отладочные процессы 
    printf("Process %d of %d started.\n", rank, size);
    fflush(stdout);

    //Начало измерения времени
    double start_time = MPI_Wtime();

    int rows = N / size;  // Число строк, которыми управляет каждый процесс
    int cols = N;  // Число столбцов одинаково для всех

    if (rank == 0 && N % size != 0) 
            {
            printf("Error: N must be divisible by the number of processes.\n");
            MPI_Abort(MPI_COMM_WORLD, 1);
            }
//если N не делится на число процессов,процесс с rank=0 заверашет программу через
    // Выделение памяти для локальной области и временной сетки
    double* local_grid = (double*)malloc((rows + 2) * cols * sizeof(double));  // +2 для "призрачных" строк
    double* temp_grid = (double*)malloc((rows + 2) * cols * sizeof(double));

    // Инициализация сетки
    initialize_grid(local_grid + cols, rows, cols);

    // Основной цикл итераций метода Якоби
    double global_diff = 0.0;
    int iter;
    for (iter = 0; iter < MAX_ITER; iter++) 
        {
        if (rank == 0)
            	{
		//printf("Iteration %d, global_diff = %f/n", iter, global_diff);
		//fflush(stdout);
	        }

	    // Обмен граничными строками
        MPI_Status status;
        if (rank > 0) 
            {  // Отправить верхнюю строку вверх и получить нижнюю строку от соседа
            MPI_Sendrecv(local_grid + cols, cols, MPI_DOUBLE, rank - 1, 0,
                         local_grid, cols, MPI_DOUBLE, rank - 1, 0, MPI_COMM_WORLD, &status);
            }
        if (rank < size - 1) 
 	    {  // Отправить нижнюю строку вниз и получить верхнюю строку от соседа
            MPI_Sendrecv(local_grid + rows * cols, cols, MPI_DOUBLE, rank + 1, 0,
                         local_grid + (rows + 1) * cols, cols, MPI_DOUBLE, rank + 1, 0, MPI_COMM_WORLD, &status);
            }
 
//здесь добавила 
//printf("Process %d completed boundary exchange for iteration %d.\n", rank, iter);
//fflush(stdout);

// Вычисление нового значения на основе метода Якоби
        double local_diff = 0.0;
        for (int i = 1; i <= rows; i++)
            {
            for (int j = 1; j < cols - 1; j++)
	        {
                temp_grid[i * cols + j] = 0.25 * (local_grid[(i - 1) * cols + j] +
                                                  local_grid[(i + 1) * cols + j] +
                                                  local_grid[i * cols + (j - 1)] +
                                                  local_grid[i * cols + (j + 1)]);
                local_diff += fabs(temp_grid[i * cols + j] - local_grid[i * cols + j]);
                } 
            }
// Обновление сетки
        double* swap = local_grid;
        local_grid = temp_grid;
        temp_grid = swap;

        // Суммирование локальных разностей для проверки сходимости
        MPI_Allreduce(&local_diff, &global_diff, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        if (global_diff / (N * N) < TOL) 
	    {
            if (rank == 0) 
	      {
//еще дополнила
  printf("Process %d converged at iteration %d with global_diff = %f.\n", rank, iter, global_diff);
fflush(stdout);            
		  
		  
//  printf("Converged after %d iterations with tolerance %.6f\n", iter, global_diff / (N * N));
            }
            break;
        }
    }
    
    //Конец измерения времени
    double end_time = MPI_Wtime();
    if (rank == 0) 
            {
	    printf("Total execution time: %f seconds\n", end_time - start_time);
            }
// Освобождение памяти
    free(local_grid);
    free(temp_grid);

    MPI_Finalize();
    return 0;
}

