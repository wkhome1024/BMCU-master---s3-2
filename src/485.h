#pragma once
#include <main.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <HardwareSerial.h>
#include "MyRingBuffer.h"

extern MyRingBuffer rxBuffer0;
extern MyRingBuffer rxBuffer1;

extern void send_bmcu_uart(const unsigned char *data, size_t length);
extern void send_bambu_uart(const unsigned char *data, size_t length);
extern void RS485_init();