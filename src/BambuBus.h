#pragma once
#include <main.h>

#define Bambubus_version 6

#ifdef __cplusplus
extern "C"
{
#endif

    enum _filament_status
    {
        offline,
        online,
        NFC_waiting
    };
    enum _filament_motion_state_set
    {
        need_pull_back,
        need_send_out,
        on_use,
        pre_pull,
        idle
    };
    enum package_type
    {
        BambuBus_package_ERROR = -1,
        BambuBus_package_NONE = 0,
        BambuBus_package_filament_motion_short,
        BambuBus_package_filament_motion_long,
        BambuBus_package_online_detect,
        BambuBus_package_REQx6,
        BambuBus_package_NFC_detect,
        BambuBus_package_set_filament,
        BambuBus_long_package_MC_online,
        BambuBus_long_package_filament,
        BambuBus_long_package_set_filament,
        BambuBus_long_package_version,
        BambuBus_package_heartbeat,
        BambuBus_package_ETC,
        BambuBus_package_test1,
        BambuBus_package_test2,
        BambuBus_read_cert,
        BambuBus_send_cert_verify,
        BambuBus_cert_datas_sync,

        __BambuBus_package_packge_type_size
    };
    enum BambuBus_device_type
    {
        BambuBus_none = 0x0000,
        BambuBus_AMS = 0x0700,
        BambuBus_AMS_lite = 0x1200,
    };
    extern void BambuBus_init();
    extern package_type BambuBus_run();
    extern package_type BambuBus_stu();
    extern uint8_t Bmcu_have_data;
    extern int BambuBus_have_data;
    extern uint8_t motor_unready;
    extern bool bambubus_save_flag;
#define max_filament_num 4
    extern bool Bambubus_read();
    extern void Bambu_readuart();
    extern void Bmcu_readuart();
    extern void Bambubus_set_need_to_save();
    extern int get_now_filament_num();
    // extern uint16_t get_now_BambuBus_device_type();
    extern void reset_filament_meters(uint8_t AMS_num, uint8_t read_num);
    extern void add_filament_meters(int num, float meters);
    extern void set_filament_meters(int num, float meters);
    extern void set_filament_online(int num, bool if_online);
    extern bool get_filament_online(int num);
    _filament_motion_state_set get_filament_motion(int num);
    extern void set_filament_motion(int num, _filament_motion_state_set motion);
    extern bool Bambus_onflush();
    extern String Bmcu_set_json(int ams_num, int i);
    extern int get_AMS_num_max();
    extern void RX_IRQ(unsigned char _RX_IRQ_data);
    extern void RX_BMCU(unsigned char inChar);
    extern uint16_t get_tay_color(uint8_t num);
#ifdef __cplusplus
}
#endif