#pragma once

#include "main.h"
#ifdef __cplusplus
extern "C"
{
#endif
    extern void Switch_init();
    extern bool Switch_read();
    extern uint8_t get_ams_map_to(uint8_t num);
    extern uint8_t Switch_set_filament(unsigned char *buf, int length, uint8_t AMS_num, uint8_t read_num);
    extern void Switch_set_need_to_save();
    extern bool Switch_need_to_save();
    extern void Switch_save();
    extern void Switch_set_refresh(bool refresh);
    extern bool Switch_need_refresh(); 
#ifdef __cplusplus
}
#endif