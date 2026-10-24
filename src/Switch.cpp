#include "switch.h"
#include "BambuBus.h"

const unsigned char select_bmcu_filament_name[] = "TPU-AMS"; //ID: GFU02
const unsigned char reset_bmcu_meter_color[4] = {0xFF, 0xFF, 0xFF, 0xFF}; //white
const unsigned char set_meter50_color[4] = {0xD3, 0xC5, 0xA3, 0xFF};//沙漠黄
const unsigned char reset_bmcu_channel_color[4] = {0xFF, 0xF1, 0x44, 0xFF}; //黄色
const unsigned char set_bmcu_filament_color0[4] = {0xAF, 0x79, 0x33, 0xFF}; //棕色
const unsigned char set_bmcu_filament_color1[4] = {0x89, 0x89, 0x89, 0xFF}; //岩石灰
const unsigned char set_bmcu_filament_color2[4] = {0xBC, 0xBC, 0xBC, 0xFF}; //灰色
const unsigned char set_bmcu_filament_color3[4] = {0x16, 0x16, 0x16, 0xFF}; //黑色

uint8_t Switch_set_filament(unsigned char *buf, int length, uint8_t AMS_num, uint8_t read_num)
{
    if (memcmp(select_bmcu_filament_name, buf + 23, sizeof(select_bmcu_filament_name)) == 0)
    {
        if (memcmp(buf + 15, set_meter50_color, 2) == 0)
        {
            set_filament_meters(AMS_num * 4 + read_num, 175.0f);
        }
        else if (memcmp(buf + 15, reset_bmcu_channel_color, 4) == 0)
        {
            if (BambuBus_not_on_print())
            {
                Switch_set_refresh(true);
            }
            return 0x0D;
        }
        else if (memcmp(buf + 15, reset_bmcu_meter_color, 4) == 0)
        {
            return 0xD1;
        }
        else if (memcmp(buf + 15, set_bmcu_filament_color0, 4) == 0)
        {
            return 0xD3;
        }
        else if (memcmp(buf + 15, set_bmcu_filament_color1, 4) == 0)
        {
            return 0xD5;
        }
        else if (memcmp(buf + 15, set_bmcu_filament_color2, 4) == 0)
        {
            return 0xD7;
        }
        else if (memcmp(buf + 15, set_bmcu_filament_color3, 4) == 0)
        {
            return 0xD9;
        }
        return 0xE0;
    }
    return 0;
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
