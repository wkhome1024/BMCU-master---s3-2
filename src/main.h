#pragma once
#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "stdio.h"
#include "time64.h"
#include "485.h"
#include "BambuBus.h"
#include "Data.h"
#include "switch.h"
#include <PubSubClient.h>
#include <sht30.h>
#include <Adafruit_NeoPixel.h>

#include "WiFiUser.h"
//#include <ArduinoOTA.h>
extern const char *host_name;
#define SYS_RGB 8    // RGB灯针脚
#define ledPixels 3  //led数量
extern int catch_key; // 抓包开关
extern bool catch_mode;             // 抓包模式
#define EN_log true                          // 日志开关



#define delay_any_us(time)\
{\
    const uint64_t _delay_any_div_time =(uint64_t)(8000000.0/time);\
    SysTick->SR &= ~(1 << 0);\
    SysTick->CMP = SystemCoreClock/_delay_any_div_time;\
    SysTick->CTLR |= (1 << 5) |(1 << 4)| (1 << 0);\
\
    while(!(SysTick->SR & 1));\
    SysTick->CTLR &= ~(1 << 0);\
}

#define delay_any_ms(time)\
{\
    const uint64_t _delay_any_div_time =(uint64_t)(80000.0/time);\
    SysTick->SR &= ~(1 << 0);\
    SysTick->CMP = SystemCoreClock/_delay_any_div_time;\
    SysTick->CTLR |= (1 << 5) |(1 << 4)| (1 << 0);\
\
    while(!(SysTick->SR & 1));\
    SysTick->CTLR &= ~(1 << 0);\
}
//extern void RGB_set(unsigned char CHx,unsigned char R, unsigned char G, unsigned char B);

//#include "AMCU.h"
