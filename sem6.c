#include <stdio.h>
#include <stdlib.h>
#include <omp.h>
#include <stdbool.h>


// слияние  двух отсортированных подмассивов
void merge_arrays(int* source, int* buffer, int start, int midpoint, int end)
{
     int left_index = start; 
     int right_index = midpoint + 1; 
     int merge_index = start; // Индекс  результирующего массива

// Слияние двух подмассивов в один отсортированный массив
while (left_index <= midpoint && right_index <= end) 
     {
     if (source[left_index] <= source[right_index]) 
           {
           buffer[merge_index++] = source[left_index++]; // копирование меньшего элемента в буфер
           }
    
     else {
          buffer[merge_index++] = source[right_index++]; // Коп меньший элемента в буфер
          }
     }

// Копир оставшиеся элементы левой части
while (left_index <= midpoint)
      {
      buffer[merge_index++] = source[left_index++];
      }

// Копир оставшиеся элементы правой части
while (right_index <= end)
      {
      buffer[merge_index++] = source[right_index++];
      }

// Копир отсортированные элементы обратно в исходный массив
for (int i = start; i <= end; i++) 
      {
      source[i] = buffer[i];
      }
}

// параллельная сортировка слиянием
void parallel_merge_sort(int* source, int* buffer, int start, int end) 
     {
     if (start < end) 
          {
          int midpoint = start + (end - start) / 2; // ищем середину массива

          // Параллельное создание задач для сортировки двух половин массива
          #pragma omp task shared(source, buffer) if (end - start > 1000)
          parallel_merge_sort(source, buffer, start, midpoint);

          #pragma omp task shared(source, buffer) if (end - start > 1000)
          parallel_merge_sort(source, buffer, midpoint + 1, end);

          #pragma omp taskwait // Ждем завершения всех задач
          merge_arrays(source, buffer, start, midpoint, end); // Сливаем отсортир половины
          }
     }

// сравнение для qsort
int compare(const void* a, const void* b) 
     {
     return (*(int*)a - *(int*)b); // Возвращаем разницу между элементами
     }

// проверка отсортирован ли массив
bool is_sorted(int* array, int size) 
     {
     for (int i = 0; i < size - 1; ++i) 
           {
           if (array[i] > array[i + 1]) 
	         {
                 return false; // Если найден элемент, который больше следующего, массив не отсортирован
                 }
           }
     return true; // Если массив отсортирован
     }

int main(int argc, char* argv[]) 
     {
     if (argc != 3) 
           { 
           fprintf(stderr, "Usage: %s <N> <p>\n", argv[0]); 
           return -1; 
           }

     int element_count = atoi(argv[1]); // Число элементов массива
     int thread_count = atoi(argv[2]); // Число потоков для OpenMP

     // Выделение памяти для массивов
     int* data_array = (int*)malloc(element_count * sizeof(int));
     int* data_copy = (int*)malloc(element_count * sizeof(int));
     int* temp_array = (int*)malloc(element_count * sizeof(int));

     srand(omp_get_wtime()); // генератор случайных чисел
     for (int i = 0; i < element_count; i++) 
           {
           data_array[i] = rand() % 1000000; // Заполнение массива случайными числами
           }

     // Копирование оригинального массива для сравнения
     for (int i = 0; i < element_count; i++) 
           {
           data_copy[i] = data_array[i];
           }

     double start_qsort, end_qsort; // Переменные для измерения времени
     start_qsort = omp_get_wtime(); // Запоминаем время перед qsort
     qsort(data_copy, element_count, sizeof(int), compare); // Сортировка с помощью стандартного qsort
     end_qsort = omp_get_wtime() - start_qsort; // Запоминаем время после qsort

     double start_merge, end_merge; // Переменные для измерения времени сортировки слиянием
     start_merge = omp_get_wtime(); // Запоминаем время перед сортировкой

     // Параллельная сортировка слиянием
     #pragma omp parallel num_threads(thread_count)
          {
          #pragma omp single // Запускаем параллельную сортировку от одного потока
          {

    parallel_merge_sort(data_array, temp_array, 0, element_count - 1);
    }
  }

    end_merge = omp_get_wtime() - start_merge; // Запоминаем время после сортировки



    // Вывод времени работы
    printf("Time for qsort: %lf sec\n", end_qsort);
    printf("Time for multithreaded sorting: %lf sec\n", end_merge);

    // Освобождение выделенной памяти
    free(data_array);
    free(data_copy);
    free(temp_array);
    return 0;
}
