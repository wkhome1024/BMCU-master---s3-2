#pragma once


extern void BMCU_UART_Init();
extern void send_bmcu_uart(const unsigned char *data, size_t length);
extern void BambuBUS_UART_Init();
extern void send_bambu_uart(const unsigned char *data, size_t length);
extern void LED_Init();