#include <mpi.h>  
#include <stdio.h> 
#include <stdlib.h> 
#include <math.h>   

// заполняем массив случайными числами от 0 до 9
void create_random_matrix(float *matrix, int rows, int cols) //float *matrix - указатель на массив, create_random_matrix - моя функция, которая принимает указатель на массив и размер матрицы (строки и столбцы)

    {
    for (int i = 0; i < rows * cols; i++) // начальное значение счетчика 0,затем итерируем по всем элементам матрицы, rows * cols - размер матрицы 
        {
        matrix[i] = rand() % 10; // генерируем случайное число от 0 до 9 и записываем в ячейку i
        }
    } 

// выводим матрицу построчно
void print_matrix(float *matrix, int rows, int cols)
    {
    for (int i = 0; i < rows; i++)
        {
        for (int j = 0; j < cols; j++) //j управляет столбцами в текущей строке, цикл выполняется cols раз для каждой строки
            {
            printf("%.2f ", matrix[i * cols + j]); // печать элемента, i * cols — смещение к началу строки i, + j — смещение к столбцу


            }
        printf("\n");
        }
    }

int main(int argc, char **argv)
    {
    int rank, size; // ранг  общее число
    int dims[2], periods[2] = {0, 0}, coords[2]; // dims[2]- задаёт количество процессов в каждом из двух измерений декартовой решётки
    int N = 8;  // размер матрицы
    int b = 2;  // размер блока
    float *A_local, *B_local, *C_local;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size); // определение общего числа процессов

    // проверяем, что количество процессов образует квадрат
    dims[0] = dims[1] = (int)sqrt(size); // какой  размеры декартовой решетки
    if (dims[0] * dims[1] != size)
       { // Проверяем, что размер равен общему числу процессов
        if (rank == 0)
            { // Сообщение выводится только процессом 0
            printf("Количество процессов должно быть квадратным числом.\n");
            }
        MPI_Abort(MPI_COMM_WORLD, 1); // завершается программа, если условие не выполнено
    }

    // создание декартовой топологии
    MPI_Comm cart_comm; //новый коммуникатор для работы с декартовой топологией
    MPI_Cart_create(MPI_COMM_WORLD, 2, dims, periods, 1, &cart_comm); // 2-число изменений в решетке,dims-массив, задающий размеры решётки по каждому измерению
    MPI_Comm_rank(cart_comm, &rank); // получаем новый ранг процесса в топологии
    MPI_Cart_coords(cart_comm, rank, 2, coords); // получаем координаты процесса в решетке

    // размер локальных блоков
    int local_N = N / dims[0]; // Размер блока матрицы, который хранится у одного процесса
    if (local_N % b != 0)
       { // Проверяем, что размер блока делится на b
        if (rank == 0)
            { // Сообщение выводится только процессом 0
            printf("Размер блока b должен делить local_N без остатка.\n");
             }
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

     // Выделяем память для локальных блоков матриц
    A_local = (float *)malloc(local_N * local_N * sizeof(float)); // Локальный блок A
    B_local = (float *)malloc(local_N * local_N * sizeof(float)); // Локальный блок B
    C_local = (float *)calloc(local_N * local_N, sizeof(float)); // Локальный блок C (заполняется нулями)

    // Генерация данных в процессе 0
    float *A_global = NULL, *B_global = NULL; // полные матрицы, хранятся только в процессе 0
    if (rank == 0) { // Если это главный процесс
        A_global = (float *)malloc(N * N * sizeof(float)); // выделение памяти для матрицы A
        B_global = (float *)malloc(N * N * sizeof(float)); // и для  B так же
        create_random_matrix(A_global, N, N); // заполнение A случайными числами

	create_random_matrix(B_global, N, N);
        printf("Матрица A:\n");
        print_matrix(A_global, N, N);
        printf("Матрица B:\n");
        print_matrix(B_global, N, N);
    }


    // определение типа данных для отправки блоков
    MPI_Datatype block_type;
    MPI_Type_vector(local_N, local_N, N, MPI_FLOAT, &block_type); // создаем  тип данных для блоков
    MPI_Type_create_resized(block_type, 0, local_N * sizeof(float), &block_type); // MPI_Type_create_resized-она создаёт тип данных, который будет использоваться для передачи блоков данных между процессами.
    MPI_Type_commit(&block_type); // фиксируем тип данных

    // Вычисляем массивы sendcounts и displs для Scatterv
    int *sendcounts = (int *)malloc(size * sizeof(int)); // массив количества отправляемых элементов
    int *displs = (int *)malloc(size * sizeof(int)); // массив смещений
    for (int i = 0; i < dims[0]; i++)
        {
        for (int j = 0; j < dims[1]; j++)
            {
            displs[i * dims[1] + j] = i * N * local_N + j * local_N; // смещения по строкам и столбцам
            sendcounts[i * dims[1] + j] = 1; // отправляем по одному блоку
            }
        }

    // распределение блоков матриц A  B по всем процессам
    MPI_Scatterv(A_global, sendcounts, displs, block_type, A_local, local_N * local_N, MPI_FLOAT, 0, cart_comm);
    MPI_Scatterv(B_global, sendcounts, displs, block_type, B_local, local_N * local_N, MPI_FLOAT, 0, cart_comm);
    //int *sendcounts -м ассив, который указывает количество элементов, отправляемых каждому процессу
    //int *displs- массив смещений для каждого процесса, который показывает, с какого элемента в sendbuf процесс получает данные
    //MPI_Datatype sendtype-тип данных в sendbuf
    //void *recvbuf-буфер, куда каждый процесс будет получать свои данные
    //int recvcount-количество данных, которые процесс получает
    //MPI_Datatype recvtype -тип данных в recvbuf
    //int root-ранк процесса, который отправляет данные

    // SUMMA
    for (int k = 0; k < dims[0]; k++)
       { // Цикл по всем блокам
        int src, dest;

        // Распространение блоков A вдоль строк
        MPI_Cart_shift(cart_comm, 0, k - coords[0], &src, &dest); // находим источник и получателя для сдвига
        MPI_Sendrecv_replace(A_local, local_N * local_N, MPI_FLOAT, dest, 0, src, 0, cart_comm, MPI_STATUS_IGNORE);//обмен данными между двумя процессами

        //local_N * local_N - кол-во элементов, которые будут отправлены и получены
        //dest — ранг процесса, на который будут отправляться данные
        //0  — тег отправки для идентификации сообщения

        // Распространение блоков B вдоль столбцов
        MPI_Cart_shift(cart_comm, 1, k - coords[1], &src, &dest);
        MPI_Sendrecv_replace(B_local, local_N * local_N, MPI_FLOAT, dest, 0, src, 0, cart_comm, MPI_STATUS_IGNORE);

	// Умножение локальных блоков
        for (int i = 0; i < local_N; i++)
           {
            for (int j = 0; j < local_N; j++)
                {
                for (int l = 0; l < local_N; l++)
                    {
                    C_local[i * local_N + j] += A_local[i * local_N + l] * B_local[l * local_N + j];
                    }
                }
            }
        }

    // сбор результатов матрицы C
    float *C_global = NULL;
    if (rank == 0)
        { // если это главный процесс
        C_global = (float *)malloc(N * N * sizeof(float)); // то выделить  память для полной матрицы C
        }
    MPI_Gatherv(C_local, local_N * local_N, MPI_FLOAT, C_global, sendcounts, displs, block_type, 0, cart_comm);

    // вывод результата в процессе 0 , а если без него?
    if (rank == 0)
        {
        printf("Результат умножения матриц C:\n");
        print_matrix(C_global, N, N);
        free(C_global); // освобождаем память
        }

     // Освобождаем память для локальных матриц
    free(A_local);
    free(B_local);
    free(C_local);
    if (rank == 0)
        {
        free(A_global);
        free(B_global);
        }

    MPI_Finalize();
    return 0;
}

