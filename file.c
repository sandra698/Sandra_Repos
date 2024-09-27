#include <pthread.h>//для работы с потоками 
#include <stdio.h>// для функций ввода/вывода
#include <stdlib.h>// для функций выделения памяти
#include <sys/time.h>// для работы со временем
#include  <math.h>//для математических операций

struct timeval start_time, end_time, time_diff;
//структуры для хранения времени начала и конца программы
struct timezone timezone_info;
//структура для хранения информации о временной зоне 

void time_start() {
        gettimeofday(&start_time, &timezone_info);
}
//функция для начала отсчета времени  и сохранения его в переменную start time

long end_timing()
//функция для завершения отсчета времени и возврата результата в миллисекундах
{


        gettimeofday(&end_time, &timezone_info);//сохранить текущее время завершения

	//вычислили разницу во времени
        time_diff.tv_sec = end_time.tv_sec - start_time.tv_sec;
        time_diff.tv_usec = end_time.tv_usec -start_time.tv_usec;


	//проверяем, если микросекунды ушли в отрицания
        if (time_diff.tv_usec<0) {
                time_diff.tv_sec--;// уменьшаем кол-во сек
                time_diff.tv_usec+=1000000;// корректируем микросекунды
        }


	
   return time_diff.tv_sec*1000+time_diff.tv_usec/1000;//возвращаем время в миллисекундах
}

void *nit_func(void * arg){ //функция которую выполняет каждая нить
    long k, j, *arg_int = (long*)arg, count;// ввели 3 переменные k g as. Указали массив сторк и кол-во 

    double chunk, s, paf = 0, *arg_double = (double*)arg_int;//создали еще указатель  result который привели к веществ типу, для того чтоы вернуть значение из функции 
    chunk = 1.0 / (double)arg_int[1];  //вычисляем ширину интервала 
    k = (long)(arg_int[1] / arg_int[2]);//вычисляем кол-во интервалов 
    count = k * (arg_int[0] + 1);//число операций в цикле или длина в цикле 
    for (j = k *arg_int[0]; j < count; j++)//сам цикл 
    {
         s = j * chunk;//то значение, откуда считаем функцию
         paf += chunk * (4.0 / (1.0 + s * s ));//под счет функции и при прибавлении ее к паф - то значение, которое мы вернем 
    }
arg_double[0] = paf;// записали паф в возвращемый указатель

}
int main (int argc, char* argv[]){
     time_start();
     double pi = 0;
     long i,N, **a, kol;
     if (argc != 3) return 1;
     N = strtol(argv[1], NULL, 10);
     kol = strtol(argv[2], NULL, 10);
     a = (long**)malloc(kol *sizeof(long*));
     for (i = 0; i < kol; i++){
             a[i] = (long*)malloc(3 * sizeof(long));
             a[i][1] = N;  a[i][2] = kol;
     }
     pthread_t pth[kol];

for (i =0; i < kol; i++){
             a[i][0] = i;
             pthread_create(&pth[i], NULL, nit_func, a[i]);
     }

//ждем завершение потока
     for (i = 0; i <  kol; i++) {
             pthread_join(pth[i],  NULL);
             pi += ((double*)a[i]) [0];//добовляем результат от каждого потока в общую сумму
             free(a[i]);
    }
     free(a);
     printf("выводим число:%f\n", pi);
     printf("выводим время работы программы:%lu\n" , end_timing());
     return 0;
}








