#include "BambuBus.h"
#include "CRC16.h"
#include "CRC8.h"

CRC16 crc_16;
CRC8 crc_8;

package_type bambu_stu = BambuBus_package_NONE;
uint8_t BambuBus_data_buf[512];
int BambuBus_have_data = 0;
BambuBus_device_type BambuBus_address = BambuBus_none;
uint8_t AMS_num_c = 0;
uint8_t Tay_num_c = 0;
uint8_t AMS_num_max = 2;
bool bambus_onflush = false;
bool bambus_error = false;
uint8_t motor_unready = 0;
uint64_t pullback_time = 30000; // 30s
_filament_motion_state_set motion_temp[4][4];
uint8_t statu_temp[4][4];
uint8_t slave_pull_statu[4][4] = {0};
struct _filament
{
    // AMS statu
    char ID[8] = "GFG00";
    uint8_t color_R = 0xFF;
    uint8_t color_G = 0xFF;
    uint8_t color_B = 0xFF;
    uint8_t color_A = 0xFF;
    int16_t temperature_min = 220;
    int16_t temperature_max = 240;
    char name[20] = "PETG";

    float meters = 0;
    // uint64_t meters_virtual_count = 0;
    _filament_status statu = online;
    // printer_set
    _filament_motion_state_set motion_set = idle;
    uint16_t pressure = 0xFFFF;
};

const char *bmcu_addr = "bmcu";
enum class p2s_runtime_state : uint8_t
{
    boot = 0,
    runtime = 1,
    printing = 2
};

static p2s_runtime_state p2s_state = p2s_runtime_state::boot;
static uint8_t p2s_a0_seq_pos = 0;
static uint8_t p2s_0237_seq_pos[4] = {0};
static uint8_t p2s_023c_seq_pos[4] = {0};
struct alignas(4) flash_save_struct
{
    _filament filament[8][4];
    uint8_t BambuBus_now_filament_num = 0;
    uint32_t version = Bambubus_version;
    uint32_t check = 0x40614061;
} data_save;

bool Bambubus_read()
{
    flash_save_struct ptr;
    if (!Flash_read(&ptr, sizeof(data_save), bmcu_addr))
        return false;
    if ((ptr.check == 0x40614061) && (ptr.version == Bambubus_version))
    {
        memcpy(&data_save, &ptr, sizeof(data_save));
        return true;
    }
    return false;
}
bool Bambubus_need_to_save = false;
bool bambubus_save_flag = false;
void Bambubus_set_need_to_save()
{
    Bambubus_need_to_save = true;
    save_count++;
}
void Bambubus_save()
{
    if (!Flash_saves(&data_save, sizeof(data_save), bmcu_addr))
        my_printf("(FLASH) Bambubus保存失败");
    bambubus_save_flag = false;
    Bambubus_need_to_save = false;
}

uint8_t get_now_filament_num()
{
    return data_save.BambuBus_now_filament_num;
}

void reset_filament_meters(uint8_t AMS_num, uint8_t read_num)
{
    data_save.filament[AMS_num][read_num].meters = 0;
}
void add_filament_meters(int num, float meters)
{
    if (num < 32)
    {
        int AMS = num / 4, filament = num % 4;
        if (data_save.filament[AMS][filament].motion_set != idle)
            data_save.filament[AMS][filament].meters += meters;
    }
}
void set_filament_meters(int num, float meters)
{
    if (num < 32)
        data_save.filament[num / 4][num % 4].meters = meters;
}
void set_filament_online(int num, bool if_online)
{
    if (num < 32)
        if (if_online)
        {
            data_save.filament[num / 4][num % 4].statu = online;
        }
        else
        {
            data_save.filament[num / 4][num % 4].statu = offline;
        }
}

bool get_filament_online(int num)
{
    if (num < 32)
        if (data_save.filament[num / 4][num % 4].statu == offline)
        {
            return false;
        }
        else
        {
            return true;
        }
    return true;
}
void set_filament_motion(int num, _filament_motion_state_set motion)
{
    if (num < 32)
        data_save.filament[num / 4][num % 4].motion_set = motion;
}
_filament_motion_state_set get_filament_motion(int num)
{
    if (num < 32)
        return data_save.filament[num / 4][num % 4].motion_set;
    else
        return idle;
}
bool BambuBus_not_on_print()
{
    return (!bambus_onflush && data_save.filament[data_save.BambuBus_now_filament_num / 4][data_save.BambuBus_now_filament_num % 4].motion_set == idle);
}
uint8_t buf_X[512];
CRC8 _RX_IRQ_crcx(0x39, 0x66, 0x00, false, false);
void RX_IRQ(unsigned char _RX_IRQ_data)
{
    static int _index = 0;
    static int length = 200;
    static uint8_t data_length_index;
    static uint8_t data_CRC8_index;
    unsigned char data = _RX_IRQ_data;

    if (_index == 0)
    {
        if (data == 0x3D)
        {
            BambuBus_data_buf[0] = 0x3D;
            _RX_IRQ_crcx.restart();
            _RX_IRQ_crcx.add(0x3D);
            data_length_index = 4;
            length = data_CRC8_index = 6;
            _index = 1;
        }
        return;
    }
    else
    {
        BambuBus_data_buf[_index] = data;
        if (_index == 1)
        {
            if (data & 0x80)
            {
                data_length_index = 2;
                data_CRC8_index = 3;
            }
            else if (data == 0)
            {
                _index = 0;
                return;
            }
            else
            {
                data_length_index = 4;
                data_CRC8_index = 6;
            }
        }
        if (_index == data_length_index)
        {
            length = data;
        }
        if (_index < data_CRC8_index)
        {
            _RX_IRQ_crcx.add(data);
        }
        else if (_index == data_CRC8_index)
        {
            if (data != _RX_IRQ_crcx.calc())
            {
                _index = 0;
                return;
            }
        }
        ++_index;
        if (_index >= length)
        {
            _index = 0;
            memcpy(buf_X, BambuBus_data_buf, length);
            BambuBus_have_data = length;
        }
        if (_index >= 199)
        {
            _index = 0;
        }
    }
}

#include <stdio.h>

void Bambu_readuart()
{
    while (Serial0.available() > 0)
    {
        char inChar = (char)Serial0.read(); // 读取串口0数据
        RX_IRQ(inChar);
    }
}

uint8_t buf_Bmcu[100];
uint8_t buf_B[100];
uint8_t Bmcu_have_data = 0;
CRC8 _RX_BMCU_crcx(0x39, 0x66, 0x00, false, false);
void RX_BMCU(unsigned char inChar)
{
    static int _index1 = 0;
    static int length = 0;

    if (_index1 == 0)
    {
        if (inChar == 0x7D)
        {
            _RX_BMCU_crcx.restart();
            _RX_BMCU_crcx.add(0x7D);
            buf_B[0] = 0x7D;
            _index1 = 1;
        }
        return;
    }
    else
    {
        buf_B[_index1] = inChar;
        if (_index1 == 1)
        {
            length = inChar;
        }
        if (_index1 < length - 1)
        {

            _RX_BMCU_crcx.add(inChar);
        }
        else if (_index1 == length - 1)
        {
            if (inChar != _RX_BMCU_crcx.calc())
            {
                _index1 = 0;
                return;
            }
        }
        ++_index1;
        if (_index1 >= length)
        {
            _index1 = 0;
            memcpy(buf_Bmcu, buf_B, length);
            Bmcu_have_data = length;
        }
        if (_index1 >= 49)
        {
            _index1 = 0;
        }
    }
}

void Bmcu_readuart()
{
    while (Serial1.available() > 0)
    {
        char inChar = (char)Serial1.read(); // 读取串口1数据
        RX_BMCU(inChar);
    }
}

uint8_t online_detect_num[5][16] = {
    {0x0E, 0x7D, 0x32, 0x31, 0x31, 0x38, 0x15, 0x00, 0x36, 0x39, 0x37, 0x33, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x9B, 0x31, 0x33, 0x34, 0x36, 0x35, 0x02, 0x00, 0x37, 0x39, 0x33, 0x38, 0xFF, 0xFF, 0xFF, 0xFF},
    {0x2E, 0xC2, 0x35, 0x31, 0x38, 0x37, 0x18, 0x00, 0x36, 0x36, 0x38, 0x30, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xC9, 0xD2, 0x36, 0x38, 0x34, 0x36, 0x17, 0x00, 0x53, 0x33, 0x32, 0x33, 0xFF, 0xFF, 0xFF, 0xFF},
    {0xBF, 0xCF, 0x30, 0x35, 0x39, 0x37, 0x18, 0x00, 0x4A, 0x36, 0x30, 0x35, 0xFF, 0xFF, 0xFF, 0xFF}};
