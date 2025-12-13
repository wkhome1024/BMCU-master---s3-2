#pragma once

#include "main.h"
#include <ESP32_MCPWM.h>

extern void Motion_control_init();
extern void Motion_control_set_PWM(int PWM);
extern void Motion_control_run(int error);
extern uint8_t GET_MC_Online_stu();
//extern float MC_ONLINE_key_stu_raw;
extern String Motion_get_status();
extern float GET_MC_PULL_raw();
//extern float H_PULL_stu_raw;
extern float last_total_distance;
extern int motor_pwm;