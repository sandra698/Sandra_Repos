    #include <immintrin.h>
    #include <stdio.h>
    #include <stdlib.h>
    #include <omp.h>
    #include <math.h>
    void mat_mult_avx(float* A, float* B, float* C, int N)//mat_mult_avx - функция которая умножаем матрицы и результат в С, н размерность
    {
    for (int i = 0; i < N; i++)// цикл по СТРОКАМ  А
   {
    for (int j = 0; j < N; j++)// цикл по СТОЛБЦАМ В
       {
        __m256 sum = _mm256_setzero_ps(); // Инициализация суммы в AVX-регистре
        for (int k = 0; k < N; k += 8)  // Обрабатываем по 8 элементов за раз 
	
           {
           // Загружаем 8 элементов из строки A и 8 элементов из столбца B
           __m256 vec_a = _mm256_loadu_ps(&A[i * N + k]);
           __m256 vec_b = _mm256_set_ps(B[(k + 7) * N + j], B[(k + 6) * N + j], B[(k + 5) * N + j], B[(k + 4) * N + j], B[(k + 3) * N + j], B[(k + 2) * N + j], B[(k + 1) * N + j], B[k * N + j]);

           // Умножаем и аккумулируем результат
           sum = _mm256_fmadd_ps(vec_a, vec_b, sum);//умножаем  векторы А и В  ирезультат в сумму (fmadd - выполняет умножение и сложение за одну операцию)
           }

    // Сохраняем результат в массив AVX в temp
    float temp[8];
    _mm256_storeu_ps(temp, sum);

    // сумму элементов temp записываем в С
    C[i * N + j] = temp[0] + temp[1] + temp[2] + temp[3] + temp[4] + temp[5] + temp[6] + temp[7];
    }
    }
    }


    int main()
    {
    // сделаем так, чтобы могли вводить размер матрицы сами
    int N ; 
	    
    printf ("Введите размер матрицы");
    scanf ("%d", &N);
// проверка, чтобы корректно вводили число 
   if (N <= 0 || N % 8 != 0)
         {
         printf ("Размер должен быть кратным 8 и положительным");
	 return 1; //завершить, но с ошибкой
	 }

   // выделение памяти для матриц и выравниваем по границе 32 байта
    float* A = (float*) aligned_alloc(32, N * N * sizeof(float));
    float* B = (float*) aligned_alloc(32, N * N * sizeof(float));
    float* C = (float*) aligned_alloc(32, N * N * sizeof(float));
    float* D = (float*) aligned_alloc(32, N * N * sizeof(float));//D- результат векторизованной матрицы

    // Инициализация матриц A и B случайными числами
    for (int i = 0; i < N * N; i++) {
    A[i] = rand() % 10;// заполнение А
    B[i] = rand() % 10;// заполнение В  от 0 до 9 числа случайны
    }

    // Векторизованное умножение с AVX
    double start_time = omp_get_wtime();// записываем время начала выполнения
    mat_mult_avx(A, B, C, N);// умножаем
    double end_time = omp_get_wtime();// записываем время окончания
 
    printf("Время выполнения векторизованной версии: %f секунд\n", end_time - start_time);

    // последовательный вариант
    double tp = omp_get_wtime();
    for (int i = 0; i < N; i++){
    for (int j = 0; j < N; j++){
    float sum = 0.0;
    for (int k = 0; k < N; k++){
    sum += A[i*N+k] * B[k*N+j];
    }
    D[i*N+j] = sum;
    }
    }
    tp = omp_get_wtime() - tp;

    //написать вывод результата
    float eps = 0, epmax = 0, ep = 0;
    for (int i = 0; i < N; i++){
    for (int j = 0; j < N; j++){
    ep = fabsf(C[i*N+j] - D[i*N+j]);
    eps += ep;
    if (ep > epmax) epmax = ep;
    //printf("%lf ", masr[i][j]);
    }
    }
   
    printf ("otklonenye:  \n");
    printf ("%f %f\n" , epmax, eps);
    printf("time posl: %lf", tp);

    printf("Результат матричного умножения:\n");
    for (int i = 0; i < 3 && i < N; i++) // выводим только первые 3 строки
  { for (int j = 0; j < 3 && j < N; j++) // выводим только первые 3 столбца
	    {
            printf("%f ", C[i * N +j]);
	    
	    }
	    printf("\n");
  }

    
    // освобождение памяти
    free(A);
    free(B);
    free(C);
    free(D);


    return 0;
    }



