#pragma once
#include <main.h>

#include <freertos/FreeRTOS.h>
#include <HardwareSerial.h>
#include "MyRingBuffer.h"

// 接收缓冲区在 485.cpp 内部定义

extern void send_bmcu_uart(const unsigned char *data, size_t length);
extern void send_bambu_uart(const unsigned char *data, size_t length);
extern void RS485_init();