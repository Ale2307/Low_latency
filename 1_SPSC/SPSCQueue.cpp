#include <iostream>
#include<atomic>
#include<thread>
#include<vector>
#include<chrono>


template<typename T, std::size_t Capacity>
class SPSCQueue{
    private:
    T buffer[Capacity];
    alignas(64)
    std::atomic<size_t> head{0};
    alignas(64)
    std::atomic<size_t> tail{0};

    size_t next(size_t index)
    {
        return (index + 1 ) % Capacity;
    }

    public:
    bool push(const T& value);
    bool pop(T& value);
    bool empty() const;
    bool full() const;


};

template<typename T, std::size_t Capacity>
bool SPSCQueue<T, Capacity>::push(const T& value)
{
    std::size_t current_tail = tail.load(std::memory_order_relaxed);

    std::size_t next_tail = next(current_tail);

    if(head.load(std::memory_order_acquire) == next_tail) return false;

    buffer[current_tail] = value;

    tail.store(next_tail, std::memory_order_release);

    return true;

}

template<typename T, std::size_t Capacity>
bool SPSCQueue<T, Capacity>::pop(T& value)
{
    std::size_t current_head = head.load(std::memory_order_relaxed);

    if(current_head == tail.load(std::memory_order_acquire)) return false;

    value = buffer[current_head];

    head.store(next(current_head), std::memory_order_release);

    return true;
    
}

template<typename T, std::size_t Capacity>
bool SPSCQueue<T, Capacity>::empty() const
{
    return(head.load(std::memory_order_acquire) == tail.load(std::memory_order_acquire));
}

template<typename T, std::size_t Capacity>
bool SPSCQueue<T, Capacity>::full() const
{
    return (head.load(std::memory_order_acquire) == next(tail.load(std::memory_order_acquire)));
}

int main()
{
    const int n = 1000000;
    SPSCQueue<int, 1024>q;

    auto start = std::chrono::high_resolution_clock::now();

    std::thread producer([&q](){
        for (int i = 0; i < n; i++)
        {
            q.push(i);
            std::cout << "producer produced resource\n";
        }
    
    });

    std::thread consumer([&q](){
        int value;
        int count = 0;

        while(value < n)
        {
            q.pop(value);
            count++;
            std::cout <<"consumer consumed " << value <<"\n";
        }
    });

    auto end = std::chrono::high_resolution_clock::now();

    auto time_spent = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    std::cout <<"Time: " << time_spent <<" ns\n";

    producer.join();
    consumer.join();


    return 0;
}