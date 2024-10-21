#include <iostream>
#include <pthread.h>
#include <queue>

#define QUEUE_CAPACITY 30

pthread_mutex_t queueMutex = PTHREAD_MUTEX_INITIALIZER; // Мьютекс для защиты очереди
pthread_cond_t canPut, canGet; // Условные переменные для управления потоками

class CustomQueue
      {
      private:
      std::queue<int> internalQueue; // Внутренняя очередь
      int capacity; // Максимальная ёмкость очереди

      public:
      CustomQueue(int size) 
             {
             capacity = size; // Инициализация максимальной ёмкости очереди
             }

      // Метод для добавления элемента в очередь
      void put(int value)
             {
             pthread_mutex_lock(&queueMutex); // Блокировка мьютекса для доступа к очереди

             // Если очередь заполнена, то будет ждать
             while (internalQueue.size() == capacity)
	            {
                    pthread_cond_wait(&canPut, &queueMutex); // Ожидает пока место освободится
                    }

            internalQueue.push(value); // Добавляем элемент в очередь
            std::cout << "Добавлено: " << value << std::endl; 
            pthread_cond_signal(&canGet); // Уведомляем, что есть элемент для извлечения
            pthread_mutex_unlock(&queueMutex); // Освобождаем мьютекс
            }

// Метод для извлечения элемента из очереди
int get() 
     {
     pthread_mutex_lock(&queueMutex); // Блокировка мьютекса для доступа к очереди

     // Если очередь пуста будет ждать
     while (internalQueue.empty())
           {
           pthread_cond_wait(&canGet, &queueMutex); // Ожидание появления элемента
           }

     int frontValue = internalQueue.front(); // Получаем первый элемент
     internalQueue.pop(); // Удаляем элемент из очереди
     std::cout << "Извлечено: " << frontValue << std::endl; 
     pthread_cond_signal(&canPut); // Уведомляем, что есть место для добавления нового элемента
     pthread_mutex_unlock(&queueMutex); // Освобождаем мьютекс
     return frontValue; // Возвращаем извлечённое значение
     }
};

// Глобальный объект очереди
CustomQueue myQueue(10); 

// Функция для потока-потребителя
void* consumerFunction(void* arg)
      {
      myQueue.get(); // Извлекаем элемент из очереди
      return nullptr; 
      }

// Функция для потока-производителя
void* producerFunction(void* arg)
      {
      int* val = (int*)arg; // Преобразование аргумента
      myQueue.put(*val); // Добавляем элемент в очередь
      return nullptr; 
      }

int main()
     {
     pthread_t threads[QUEUE_CAPACITY]; // Массив потоков

     pthread_cond_init(&canPut, NULL); // Инициализация условной переменной для добавления
     pthread_cond_init(&canGet, NULL); // Инициализация условной переменной для извлечения

     int values[QUEUE_CAPACITY / 2]; // Массив значений для добавления

     // Создаем потоки-потребители
     for (int i = 0; i < QUEUE_CAPACITY / 2; i++) 
            {
            pthread_create(&threads[i], NULL, consumerFunction, NULL);
            }

     // Создаем потоки-производители
     for (int i = QUEUE_CAPACITY / 2; i < QUEUE_CAPACITY; i++)
            {
            values[i - QUEUE_CAPACITY / 2] = i; // Заполняем массив значениями
            pthread_create(&threads[i], NULL, producerFunction, &values[i - QUEUE_CAPACITY / 2]);
            }

      // Ожидаем завершения потоков
      for (int i = 0; i < QUEUE_CAPACITY; i++)
            {
            pthread_join(threads[i], NULL);
            }

      // Освобождаем ресурсы
      pthread_cond_destroy(&canGet);
      pthread_cond_destroy(&canPut);
      pthread_mutex_destroy(&queueMutex);

return 0; 
}

