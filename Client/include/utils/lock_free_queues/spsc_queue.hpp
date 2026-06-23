#pragma once

#include <atomic>
#include <cassert>
#include <memory>
#include <new>
#include <iostream>
#include <vector>

template<typename T>
class SPSCQueue {
public:
    explicit SPSCQueue(uint64_t capacity) :  
       m_capacity{capacity}, m_mask{capacity - 1}, m_buffer(capacity)
    {
        assert((capacity & (capacity - 1)) == 0);
        m_head.store(0, std::memory_order_relaxed);
        m_tail.store(0, std::memory_order_relaxed);
    };

    ~SPSCQueue() = default;
    
    bool push(const T& value) {
        uint64_t head = m_head.load(std::memory_order_relaxed);
        uint64_t tail = m_tail.load(std::memory_order_acquire);

        if (head + 1 - tail > m_capacity) {
            return false;
        }

        m_buffer[head & m_mask] = value;
        m_head.store(head+ 1, std::memory_order_release);
        return true;
    }

    bool pop(T& value) {
        uint64_t tail = m_tail.load(std::memory_order_relaxed);
        uint64_t head = m_head.load(std::memory_order_acquire);

        if (tail == head) {
            return false;
        }

        value = m_buffer[tail & m_mask];
        m_tail.store(tail + 1, std::memory_order_release);
        return true;
    }
private:
private:
    const uint64_t m_capacity;
    const uint64_t m_mask;

    std::vector<T> m_buffer{};

    std::atomic<uint64_t> m_head = 0;
    std::atomic<uint64_t> m_tail = 0;
};