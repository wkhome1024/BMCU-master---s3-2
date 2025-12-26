#include "switch.h"
#include "BambuBus.h"

#define BMCUSwitch_version 6
const char *switch_addr = "switch";
struct alignas(4) switch_save_struct
{
    uint32_t version = BMCUSwitch_version;
    uint8_t F_AMS_num = 0;
    unsigned char ams_map[4];
} switch_save;

const unsigned char select_bmcu_filament_name[] = "TPU-AMS"; //ID: GFU02
const unsigned char reset_bmcu_meter_color[4] = {0xFF, 0xFF, 0xFF, 0xFF}; //white
const unsigned char set_meter50_color[4] = {0xD3, 0xC5, 0xA3, 0xFF};//沙漠黄
//const unsigned char haset_bmcu_channel_color[4] = {0x40, 0x61, 0x00, 0xFF};
//const unsigned char hacheck_bmcu_channel_color[16] = {0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F};
const unsigned char reset_bmcu_channel_color[4] = {0xFF, 0xF1, 0x44, 0xFF}; //黄色
const unsigned char set_bmcu_filament_color0[4] = {0xAF, 0x79, 0x33, 0xFF}; //棕色
const unsigned char set_bmcu_filament_color1[4] = {0x89, 0x89, 0x89, 0xFF}; //岩石灰
const unsigned char set_bmcu_filament_color2[4] = {0xBC, 0xBC, 0xBC, 0xFF}; //灰色
const unsigned char set_bmcu_filament_color3[4] = {0x16, 0x16, 0x16, 0xFF}; //黑色
void Switch_init()
{
    //bool _init_ready = Switch_read();
    if (F_AMS_num == 0)
    {
        switch_save.F_AMS_num = 0;
        switch_save.ams_map[0] = 0;
        switch_save.ams_map[1] = 1;
        switch_save.ams_map[2] = 2;
        switch_save.ams_map[3] = 3;
        //Switch_save();
    }
    else 
    {
        switch_save.F_AMS_num = F_AMS_num;
        for (int i = 0; i < 4; i++)
        {
            if (i < F_AMS_num)
                switch_save.ams_map[i] = 0x00; // 官方AMS 不映射
            else 
                switch_save.ams_map[i] = i - F_AMS_num;
        }
    }
}
bool Switch_read()
{
    switch_save_struct ptr;
    if (!Flash_read(&ptr,sizeof(switch_save),switch_addr)) return false;  
    if (ptr.version == BMCUSwitch_version)
    {
        memcpy(&switch_save, &ptr, sizeof(switch_save));
        return true;
    }
    return false;
}

uint8_t get_ams_map_to(uint8_t num)
{
    return switch_save.ams_map[num];
}

uint8_t Switch_set_filament(unsigned char *buf, int length, uint8_t AMS_num, uint8_t read_num)
{
    if (memcmp(select_bmcu_filament_name, buf + 23, sizeof(select_bmcu_filament_name)) == 0)
    { 

        if(memcmp(buf + 15, set_meter50_color, 2) == 0)
        {
            set_filament_meters(AMS_num * 4 + read_num, 175.0f);  
        }
        else if(memcmp(buf + 15, reset_bmcu_channel_color, 4) == 0)
        {
            //switch_save.filament_map_to[0] = read_num * 4;
            //switch_save.filament_map_to[1] = read_num * 4 + 1;
            //switch_save.filament_map_to[2] = read_num * 4 + 2;
            //switch_save.filament_map_to[3] = read_num * 4 + 3;
            //my_printf("(switch)reset_map_to bmcu-%d" ,read_num);
            if (BambuBus_not_on_print())
            {
                Switch_set_refresh(true);                
            }
            //Switch_set_need_to_save();
            return 0x0D;
        }
        else if(memcmp(buf + 15, reset_bmcu_meter_color, 4) == 0)
        {
            return 0xD1;
        }
        else if(memcmp(buf + 15, set_bmcu_filament_color0, 4) == 0)
        {
            return 0xD3;
        }
        else if(memcmp(buf + 15, set_bmcu_filament_color1, 4) == 0)
        {
            return 0xD5;
        }
        else if(memcmp(buf + 15, set_bmcu_filament_color2, 4) == 0)
        {
            return 0xD7;
        }
        else if(memcmp(buf + 15, set_bmcu_filament_color3, 4) == 0)
        {
            return 0xD9;
        }

        return 0xE0;

    }

    return 0;
}
bool switch_need_to_save = false;
void Switch_set_need_to_save()
{
    switch_need_to_save = true;
}
void Switch_save()
{
    //Flash_saves(&switch_save, sizeof(switch_save), use_flash_addr + sizeof(switch_save));
    if(!Flash_saves(&switch_save, sizeof(switch_save), switch_addr))  
        my_printf("(FLASH) switch保存失败");
    switch_need_to_save = false;
}
bool Switch_need_to_save()
{
    return switch_need_to_save;
}

bool switch_need_to_refresh = false;
void Switch_set_refresh(bool refresh)
{
    switch_need_to_refresh = refresh;
}
bool Switch_need_refresh()
{
    return switch_need_to_refresh;
}


