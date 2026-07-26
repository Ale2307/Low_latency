//#pragma once

#include <atomic>
#include <iostream>
#include <thread>
#include <vector>

template <typename T, size_t Capacity> class MPMC {
private:
  struct Cell 
  {
    std::atomic<size_t> sequence;
    T data;
  };

  Cell buffer[Capacity];

  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};

public:
  MPMC();

  bool push(const T &value);
  bool pop(T &value);
};

template <typename T, size_t Capacity> MPMC<T, Capacity>::MPMC() 
{
  for (int i = 0; i < Capacity; i++) {
    buffer[i].sequence.store(i, std::memory_order_relaxed);
  }
}

template <typename T, size_t Capacity>
bool MPMC<T, Capacity>::push(const T &value) 
{
  Cell *cell;
  size_t current_tail = tail.load(std::memory_order_relaxed);

  while (true) 
  {
    cell = &buffer[current_tail & (Capacity - 1)];

    static_assert((Capacity & (Capacity - 1)) == 0,
                  "Capacity must be a power of 2");

    size_t seq = cell->sequence.load(std::memory_order_acquire);

    std::intptr_t diff = static_cast<std::intptr_t>(seq) -
                         static_cast<std::intptr_t>(current_tail);

    if (diff == 0) 
    {
      if (tail.compare_exchange_weak(current_tail, current_tail + 1,
                                     std::memory_order_acq_rel,
                                     std::memory_order_relaxed))
        break;
    }
    else if(diff < 0) 
    {
        return false;  
    }
    else
    {
      current_tail = tail.load(std::memory_order_relaxed);
    }
  }

  cell->data = value;
  cell->sequence.store(current_tail + 1, std::memory_order_release);

  return true;
}

template <typename T, size_t Capacity> bool MPMC<T, Capacity>::pop(T &value) 
{
  size_t current_head = head.load(std::memory_order_relaxed);
  Cell* cell;

  while (true) 
  {
    cell = &buffer[current_head % Capacity];

    size_t seq = cell->sequence.load(std::memory_order_acquire);

    std::ptrdiff_t diff = static_cast<std::ptrdiff_t>(seq) -
                          static_cast<std::ptrdiff_t>(current_head + 1);

    if (diff == 0) 
    {
      if (head.compare_exchange_weak(current_head, current_head + 1,
                                     std::memory_order_acq_rel,
                                     std::memory_order_relaxed))

        break;
    } 
    else if (diff < 0) 
    {
      return false;
    } 
    else 
    {
      current_head = head.load(std::memory_order_relaxed);
    }
  }

  value = cell->data;

  cell->sequence.store(current_head + Capacity, std::memory_order_release);

  return true;
}

