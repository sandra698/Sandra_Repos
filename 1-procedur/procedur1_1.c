#include <stdio.h>
#include <mpi.h>

int main(int argc, char **argv)  //число аргументов и массив строк
    {
    printf ("Before MPI_INIT\n"); // вывод строки перед инициализацией MPI
    
    MPI_Init (&argc, &argv);
    printf ("Parallel section\n");

    MPI_Finalize();
    printf ("After MPI_Finalize\n");
    }

