#pragma once

#include "main.h"
#ifdef __cplusplus
extern "C"
{
#endif
    extern uint8_t Switch_set_filament(unsigned char *buf, int length, uint8_t AMS_num, uint8_t read_num);
    extern void Switch_set_refresh(bool refresh);
    extern bool Switch_need_refresh();
#ifdef __cplusplus
}
#endif