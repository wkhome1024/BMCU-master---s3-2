#pragma once

#include "main.h"
#include <ESP32_MCPWM.h>

extern void Motion_control_init();
extern void Motion_control_set_PWM(int PWM);
extern void Motion_control_run(int error);
extern int MC_ONLINE_key_stu;
extern int MC_PULL_stu;
extern float MC_PULL_stu_raw;

