#include "Motion_control.h"

/******************************     电机控制接口       *******************************/
#define Motor_H_pin 17
#define Motor_L_pin 18
MotorMCPWMConfig hw{Motor_H_pin, Motor_L_pin, -1, MCPWM_UNIT_0, MCPWM_TIMER_0, MCPWM0A, MCPWM0B};
Motor motor;

/******************************     AS5600 角度传感器接口       *******************************/
AS5600 as5600;
#define AS5600_PI 3.1415926535897932384626433832795
#define speed_filter_k 10
float speed_as5600 = 0;

/******************************     控制相关变量       *******************************/
float MC_PULL_stu_raw = 0;
//  -2-1.3过低  -1-1.45低 0 正常 1-1.8高 2-1.9过高
int MC_PULL_stu = 0;
float MC_ONLINE_key_stu_raw = 0;
// 0-离线 1-在线单微动触发 2-双微动触发 3-抖动
uint8_t MC_ONLINE_key_stu = 3;
float H_PULL_stu_raw = 0;
// 30(-2) 低 40(-1)正常低 60(1)正常高 85(2)高
int H_PULL_stu = 0;
int motor_pwm = 0;
// 电压控制相关常量
float PULL_voltage_up = 1.80f;   // 状态 压力高 红灯
float PULL_voltage_down = 1.45f; // 状态 压力低 蓝灯
// 微动触发控制相关常量
float MC_PULL_voltage_pull = 1.60f; // 压力平衡点 1.60
// bool Assist_send_filament[4] = {false, false, false, false};
//  bool pull_state_old = false; // 上次触发状态——True：未触发，False：进料完成
//  bool is_backing_out = false;
// uint64_t Assist_filament_time[4] = {0, 0, 0, 0};
uint64_t Assist_send_time = 3000; // 仅触发外侧后，送料时长
// 退料距离 单位 MM
// float_t P1X_OUT_filament_meters = 200.0f;                  // 内置200mm 外置700mm
float last_total_distance = 0.0f; // 每个耗材使用的距离
// bool filament_channel_inserted[4]={false,false,false,false};//通道是否插入
void MC_IO_read()
{
    static float MC_pull_old = 0;
    static float MC_ONLINE_old = 0;
    static float H_PULL_old = 0;
    auto voltage = ADC_read();
    if (voltage.first > 1.0f)
        MC_PULL_stu_raw = voltage.first * 0.5f + MC_pull_old * 0.5f;          // 滤波处理
    MC_ONLINE_key_stu_raw = voltage.second * 0.5f + MC_ONLINE_old * 0.5f; // 滤波处理
    H_PULL_stu_raw = Buf_pwm_read() * 0.5f + H_PULL_old * 0.5f;           // 滤波处理
    MC_pull_old = MC_PULL_stu_raw;
    MC_ONLINE_old = MC_ONLINE_key_stu_raw;
    H_PULL_old = H_PULL_stu_raw;
}
uint8_t GET_MC_Online_stu()
{
    return MC_ONLINE_key_stu;
}
float GET_MC_PULL_raw()
{
    return MC_PULL_stu_raw;
}
String Motion_get_status() 
{    
    return String(MC_PULL_stu_raw) + "++" + String(MC_ONLINE_key_stu_raw) + "++" + String(H_PULL_stu_raw);
}
void MC_PULL_ONLINE_read()
{
    MC_IO_read();
    if (MC_PULL_stu_raw > 1.9f) // 大于2V,表示压力过高
    {
        MC_PULL_stu = 2;
        LED_setColor(2, 0xFF, 0x00, 0x00); // 红灯
    }
    else if (MC_PULL_stu_raw < 1.3f) // 小于1.3V，表示压力过低
    {
        MC_PULL_stu = -2;
        LED_setColor(2, 0x00, 0x00, 0xFF); // 蓝灯
    }
    else if (MC_PULL_stu_raw > PULL_voltage_up) // 大于1.80V,表示压力高
    {
        MC_PULL_stu = 1;
        LED_setColor(2, 0xFF, 0xFF, 0x00); // 黄灯
    }
    else if (MC_PULL_stu_raw < PULL_voltage_down) // 小于1.45V，表示压力低
    {
        MC_PULL_stu = -1;
        LED_setColor(2, 0x00, 0xFF, 0xFF); // 青灯
    }
    else // 1.45~1.80之间，在正常缓冲范围内按线性位置反馈
    {
        MC_PULL_stu = 0;
        LED_setColor(2, 0x00, 0xFF, 0x00); // 绿灯
    }

    /*在线状态*/
    // 双微动
    if (MC_ONLINE_key_stu_raw < 0.4f)
    { // 小于则离线.
        MC_ONLINE_key_stu = 0;
    }
    else if ((MC_ONLINE_key_stu_raw < 1.8f) & (MC_ONLINE_key_stu_raw > 1.4f))
    { // 仅触发1个微动，需辅助进料
        MC_ONLINE_key_stu = 1;
    }
    else if (MC_ONLINE_key_stu_raw > 1.8f)
    { // 双微动同时触发, 在线状态
        MC_ONLINE_key_stu = 2;
    }
    else if (MC_ONLINE_key_stu_raw < 1.4f)
    { // 仅触发内侧微动 , 需确认是缺料还是抖动.
        MC_ONLINE_key_stu = 3;
    }

    // 缓冲压力pwm读取
    if (H_PULL_stu_raw > 85.0f) // 大于85.0，表示压力过高
    {
        H_PULL_stu = 2;
    }
    else if (H_PULL_stu_raw < 30.0f) // 小于30.0，表示压力过低
    {
        H_PULL_stu = -2;
    }
    else if (H_PULL_stu_raw > 60.0f) // 大于60.0，表示压力高
    {
        H_PULL_stu = 1;
    }
    else if (H_PULL_stu_raw < 40.0f) // 小于40.0，表示压力低
    {
        H_PULL_stu = -1;
    }
    else // 40~80之间，在正常缓冲范围内按线性位置反馈
    {
        H_PULL_stu = 0;
    }
}

