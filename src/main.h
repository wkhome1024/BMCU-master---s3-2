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
#include <IO_init.h>
#include <Adafruit_NeoPixel.h>
#include "Motion_control.h"
#include <AS5600.h>
#include "WiFiUser.h"
//#include <ArduinoOTA.h>
extern char host_name[20];
extern uint8_t hub_num;
extern uint8_t save_count;
extern int catch_key; // 抓包开关
extern bool catch_mode;             // 抓包模式
extern bool server_key;
extern uint8_t mqtt_status;
#define EN_log true                          // 日志开关

extern void LED_setColor(uint8_t num, uint8_t r, uint8_t g, uint8_t b);


//extern void RGB_set(unsigned char CHx,unsigned char R, unsigned char G, unsigned char B);

//#include "AMCU.h"
