#pragma once
#include <main.h>
#include <stdint.h>

class MyRingBuffer {
private:
    uint8_t* buffer = nullptr;
    size_t capacity;
    size_t head = 0;
    size_t tail = 0;

public:
    explicit MyRingBuffer(size_t size) {
        buffer = (uint8_t*)malloc(size);
        capacity = size;
    }

    ~MyRingBuffer() {
        if (buffer) free(buffer);
    }

    bool isEmpty() const {
        return head == tail;
    }

    bool isFull() const {
        return (head + 1) % capacity == tail;
    }

    void write(uint8_t c) {
        buffer[head] = c;
        head = (head + 1) % capacity;
        if (head == tail) {  // 覆盖尾指针
            tail = (tail + 1) % capacity;
        }
    }

    int read() {
        if (isEmpty()) return -1;
        uint8_t c = buffer[tail];
        tail = (tail + 1) % capacity;
        return c;
    }

    size_t available() const {
        return (capacity + head - tail) % capacity;
    }
};