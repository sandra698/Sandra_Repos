#include <stdio.h>
#include <omp.h>
#include <stdlib.h>
#include <time.h>

// параметры программы 
#define A  0 
#define B  100
#define x  50

int main(int argc, char* argv[])
        {
	if (argc != 2) return 1;//если аргументов не 2 возвращаем
	int N; 
	N = strtol(argv[1], NULL , 10); //ввод числа точек 
	struct drand48_data randBuffer;// буфер для чисел
	int  k[N], x1;
	double p = 0.5, s, t[N], generaltime;
	if ((x < A) || (x > B))
           	{
		printf("incorrect x\n");// если х не в пределах то вывод ошибки 
		return 1;
	        }
	for (int i = 0; i < N; i++)
           	{ 
		k[i] = 0;
		t[i] = 0;
         	}
        generaltime = omp_get_wtime();// старт времени
#pragma omp parallel private(s, randBuffer)  //распараллеливаем на число потоков N
	        {		   //копируем переменные р, х и s в каждый поток
            srand48_r(time(NULL) * omp_get_thread_num(), &randBuffer);// для каждого потока генерировалось последовательноность случайных чисел 
      
       #pragma omp for firstprivate(x1)
       for (int i = 0; i < N; i++) 
                   {
	           x1 = x;
                   t[i] = omp_get_wtime();// время жизни частицы
                   while ((x1 != A) && (x1 != B))
		           { // пока частицу не поглотила граница
	                   drand48_r(&randBuffer, &s);// генерируем случайное число
	                   if (s <= p)
			       { // с вероятностью р шаг вправо 
		               x1 +=1;
	                       }
	                   else
		               {
		               x1 -= 1;// с вероятностью 1-р шаг  влево 
	                       }
                   }
       if (x1 == B) k[i] = 1; // попала ли частица с номером i в границу b

       t[i] = omp_get_wtime() - t[i]; 
               }

	}	
       generaltime = omp_get_wtime() - generaltime;// общее время программы 
       int sum = 0;
       double sumt = 0;
       for (int i = 0; i < N; i++)
               { // считаем вероятность и среднее время жизни
	       sum += k[i];
	       sumt += t[i];
                }
       printf("generaltime: %lf\n time_of_one_point: %lf\n", generaltime, sumt / N);
       printf("%lf\n", ((double)sum) / N);
        
       return 0;
}
        


