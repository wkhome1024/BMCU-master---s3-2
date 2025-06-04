#pragma once
#include <Wire.h>
#include <main.h>
#include <Adafruit_GFX.h>      
#include <Adafruit_ST7735.h> 
//#include <TFT_eSPI.h> 
#define Addr_SHT30 0x44
#define SDA_PIN 6   
#define SCL_PIN 7

extern void Sht30_init();
extern std::pair<float, float> Sht30_read();
extern String Sht30_read_mqtt();
extern void tft_init();
extern void tft_print(bool flag);


/*
#include <TFT_eSPI.h>
#define TFT_MOSI 13 // In some display driver board, it might be written as "SDA" and so on.
#define TFT_SCLK 12
#define TFT_CS   10  // Chip select control pin
#define TFT_DC   9  // Data Command control pin
#define TFT_RST  14  // Reset pin (could connect to Arduino RESET pin)
#define TFT_BL   21  // LED back-light
*/