#define PWM_lim 880

class MOTOR_PID
{
public:
    float P = 1.5;
    // float I = 1;
    float I = 10;
    float D = 0;
    // float D = 0.008;
    float I_save = 0;
    float E_last = 0;
    float pid_MAX = PWM_lim;
    float pid_MIN = -PWM_lim;
    float pid_range = (pid_MAX - pid_MIN) / 2;
    void init(float P_set, float I_set, float D_set)
    {
        P = P_set;
        I = I_set;
        D = D_set;
        I_save = 0;
    }
    float caculate(float E, float time_E)
    {

        float I_save_set = (I_save + E * time_E);
        if ((abs(I * I_save_set) < pid_range / 2)) // 对I限幅
            I_save = I_save_set;                   // 线性I系数

        float ouput_buf = P * (E + I * (I_save) + D * (E - E_last) / time_E);
        if (ouput_buf > pid_MAX)
            ouput_buf = pid_MAX;
        if (ouput_buf < pid_MIN)
            ouput_buf = pid_MIN;

        E_last = E;
        return ouput_buf;
    }
    void clear()
    {
        I_save = 0;
        E_last = 0;
    }
};
class _MOTOR_CONTROL
{
public:
    int motion = 0;
    int pwm_zero = 380;
    uint64_t motor_stop_time = 0;
    MOTOR_PID PID;

