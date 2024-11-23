#include <stdio.h>
#include <mpi.h>

int main (int argc, char **argv)
        {
	int np, rank;  //np-общее число процессоров  rank-ранг текущего процессора
	printf ("Hellow before Init!\n");
	
        MPI_Init(&argc, &argv);

	MPI_Comm_rank(MPI_COMM_WORLD, &rank);// узнаем ранг из всей группы процессоров
	MPI_Comm_size(MPI_COMM_WORLD, &np);// узнаем кол-во процессоров в коммутаторе

	printf("%d%d, hello\n", rank, np);

	MPI_Finalize();
	return 0 ;
	}


