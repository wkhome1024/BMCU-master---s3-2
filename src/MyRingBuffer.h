#pragma once
#include <main.h>
#include <stdint.h>

// 固定容量、2 的幂的环形缓冲区，单生产者/单消费者。
template <size_t N>
class MyRingBuffer
{
    static_assert(N > 1 && (N & (N - 1)) == 0, "ring size must be a power of two");

public:
    bool write(uint8_t c)
    {
        uint32_t head = head_;
        uint32_t next = (head + 1) & (N - 1);
        if (next == tail_)
        {
            ++dropped_;
            return false;
        }
        buffer_[head] = c;
        head_ = next;
        return true;
    }

    int read()
    {
        if (head_ == tail_)
            return -1;
        uint8_t c = buffer_[tail_];
        tail_ = (tail_ + 1) & (N - 1);
        return c;
    }

    size_t available() const
    {
        return (N + head_ - tail_) & (N - 1);
    }

    bool isEmpty() const { return head_ == tail_; }

    uint32_t dropped() const { return dropped_; }

private:
    uint8_t buffer_[N] = {0};
    volatile uint32_t head_ = 0;
    volatile uint32_t tail_ = 0;
    volatile uint32_t dropped_ = 0;
};
