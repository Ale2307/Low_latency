#include <iostream>
#include<atomic>
#include<thread>
#include <vector>

template <typename T, size_t Capacity>
class SPMC
{
    private:
    alignas(64)
    std::atomic<size_t> head {0};
    alignas(64)
    std::atomic<size_t> tail {0};

    T buffer[Capacity];

    static constexpr size_t next(size_t index)
    {
        return (index + 1) % Capacity;
    }

    public:
    SPMC() = default;
    bool push(const T& value);
    bool pop( T& value);
    bool empty() const;
    bool full() const;

    
};

template <typename T, size_t Capacity>
bool SPMC<T, Capacity>::push(const T& value)
{
    size_t current_tail = tail.load(std::memory_order_relaxed);

    size_t next_tail = next(current_tail);

    if(next(current_tail) == head.load(std::memory_order_acquire))
    return false;

    buffer[current_tail] = value;

    tail.store(next_tail, std::memory_order_release);

    return true;
}

template <typename T, size_t Capacity>
bool SPMC<T, Capacity>::pop(T& value)
{
    while (true)
    {
        size_t current_head = head.load(std::memory_order_relaxed);
        size_t next_head = next(current_head);

        if(current_head == tail.load(std::memory_order_relaxed)) return false;

        if(head.compare_exchange_weak(current_head, next_head, std::memory_order_acq_rel, std::memory_order_relaxed))
        {
            value = buffer[current_head];

            return true;
        }

    }
}

template <typename T, size_t Capacity>
bool SPMC<T, Capacity>::empty() const
{
    return tail.load(std::memory_order_relaxed) == head.load(std::memory_order_relaxed);
}


template <typename T, size_t Capacity>
bool SPMC<T, Capacity>::full() const
{
    size_t current_tail = tail.load(std::memory_order_relaxed);
    return next(current_tail) == head.load(std::memory_order_relaxed);
}

int main()
{
    constexpr int nr_consumers {4};
    constexpr int nr_messagesperproducer{100};

    std::atomic<int> messages_consumed{0};

    SPMC<int, 1024> q;

    std::thread producer([&q](){
        for(int i = 0; i < nr_messagesperproducer; i++)
        {
        if(!q.push(i))
        {
        std::this_thread::yield();
        }
        std::cout << "producer produced resource " << i <<"\n";
    }
    });

    std::vector<std::thread> consumers;
    for(int i = 0;  i< nr_consumers; i++)
    consumers.emplace_back([&q, &messages_consumed, i]()
{
    int value;

    while(messages_consumed.load(std::memory_order_relaxed) < nr_messagesperproducer)
    {
        if(q.pop(value))
        {
            messages_consumed.fetch_add(1, std::memory_order_relaxed);
            std::cout << "consumer finished\n";
        }
        std::this_thread::yield();
    }
});

for(auto& t : consumers)  t.join();
producer.join();


    return 0;
}