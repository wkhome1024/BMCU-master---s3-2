#pragma once
#include <Wire.h>
#include <main.h>
//#include <Adafruit_GFX.h>      
//#include <Adafruit_ST7735.h> 
//#include <TFT_eSPI.h> 
#define Addr_SHT30 0x44
#define SDA_PIN 6   
#define SCL_PIN 7

#define EN_24  5
#define OUT_1  4
#define ONline_1  39
#define ONline_2  40
#define ONline_3  41
#define ONline_4  42




extern void Sht30_init();
extern std::pair<float, float> Sht30_read();
extern String Sht30_read_mqtt();
//extern void tft_init();
//extern void tft_print(bool flag);
extern bool Temp_read(int temp1);
extern void set_fan(int temp1);
extern void IO_init();
extern uint8_t sw_read(); 
extern void set_24(bool t);
extern void set_out1(bool enable);
extern bool enable_24();