void BambuBus_init()
{
    bool _init_ready = Bambubus_read();
    crc_8.reset(0x39, 0x66, 0, false, false);
    crc_16.reset(0x1021, 0x913D, 0, false, false);

    online_detect_num[0][0] += (hub_num - 1);
    online_detect_num[1][0] += (hub_num - 1);
    online_detect_num[2][0] += (hub_num - 1);
    online_detect_num[3][0] += (hub_num - 1);

    if (!_init_ready)
    {
        data_save.filament[0][0].color_R = 0xFF;
        data_save.filament[0][0].color_G = 0x00;
        data_save.filament[0][0].color_B = 0x00;
        data_save.filament[0][1].color_R = 0xFF;
        data_save.filament[0][1].color_G = 0xFF;
        data_save.filament[0][1].color_B = 0xFF;
        data_save.filament[0][2].color_R = 0xF9;
        data_save.filament[0][2].color_G = 0x8C;
        data_save.filament[0][2].color_B = 0x36;
        data_save.filament[0][3].color_R = 0x16;
        data_save.filament[0][3].color_G = 0x16;
        data_save.filament[0][3].color_B = 0x16;

        data_save.filament[1][0].color_R = 0x89;
        data_save.filament[1][0].color_G = 0x89;
        data_save.filament[1][0].color_B = 0x89;
        data_save.filament[1][1].color_R = 0x05;
        data_save.filament[1][1].color_G = 0x77;
        data_save.filament[1][1].color_B = 0x48;
        data_save.filament[1][2].color_R = 0x0A;
        data_save.filament[1][2].color_G = 0xCC;
        data_save.filament[1][2].color_B = 0x38;
        data_save.filament[1][3].color_R = 0xA0;
        data_save.filament[1][3].color_G = 0x3C;
        data_save.filament[1][3].color_B = 0xF7;

        data_save.filament[2][0].color_R = 0x79;
        data_save.filament[2][0].color_G = 0xD9;
        data_save.filament[2][0].color_B = 0xF4;
        data_save.filament[2][1].color_R = 0xF9;
        data_save.filament[2][1].color_G = 0x5D;
        data_save.filament[2][1].color_B = 0x73;
        data_save.filament[2][2].color_R = 0x00;
        data_save.filament[2][2].color_G = 0x00;
        data_save.filament[2][2].color_B = 0xFF;
        data_save.filament[2][3].color_R = 0xD3;
        data_save.filament[2][3].color_G = 0xC5;
        data_save.filament[2][3].color_B = 0xA2;

        data_save.filament[3][0].color_R = 0xF9;
        data_save.filament[3][0].color_G = 0xA8;
        data_save.filament[3][0].color_B = 0x46;
        data_save.filament[3][1].color_R = 0x0E;
        data_save.filament[3][1].color_G = 0xE2;
        data_save.filament[3][1].color_B = 0xA0;
        data_save.filament[3][2].color_R = 0xFF;
        data_save.filament[3][2].color_G = 0xF1;
        data_save.filament[3][2].color_B = 0x44;
        data_save.filament[3][3].color_R = 0xE0;
        data_save.filament[3][3].color_G = 0xE0;
        data_save.filament[3][3].color_B = 0xE0;

        data_save.filament[4][0].color_R = 0xFF;
        data_save.filament[4][0].color_G = 0x00;
        data_save.filament[4][0].color_B = 0x00;
        data_save.filament[4][1].color_R = 0x00;
        data_save.filament[4][1].color_G = 0xFF;
        data_save.filament[4][1].color_B = 0x00;
        data_save.filament[4][2].color_R = 0x00;
        data_save.filament[4][2].color_G = 0x00;
        data_save.filament[4][2].color_B = 0xFF;
        data_save.filament[4][3].color_R = 0x88;
        data_save.filament[4][3].color_G = 0x88;
        data_save.filament[4][3].color_B = 0x88;

        data_save.filament[5][0].color_R = 0xC0;
        data_save.filament[5][0].color_G = 0x20;
        data_save.filament[5][0].color_B = 0x20;
        data_save.filament[5][1].color_R = 0x20;
        data_save.filament[5][1].color_G = 0xC0;
        data_save.filament[5][1].color_B = 0x20;
        data_save.filament[5][2].color_R = 0x20;
        data_save.filament[5][2].color_G = 0x20;
        data_save.filament[5][2].color_B = 0xC0;
        data_save.filament[5][3].color_R = 0x60;
        data_save.filament[5][3].color_G = 0x60;
        data_save.filament[5][3].color_B = 0x60;

        data_save.filament[6][0].color_R = 0x80;
        data_save.filament[6][0].color_G = 0x40;
        data_save.filament[6][0].color_B = 0x40;
        data_save.filament[6][1].color_R = 0x40;
        data_save.filament[6][1].color_G = 0x80;
        data_save.filament[6][1].color_B = 0x40;
        data_save.filament[6][2].color_R = 0x40;
        data_save.filament[6][2].color_G = 0x40;
        data_save.filament[6][2].color_B = 0x80;
        data_save.filament[6][3].color_R = 0x40;
        data_save.filament[6][3].color_G = 0x40;
        data_save.filament[6][3].color_B = 0x40;

        data_save.filament[7][0].color_R = 0x40;
        data_save.filament[7][0].color_G = 0x20;
        data_save.filament[7][0].color_B = 0x20;
        data_save.filament[7][1].color_R = 0x20;
        data_save.filament[7][1].color_G = 0x40;
        data_save.filament[7][1].color_B = 0x20;
        data_save.filament[7][2].color_R = 0x20;
        data_save.filament[7][2].color_G = 0x20;
        data_save.filament[7][2].color_B = 0x40;
        data_save.filament[7][3].color_R = 0x20;
        data_save.filament[7][3].color_G = 0x20;
        data_save.filament[7][3].color_B = 0x20;

        // Bambubus_save();
    }
    for (auto &i : data_save.filament)
    {
        for (auto &j : i)
        {
#ifdef _Bambubus_DEBUG_mode_
            //   j.statu = online;
#else
            //   j.statu = offline;
#endif // DEBUG

            j.motion_set = idle;
            //   j.meters = 0;
        }
    }
}

bool package_check_crc16(uint8_t *data, int data_length)
{
    crc_16.restart();
    data_length -= 2;
    for (auto i = 0; i < data_length; i++)
    {
        crc_16.add(data[i]);
    }
    uint16_t num = crc_16.calc();
    if ((data[(data_length)] == (num & 0xFF)) && (data[(data_length + 1)] == ((num >> 8) & 0xFF)))
        return true;
    return false;
}
bool need_debug = false;
void package_send_with_crc(uint8_t *data, int data_length)
{

    crc_8.restart();
    if (data[1] & 0x80)
    {
        for (auto i = 0; i < 3; i++)
        {
            crc_8.add(data[i]);
        }
        data[3] = crc_8.calc();
    }
    else
    {
        for (auto i = 0; i < 6; i++)
        {
            crc_8.add(data[i]);
        }
        data[6] = crc_8.calc();
    }
    crc_16.restart();
    data_length -= 2;
    for (auto i = 0; i < data_length; i++)
    {
        crc_16.add(data[i]);
    }
    uint16_t num = crc_16.calc();
    data[(data_length)] = num & 0xFF;
    data[(data_length + 1)] = num >> 8;
    data_length += 2;
    send_bambu_uart(data, data_length);
    if (need_debug)
    {
        // DEBUG_num(data, data_length);
        need_debug = false;
    }
}

uint8_t packge_send_buf[500];

#pragma pack(push, 1) // 将结构体按1字节对齐
struct long_packge_data
{
    uint16_t package_number;
    uint16_t package_length;
    uint8_t crc8;
    uint16_t target_address;
    uint16_t source_address;
    uint16_t type;
    uint8_t *datas;
    uint16_t data_length;
};
#pragma pack(pop) // 恢复默认对齐

void Bambubus_long_package_send(long_packge_data *data)
{
    packge_send_buf[0] = 0x3D;
    packge_send_buf[1] = 0x00;
    data->package_length = data->data_length + 15;
    memcpy(packge_send_buf + 2, data, 11);
    memcpy(packge_send_buf + 13, data->datas, data->data_length);
    package_send_with_crc(packge_send_buf, data->data_length + 15);
}

void Bambubus_long_package_analysis(uint8_t *buf, int data_length, long_packge_data *data)
{
    memcpy(data, buf + 2, 11);
    data->datas = buf + 13;
    data->data_length = data_length - 15; // +2byte CRC16
}

long_packge_data printer_data_long;
package_type get_packge_type(unsigned char *buf, int length)
{
    if (package_check_crc16(buf, length) == false)
    {
        if (buf[1] == 0x05)
            my_printf("(bambu) CRC16 check failed:%02X %02X %02X", buf[1], buf[11], buf[12]);
        else if (buf[4] == 0x20)
        {
            // my_printf("(bambu) CRC16 check failed:%02X %02X", buf[1], buf[4]);
            if (buf[1] == 0xC0 || buf[1] == 0xC8 || buf[1] == 0xE0 || buf[1] == 0xE8 || buf[1] == 0xF0 || buf[1] == 0xF8)
                return BambuBus_package_heartbeat;
        }
        else
            my_printf("(bambu) CRC16 check failed:%02X %02X", buf[1], buf[4]);
        return BambuBus_package_NONE;
    }
    if (buf[4] == 0x20)
    {
        if (buf[1] == 0xC0 || buf[1] == 0xC8 || buf[1] == 0xE0 || buf[1] == 0xE8 || buf[1] == 0xF0 || buf[1] == 0xF8)
            return BambuBus_package_heartbeat;
    }
    if (buf[1] == 0xC5)
    {

        switch (buf[4])
        {
        case 0x03:
            return BambuBus_package_filament_motion_short;
        case 0x04:
            return BambuBus_package_filament_motion_long;
        case 0x05:
            return BambuBus_package_online_detect;
        case 0x06:
            return BambuBus_package_REQx6;
        case 0x07:
            return BambuBus_package_NFC_detect;
        case 0x08:
            return BambuBus_package_set_filament;
        case 0x20:
            return BambuBus_package_heartbeat;
        case 0xa0:
            return BambuBus_package_a0; // p2 排风？
        default:
            return BambuBus_package_ETC;
        }
    }
    else if (buf[1] == 0x05 || buf[1] == 0x04)
    {
        Bambubus_long_package_analysis(buf, length, &printer_data_long);
        if (printer_data_long.target_address == BambuBus_AMS)
        {
            if (BambuBus_address == BambuBus_none)
            {
                my_printf("(bambu) Bambubus工作模式: AMS08");
                BambuBus_address = BambuBus_AMS;
            }
        }
        /*
        else if (printer_data_long.target_address == BambuBus_AMS_lite)
        {
            if (BambuBus_address == BambuBus_none)
            {
                my_printf("(bambu) Bambubus工作模式: AMS_lite");
                BambuBus_address = BambuBus_AMS_lite;
            }
        }
        */

        switch (printer_data_long.type)
        {
        case 0x21A:
            return BambuBus_long_package_MC_online;
        case 0x211:
            return BambuBus_long_package_filament; // 04  11 02 耗材状态
        case 0x218:
            return BambuBus_long_package_set_filament;
        case 0x103: // 04  03 01 序列号
            return BambuBus_long_package_version;
        case 0x402: // 04  02 04 版本信息
            return BambuBus_long_package_version;
        case 0x237:
            return BambuBus_package_0237; // p2
        case 0x23C:
            return BambuBus_package_023c; // p2
        case 0x40D:
            return BambuBus_read_cert;
        case 0x40E:
            return BambuBus_send_cert_verify;
        case 0x411:
            return BambuBus_cert_datas_sync;
        default:
            return BambuBus_package_ETC;
        }
    }
    return BambuBus_package_NONE;
}
uint8_t package_num[4] = {0};
uint8_t bmcu_package_num = 0;