    _MOTOR_CONTROL()
    {
        PID.init(1.5, 10, 0);
        motor_stop_time = 0;
        motion = 0;
    }
    void set_motion(int _motion, uint64_t over_time)
    {
        uint64_t time_now = get_time64();
        motor_stop_time = time_now + over_time;
        motion = _motion;
    }
    void set_motion_add(int _motion, uint64_t over_time)
    {
        motor_stop_time += over_time;
        motion = _motion;
    }
    int get_motion()
    {
        return motion;
    }
    void set_pwm_zero(int _pwm_zero)
    {
        pwm_zero = _pwm_zero;
    }
    void run()
    {
        uint64_t time_now = get_time64();
        static uint64_t time_set_speed = 0;
        static uint64_t time_last = 0;
        float speed_set = 0;
        uint8_t CHx = get_now_filament_num();
        if (time_now >= motor_stop_time)
        {
            motion = 0;
        }
        if ((motion == 99 || motion == 0)) // 刹车
        {
            speed_set = 0;
            PID.clear();
            Motion_control_set_PWM(0);
            time_last = time_now;
            return;
        }
        else if (motion == 1) // send 370 40  130 15
        {
            speed_set = 40;
        }
        else if (motion == 2 || motion == 3) // over pressure
        {
            speed_set = (80 - H_PULL_stu_raw) * 1; // 线性压力反馈
            if (speed_set < 0 && speed_set > -5)      // 防止电机抖动
                speed_set = 0;
        }
        else if (motion == -2) //  prepull
        {
            speed_set = (30 - H_PULL_stu_raw) * 0.75; // 线性压力反馈
            if (speed_set < 5 && speed_set > 0)      // 防止电机抖动
                speed_set = 0;
        }
        else if (motion == -1) // pull 370 70 130 18
        {
            speed_set = -60;
        }
        else if (motion == 66) // onuse pressure
        {
            speed_set = (70 - H_PULL_stu_raw) * 0.75; // 线性压力反馈
            if (speed_set < 0 && speed_set > -5)      // 防止电机抖动
                speed_set = 0;
        }
        else if (motion == -66) // pull on hall
        {
            speed_set = (MC_PULL_stu_raw - 1.9f) * 150; // 线性压力反馈
            if (speed_set > 0)                         // 防止电机抖动
                speed_set = 0;
            if (MC_PULL_stu == -2 && MC_ONLINE_key_stu == 1)
                speed_set = -10;
        }
        float x = PID.caculate(speed_set - speed_as5600, (float)(time_now - time_last) / 1000);
        if (x > 5)
            x += pwm_zero;
        else if (x < -5)
            x -= pwm_zero;
        else
            x = 0;
        if (x > PWM_lim)
            x = PWM_lim;
        if (x < -PWM_lim)
            x = -PWM_lim;
        if (speed_as5600 > 0.2 || motion == 0 || speed_as5600 < -0.2 || time_set_speed < time_now - 2000)
        {
            time_set_speed = time_now + 2000;
        }
        if (time_set_speed < time_now && time_set_speed != 0)
        {
            if (x > 820 || x < -820)
                x = 0; // 防止电机卡死过热
        }
        Motion_control_set_PWM(x);
        time_last = time_now;
    }
};
_MOTOR_CONTROL MOTOR_CONTROL;

void Motion_control_set_PWM(int PWM)
{
    motor_pwm = PWM;
    if (MC_ONLINE_key_stu == 0)
    {
        //ledcWrite(1, 0);
        //ledcWrite(3, 0);
        motor.setFreewheel();
        return;
    }
    if (PWM == 0)
    {
        //ledcWrite(1, 255);
        //ledcWrite(3, 255);
        motor.setFreewheel();
    }
    else if (PWM > 0)
    {
        //ledcWrite(1, PWM / 4);
        //ledcWrite(3, 0);
        motor.setSpeed(PWM, Dir::CW);
    }
    else if (PWM < 0)
    {
        //ledcWrite(1, 0);
        //ledcWrite(3, -PWM / 4);
        motor.setSpeed(-PWM, Dir::CCW);
    }
}

void Motor_init()
{
    //pinMode(Motor_H_pin, OUTPUT);
    //pinMode(Motor_L_pin, OUTPUT);
    motor.setup(hw);
    motor.reconfigureFrequency(100000);
    motor.setFreewheelMode(FreewheelMode::HiZ_Awake);
    motor.start();
    /*
    ledcSetup(1, 100000, 8);       // 100kHz, 8-bit
    ledcAttachPin(Motor_H_pin, 1); // 将Motor_H_pin引脚连接到通道1
    ledcWrite(1, 0);               // 初始占空比为 0
    ledcSetup(3, 100000, 8);       // 100kHz, 8-bit
    ledcAttachPin(Motor_L_pin, 3); // 将Motor_L_pin引脚连接到通道3
    ledcWrite(3, 0);               // 初始占空比为 0
    */

}
void AS5600_distance_updata()
{
    static int32_t distance_save = 0;
    static uint64_t time_last = 0;
    uint64_t time_now = get_time64();
    uint8_t filament_num = get_now_filament_num();
    if (as5600.updateRawAngleAsync() == false)
        return;
    int32_t cir_E = 0;
    int32_t last_distance = distance_save;
    int32_t now_distance = as5600.getRawAngleResult();
    float distance_E;
    if ((now_distance > 3072) && (last_distance <= 1024))
    {
        cir_E = -4096;
    }
    else if ((now_distance <= 1024) && (last_distance > 3072))
    {
        cir_E = 4096;
    }

    distance_E = (float)(now_distance - last_distance + cir_E) * AS5600_PI * 7.5 / 4096; // D=7.5mm
    distance_save = now_distance;
    float T = (float)(time_now - time_last);
    float speedx = distance_E / T * 1000;
    T = speed_filter_k / (T + speed_filter_k);
    speed_as5600 = speedx * (1 - T) + speed_as5600 * T; // mm/s
    if (MC_ONLINE_key_stu == 2 || (distance_E > 0 && MC_ONLINE_key_stu > 0))
    {
        add_filament_meters(filament_num, distance_E / 1000); 
        last_total_distance += distance_E; // mm               
    }
    time_last = time_now;
}
void motorTask(void *pvParameters)
{
  while (1)
  {
    AS5600_distance_updata();  //异步刷新测速
    vTaskDelay(pdMS_TO_TICKS(5));
    Motion_control_run(0);
    vTaskDelay(pdMS_TO_TICKS(5)); // 每10ms调用一次
  }
}
void setup_motor_task()
{
  BaseType_t motorResult = xTaskCreate(motorTask, "Motor Task", 8192, NULL, 2, NULL);
  if (motorResult != pdPASS)
  {
    ESP_LOGE("(rs485)", "Failed to create Motor Task");
  }
}
void Motion_control_init()
{
    MC_PULL_ONLINE_read();
    as5600.begin();
    my_printf("(AS5600) AS5600 MagnetStatus: %d", as5600.getMagnetStatus());
    Motor_init();
    setup_motor_task();
}

