#pragma once
#include "main.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "Preferences.h"
#include <freertos/semphr.h>

// 文本缓冲区：显式长度 + 互斥锁，Web/日志/抓包共用同一套语义。
class TextBuffer
{
public:
    bool init(size_t capacity, bool psram);
    bool append(const char *data, size_t length);
    size_t length() const { return length_; }
    char *data() { return buffer_; }
    const char *data() const { return buffer_; }
    size_t copy_to(char *dst, size_t max_len) const;
    void clear();

private:
    char *buffer_ = nullptr;
    size_t capacity_ = 0;
    size_t length_ = 0;
    SemaphoreHandle_t mutex_ = nullptr;
};

extern TextBuffer C_capture;
extern TextBuffer L_log;
extern bool EN_catch;
extern bool Flash_saves(void *buf, uint16_t length, const char *address);
extern bool Flash_read(void *buf, uint16_t length, const char *address);
extern void get_C_data(uint8_t *buf_X, int data_length);
extern void INIT_DATA();
extern int my_printf(const char *format, ...);
void WriteData(const char *data);