uint8_t get_filament_left_char(uint8_t AMS_num, uint8_t checknum)
{
    uint8_t data = 0;
    for (int i = 0; i < 4; i++)
    {

        if (data_save.filament[AMS_num][i].statu == online)
        {
            data |= (0x1 << i) << i; // 1<<(2*i)
            if (BambuBus_address == BambuBus_AMS)
                if (data_save.filament[AMS_num][i].motion_set != idle && i != checknum)
                {
                    data |= (0x2 << i) << i; // 2<<(2*i)
                }
        }
    }
    return data;
}

void set_motion_res_datas(unsigned char *set_buf, unsigned char AMS_num, unsigned char read_num, unsigned char statu_flags)
{
    static float last_meters = 0;
    static uint64_t last_time = 0;
    uint64_t now_time = get_time64();
    float meters = 0;
    uint16_t pressure = 0xFFFF;
    uint8_t motion_flag = 0x00;
    uint8_t checknum = 0xFF;
    if ((read_num != 0xFF) && (read_num < 4))
    {
        _filament *filament = &data_save.filament[AMS_num][read_num];
        // meters = data_save.filament[AMS_num][read_num].meters + ((float)meters_virtual_count / 100000);
        meters = filament->meters;
        if ((filament->motion_set == idle)) // idle
        {
            motion_flag = 0x00;
            pressure = 0xFFFF;
        }
        else if ((filament->motion_set == need_send_out) || (filament->motion_set == need_pull_back)) // sending or pull back
        {
            motion_flag = 0x02;
            pressure = 0x4700;
        }
        else if ((filament->motion_set == on_use || filament->motion_set == pre_pull)) // on use
        {
            motion_flag = 0x04;
            pressure = filament->pressure;
        }
        // if (statu_flags == 0x07)
        last_time = now_time;
    }
    else if (read_num == 0xFF && statu_flags == 0x03) // ams退料状态更新
    {
        if (data_save.filament[AMS_num][data_save.BambuBus_now_filament_num % 4].motion_set == need_pull_back)
        {
            if (last_time > now_time - pullback_time && GET_MC_Online_stu() > 1)
            {
                meters = data_save.filament[AMS_num][data_save.BambuBus_now_filament_num % 4].meters;
                last_meters = meters;
                read_num = data_save.BambuBus_now_filament_num % 4;
                motion_flag = 0x02;
                pressure = 0x4700;
            }
            else
            {
                meters = last_meters;
                checknum = data_save.BambuBus_now_filament_num % 4;
            }
        }
    }

    set_buf[0] = AMS_num; // real ams_num
    set_buf[1] = 0x00;
    set_buf[2] = motion_flag;
    set_buf[3] = read_num; // filament number or maybe using number
    memcpy(set_buf + 4, &meters, sizeof(float));
    memcpy(set_buf + 8, &pressure, sizeof(uint16_t));
    set_buf[24] = get_filament_left_char(AMS_num, checknum);
}
bool set_motion(unsigned char AMS_num, unsigned char read_num, unsigned char statu_flags, unsigned char fliment_motion_flag)
{
    static uint64_t time_last[4] = {0};
    static uint64_t pre_pull_count = 0;
    static uint64_t meters_virtual_count = 0;
    static uint64_t idle_count = 0;
    uint64_t time_now = get_time64();
    uint64_t time_used = time_now - time_last[AMS_num];
    time_last[AMS_num] = time_now;
    if (BambuBus_address == BambuBus_AMS) // AMS08
    {
        if (read_num < 4)
        {
            if ((statu_flags == 0x03) && (fliment_motion_flag == 0x00)) // 03 00
            {
                uint8_t numx = AMS_num * 4 + read_num;
                if (data_save.BambuBus_now_filament_num != numx) // on change
                {
                    if (data_save.BambuBus_now_filament_num < 16)
                    {
                        data_save.filament[data_save.BambuBus_now_filament_num / 4][data_save.BambuBus_now_filament_num % 4].motion_set = idle;
                        data_save.filament[data_save.BambuBus_now_filament_num / 4][data_save.BambuBus_now_filament_num % 4].pressure = 0xFFFF;
                    }
                    if (!motor_unready && GET_MC_Online_stu() < 2) // 等待bmcu就绪
                    {
                        data_save.BambuBus_now_filament_num = numx;
                        Motor_reboot();
                    }
                }else if (data_save.filament[data_save.BambuBus_now_filament_num / 4][data_save.BambuBus_now_filament_num % 4].motion_set == idle) // on same filament but idle
                {
                    Motor_reboot();
                }
                data_save.filament[AMS_num][read_num].motion_set = need_send_out;
                // data_save.filament[AMS_num][read_num].pressure = 0x4700;
                meters_virtual_count = 0;
                p2s_state = p2s_runtime_state::runtime;
            }
            else if ((statu_flags == 0x09)) // 09 A5 / 09 3F
            {
                if (data_save.filament[AMS_num][read_num].motion_set == need_send_out)
                {
                    data_save.filament[AMS_num][read_num].motion_set = on_use;
                    meters_virtual_count = 0;
                    data_save.filament[AMS_num][read_num].pressure = 0x3700;
                }
                else if (meters_virtual_count < 3500) // 10s virtual data
                {
                    data_save.filament[AMS_num][read_num].meters += (float)time_used / 100000; // 10mm/s
                    meters_virtual_count += time_used;
                }
                data_save.filament[AMS_num][read_num].motion_set = on_use;
                data_save.BambuBus_now_filament_num = AMS_num * 4 + read_num;
                data_save.filament[AMS_num][read_num].pressure -= (uint16_t)time_used * 10; // reduce pressure
                if (data_save.filament[AMS_num][read_num].pressure < 0x0700)
                    data_save.filament[AMS_num][read_num].pressure = 0x3700;
                p2s_state = p2s_runtime_state::runtime;
                idle_count = 0;
                pre_pull_count = 0;
            }
            else if ((statu_flags == 0x07) && (fliment_motion_flag == 0x7F)) // 07 7F
            {
                data_save.filament[AMS_num][read_num].motion_set = on_use;
                data_save.BambuBus_now_filament_num = AMS_num * 4 + read_num;
                data_save.filament[AMS_num][read_num].pressure = 0x2B00;
                meters_virtual_count = 0;
                p2s_state = p2s_runtime_state::printing;
            }
            else if ((statu_flags == 0x07) && (fliment_motion_flag == 0x00)) // 07 00
            {
                data_save.filament[AMS_num][read_num].pressure = 0x2B00;
                if (pre_pull_count < 10000 && pre_pull_count > 1000) // 10s pre pull
                {
                    if (data_save.filament[AMS_num][read_num].motion_set == on_use)
                    {
                        data_save.filament[AMS_num][read_num].motion_set = pre_pull;
                    }
                    pre_pull_count += time_used;
                }
                else
                {
                    data_save.filament[AMS_num][read_num].motion_set = on_use;
                }
                p2s_state = p2s_runtime_state::runtime;
            }
        }
        else if ((read_num == 0xFF))
        {
            if ((statu_flags == 0x03) && (fliment_motion_flag == 0x00)) // 03 00(FF)
            {
                _filament *filament = &(data_save.filament[AMS_num][data_save.BambuBus_now_filament_num % 4]);
                if (data_save.BambuBus_now_filament_num < 16)
                {

                    if (filament->motion_set != idle)
                        filament->motion_set = need_pull_back;
                    filament->pressure = 0x4700;
                }
                p2s_state = p2s_runtime_state::runtime;
            }
            else if ((statu_flags == 0x01) && (fliment_motion_flag == 0x00)) // 01 00(FF)
            {
                for (auto i = 0; i < 4; i++)
                {
                    if (data_save.BambuBus_now_filament_num == (AMS_num * 4 + i))
                    {
                        if (idle_count < pullback_time && (data_save.filament[AMS_num][i].motion_set == need_pull_back && GET_MC_Online_stu() > 1)) // 30s idle
                        {
                            idle_count += time_used;
                            continue;
                        }
                    }
                    if (data_save.filament[AMS_num][i].motion_set != on_use)
                        data_save.filament[AMS_num][i].motion_set = idle;
                    data_save.filament[AMS_num][i].pressure = 0xFFFF;
                }
                if (p2s_state != p2s_runtime_state::boot)
                    p2s_state = p2s_runtime_state::runtime;
            }
        }
    }
    else if (BambuBus_address == BambuBus_none) // none
    {
        /*if ((read_num != 0xFF) && (read_num < 4))
        {
            if ((statu_flags == 0x07) && (fliment_motion_flag == 0x00)) // 07 00
            {
                data_save.BambuBus_now_filament_num = AMS_num * 4 + read_num;
                data_save.filament[AMS_num][read_num].motion_set = on_use;
            }
        }*/
    }
    else
        return false;
    return true;
}
void Bmcu_package_send_with_crc(uint8_t *data, int data_length)
{
    data[0] = 0x9D;
    data[1] = data_length;
    crc_8.restart();
    for (auto i = 0; i < data_length - 1; i++)
    {
        crc_8.add(data[i]);
    }
    uint8_t num = crc_8.calc();
    data[(data_length - 1)] = num;
    send_bmcu_uart(data, data_length);
}
void online_buf_set(unsigned char *set_buf)
{
    set_buf[0] = GET_MC_Online_stu();
    if (motor_unready)
        set_buf[0] |= 0x30;
    set_buf[1] = (uint8_t)((GET_MC_PULL_raw() - 0.9f) * 128); // 通道压力值/128 增大0.1
}
unsigned char Hit_res[] = {0x9D, 0x0A, 0x20,
                           0x00, 0x00, // amsnum + taynum
                           0x00, 0x00, // 在线检测+电机检测
                           0x00};      // crc8 校验
