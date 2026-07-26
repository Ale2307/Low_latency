//#pragma once

#include<atomic>
#include<iostream>

template<typename T, size_t Capacity>
class RingBuffer{
    private:
    T buffer[Capacity];
    size_t head{0};
    size_t tail{0};

    public:
    bool push(const T& value);
    bool pop(T& value);
    bool empty() const;
    bool full() const;

    size_t ringsize() const;
    size_t next(size_t idx) const;
};

template<typename T, size_t Capacity>
size_t RingBuffer<T, Capacity>:: next(size_t idx) const
{
    return (idx+1) % Capacity;
}

template<typename T, size_t Capacity>
bool RingBuffer<T, Capacity>::empty() const
{
    return head == tail;
}

template<typename T, size_t Capacity>
bool RingBuffer<T, Capacity>::full() const{
    return next(tail) == head;
}

template<typename T, size_t Capacity>
bool RingBuffer<T, Capacity>::push(const T& value){

    if(full()) return false;

    buffer[tail] = value;
    tail = next(tail);
    return true;
}

template<typename T, size_t Capacity>
bool RingBuffer<T, Capacity>::pop(T& value){

    if(empty()) return false;

    value = buffer[head];
    head = next(head);
    return true;
}

template<typename T, size_t Capacity>
size_t RingBuffer<T, Capacity>::ringsize() const
{
    if(tail >= head) return tail-head;

    return Capacity -head + tail;
}

int main()
{
    RingBuffer<int, 8> ring;

    for(int i = 0; i < 7; i++)
    {
        if(ring.push(i))
        {
            std::cout <<"pushed " << i <<"\n";
        }
    }

    int value;
    for(int i = 0; i < 7; i++)
    {
        if(ring.pop(value))
        {
            std::cout << "pop out " << i <<"\n";
        }
    }

    return 0;
}

