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
extern int save_count;
extern int catch_key; // 抓包开关
extern bool catch_mode;             // 抓包模式
extern bool server_key;
#define EN_log true                          // 日志开关




//extern void RGB_set(unsigned char CHx,unsigned char R, unsigned char G, unsigned char B);

//#include "AMCU.h"