int main() 
{
  MPMC<int, 1024> q;
  {
    constexpr int values_tobe_consumed = 5;

      //SIMPLE TEST  push/pop

      for(int i = 0; i < 5; i++)
      {
          if(q.push(i))
          {
          std::cout <<"value " <<i <<" pushed\n";
          }
      }

      int value;
      int count = 0;
      while(count < values_tobe_consumed)
      {
        if(q.pop(value))
        {
          count++;
          std::cout <<"consumed: "<< value <<"\n";
        }
      }
  }

  // TEST SINGLE THREAD
// {
//   constexpr int nr_resourcesToBeCreatedandConsumed = 10;
//   std::thread producer([&q](){
//       for(int i = 0; i < nr_resourcesToBeCreatedandConsumed; i++)
//       {
//           if(q.push(i * nr_resourcesToBeCreatedandConsumed))
//           {
//           std::cout <<"resource " << i <<" created\n";
//           }
//           std::this_thread::yield();
//       }
//   });

//   std::thread consumer([&q]()
//   {
//           int val;
//           int count = 0;
//           while(count < nr_resourcesToBeCreatedandConsumed)
//           {
//             if(q.pop(val))
//             {
//               count++;
//               std::cout <<val <<"resource consumed\n";
//             }
//             std::this_thread::yield();
//           }
//   });

//   producer.join();
//   consumer.join();
// }

  // TEST MULTIPLE PRODUCERS SINGLE CONSUMER
// {
//   constexpr int nr_producers = 10;
//   constexpr int nr_resourcesPerProducerToBeCreated = 10;

//   std::vector<std::thread> producers;
//   std::atomic<int> consumed{0};

//   for(int i = 0; i < nr_producers; i++)
//   producers.emplace_back([&q, i](){
//       for(int j = 0; j < nr_resourcesPerProducerToBeCreated; j++)
//       {
//           while(!q.push(j * nr_resourcesPerProducerToBeCreated + i))
//           {
//             std::this_thread::yield();
//           }
//           std::cout <<"resource created by producer " << i <<"\n";
//       }
//   });

//   std::thread consumer([&q, &consumed]()
//   {
//           int val;
//           while(consumed.load(std::memory_order_relaxed) < nr_resourcesPerProducerToBeCreated)
//           {
//             if(q.pop(val))
//             {
//               consumed.fetch_add(1, std::memory_order_relaxed);
//               std::cout <<val <<"resource consumed\n";
//             }

//             std::this_thread::yield();
//           }
//   });

//   for(auto& t : producers) t.join();
//   consumer.join();
// }

  // TEST SINGLE PRODUCER MULTIPLE CONSUMERS
// {
//   constexpr int nr_consumers = 10;
//   constexpr int nr_resourcesPerProducerToBeCreated = 10;

//   std::vector<std::thread> consumers;
//   std::atomic<int> values_consumed{0};

//   std::thread producer([&q]()
//   {
//       for(int j = 0; j < nr_resourcesPerProducerToBeCreated; j++)
//       {
//           while(!q.push(j * nr_resourcesPerProducerToBeCreated))
//           {
//             std::this_thread::yield();
//           }
//           std::cout <<"resource " << j <<" created by producer\n";
//       }
//   });

//   for(int i = 0; i < nr_consumers; i++)
//   consumers.emplace_back([&q, &values_consumed, i]()
// {
//           int val;
//           while(values_consumed.load(std::memory_order_relaxed) < nr_resourcesPerProducerToBeCreated)
//           {
//             if(q.pop(val))
//             {
//               values_consumed.fetch_add(1, std::memory_order_relaxed);
//               std::cout << val <<"resource consumed by consumer" << i <<"\n";
//             }
//           }
//   });

//   for(auto& t : consumers) t.join();
//   producer.join();
// }

//   // TEST MULTIPLE PRODUCERS MULTIPLE CONSUMER
// {
//   constexpr int nr_producers = 10;
//   constexpr int nr_consumers = 10;
//   constexpr int nr_resourcesPerProducerToBeCreated = 10;
//   constexpr int nr_totalresources = nr_producers * nr_resourcesPerProducerToBeCreated;

//   std::atomic<int> messages_consumed{0};

//   std::vector<std::thread> producers;
//   std::vector<std::thread> consumers;

//   for (int i = 0; i < nr_producers; i++)
//     producers.emplace_back([&q, i]() {
//       for (int j = 0; j < nr_resourcesPerProducerToBeCreated; j++) {
//         if (q.push(j * nr_resourcesPerProducerToBeCreated + i)) {
//           std::cout << "resource created by producer" << i << "\n";
//         }
//       }
//     });


//   for (int i = 0; i < nr_consumers; i++)
//     consumers.emplace_back([&q, &messages_consumed]() 
//    {
//         int val;

//         while (messages_consumed.load(std::memory_order_relaxed) <
//                nr_totalresources) 
//         {
//           if ((q.pop(val))) {
//             messages_consumed.fetch_add(1, std::memory_order_relaxed);
//             std::cout << "resource consumed by consumer " << val << "\n";
//           }
//         }

//     });

//   for (auto &t : producers)
//     t.join();
//   for (auto &t : consumers)
//     t.join();
// }

  return 0;
}
