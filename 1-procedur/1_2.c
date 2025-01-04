//Каждый запущенный процесс печатает свой уникальный номер в коммуникаторе COMM WORLD  и число процессов в этом коммуникаторе

#include <stdio.h>
#include <mpi.h>

	int main(int argc, char **argv)
	{
        int rank, size; //объявляем 2 переменные rank-номер процесса size-общее кол-во процессов
	MPI_Init (&argc, &argv);//принимает параметры запуска
	MPI_Comm_size (MPI_COMM_WORLD, &size);// сколько всего  процессов в данном коммуникаторе
	MPI_Comm_rank (MPI_COMM_WORLD, &rank);//сообщает номер (ранг) процесса

	printf ("process %d, size %d\n", rank, size);//выводит номер процесса и общее кол-во процеса / rank подставляется в process %d /size поставляется во второй %d
	MPI_Finalize();
	}

// ВЫВОД
// process 0, size 4
// process 1, size 4
// process 2, size 4
// process 3, size 4

