#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>

#define N 10  // Размер сетки
#define K 100 // Максимальное количество итераций

// Функция для вычисления количества живых соседей для клетки (i, j)
int count_neighbors(int *grid, int rows, int cols, int i, int j)
   //count_neighbors- функция которая возвращает кол-во живых соседей клетки
   //int *grid - указатель на массив где хранится состоние клеток
    {
    int count = 0; //переменная для хранения живых соседей клетки 
    int dirs[8][2] = {{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}};
//dirs-указаны координаты всех 8 соседей
    for (int d = 0; d < 8; d++) //перебор все 8 направлений из цикла dirs
        {
        int ni = i + dirs[d][0];// вычисляем индекс СТРОКИ соседа
        int nj = j + dirs[d][1];// вычисляем индекс СТОЛБЦА соседа

        // Проверяем, что сосед ni,nj находится внутри границы
        if (ni >= 0 && ni < rows && nj >= 0 && nj < cols)//ni >= 0 - не вышел за верхнюю и тд  
            {
            count += grid[ni * cols + nj];//подсчет живых соседей и если клетка живая то count +1
            }
        }
    return count;
    }
// Функция для применения правил игры жизни и обновления сетки
void game_of_life(int *grid, int *new_grid, int rows, int cols) 
	//game_of_life - обновляет состояние сетки
	//int *grid - указатель на текущую сетку
	//int *new_grid - указатель на сетку для обновленного состояния
   {
    for (int i = 0; i < rows; i++) //перебор строк с 0 и до последней
        {
        for (int j = 0; j < cols; j++)// перебор по столбцам
            {
            int neighbors = count_neighbors(grid, rows, cols, i, j);//count_neighbors - подсчет живых соседей
            if (grid[i * cols + j] == 1) // проверяем клетку, если ==1(живая), то перейдет к проверке условий выживания
	        {
		//правила для живой клетки	
                if (neighbors < 2 || neighbors > 3) //если меньше 2 соседей -умирает, если больше 3х- тоже умирает
	            {
                    new_grid[i * cols + j] = 0; // Умирает
                    } else 
		    {
                    new_grid[i * cols + j] = 1; // Живет
                    }
                 } else 
		    {
                    //правила для мертвой клетки
		    if (neighbors == 3) 
		       {
                       new_grid[i * cols + j] = 1; // если ровно 3 соседа,то оживает
                       } else 
		       {
                       new_grid[i * cols + j] = 0; // Остается мертвой
                       }
                    }
           }
        }
 }

// count_live_cells - функция для подсчета  живых клеток в сетке
int count_live_cells(int *grid, int rows, int cols) //int *grid - указатель на сетку, которая хранит состояния клеток (0-метрвая, 1 - живая)
    {
    int live_cells = 0;
    for (int i = 0; i < rows; i++) //идем по каждой строке от 0 до i < rows
        {
        for (int j = 0; j < cols; j++)// так же перебираем столбцы
            {
            live_cells += grid[i * cols + j];// подсчет живых клеток
					     // если живая grid[i * cols + j]==1, ее значение прибавляется к live_cells, если не живая сумма не меняется
            }
        }
    return live_cells;
    }

// Функция для вывода текущего состояния сетки
void print_grid(int *grid, int rows, int cols) //print_grid -  выводит сетку на экран в виде двумерной таблицы
     {
     for (int i = 0; i < rows; i++) 
         {
         for (int j = 0; j < cols; j++) 
	      {
              printf("%d ", grid[i * cols + j]);
              }
         printf("\n");
         }
     printf("\n");
     }