void send_for_Hit(unsigned char *buf, int length, uint64_t time_now)
{
    static uint64_t last_Hit_time = 0;
    if (time_now - last_Hit_time < 80)
        return;
    last_Hit_time = time_now;
    bmcu_package_num++;
    if (bmcu_package_num >= AMS_num_max + F_AMS_num)
        bmcu_package_num = 0;
    if (AMS_num_c > AMS_num_max)
    {
        Tay_num_c++;
        AMS_num_c = 0;
    }
    if (Tay_num_c > 3)
    {
        Tay_num_c = 0;
    }
    Hit_res[3] = AMS_num_c;
    Hit_res[4] = Tay_num_c;
    Hit_res[2] = 0x20;
    AMS_num_c++; // 每个心跳包轮询一个bmcu_tay

    Hit_res[5] = 0; // sw_read();               // 五通前端状态
    Hit_res[6] = 0;
    if (BambuBus_address == BambuBus_AMS) // AMS08
    {
        Hit_res[5] |= 0x30; // 0x30
    }
    else if (BambuBus_address == BambuBus_AMS_lite) // AMS lite
    {
        Hit_res[5] |= 0xC0; // 0xC0
    }

    Bmcu_package_send_with_crc(Hit_res, sizeof(Hit_res));
}
// 3D E0 3C 12 04 00 00 00 00 09 09 09 00 00 00 00 00 00 00
// 02 00 E9 3F 14 BF 00 00 76 03 6A 03 6D 00 E5 FB 99 14 2E 19 6A 03 41 F4 C3 BE E8 01 01 01 01 00 00 00 00 64 64 64 64 0A 27
// 3D E0 2C C9 03 00 00
// 04 01 79 30 61 BE 00 00 03 00 44 00 12 00 FF FF FF FF 00 00 44 00 54 C1 F4 EE E7 01 01 01 01 00 00 00 00 FA 35
#define C_test 0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x80, 0xBF, \
               0x00, 0x00, 0xFF, 0xFF, \
               0x36, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x27, 0x00, \
               0x55,                   \
               0xFF, 0xFF, 0xFF, 0xFF, \
               0x01, 0x01, 0x01, 0x01,
/*#define C_test 0x00, 0x00, 0x02, 0x02, \
               0x00, 0x00, 0x00, 0x00, \
               0x00, 0x00, 0x00, 0xC0, \
               0x36, 0x00, 0x00, 0x00, \
               0xFC, 0xFF, 0xFC, 0xFF, \
               0x00, 0x00, 0x27, 0x00, \
               0x55,                   \
               0xC1, 0xC3, 0xEC, 0xBC, \
               0x01, 0x01, 0x01, 0x01,
00 00 02 02 EB 8F CA 3F 49 48 E7 1C 97 00 E7 1B F3 FF F2 FF 00 00 90 00 75 F8 EE FC F0 B6 B8 F8 B0 00 00 00 00 FF FF FF FF*/
/*
#define C_test 0x00, 0x00, 0x02, 0x01, \
                0xF8, 0x65, 0x30, 0xBF, \
                0x00, 0x00, 0x28, 0x03, \
                0x2A, 0x03, 0x6F, 0x00, \
                0xB6, 0x04, 0xFC, 0xEC, \
                0xDF, 0xE7, 0x44, 0x00, \
                0x04, \
                0xC3, 0xF2, 0xBF, 0xBC, \
                0x01, 0x01, 0x01, 0x01,*/
unsigned char Cxx_res[] = {0x3D, 0xE0, 0x2C, 0x1A, 0x03,
                           C_test 0x00, 0x00, 0x00, 0x00,
                           0x90, 0xE4};
unsigned char Motion_res[] = {0x9D, 0x0A, 0x03,
                              0x00, 0x00, // amsnum + taynum
                              0x00, 0x00, // statu_flags + fliment_motion
                              0x00, 0x00, // 在线检测+缓冲
                              0x00};      // crc8 校验
void send_for_motion_short(unsigned char *buf, int length)
{
    unsigned char AMS_num = buf[5];
    unsigned char statu_flags = buf[6];
    unsigned char read_num = buf[7];
    unsigned char fliment_motion_flag = buf[8];
    Cxx_res[1] = 0xC0 | (package_num[AMS_num] << 3);
    uint8_t AMS_num_REAL = AMS_num;
    if (AMS_num < F_AMS_num)
        return; // REAL AMS 不处理短包
    else
        AMS_num = get_ams_map_to(AMS_num);
    Motion_res[2] = 0x03;
    Motion_res[3] = AMS_num;
    Motion_res[4] = read_num;
    Motion_res[5] = statu_flags;
    Motion_res[6] = fliment_motion_flag;
    online_buf_set(Motion_res + 7);
    if (!set_motion(AMS_num, read_num, statu_flags, fliment_motion_flag))
        return;

    // Cxx_res[38] = read_num;
    if (statu_flags == 0x03 && read_num == 0xFF)
    {
        // Cxx_res[38] = data_save.BambuBus_now_filament_num % 4;
    }

    set_motion_res_datas(Cxx_res + 5, AMS_num, read_num, statu_flags);
    Cxx_res[5] = AMS_num_REAL; // real ams_num

    if (package_num[AMS_num] % 3 == 0)
    {
        package_send_with_crc(Cxx_res, sizeof(Cxx_res));
    }
    else if (package_num[AMS_num] % 3 == 1)
    {
        // package_send_with_crc(Cxx_res, sizeof(Cxx_res));
        Bmcu_package_send_with_crc(Motion_res, sizeof(Motion_res)); // 重写amsnum 转发bmcu
    }
    if (package_num[AMS_num] < 7)
        package_num[AMS_num]++;
    else
        package_num[AMS_num] = 0;
}
/*
0x00, 0x00, 0x00, 0xFF, // 0x0C...
0x00, 0x00, 0x80, 0xBF, // distance
0x00, 0x00, 0x00, 0xC0,
0x00, 0xC0, 0x5D, 0xFF,
0xFE, 0xFF, 0xFE, 0xFF, // 0xFE, 0xFF, 0xFE, 0xFF,
0x00, 0x44, 0x00, 0x00,
0x10,
0xC1, 0xC3, 0xEC, 0xBC,
0x01, 0x01, 0x01, 0x01,
*/
unsigned char Motion_long_res[] = {0x9D, 0x0A, 0x04,
                                   0x00, 0x00, // amsnum + taynum
                                   0x00, 0x00, // statu_flags + fliment_motion
                                   0x00, 0x00, // 在线检测+缓冲
                                   0x00};      // crc8 校验
unsigned char Dxx_res[] = {0x3D, 0xE0, 0x3C, 0x1A, 0x04,
                           0x00, //[5]AMS num
                           0x01,
                           0x01,
                           1,                      // humidity wet
                           0x04, 0x04, 0x04, 0xFF, // flags
                           0x00, 0x00, 0x00, 0x00,
                           C_test 0x00, 0x00, 0x00, 0x00,
                           0xFF, 0xFF, 0xFF, 0xFF,
                           0x90, 0xE4};

bool need_res_for_06 = false;
uint8_t res_for_06_num = 0xFF;
int last_detect = 0;
uint8_t filament_flag_detected = 0;

void send_for_motion_long(unsigned char *buf, int length)
{
    unsigned char filament_flag_on = 0x00;
    unsigned char filament_flag_NFC = 0x00;
    unsigned char AMS_num = buf[5];
    unsigned char statu_flags = buf[6];
    unsigned char fliment_motion_flag = buf[7];
    unsigned char read_num = buf[9];
    uint8_t AMS_num_REAL = AMS_num;
    if (AMS_num < F_AMS_num)
        return; // REAL AMS 不处理长包
    else
        AMS_num = get_ams_map_to(AMS_num);
    Motion_long_res[2] = 0x04;
    Motion_long_res[3] = AMS_num;
    Motion_long_res[4] = read_num;
    Motion_long_res[5] = statu_flags;
    Motion_long_res[6] = fliment_motion_flag;
    online_buf_set(Motion_long_res + 7);
    for (auto i = 0; i < 4; i++)
    {
        // filament[i].meters;
        if (data_save.filament[AMS_num][i].statu == online)
        {
            filament_flag_on |= 1 << i;
        }
        else if (data_save.filament[AMS_num][i].statu == NFC_waiting)
        {
            filament_flag_on |= 1 << i;
            filament_flag_NFC |= 1 << i;
        }
    }
    if (!set_motion(AMS_num, read_num, statu_flags, fliment_motion_flag))
        return;
    /*if (need_res_for_06)
    {
        Dxx_res2[1] = 0xC0 | (package_num[AMS_num] << 3);
        Dxx_res2[9] = filament_flag_on;
        Dxx_res2[10] = filament_flag_on - filament_flag_NFC;
        Dxx_res2[11] = filament_flag_on - filament_flag_NFC;
        Dxx_res[19] = motion_flag;
        Dxx_res[20] = Dxx_res2[12] = res_for_06_num;
        Dxx_res2[13] = filament_flag_NFC;
        Dxx_res2[41] = get_filament_left_char();
        package_send_with_crc(Dxx_res2, sizeof(Dxx_res2));
        need_res_for_06 = false;
    }
    else*/

    Dxx_res[1] = 0xC0 | (package_num[AMS_num] << 3);
    Dxx_res[5] = AMS_num_REAL; // real ams_num
    Dxx_res[9] = filament_flag_on;
    Dxx_res[10] = filament_flag_on - filament_flag_NFC;
    Dxx_res[11] = filament_flag_on - filament_flag_NFC;
    Dxx_res[12] = read_num;
    Dxx_res[13] = filament_flag_NFC;

    set_motion_res_datas(Dxx_res + 17, AMS_num, read_num, statu_flags);
    Dxx_res[17] = AMS_num_REAL; // real ams_num

    if (last_detect != 0)
    {
        if (last_detect > 10)
        {
            Dxx_res[19] = 0x01;
        }
        else
        {
            Dxx_res[12] = filament_flag_detected;
            Dxx_res[19] = 0x01;
            Dxx_res[20] = filament_flag_detected;
        }
        last_detect--;
    }
    if (bambus_onflush)
    {
        if (Dxx_res[5] == data_save.BambuBus_now_filament_num / 4 && GET_MC_Online_stu() > 0)
            package_send_with_crc(Dxx_res, sizeof(Dxx_res));
    }
    else if (Dxx_res[5] == bmcu_package_num)
        package_send_with_crc(Dxx_res, sizeof(Dxx_res));

    if (package_num[AMS_num] < 7)
        package_num[AMS_num]++;
    else
        package_num[AMS_num] = 0;
    if (Motion_long_res[3] == (bmcu_package_num + 1) % AMS_num_max)
    {
        Bmcu_package_send_with_crc(Motion_long_res, sizeof(Motion_long_res)); // 重写amsnum 转发bmcu
    }
}
unsigned char REQx6_res[] = {0x3D, 0xE0, 0x3C, 0x1A, 0x06,
                             0x00, 0x00, 0x00, 0x00,
                             0x04, 0x04, 0x04, 0xFF, // flags
                             0x00, 0x00, 0x00, 0x00,
                             C_test 0x00, 0x00, 0x00, 0x00,
                             0x64, 0x64, 0x64, 0x64,
                             0x90, 0xE4};
