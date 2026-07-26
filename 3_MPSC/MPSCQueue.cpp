#include "MPSCQueue.hpp"

#include <immintrin.h>
#include <thread>
#include <vector>

int main() {
  constexpr int nr_producers = 4;
  constexpr int messages = 100000;

  MPSCQueue<int, 1024> q;

  std::vector<std::thread> producers;

  for (int id = 0; id < nr_producers; id++)
    producers.emplace_back([&q, id]() {
      for (int j = 0; j < messages; j++) {
        while (!q.push(j * messages + id)) 
        {
          _mm_pause();
        }
        std::cout << "producer " << id << " finished\n";
       
      }
    });

    std::thread consumer([&q]()
    {
        int value;
        long long count = 0;
        

        while(count < nr_producers * messages)
        {
            if(q.pop(value))
            {
                count++;
                std::cout << "consumed " << count <<"\n";
                
            }
        } 
    });

    for(auto& t : producers)  t.join();
    consumer.join();

  return 0;
}
