#include <mpi.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define N 64  // Размер всей сетки (должен быть степенью двойки)
#define MAX_ITER 100  // Максимальное число итераций
#define TOL 1e-6  // Точность

// Функция для инициализации сетки случайными значениями
void initialize_grid(double* grid, int Nx, int Ny, int Nz) {
    for (int i = 0; i < Nx; i++) {
        for (int j = 0; j < Ny; j++) {
            for (int k = 0; k < Nz; k++) {
                grid[i * Ny * Nz + j * Nz + k] = rand() / (double)RAND_MAX;
            }
        }
    }
}

int main(int argc, char** argv) {
    int rank, size, dims[3] = {0, 0, 0};
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Определяем декартову топологию
    MPI_Dims_create(size, 3, dims);  // dims = {Px, Py, Pz}
    MPI_Comm cart_comm;
    int periods[3] = {0, 0, 0};  // Без циклической топологии
    MPI_Cart_create(MPI_COMM_WORLD, 3, dims, periods, 1, &cart_comm);

    // Получаем координаты процесса в топологии
    int coords[3];
    MPI_Cart_coords(cart_comm, rank, 3, coords);

    // Находим соседей
    int neighbors[6];
    MPI_Cart_shift(cart_comm, 0, 1, &neighbors[0], &neighbors[1]); // по оси X
    MPI_Cart_shift(cart_comm, 1, 1, &neighbors[2], &neighbors[3]); // по оси Y
    MPI_Cart_shift(cart_comm, 2, 1, &neighbors[4], &neighbors[5]); // по оси Z

    // Размеры локальной области
    int Nx = N / dims[0];
    int Ny = N / dims[1];
    int Nz = N / dims[2];

    // Проверяем, делится ли сетка на число процессов
    if (rank == 0 && (N % dims[0] != 0 || N % dims[1] != 0 || N % dims[2] != 0)) {
        printf("Error: N must be divisible by the number of processes in each dimension.\n");
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Выделяем память
    double* local_grid = (double*)malloc((Nx + 2) * (Ny + 2) * (Nz + 2) * sizeof(double)); // +2 для "призрачных" ячеек
    double* temp_grid = (double*)malloc((Nx + 2) * (Ny + 2) * (Nz + 2) * sizeof(double));

    if (!local_grid || !temp_grid) {
        printf("Error: Memory allocation failed on process %d\n", rank);
        MPI_Abort(MPI_COMM_WORLD, 1);
    }

    // Инициализация локальной сетки
    initialize_grid(local_grid + Ny * Nz + Nz, Nx, Ny, Nz);

    // Создаем производные типы данных для обмена слоями
    MPI_Datatype xy_plane, xz_plane, yz_plane;
    MPI_Type_vector(Ny, Nz, Nz + 2, MPI_DOUBLE, &xy_plane); // Плоскость XY
    MPI_Type_commit(&xy_plane);
    MPI_Type_vector(Nx, Nz, (Ny + 2) * (Nz + 2), MPI_DOUBLE, &xz_plane); // Плоскость XZ
    MPI_Type_commit(&xz_plane);
    MPI_Type_vector(Nx, Ny, Nz + 2, MPI_DOUBLE, &yz_plane); // Плоскость YZ
    MPI_Type_commit(&yz_plane);

    double global_diff = 0.0;
    double start_time = MPI_Wtime(); // Начало отсчета времени
    for (int iter = 0; iter < MAX_ITER; iter++) {
        // Обмен данными с соседями (ассинхронно)
        MPI_Request requests[12]; // 6 отправок и 6 приемов
        int req_count = 0;

        // По оси X
        if (neighbors[0] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, yz_plane, neighbors[0], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(0) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, yz_plane, neighbors[0], 0, cart_comm, &requests[req_count++]);
        }
        if (neighbors[1] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(Nx) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, yz_plane, neighbors[1], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(Nx + 1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, yz_plane, neighbors[1], 0, cart_comm, &requests[req_count++]);
        }

        // По оси Y
        if (neighbors[2] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, xz_plane, neighbors[2], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (0) * (Nz + 2) + 1], 1, xz_plane, neighbors[2], 0, cart_comm, &requests[req_count++]);
        }
        if (neighbors[3] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (Ny) * (Nz + 2) + 1], 1, xz_plane, neighbors[3], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (Ny + 1) * (Nz + 2) + 1], 1, xz_plane, neighbors[3], 0, cart_comm, &requests[req_count++]);
        }

        // По оси Z
        if (neighbors[4] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 1], 1, xy_plane, neighbors[4], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + 0], 1, xy_plane, neighbors[4], 0, cart_comm, &requests[req_count++]);
        }
        if (neighbors[5] != MPI_PROC_NULL) {
            MPI_Isend(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + (Nz)], 1, xy_plane, neighbors[5], 0, cart_comm, &requests[req_count++]);
            MPI_Irecv(&local_grid[(1) * (Ny + 2) * (Nz + 2) + (1) * (Nz + 2) + (Nz + 1)], 1, xy_plane, neighbors[5], 0, cart_comm, &requests[req_count++]);
        }

        // Ждем завершения всех операций обмена
        MPI_Waitall(req_count, requests, MPI_STATUSES_IGNORE);

        // Выполнение итерации метода Якоби
        double local_diff = 0.0;
        for (int i = 1; i <= Nx; i++) {
            for (int j = 1; j <= Ny; j++) {
                for (int k = 1; k <= Nz; k++) {
                    temp_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k] =
                        (local_grid[(i - 1) * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k] +
                         local_grid[(i + 1) * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k] +
                         local_grid[i * (Ny + 2) * (Nz + 2) + (j - 1) * (Nz + 2) + k] +
                         local_grid[i * (Ny + 2) * (Nz + 2) + (j + 1) * (Nz + 2) + k] +
                         local_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + (k - 1)] +
                         local_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + (k + 1)]) / 6.0;
                    local_diff += fabs(temp_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k] -
                                       local_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k]);
                }
            }
        }

        // Обновление сетки
        double* swap = local_grid;
        local_grid = temp_grid;
        temp_grid = swap;

        // Суммирование локальных разностей
        MPI_Allreduce(&local_diff, &global_diff, 1, MPI_DOUBLE, MPI_SUM, cart_comm);

        // Проверка на сходимость
        if (global_diff / (N * N * N) < TOL) {
            if (rank == 0) {
                printf("Converged after %d iterations with global_diff = %f.\n", iter, global_diff);
            }
            break;
        }
    }
    double end_time = MPI_Wtime(); // Конец отсчета времени

    // Вычисление нормы
    double local_count = 0.0;
    for (int i = 1; i <= Nx; i++) {
        for (int j = 1; j <= Ny; j++) {
            for (int k = 1; k <= Nz; k++) {
                local_count += pow(temp_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k] -
                                   local_grid[i * (Ny + 2) * (Nz + 2) + j * (Nz + 2) + k], 2);
            }
        }
    }

    double global_count = 0.0;
    MPI_Reduce(&local_count, &global_count, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

    // Вывод результата
    if (rank == 0) {
        printf("Norma: %lf\n", sqrt(global_count / N / N / N));
        printf("Execution time: %lf seconds\n", end_time - start_time);
    }

    // Очистка памяти и завершение
    free(local_grid);
    free(temp_grid);
    MPI_Type_free(&xy_plane);
    MPI_Type_free(&xz_plane);
    MPI_Type_free(&yz_plane);

    MPI_Finalize();
    return 0;
}