void send_for_REQx6(unsigned char *buf, int length)
{
    /*
        unsigned char filament_flag_on = 0x00;
        unsigned char filament_flag_NFC = 0x00;
        for (auto i = 0; i < 4; i++)
        {
            if (data_save.filament[AMS_num][i].statu == online)
            {
                filament_flag_on |= 1 << i;
            }
            else if (data_save.filament[AMS_num][i].statu == NFC_waiting)
            {
                filament_flag_on |= 1 << i;
                filament_flag_NFC |= 1 << i;
            }
        }
        REQx6_res[1] = 0xC0 | (package_num[AMS_num] << 3);
        res_for_06_num = buf[7];
        REQx6_res[9] = filament_flag_on;
        REQx6_res[10] = filament_flag_on - filament_flag_NFC;
        REQx6_res[11] = filament_flag_on - filament_flag_NFC;
        Dxx_res2[12] = res_for_06_num;
        Dxx_res2[12] = res_for_06_num;
        package_send_with_crc(REQx6_res, sizeof(REQx6_res));
        need_res_for_06 = true;
        if (package_num[AMS_num] < 7)
            package_num[AMS_num]++;
        else
            package_num[AMS_num] = 0;*/
}

void NFC_detect_run()
{
    /*uint64_t time = GetTick();
    return;
    if (time > last_detect + 3000)
    {
        filament_flag_detected = 0;
    }*/
}