int main(int argc, char **argv) {
    int rank, size;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int rows_per_proc = N / size;//rows_per_proc - колво строк обрабатываемых каждым процессом (размер/процесс)
    int *grid = (int *)malloc(rows_per_proc * N * sizeof(int));//malloc(rows_per_proc * N * sizeof(int) - вычисление объема памяти
    int *new_grid = (int *)malloc(rows_per_proc * N * sizeof(int));

    // Инициализация начальной сетки
    if (rank == 0) 
        {
        // Инициализация всей сетки 
        int *full_grid = (int *)malloc(N * N * sizeof(int));
        for (int i = 0; i < N * N; i++) 
	    {
            full_grid[i] = rand() % 2; // Заполнение случайными значениями 0 или 1
            }

	// Распределение данных по процессам
        MPI_Scatter(full_grid, rows_per_proc * N, MPI_INT, grid, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);
        free(full_grid);
        } 
        else 
        {
        // Получаем данные от процесса 0
        MPI_Scatter(NULL, 0, MPI_INT, grid, rows_per_proc * N, MPI_INT, 0, MPI_COMM_WORLD);
        }

    //засекаем время начала работы программы 
    double start_time = MPI_Wtime();

    // Основной цикл игры
    int iteration = 0;
    int game_running = 1;// флаг который управляет продолжением игры
    int previous_total_live_cells = -1;// хранит колво живых клеток на предыдущей итерации
    int total_live_cells = 0;// кол-во на текущей итерации

    while (iteration < K && game_running) 
        {
        // Обмен граничными строками с соседними процессами
        MPI_Request reqs[4];// массив для хранения запросов на неблокирующие операции (2 на отправку, 2 на получение)
	int req_count = 0;// счетчик для кол-ва активных запросов а если без него??

        // Отправка верхней строки и получение нижней строки
        if (rank > 0) //если процесс не первый
            {
            MPI_Isend(grid, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &reqs[0]);
            MPI_Irecv(grid + (rows_per_proc - 1) * N, N, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &reqs[1]);
            }
        if (rank < size - 1) // если не последний
	    {
            MPI_Isend(grid + (rows_per_proc - 1) * N, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &reqs[2]);
            MPI_Irecv(grid, N, MPI_INT, rank + 1, 0, MPI_COMM_WORLD, &reqs[3]);
            }


	// Ждем завершения обмена
        MPI_Waitall(req_count, reqs, MPI_STATUSES_IGNORE);

        // Применение правил игры
        game_of_life(grid, new_grid, rows_per_proc, N);

// Подсчет живых клеток в текущем процессе
int local_live_cells = count_live_cells(grid, rows_per_proc, N);

//int local_live_cells = 0;
//for (int i = 0; i < rows_per_proc; i++) {
//for (int j = 0; j < N; j++) {
//local_live_cells += grid[i * N + j];
//}
//}

// Общее количество живых клеток по всем процессам
MPI_Reduce(&local_live_cells, &total_live_cells, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

// Если процесс 0, проверяем, изменилась ли ситуация
if (rank == 0) {
if (total_live_cells == previous_total_live_cells) {
game_running = 0; // Завершаем игру
}
previous_total_live_cells = total_live_cells; // Обновляем количество живых кле
printf("Iteration %d: Live cells = %d\n", iteration, total_live_cells);
print_grid(grid, N, N); //вывод текущего значения сетки
}

// Обмен значениями между текущим и новым состоянием
int *temp = grid;
grid = new_grid;
new_grid = temp;

iteration++;
}      
     //засекаем время работы программы 
    double end_time = MPI_Wtime();

    //вывод время выполнения программы
   if (rank ==0)
        {
	printf("Total execution time: %f seconds\n", end_time - start_time);
        }

	// Проверка, изменилось ли состояние сетки
      // int total_live_cells;
       // MPI_Reduce(&total_live_cells, &total_live_cells, 1, MPI_INT, MPI_SUM, 0, MPI_COMM_WORLD);

       // if (rank == 0) {
            // Если изменения не было, завершаем игру
        //    if (total_live_cells == previous_total_live_cells) {
          //      game_running = 0;
           // }
       // }


	// Обмен значениями между текущим и новым состоянием
       // int *temp = grid;
       // grid = new_grid;
       // new_grid = temp;

       // iteration++;
    //}

    // Завершаем MPI
    MPI_Finalize();
    free(grid);
    free(new_grid);

    return 0;
}

