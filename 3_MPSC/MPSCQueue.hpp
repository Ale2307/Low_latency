#pragma once

#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
#include <vector>

template <typename T, std::size_t Capacity> 
class MPSCQueue 
{
private:
  T buffer[Capacity];
  alignas(64) std::atomic<size_t> head{0};
  alignas(64) std::atomic<size_t> tail{0};

  static constexpr std::size_t next(size_t index) {
    return (index + 1) % Capacity;
  }

public:
  MPSCQueue() = default;
  bool push(const T& value);
  bool pop(T& value);
  bool empty() const;
  bool full() const;
};

template <typename T, std::size_t Capacity>
bool MPSCQueue<T, Capacity>::push(const T &value) 
{
  while(true) 
  {

    std::size_t current_tail = tail.load(std::memory_order_relaxed);

    std::size_t next_tail = next(current_tail);

    if (head.load(std::memory_order_relaxed) == next_tail)
      return false;

    if (tail.compare_exchange_weak(current_tail, next_tail,
                                   std::memory_order_acq_rel,
                                   std::memory_order_relaxed)) 
    {
      buffer[current_tail] = value;
      tail.store(next_tail, std::memory_order_release);
      return true;
    }
  }
}

template <typename T, std::size_t Capacity>
bool MPSCQueue<T, Capacity>::pop(T &value) 
{
  size_t current_head = head.load(std::memory_order_relaxed);

  size_t next_head = next(current_head);

  if(current_head == tail.load(std::memory_order_acquire))
    return false;

  value = buffer[current_head];

  head.store(next_head, std::memory_order_release);

  return true;
}

template <typename T, std::size_t Capacity>
bool MPSCQueue<T, Capacity>::empty() const {
  return (head.load(std::memory_order_relaxed) ==
          tail.load(std::memory_order_relaxed));
}

template <typename T, std::size_t Capacity>
bool MPSCQueue<T, Capacity>::full() const {
  return (head.load(std::memory_order_relaxed) ==
          next(tail.load(std::memory_order_relaxed)));
}