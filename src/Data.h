#pragma once
#include "main.h"
#include "EEPROM.h"

extern char *C_data;
extern char *L_data;
extern bool Flash_saves(void*buf,uint16_t length,uint16_t address);
extern bool Flash_read(void*buf,uint16_t length,uint16_t address);
extern void get_C_data(uint8_t *buf_X, int data_length);
extern void INIT_DATA();
extern void Flash_commit();
extern int my_printf(const char *format, ...);
void WriteData(const char *data);