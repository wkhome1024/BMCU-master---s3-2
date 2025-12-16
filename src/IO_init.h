#pragma once
#include <Wire.h>
#include <main.h>
//#include "pwm_analyzer.h"

//#include <Adafruit_GFX.h>      
//#include <Adafruit_ST7735.h> 
//#include <TFT_eSPI.h> 
#define Addr_SHT30 0x44
#define SDA_PIN 41   
#define SCL_PIN 42

#define EN_24  39
#define FAN_PIN  40
#define FAN_channel  7

#define Bufio_pin 38

#define Pull_pin 5
#define Online_pin 4


extern float pull_voltage;
extern float online_voltage;

extern float Buf_pwm_read();
extern std::pair<float, float> ADC_read();
extern void Sht30_init();
extern std::pair<float, float> Sht30_read();
extern String Sht30_read_mqtt();
//extern void tft_init();
//extern void tft_print(bool flag);
extern bool Temp_read(int temp1);
extern void Set_fan_t(int temp1);
extern void IO_init();
extern uint8_t sw_read(); 
extern void Set_24(bool t);
extern void Set_fan(bool enable);
extern bool enable_24();

