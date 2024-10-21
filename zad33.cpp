#include <iostream>  
#include <vector>    
#include "papi.h"    

// Класс для представления графа в формате CSR 
class CSR_graph
{
    int row_count;  // Количество вершин в графе
    unsigned int col_count;  // Количество рёбер  в графе

    std::vector<unsigned int> row_ptr;  // Вектор указателей на начало строк в графе
    std::vector<int> col_ids;           
    std::vector<double> vals;           

public:

    // Метод для чтения графа из файла
    void read(const char* filename)
    {
        // Открытие файла для чтения в бинарном режиме
        FILE *graph_file = fopen(filename, "rb");
        
        // Чтение количества вершин и рёбер
        fread(reinterpret_cast<char*>(&row_count), sizeof(int), 1, graph_file);
        fread(reinterpret_cast<char*>(&col_count), sizeof(unsigned int), 1, graph_file);

        // Вывод информации о графе
        std::cout << "Row_count = " << row_count << ", col_count = " << col_count << std::endl;

        // Изменение размера векторов для хранения данных графа
        row_ptr.resize(row_count + 1); 
        col_ids.resize(col_count);
        vals.resize(col_count);

        // Чтение данных о графе (указатели на строки, столбцы и веса рёбер)
        fread(reinterpret_cast<char*>(row_ptr.data()), sizeof(unsigned int), row_count + 1, graph_file);
        fread(reinterpret_cast<char*>(col_ids.data()), sizeof(int), col_count, graph_file);
        fread(reinterpret_cast<char*>(vals.data()), sizeof(double), col_count, graph_file);

        // Закрытие файла
        fclose(graph_file);
    }

    // Функция для вычисления вершины с максимальным весом среди чётных рёбер
    int indexweight() 
    {
        int idm = 0;  // Индекс вершины с максимальным весом
        double count, maxcount = 0;  // Текущий и максимальный вес
        
        // Цикл по вершинам графа
        for (int i = 0; i < row_ptr.size() - 1; i++)
        {
            count = 0;  

            // Цикл по рёбрам текущей вершины
            for (int lt = row_ptr[i]; lt < row_ptr[i + 1]; lt++)
            {
                // Если номер столбца (рёбра) чётный, то добавляем его вес
                if (col_ids[lt] % 2 == 0)
                {
                    count += vals[lt];
                }
            }

            // Если текущий вес больше максимального, обновляем максимальный вес и индекс вершины
            if (count > maxcount)
            {
                idm = i;
                maxcount = count;
            }
        }

        
        std::cout << "Максимальный вес вершины: " << maxcount << std::endl;
        return idm; 
    }

    // Функция для вычисления вершины с максимальным рангом
    int indexrang() 
    {
        int idm = 0;  // Индекс вершины с максимальным рангом
        double count, maxcount = 0;  // Текущий и максимальный ранг
        
        // Цикл по вершинам графа
        for (int i = 0; i < row_ptr.size() - 1; i++)
        {
            count = 0;  

            // Цикл по рёбрам текущей вершины
            for (int lt = row_ptr[i]; lt < row_ptr[i + 1]; lt++)
            {
                double w = 0;  
                
                // Цикл по рёбрам вершины-соседа
                for (int j = row_ptr[col_ids[lt]]; j < row_ptr[col_ids[lt] + 1]; j++)
                {
                    w += vals[j] * (row_ptr[col_ids[j] + 1] - row_ptr[col_ids[j]]);
                }

                // Увеличиваем счётчик ранга
                count += vals[lt] * w;
            }

            // Если текущий ранг больше максимального, обновляем максимальный ранг и индекс вершины
            if (count > maxcount)
            {
                idm = i;
                maxcount = count;
            }
        }

        
        std::cout << "Наибольший ранг: " << maxcount << std::endl;
        return idm;  // Возврат индекса вершины с максимальным рангом
    }

    
    void print_vertex(int idx) 
    {
        // Цикл по рёбрам вершины
        for (int col = row_ptr[idx]; col < row_ptr[idx + 1]; col++)
        {
            std::cout << col_ids[col] << " " << vals[col] << std::endl;  // Вывод номера столбца и веса
        }
        std::cout << std::endl;
    }

    // Метод для сброса данных графа
    void reset() 
    {
        row_count = 0;  
        col_count = 0;  
        row_ptr.clear();  
        col_ids.clear();  
        vals.clear();     
    }
};

#define N_TESTS 5  // Количество тестов

int main ()
{
    const char* filenames[N_TESTS];  // Массив с именами файлов графов
    filenames[0] = "synt";
    filenames[1] = "road_graph";
    filenames[2] = "stanford";
    filenames[3] = "youtube";
    filenames[4] = "syn_rmat";

    long long result[3];  // Массив для хранения результатов PAPI
    int Eventset = PAPI_NULL, code, retval = PAPI_NULL;  

    
    PAPI_library_init(PAPI_VER_CURRENT);

    
    PAPI_create_eventset(&Eventset);
    
    
    PAPI_add_event(Eventset, PAPI_L1_TCM);
    PAPI_add_event(Eventset, PAPI_L2_ICM);

    // Добавление пользовательского события для промахов кэша
    PAPI_event_name_to_code("perf::CACHE-MISSES", &code);
    PAPI_add_event(Eventset, code);

    // Цикл по каждому тестовому файлу
    for (int n_test = 0; n_test < N_TESTS; n_test++) 
    {
        CSR_graph a;  // Создание объекта графа
        a.read(filenames[n_test]);  
        
        // Запуск измерений PAPI для функции вычисления веса вершины
        PAPI_start(Eventset);
        std::cout << a.indexweight() << std::endl;  // Вывод индекса вершины с максимальным весом
        PAPI_stop(Eventset, result);  
        
        // Вывод результатов измерений (кэш-промахи)
        std::cout << "1alg_papi: L1_TCM: " << result[0] << " L2_ICM: " << result[1] << std::endl;
        std::cout << "my event: " << result[2] << std::endl;
        
        PAPI_reset(Eventset);  
        
        // Запуск измерений PAPI для функции вычисления ранга вершины
        PAPI_start(Eventset);
        std::cout << "index of max rang: " << a.indexrang() << std::endl;  // Вывод индекса вершины с максимальным рангом
        PAPI_stop(Eventset, result);  

        // Вывод результатов измерений (кэш-промахи)
        std::cout << "2alg_papi: L1_TCM: " << result[0] << " L2_ICM: " << result[1] << std::endl;
        std::cout << "my event: " << result[2] << std::

//


   }
   PAPI_destroy_eventset(&Eventset);
   PAPI_shutdown();
   return 0;
   }

