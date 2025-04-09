#pragma once
#include "main.h"
#include "EEPROM.h"


extern bool Flash_saves(void*buf,uint32_t length,uint32_t address);
extern bool Flash_read(void*buf,uint32_t length,uint32_t address);