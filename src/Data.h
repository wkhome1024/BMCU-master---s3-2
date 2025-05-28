#pragma once
#include "main.h"
#include "EEPROM.h"

extern char *C_data;
extern char *L_data;
extern bool Flash_saves(void*buf,uint32_t length,uint32_t address);
extern bool Flash_read(void*buf,uint32_t length,uint32_t address);
extern void get_C_data(uint8_t *buf_X, int data_length);
extern void INIT_DATA();
extern int my_printf(const char *format, ...);
void WriteData(const char *data);