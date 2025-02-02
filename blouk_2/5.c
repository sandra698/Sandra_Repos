#include <mpi.h>
#include <stdio.h>

int main (int argc, char **argv)
     {
     int rank;
     float a, b;
     MPI_Status status;// структура, которая даст информ как прошел обмен

     MPI_Init (&argc, &argv);
     MPI_Comm_rank (MPI_COMM_WORLD, &rank);//каждому сообщили его ранг
     
     a = 0.0;
     b = 0.0;

     if (rank == 0)
        {
	b = 1.0;            //процессс 0 устанавливает что b =1
        MPI_Send (&b, 1, MPI_FLOAT, 1, 5, MPI_COMM_WORLD);
        MPI_Recv (&a, 1, MPI_FLOAT, 1, 5, MPI_COMM_WORLD, &status);
        }
     
     if (rank ==1)
        {
        a = 2.0;
	MPI_Recv (&b, 1, MPI_FLOAT, 0, 5, MPI_COMM_WORLD, &status);
	MPI_Send (&a, 1, MPI_FLOAT, 0, 5, MPI_COMM_WORLD);
	}

     printf ("process %d a = %f, b = %f\n", rank, a, b);
     MPI_Finalize() ;
     return 0;
     }

