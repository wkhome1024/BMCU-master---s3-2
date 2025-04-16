#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "stdio.h"

#define Bambu_RX_PIN 20
#define Bambu_TX_PIN 21
#define Bambu_RTS_PIN 1
#define BMCU_RX_PIN 18
#define BMCU_TX_PIN 19
#define BMCU_RTS_PIN 0


void send_bambu_uart(const unsigned char *data, size_t length)
{
    Serial.write(data, length);
}

void BambuBUS_UART_Init()
{
    Serial.begin(1250000,SERIAL_8E1);        //  RX 20   TX  21  
    while (!Serial) {
        delay(10);
    }
    Serial.setPins(-1, -1, -1, Bambu_RTS_PIN);
    Serial.setMode(UART_MODE_RS485_HALF_DUPLEX);
}


void send_bmcu_uart(const unsigned char *data, size_t length)
{
    Serial1.write(data, length);
}

void BMCU_UART_Init()
{
    Serial1.begin(1250000,SERIAL_8E1);    //  RX1  18  TX1  19
    while (!Serial1) {
        delay(10);
    }
    Serial1.setPins(-1, -1, -1, BMCU_RTS_PIN);
    Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX);
}




#define LED_BUILTIN1 12
#define LED_BUILTIN2 13

void LED_Init()
{
    pinMode(LED_BUILTIN1, OUTPUT);
    pinMode(LED_BUILTIN2, OUTPUT);
    digitalWrite(LED_BUILTIN1, LOW);
    digitalWrite(LED_BUILTIN2, LOW);
}