void p2s_reset_startup_seq(void)
{
    for (int i = 0; i < 4; i++)
    {
        p2s_a0_seq_pos = 0;
        p2s_0237_seq_pos[i] = 0;
        p2s_023c_seq_pos[i] = 0;
    }
    p2s_state = p2s_runtime_state::runtime;
}
static const uint8_t p2s_a0_payload_seq[][10] = {
    {0x86, 0x36, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x11, 0x00},
    {0x66, 0x98, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x10, 0x00},
    {0x82, 0x0E, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00},
    {0x68, 0x06, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00},
    {0x1B, 0x06, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00},
    {0xEF, 0x04, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00},
    {0xBD, 0x03, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
    {0x8F, 0x03, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x07, 0x00},
    {0xC7, 0x01, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
    {0x1D, 0x01, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
    {0x64, 0x01, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x04, 0x00},
    {0x01, 0x00, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x06, 0x00},
    {0x75, 0x00, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x7D, 0x00, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x30, 0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
};

static const uint8_t p2s_0237_resp_boot0[25] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x1A, 0x00, 0x00, 0x00};
static const uint8_t p2s_0237_resp_boot1[25] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0xFA, 0x12, 0x6A, 0x6C, 0x94, 0x0F, 0x15, 0x6E, 0x73, 0x0C, 0x98, 0x0C, 0x90, 0x0C,
    0x00, 0x1A, 0x00, 0x00, 0x00};
static const uint8_t p2s_0237_resp_boot2[25] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0xFA, 0x12, 0x6A, 0x6C, 0x94, 0x0F, 0x15, 0x6E, 0x73, 0x0C, 0x98, 0x0C, 0x90, 0x0C,
    0x02, 0x1A, 0x00, 0x00, 0x00};
static const uint8_t p2s_0237_resp_run[25] = {
    0x00, 0x01, 0x00, 0x00, 0x00, 0x00,
    0xFA, 0x12, 0x6A, 0x6C, 0x94, 0x0F, 0x15, 0x6E, 0x73, 0x0C, 0x98, 0x0C, 0x90, 0x0C,
    0x01, 0x1A, 0x00, 0xF1, 0xFB};

static const uint8_t p2s_023c_const_prefix[14] = {
    0x00, 0xE8, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x15, 0x16, 0x16, 0x15};
static const uint8_t p2s_023c_const_zeros9[9] = {
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
static const uint8_t p2s_023c_const_tail[3] = {0x34, 0x00, 0x31};

static void build_023c_response(uint8_t *resp, uint8_t byte14, uint8_t lo1516, bool phase_done, uint8_t hi44, uint8_t lo43)
{
    memcpy(resp, p2s_023c_const_prefix, 14);
    resp[14] = byte14;
    resp[15] = lo1516;
    resp[16] = lo1516;
    memcpy(resp + 17, p2s_023c_const_zeros9, 9);
    if (phase_done)
    {
        resp[26] = 0x1A;
        resp[27] = 0x00;
        resp[28] = 0x00;
        resp[29] = 0x00;
        resp[30] = 0x1A;
        resp[31] = 0x00;
        resp[32] = 0x00;
        resp[33] = 0x00;
    }
    else
    {
        resp[26] = 0x00;
        resp[27] = 0x00;
        resp[28] = 0x00;
        resp[29] = 0x00;
        resp[30] = 0x00;
        resp[31] = 0x00;
        resp[32] = 0x00;
        resp[33] = 0x00;
    }
    memcpy(resp + 34, p2s_023c_const_zeros9, 9);
    resp[43] = lo43;
    resp[44] = hi44;
    resp[45] = 0x00;
    resp[46] = 0x00;
    resp[47] = 0x00;
    resp[48] = 0x00;
    resp[49] = 0x00;
    resp[50] = 0x00;
    resp[51] = 0x00;
    memcpy(resp + 52, p2s_023c_const_tail, 3);
}

static void send_for_a0(unsigned char *buf, int length)
{
    (void)buf;
    (void)length;
    const size_t max_pos = sizeof(p2s_a0_payload_seq) / sizeof(p2s_a0_payload_seq[0]);
    const size_t pos = (p2s_a0_seq_pos < (uint8_t)max_pos) ? p2s_a0_seq_pos : (max_pos - 1u);
    const uint8_t *payload = p2s_a0_payload_seq[pos];

    uint8_t out[19];
    out[0] = 0x3D;
    out[1] = 0xC0;
    out[2] = 0x13;
    out[4] = 0xA0;
    out[5] = 0x03;
    out[6] = (p2s_a0_seq_pos == 0u) ? 0x00u : 0x02u;
    memcpy(out + 7, payload, 10);
    package_send_with_crc(out, 19);
    if (p2s_a0_seq_pos < 0xFFu)
        p2s_a0_seq_pos++;
}

static void send_for_0237(unsigned char *buf, int length)
{
    (void)buf;
    (void)length;
    uint8_t AMS_num = printer_data_long.datas[0];
    if (printer_data_long.target_address != BambuBus_address || AMS_num < F_AMS_num)
        return;
    const bool is_boot_req = (printer_data_long.data_length >= 3) && (printer_data_long.datas[2] == 0x01);
    const uint8_t *payload;

    if (is_boot_req)
    {
        if (p2s_0237_seq_pos[AMS_num] == 0u)
            payload = p2s_0237_resp_boot0;
        else if (p2s_0237_seq_pos[AMS_num] == 1u)
            payload = p2s_0237_resp_boot1;
        else
            payload = p2s_0237_resp_boot2;
    }
    else
    {
        payload = p2s_0237_resp_run;
    }

    uint8_t resp[25];
    memcpy(resp, payload, sizeof(resp));

    if (payload != p2s_0237_resp_boot0)
        resp[0] = (uint8_t)AMS_num;

    if (!is_boot_req)
    {
        if (p2s_state == p2s_runtime_state::boot)
            resp[21] = 0x1A;
        else if (p2s_state == p2s_runtime_state::runtime)
            resp[21] = 0x1B;
        else
            resp[21] = 0x1C;
    }

    long_packge_data data;
    data.datas = resp;
    data.data_length = sizeof(resp);
    data.package_number = printer_data_long.package_number;
    data.type = printer_data_long.type;
    data.source_address = printer_data_long.target_address;
    data.target_address = printer_data_long.source_address;
    Bambubus_long_package_send(&data);

    if (is_boot_req && p2s_0237_seq_pos[AMS_num] < 0xFFu)
        p2s_0237_seq_pos[AMS_num]++;
}

static void send_for_023c(unsigned char *buf, int length)
{
    (void)buf;
    (void)length;
    uint8_t AMS_num = printer_data_long.datas[0];
    if (printer_data_long.target_address != BambuBus_address || AMS_num < F_AMS_num)
        return;
    uint8_t resp[55];
    const uint8_t byte14 = (p2s_023c_seq_pos[AMS_num] & 1u) ? 0x06u : 0x10u;

    if (p2s_state == p2s_runtime_state::boot)
    {
        if (p2s_023c_seq_pos[AMS_num] < 2u)
            build_023c_response(resp, byte14, 0x63, false, 0xF4, 0xE3);
        else if (p2s_023c_seq_pos[AMS_num] < 8u)
            build_023c_response(resp, byte14, 0x63, true, 0xF4, 0xE3);
        else if (p2s_023c_seq_pos[AMS_num] < 12u)
            build_023c_response(resp, byte14, 0x63, true, 0x4A, 0x45);
        else
            build_023c_response(resp, byte14, 0x00, true, 0x4A, 0x45);
    }
    else
    {
        build_023c_response(resp, byte14, 0x00, true, 0x4A, 0x45);
        resp[49] = 0xFF;
        resp[50] = 0x00;
    }
    resp[0] = (uint8_t)AMS_num;
    long_packge_data data;
    data.datas = resp;
    data.data_length = sizeof(resp);
    data.package_number = printer_data_long.package_number;
    data.type = printer_data_long.type;
    data.source_address = printer_data_long.target_address;
    data.target_address = printer_data_long.source_address;
    Bambubus_long_package_send(&data);

    if (p2s_023c_seq_pos[AMS_num] < 0xFFu)
        p2s_023c_seq_pos[AMS_num]++;
}
unsigned char F01_res[] = {
    0x3D, 0xC0, 0x1D, 0xB4, 0x05, 0x01, 0x00,
    0x20,
    0x0E, 0x7D, 0x32, 0x31, 0x31, 0x38, 0x15, 0x00, 0x36, 0x39, 0x37, 0x33, 0xFF, 0xFF, 0xFF, 0xFF,
    0x00, 0x00, 0x00, 0x33, 0xF0};
int num_F00 = 0;
uint8_t detect_num = 14;
bool have_registered[4] = {false, false, false, false};
void send_for_online_detect(unsigned char *buf, int length)
{
    uint8_t F00_res[sizeof(F01_res)];
    memcpy(F00_res, F01_res, sizeof(F01_res));
    if (BambuBus_address == BambuBus_none)
    {
        num_F00 = 0;
    }
    if ((buf[5] == 0x00))
    {
        if (millis() < 5000)
        {
            return;
        }
        for (num_F00 = F_AMS_num; num_F00 < (AMS_num_max + F_AMS_num); num_F00++)
        {
            // F00_res[7] = 3 - num_F00;
            if (have_registered[num_F00] == true)
                continue;
            F00_res[5] = 0;
            F00_res[6] = num_F00;
            F00_res[7] = detect_num - num_F00;
            memcpy(F00_res + 8, online_detect_num[num_F00], sizeof(online_detect_num[num_F00]));
            package_send_with_crc(F00_res, sizeof(F00_res));
            //vTaskDelay(pdMS_TO_TICKS(5));
        }
    }
    else if ((buf[5] == 0x01) && (buf[6] < (AMS_num_max + F_AMS_num)))
    {
        if (buf[6] < F_AMS_num)
        {
            //if (buf[7] != 0)
            //    detect_num = buf[7];
            return; // REAL AMS 不处理ONLINE DETECT
        }
        F00_res[6] = buf[6];
        F00_res[7] = buf[7];
        memcpy(F00_res + 8, online_detect_num[buf[6]], sizeof(online_detect_num[buf[6]]));        
        if (memcmp(buf + 8, online_detect_num[buf[6]], sizeof(online_detect_num[buf[6]])) == 0)
        {
            have_registered[buf[6]] = true;
            package_send_with_crc(F00_res, sizeof(F00_res));
        }
        else
        {
            have_registered[buf[6]] = false;
        }
    }
}
// 3D C5 0D F1 07 00 00 00 00 00 00 CE EC
// 3D C0 0D 6F 07 00 00 00 00 00 00 9A 70

unsigned char NFC_detect_res[] = {0x3D, 0xC0, 0x0D, 0x6F, 0x07, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFC, 0xE8};
void send_for_NFC_detect(unsigned char *buf, int length)
{
    last_detect = 20;
    filament_flag_detected = 1 << buf[6];
    NFC_detect_res[6] = buf[6];
    NFC_detect_res[7] = buf[7];
    package_send_with_crc(NFC_detect_res, sizeof(NFC_detect_res));
}

unsigned char long_packge_MC_online[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void send_for_long_packge_MC_online(unsigned char *buf, int length)
{
    long_packge_data data;
    uint8_t AMS_num = printer_data_long.datas[0];
    Bambubus_long_package_analysis(buf, length, &printer_data_long);
    if (printer_data_long.target_address == 0x0700)
    {
    }
    else if (printer_data_long.target_address == 0x1200)
    {
    }
    /*else if(printer_data_long.target_address==0x0F00)
    {

    }*/
    else
    {
        return;
    }
    if (printer_data_long.target_address != BambuBus_address || AMS_num < F_AMS_num || AMS_num >= AMS_num_max + F_AMS_num)
        return;
    data.datas = long_packge_MC_online;
    data.datas[0] = AMS_num;
    data.data_length = sizeof(long_packge_MC_online);

    data.package_number = printer_data_long.package_number;
    data.type = printer_data_long.type;
    data.source_address = printer_data_long.target_address;
    data.target_address = printer_data_long.source_address;
    if (have_registered[AMS_num] == true)
        Bambubus_long_package_send(&data);

    // Bmcu_package_send_with_crc(buf,length);                        //转发bmcu
}
unsigned char long_packge_filament[] =
    {
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x47, 0x46, 0x42, 0x30, 0x30, 0x00, 0x00, 0x00,
        0x41, 0x42, 0x53, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0xDD, 0xB1, 0xD4, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x18, 0x01, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
void send_for_long_packge_filament(unsigned char *buf, int length)
{
    long_packge_data data;
    Bambubus_long_package_analysis(buf, length, &printer_data_long);

    uint8_t AMS_num = printer_data_long.datas[0];
    uint8_t filament_num = printer_data_long.datas[1];
    long_packge_filament[0] = AMS_num;
    long_packge_filament[1] = filament_num;
    if (AMS_num < F_AMS_num)
        return; // REAL AMS 不处理耗材长包
    else
        AMS_num = get_ams_map_to(AMS_num);
    if (filament_num > 3)
    {
        my_printf("(bambu) 错误的通道耗材长包裹数据");
        return;
    }

    memcpy(long_packge_filament + 19, data_save.filament[AMS_num][filament_num].ID, sizeof(data_save.filament[AMS_num][filament_num].ID));
    memcpy(long_packge_filament + 27, data_save.filament[AMS_num][filament_num].name, sizeof(data_save.filament[AMS_num][filament_num].name));
    long_packge_filament[59] = data_save.filament[AMS_num][filament_num].color_R;
    long_packge_filament[60] = data_save.filament[AMS_num][filament_num].color_G;
    long_packge_filament[61] = data_save.filament[AMS_num][filament_num].color_B;
    long_packge_filament[62] = data_save.filament[AMS_num][filament_num].color_A;
    memcpy(long_packge_filament + 79, &data_save.filament[AMS_num][filament_num].temperature_max, 2);
    memcpy(long_packge_filament + 81, &data_save.filament[AMS_num][filament_num].temperature_min, 2);

    data.datas = long_packge_filament;
    data.data_length = sizeof(long_packge_filament);

    data.package_number = printer_data_long.package_number;
    data.type = printer_data_long.type;
    data.source_address = printer_data_long.target_address;
    data.target_address = printer_data_long.source_address;
    Bambubus_long_package_send(&data);
}
unsigned char serial_number[] = {"03919D492302599"};     // 03919D492302599
unsigned char long_packge_version_serial_number[] = {16, // length
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
                                                     0x0E, 0x7D, 0x32, 0x31, 0x31, 0x38, 0x15, 0x00, // serial_number#2
                                                     0x36, 0x39, 0x37, 0x33,
                                                     0xFF, 0xFF, 0xFF, 0xFF,
                                                     0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xBB, 0x44, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0x00};

unsigned char long_packge_version_version_and_name_AMS_lite[] = {0x3E, 0x06, 0x01, 0x00, // verison number
                                                                 0x41, 0x4D, 0x53, 0x5F, 0x46, 0x31, 0x30, 0x32, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
unsigned char long_packge_version_version_and_name_AMS08[] = {0x00, 0x00, 0x00, 0x00, // verison number
                                                              0x41, 0x4D, 0x53, 0x30, 0x38, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};

void send_for_long_packge_version(unsigned char *buf, int length)
{
    long_packge_data data;
    Bambubus_long_package_analysis(buf, length, &printer_data_long);
    uint8_t AMS_num = printer_data_long.datas[0];
    unsigned char *long_packge_version_version_and_name;

    if (printer_data_long.target_address == BambuBus_AMS)
    {
        long_packge_version_version_and_name = long_packge_version_version_and_name_AMS08;
    }
    else if (printer_data_long.target_address == BambuBus_AMS_lite)
    {
        long_packge_version_version_and_name = long_packge_version_version_and_name_AMS_lite;
    }
    else
    {
        return;
    }
    if (printer_data_long.target_address != BambuBus_address)
        return;
    switch (printer_data_long.type)
    {
    case 0x402:
        AMS_num = printer_data_long.datas[33];
        if (AMS_num < F_AMS_num || AMS_num >= AMS_num_max + F_AMS_num)
            return; // REAL AMS 不处理版本序列号长包
        serial_number[14] = AMS_num + 1;
        long_packge_version_serial_number[0] = sizeof(serial_number);
        memcpy(long_packge_version_serial_number + 1, serial_number, sizeof(serial_number));
        if (printer_data_long.target_address == BambuBus_AMS)
        {
            memcpy(long_packge_version_serial_number + 33, online_detect_num[AMS_num], sizeof(online_detect_num[AMS_num]));

        }
        data.datas = long_packge_version_serial_number;
        data.data_length = sizeof(long_packge_version_serial_number);
        data.datas[65] = AMS_num;
        break;
    case 0x103:
        AMS_num = printer_data_long.datas[0];
        if (AMS_num < F_AMS_num || AMS_num >= AMS_num_max + F_AMS_num)
            return; // REAL AMS 不处理版本名称长包
        data.datas = long_packge_version_version_and_name;
        data.data_length = sizeof(long_packge_version_version_and_name_AMS08);
        data.datas[20] = AMS_num;
        break;
    default:
        return;
    }

    data.package_number = printer_data_long.package_number;
    data.type = printer_data_long.type;
    data.source_address = printer_data_long.target_address;
    data.target_address = printer_data_long.source_address;
    if (have_registered[AMS_num] == true)
        Bambubus_long_package_send(&data);
}
unsigned char s = 0x01;
unsigned char filament_res[] = {0x7D, 0x0A, 0x08,
                                0x00, 0x00, // amsnum + taynum
                                0x00, 0x00, // 公用控制位 + 专用控制位
                                0x00};      // crc8 校验
unsigned char Set_filament_res[] = {0x3D, 0xC0, 0x08, 0xB2, 0x08, 0x60, 0xB4, 0x04};
uint8_t motor_time[4] = {1, 4, 8, 12};  // 电机退料时间
uint8_t pwm_zero[4] = {22, 30, 38, 46}; // 电机pwm 零点
void send_reset()
{
    filament_res[2] = 0x08;
    filament_res[3] = 0x00;
    filament_res[4] = 0x00;
    filament_res[5] = 0xE0; // bmcu_reset
    filament_res[6] = 0x00;
    Bmcu_package_send_with_crc(filament_res, sizeof(filament_res));
}
void send_for_set_filament(unsigned char *buf, int length)
{
    uint8_t read_num = buf[5];
    uint8_t AMS_num = read_num / 4;
    read_num = read_num % 4;
    if (AMS_num < F_AMS_num)
        return; // REAL AMS 不处理设置耗材数据
    else
        AMS_num = get_ams_map_to(AMS_num);
    uint8_t sw2 = Switch_set_filament(buf, length, AMS_num, read_num);

    filament_res[5] = 0x00;
    filament_res[6] = 0x00;

    if (!sw2)
    {

        memcpy(data_save.filament[AMS_num][read_num].ID, buf + 7, sizeof(data_save.filament[AMS_num][read_num].ID));

        data_save.filament[AMS_num][read_num].color_R = buf[15];
        data_save.filament[AMS_num][read_num].color_G = buf[16];
        data_save.filament[AMS_num][read_num].color_B = buf[17];
        data_save.filament[AMS_num][read_num].color_A = buf[18];

        memcpy(&data_save.filament[AMS_num][read_num].temperature_min, buf + 19, 2);
        memcpy(&data_save.filament[AMS_num][read_num].temperature_max, buf + 21, 2);
        memcpy(data_save.filament[AMS_num][read_num].name, buf + 23, sizeof(data_save.filament[AMS_num][read_num].name));

        package_send_with_crc(Set_filament_res, sizeof(Set_filament_res));
        Bambubus_set_need_to_save();
    }
    else
    {
        filament_res[2] = 0x08;
        filament_res[3] = AMS_num;
        filament_res[4] = read_num;
        filament_res[5] = 0xE0; // bmcu_reset

        if (sw2 == 0xD1)
        {
            filament_res[6] = 0xD1; // reset meter       白色
            reset_filament_meters(AMS_num, read_num);
            my_printf("(bmcu) 重置耗材里程: Bmcu%d-%d_reset_meter", AMS_num, read_num);
        }
        else if (sw2 == 0xD3)
        {
            filament_res[6] = 0xD3; // 棕色  --电机退料时间设定
            my_printf("(bmcu) 电机二段退料时间设定: Bmcu%d-%d_motor_time = %ds", AMS_num, read_num, motor_time[read_num]);
        }
        else if (sw2 == 0xD5)
        {
            filament_res[6] = 0xD5; // 岩石灰  --电机退料时间设定
            my_printf("(bmcu) 电机一段退料时间设定: Bmcu%d-%d_motor_time = %ds", AMS_num, read_num, motor_time[read_num]);
        }
        else if (sw2 == 0xD7)
        {
            filament_res[6] = 0xD7; // 灰色  --电机pwm 自动标定
            my_printf("(bmcu) 电机pwm设定: Bmcu%d-%d_pwm_zero = %d", AMS_num, read_num, (pwm_zero[read_num] * 10));
        }
        else if (sw2 == 0xD9)
        {
            filament_res[6] = 0xD9; // 选中激活为onuse    黑色
            my_printf("(bmcu) 选中激活为onuse: Bmcu%d-%d_onuse", AMS_num, read_num);
        }
        Bmcu_package_send_with_crc(filament_res, sizeof(filament_res));
    }
}
unsigned char Set_filament_res_type2[] = {0x00, 0x00, 0x00};
void send_for_long_packge_set_filament(unsigned char *buf, int length)
{
    long_packge_data data;
    Bambubus_long_package_analysis(buf, length, &printer_data_long);
    uint8_t AMS_num = printer_data_long.datas[0];
    uint8_t read_num = printer_data_long.datas[1];
    uint8_t AMS_num_real = AMS_num;
    if (AMS_num < F_AMS_num)
        return; // REAL AMS 不处理设置耗材数据
    else
        AMS_num = get_ams_map_to(AMS_num);
    uint8_t buf_t[39] = {0x00};
    memcpy(buf_t + 7, printer_data_long.datas + 2, sizeof(buf_t) - 7);
    uint8_t sw2 = Switch_set_filament(buf_t, sizeof(buf_t), AMS_num, read_num);

    filament_res[5] = 0x00;
    filament_res[6] = 0x00;

    if (!sw2)
    {
        memcpy(data_save.filament[AMS_num][read_num].ID, printer_data_long.datas + 2, sizeof(data_save.filament[AMS_num][read_num].ID));

        data_save.filament[AMS_num][read_num].color_R = printer_data_long.datas[10];
        data_save.filament[AMS_num][read_num].color_G = printer_data_long.datas[11];
        data_save.filament[AMS_num][read_num].color_B = printer_data_long.datas[12];
        data_save.filament[AMS_num][read_num].color_A = printer_data_long.datas[13];

        memcpy(&data_save.filament[AMS_num][read_num].temperature_min, printer_data_long.datas + 14, 2);
        memcpy(&data_save.filament[AMS_num][read_num].temperature_max, printer_data_long.datas + 16, 2);
        memcpy(data_save.filament[AMS_num][read_num].name, printer_data_long.datas + 18, 16);
        Bambubus_set_need_to_save();

        Set_filament_res_type2[0] = AMS_num_real;
        Set_filament_res_type2[1] = read_num;
        Set_filament_res_type2[2] = 0x00;
        data.datas = Set_filament_res_type2;
        data.data_length = sizeof(Set_filament_res_type2);

        data.package_number = printer_data_long.package_number;
        data.type = printer_data_long.type;
        data.source_address = printer_data_long.target_address;
        data.target_address = printer_data_long.source_address;
        Bambubus_long_package_send(&data);
    }
    else
    {
        filament_res[2] = 0x08;
        filament_res[3] = AMS_num;
        filament_res[4] = read_num;
        filament_res[5] = 0xE0; // bmcu_reset

        if (sw2 == 0xD1)
        {
            filament_res[6] = 0xD1; // reset meter       白色
            reset_filament_meters(AMS_num, read_num);
            my_printf("(bmcu) 重置耗材里程: Bmcu%d-%d_reset_meter", AMS_num, read_num);
        }
        else if (sw2 == 0xD3)
        {
            filament_res[6] = 0xD3; // 棕色  --电机退料时间设定
            my_printf("(bmcu) 电机退料时间设定: Bmcu%d-%d_motor_time = %ds", AMS_num, read_num, motor_time[read_num]);
        }
        else if (sw2 == 0xD5)
        {
            filament_res[6] = 0xD5; // 岩石灰  --电机pwm 设定
            my_printf("(bmcu) 电机pwm设定: Bmcu%d-%d_pwm_zero = %d", AMS_num, read_num, (pwm_zero[read_num] * 10));
        }
        else if (sw2 == 0xD7)
        {
            filament_res[6] = 0xD7; // 灰色  --电机pwm 设定
            my_printf("(bmcu) 电机pwm设定: Bmcu%d-%d_pwm_zero = %d", AMS_num, read_num, (pwm_zero[read_num] * 10));
        }
        else if (sw2 == 0xD9)
        {
            filament_res[6] = 0xD9; // 选中激活为onuse    黑色
            my_printf("(bmcu) 选中激活为onuse: Bmcu%d-%d_onuse", AMS_num, read_num);
        }
        Bmcu_package_send_with_crc(filament_res, sizeof(filament_res));
    }
}
package_type BambuBus_stu()
{
    return bambu_stu;
}

void Bmcu_run()
{
    if (Bmcu_have_data)
    {
        Bmcu_have_data = 0;
        // delay(1);
        get_C_data(buf_Bmcu, buf_Bmcu[1]);
        if (buf_Bmcu[0] == 0x7D)
        {
            uint8_t AMS_num = buf_Bmcu[2];
            uint8_t read_num = buf_Bmcu[3];
            uint8_t bmcu_online = 0x55;
            if (AMS_num == AMS_num_max)
            {
                AMS_num_max = AMS_num + 1; // 自动添加轮询数
            }
            float meters = 0;
            memcpy(&meters, buf_Bmcu + 8, 4);
            bmcu_online = buf_Bmcu[12];
            for (int i = 0; i < 4; i++)
            {
                slave_pull_statu[AMS_num][i] = buf_Bmcu[i + 4] & 0XF0;
                _filament *filament = &data_save.filament[AMS_num][i];
                if ((buf_Bmcu[i + 4] & 0X0F) == 0x00)
                {
                    if (data_save.BambuBus_now_filament_num == (AMS_num * 4 + i))
                    {
                        filament_res[2] = 0x08;
                        filament_res[3] = AMS_num;
                        filament_res[4] = i;
                        filament_res[5] = 0x00;
                        if (filament->motion_set == on_use)
                        {
                            filament_res[6] = 0xD9;                                         // 选中激活为onuse
                            Bmcu_package_send_with_crc(filament_res, sizeof(filament_res)); // 发送选中激活为onuse
                            my_printf("(bmcu) 自动选中 Bmcu%d-%d 激活为onuse", AMS_num, i);
                        }
                        else if (filament->motion_set == need_pull_back || (filament->motion_set == idle && GET_MC_Online_stu() > 1))
                        {
                            filament_res[6] = 0xB9;                                         //--指定通道need_pull_back
                            Bmcu_package_send_with_crc(filament_res, sizeof(filament_res));
                            my_printf("(bmcu) 自动选中 Bmcu%d-%d 激活为pullback", AMS_num, i);
                        }
                        else if (filament->motion_set == need_send_out)
                        {
                            filament_res[6] = 0xC9;                                         //--指定通道need_send_out
                            Bmcu_package_send_with_crc(filament_res, sizeof(filament_res));
                            my_printf("(bmcu) 自动选中 Bmcu%d-%d 激活为sendout", AMS_num, i);
                        }
                    }
                    else
                    {
                        filament->motion_set = idle;
                    }
                    motion_temp[AMS_num][i] = idle;
                }
                else if ((buf_Bmcu[i + 4] & 0X0F) == 0x01)
                {
                    if (motion_temp[AMS_num][i] == need_pull_back)
                    {
                        filament->motion_set = motion_temp[AMS_num][i];
                    }
                    motion_temp[AMS_num][i] = need_pull_back;
                }
                else if ((buf_Bmcu[i + 4] & 0X0F) == 0x02)
                {
                    if (motion_temp[AMS_num][i] == need_send_out && filament->motion_set != on_use)
                    {
                        filament->motion_set = motion_temp[AMS_num][i];
                    }
                    motion_temp[AMS_num][i] = need_send_out;
                }
                else if ((buf_Bmcu[i + 4] & 0X0F) == 0x03)
                {
                    if (motion_temp[AMS_num][i] == pre_pull)
                    {
                    }
                    motion_temp[AMS_num][i] = pre_pull;
                }
                else if ((buf_Bmcu[i + 4] & 0X0F) == 0x04)
                {
                    if (motion_temp[AMS_num][i] == on_use)
                    {
                        filament->motion_set = motion_temp[AMS_num][i];
                    }
                    motion_temp[AMS_num][i] = on_use;
                }
                if (bmcu_online & (0x01 << (2 * i)))
                {
                    filament->statu = online;
                    statu_temp[AMS_num][i] = 0;
                }
                else if (filament->statu != offline)
                {
                    if (statu_temp[AMS_num][i] > 20)
                        filament->statu = offline;
                    if (filament->motion_set == idle || GET_MC_Online_stu() == 0)
                        statu_temp[AMS_num][i] += 1; // 离线状态计数
                }
            }
            if (bmcu_online & 0xAA)
            {
                motor_unready |= (0x01 << AMS_num);
            }
            else
            {
                motor_unready &= ~(0x01 << AMS_num);
            }
            if (read_num < 4)
            {
                if (bambus_onflush)
                {
                    // data_save.filament[AMS_num][read_num].meters = max(meters, data_save.filament[AMS_num][read_num].meters);
                }
                else if (data_save.filament[AMS_num][read_num].meters < 0 || data_save.filament[AMS_num][read_num].meters > 350)
                {
                    data_save.filament[AMS_num][read_num].meters = 0;
                }
            }
        }
    }
}
package_type BambuBus_run()
{
    package_type stu = BambuBus_package_NONE;
    static uint64_t time_set = 0;
    static uint64_t time_motion = 0;
    static uint64_t time_long_motion = 0;
    update_time();
    uint64_t timex = get_time64();
    Bmcu_run();
    if (BambuBus_have_data)
    {
        int data_length = BambuBus_have_data;
        BambuBus_have_data = 0;
        need_debug = false;
        get_C_data(buf_X, data_length);
        stu = get_packge_type(buf_X, data_length); // have_data
        // vTaskDelay(pdMS_TO_TICKS(1));
        if (!catch_mode)
        {
            switch (stu)
            {
            case BambuBus_package_heartbeat:
                send_for_Hit(buf_X, data_length, timex);
                time_set = timex + 1000;
                break;
            case BambuBus_package_filament_motion_short:
                send_for_motion_short(buf_X, data_length);
                time_motion = timex + 1000;
                break;
            case BambuBus_package_filament_motion_long:
                // DEBUG_num(buf_X, data_length);
                send_for_motion_long(buf_X, data_length);
                time_long_motion = timex + 1000;
                break;
            case BambuBus_package_online_detect:
                send_for_online_detect(buf_X, data_length);
                break;
            case BambuBus_package_REQx6:
                // send_for_REQx6(buf_X, data_length);
                break;
            case BambuBus_long_package_MC_online:
                send_for_long_packge_MC_online(buf_X, data_length);
                break;
            case BambuBus_long_package_filament:
                send_for_long_packge_filament(buf_X, data_length);
                break;
            case BambuBus_long_package_set_filament:
                send_for_long_packge_set_filament(buf_X, data_length);
                break;
            case BambuBus_long_package_version:
                send_for_long_packge_version(buf_X, data_length);
                break;
            case BambuBus_package_NFC_detect:
                // send_for_NFC_detect(buf_X, data_length);
                break;
            case BambuBus_package_set_filament:
                send_for_set_filament(buf_X, data_length);
                break;
            case BambuBus_package_a0:
                // send_for_a0(buf_X, data_length);
                break;
            case BambuBus_package_0237:
                send_for_0237(buf_X, data_length);
                break;
            case BambuBus_package_023c:
                send_for_023c(buf_X, data_length);
                break;
            default:
                break;
            }
        }
    }

    if (timex > time_set)
    {
        stu = BambuBus_package_ERROR; // offline
    }
    else
    {
        bambus_error = false;
    }
    if (!catch_key && !bambus_error && timex > time_set - 500)
    {
        my_printf("BambuBus_package_ERROR: %d", bambu_stu);
        catch_key = 450;
        bambus_error = true;
    }
    if (timex > time_long_motion)
    {
        // set_filament_motion(get_now_filament_num(), idle);
        //  if (timex < time_set)
        //  my_printf("(bmcu) Bambubus已检测到DXX回应超时!!!");
    }
    if (timex > time_motion)
    {
        // if (timex < time_set)
        // my_printf("(bmcu) Bambubus已检测到CXX回应超时!!!");
        if (bambus_onflush)
        {
            my_printf("(bmcu) Bambubus未检测到冲刷状态,冲刷完成");
            bambus_onflush = false;
        }
    }
    else
    {
        if (!bambus_onflush)
        {
            my_printf("(bmcu) Bambubus已检测到冲刷状态,已设置为打印状态");
            bambus_onflush = true;
            bambubus_save_flag = true;
        }
    }
    if (Bambubus_need_to_save)
    {
        if (save_count == 20)
        {
            if (BambuBus_not_on_print())
            {
                Bambubus_save();
                time_set = timex + 1000;
                motor_reboot_flag = true;
                my_printf("(bmcu) Bambubus已保存");
            }
            else
            {
                save_count -= 10; // 20min后重试
            }
        }
    }
    // HAL_UART_Transmit(&use_Serial.handle,&s,1,1000);

    // NFC_detect_run();
    if (!catch_mode)
        bambu_stu = stu;
    else
        bambu_stu = BambuBus_package_heartbeat;
    return stu;
}

int get_AMS_num_max()
{
    return AMS_num_max;
}
bool Bambus_onflush()
{
    return bambus_onflush;
}

/*
String Bmcu_set_json(int ams_num, int i)
{
    char colorBuf[20];
    String name = data_save.filament[ams_num][i].name;
    String id = data_save.filament[ams_num][i].ID;
    char motion = data_save.filament[ams_num][i].motion_set;
    char meter[10];
    sprintf(meter, "%6.1f", data_save.filament[ams_num][i].meters);
    char statu = data_save.filament[ams_num][i].statu;
    unsigned char color[3];
    color[0] = data_save.filament[ams_num][i].color_R;
    color[1] = data_save.filament[ams_num][i].color_G;
    color[2] = data_save.filament[ams_num][i].color_B;
    sprintf(colorBuf, "#%02X%02X%02X", color[0], color[1], color[2]);
    // String json = ("{\"name\":\"" +name +"\",\"color\":\"" +color +"\",\"meter\":\"" +meter +"\",\"motion\":\"" +motion +"\",\"statu\":\"" +statu +"\"}");
    String json = ("{\"name\":\"" + name + "\",\"color\":\"" + (String)colorBuf + "\",\"meter\":\"" + (String)meter + "\"}");
    // sprintf(jsonBuf,"{ name : %s , color : %X , meter : %f}",name,color,meter);

    return json;
}*/
uint16_t get_tay_color(uint8_t num)
{
    uint8_t AMS_num = num / 4;
    uint8_t read_num = num % 4;
    uint8_t r, g, b;
    r = data_save.filament[AMS_num][read_num].color_R;
    g = data_save.filament[AMS_num][read_num].color_G;
    b = data_save.filament[AMS_num][read_num].color_B;
    return (uint16_t)((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

String Bmcu_set_json(int ams_num, int i)
{
    const auto &filament = data_save.filament[ams_num][i];
    String on_use = (filament.motion_set != idle) ? "true" : "false";
    char colorBuf[20];
    sprintf(colorBuf, "#%02X%02X%02X",
            filament.color_R,
            filament.color_G,
            filament.color_B);
    char meterBuf[10];
    float meters = filament.meters;
    meters = meters / 350 * 100; // 转换为百分比显示
    if (meters > 95.0f)
    {
        meters = 95.0f;
    }
    if (isnan(meters) || isinf(meters))
    {
        meters = 0.0f;
    }
    sprintf(meterBuf, "%6.1f", meters);
    String json = ("{\"onuse\":\"" + on_use + "\",\"color\":\"" + (String)colorBuf + "\",\"meter\":\"" + (String)meterBuf + "\"}");
    return json;
}
