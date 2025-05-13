#pragma once

#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "stdio.h"


extern void BMCU_UART_Init();
extern void send_bmcu_uart(const unsigned char *data, size_t length);
extern void BambuBUS_UART_Init();
extern void send_bambu_uart(const unsigned char *data, size_t length);