uint8_t pullcheck[4] = {0, 0, 0, 0}; // 当前bmcu通道使用标记
uint8_t lastnum = 0;

bool Position_check()
{
    if (MC_ONLINE_key_stu > 0)
    {
        return true;
    }
    else
    {
        return false;
    }
}
void motor_motion_run()
{
    uint8_t num = get_now_filament_num();
    if (get_filament_online(num) || MC_ONLINE_key_stu)
    {
        switch (get_filament_motion(num))
        {
        case need_send_out:
            LED_setColor(0, 0x00, 0xFF, 0x00); // 绿灯
            if (H_PULL_stu < 1)
            {
                MOTOR_CONTROL.set_motion(1, 100);
            }
            else
            {
                MOTOR_CONTROL.set_motion(3, 500);
            }
            pullcheck[num] = 1;
            last_total_distance = 0;            
            break;
        case need_pull_back:
            LED_setColor(0, 0xFF, 0x00, 0xFF); // 紫色
            if (MC_ONLINE_key_stu > 1)
            {
                MOTOR_CONTROL.set_motion(-66, 100);
            }
            else if (MC_PULL_stu < 1)
            {
                MOTOR_CONTROL.set_motion(-1, 100);
            }            
            break;
        case on_use:
            LED_setColor(0, 0xFF, 0xFF, 0xFF); // 白色
            if (MOTOR_CONTROL.get_motion() == 1 || MOTOR_CONTROL.get_motion() == 3)
            {
                MOTOR_CONTROL.set_motion(2, 2000); // 保持压力延迟2s
            }
            else if (MOTOR_CONTROL.get_motion() != 2 || H_PULL_stu < 0)
            {
                MOTOR_CONTROL.set_motion(66, 100);
                pullcheck[num] = 0;                    
            }
            break;
        case pre_pull:
            LED_setColor(0, 0xFF, 0x00, 0xFF); // 紫色
            if (pullcheck[num] == 1)
            {
                MOTOR_CONTROL.set_motion(2, 2000);
                break;
            }
            if (H_PULL_stu > -1 && MC_PULL_stu < 2)
            {
                MOTOR_CONTROL.set_motion(-2, 100); // 退料时间调整
            }
            else
            {
                MOTOR_CONTROL.set_motion(0, 100);
            }
            break;
        case idle:
            LED_setColor(0, 0x00, 0x00, 0xFF); // 蓝灯
            if (MC_ONLINE_key_stu > 0)
            {
                if (H_PULL_stu == 2 || motor_unready)
                {
                    MOTOR_CONTROL.set_motion(-66, 100);
                }
                else
                {
                    MOTOR_CONTROL.set_motion(0, 100);
                }
            }
            break;
        }
    }
    else
    {
        MOTOR_CONTROL.set_motion(0, 100);
    }
    /*
    if (!select && MOTOR_CONTROL[num].get_motion() < 0)  //非选中bmcu且在退料时停机
    {
        Pullcheck_set(num, 2);
        MOTOR_CONTROL[num].set_motion(0, 100);
    }
    */

    MOTOR_CONTROL.run();
}

void Motion_control_run(int error)
{
    MC_PULL_ONLINE_read();
    AS5600_distance_updata();
    motor_motion_run();    
    if (error)
    {
        //MOTOR_CONTROL.set_motion(0, 100);
    }
}
