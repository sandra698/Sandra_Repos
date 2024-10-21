    #include "omp.h"
    #include <stdio.h>
    #include <stdlib.h>
    #include <immintrin.h>
    #include <math.h>

    int main()
        {
        // Ввод размера матрицы
        int size;
        printf("Введите размер матрицы, кратный 2: ");
        if (scanf("%d", &size) != 1 || size % 2 != 0) {
        printf("Ошибка: введенное значение должно быть кратно 2.\n");
        return 1;
        }

    printf("Используем размер матрицы: %d x %d\n", size, size);

    // Выделение памяти под матрицы
    double **matA, **matB, **matResSeq, **matResVec;
    matA = (double**)malloc(size * sizeof(double*));
    matB = (double**)malloc(size * sizeof(double*));
    matResSeq = (double**)malloc(size * sizeof(double*));
    matResVec = (double**)malloc(size * sizeof(double*));

    // Инициализация матриц и заполнение их начальными значениями
    for (int i = 0; i < size; i++)
        {
        matA[i] = (double*)malloc(size * sizeof(double));
        matB[i] = (double*)malloc(size * sizeof(double));
        matResSeq[i] = (double*)malloc(size * sizeof(double));
        matResVec[i] = (double*)malloc(size * sizeof(double));
        for (int j = 0; j < size; j++)
            {
            matA[i][j] = (double)(i + 1) / (j + 1);
            matB[i][j] = (double)(j + 1) / (i + 1);
            matResSeq[i][j] = 0;
            matResVec[i][j] = 0;
            }
         }

    // Последовательный вариант перемножения матриц
    double seqTimeStart = omp_get_wtime();
    for (int i = 0; i < size; i++)
        {
        for (int j = 0; j < size; j++)
            {
             double sum = 0.0;
             for (int k = 0; k < size; k++)
	         {
                 sum += matA[i][k] * matB[k][j];
                 }
             matResSeq[i][j] = sum; // Результат для последовательного варианта
             }
        }
    double seqTimeEnd = omp_get_wtime() - seqTimeStart;

    // Векторизованный вариант с использованием AVX
    double vecTimeStart = omp_get_wtime();
    for (int i = 0; i < size; i += 2)
          { // Шаг 2, т.к. используем AVX
          for (int j = 0; j < size; j += 2)
                 {
                  __m256d vecC1 = _mm256_setzero_pd(); // Инициализация блока результата
                  __m256d vecC2 = _mm256_setzero_pd(); // Вторая часть результата

                  for (int k = 0; k < size; k += 2)
	                 { // Перемножение 2x2 блоков
                         // Загрузка строк из матрицы A
                          __m256d vecA1 = _mm256_set_pd(matA[i][k], matA[i+1][k], matA[i][k], matA[i+1][k]);
                          __m256d vecA2 = _mm256_set_pd(matA[i][k+1], matA[i+1][k+1], matA[i][k+1], matA[i+1][k+1]);

                         // Загрузка столбцов из матрицы B
                          __m256d vecB1 = _mm256_set_pd(matB[k][j], matB[k][j], matB[k][j], matB[k][j]);
                          __m256d vecB2 = _mm256_set_pd(matB[k+1][j], matB[k+1][j], matB[k+1][j], matB[k+1][j]);

                         // Перемножение и накопление результатов
                         __m256d res1 = _mm256_mul_pd(vecA1, vecB1);
                         __m256d res2 = _mm256_mul_pd(vecA2, vecB2);
                         vecC1 = _mm256_add_pd(vecC1, res1);
                         vecC2 = _mm256_add_pd(vecC2, res2);
                         }

                 // Сохранение результатов обратно в матрицу
                 double* result1 = (double*)&vecC1;
                 double* result2 = (double*)&vecC2;
                 matResVec[i][j] = result1[3];
                 matResVec[i+1][j] = result1[2];
                 matResVec[i][j+1] = result2[3];
                 matResVec[i+1][j+1] = result2[2];
                 }
        }
    double vecTimeEnd = omp_get_wtime() - vecTimeStart;

    // Проверка точности и сравнение результатов
    double epsilon = 0.0, maxError = 0.0;
    for (int i = 0; i < size; i++)
             {
             for (int j = 0; j < size; j++)
	            {
                    double error = fabs(matResSeq[i][j] - matResVec[i][j]);
                    epsilon += error;
                    if (error > maxError) maxError = error;
                     }
             }

    // Вывод времени выполнения и ошибки
    printf("Последовательное время: %lf\n", seqTimeEnd);
    printf("Векторное время: %lf\n", vecTimeEnd);
    printf("Суммарная ошибка: %lf, Максимальная ошибка: %lf\n", epsilon, maxError);

    // Освобождение памяти
    for (int i = 0; i < size; i++)
             {
             free(matA[i]);
             free(matB[i]);
             free(matResSeq[i]);
             free(matResVec[i]);
             }
    free(matA);
    free(matB);
    free(matResSeq);
    free(matResVec);

    return 0;
    }


