#include "485.h"

#define Bambu_RX_PIN 44
#define Bambu_TX_PIN 43
#define Bambu_RTS_PIN 15
#define BMCU_RX_PIN 18
#define BMCU_TX_PIN 17
#define BMCU_RTS_PIN 16






void send_bambu_uart(const unsigned char *data, size_t length)
{
    if ((get_time64() < 20000)) 
    {
        Serial0.flush(); // 等待串口0可用
        return; // 如果串口0不可用，则不发送数据
    }
    Serial0.write(data, length);
}

void BambuBUS_UART_Init()
{
    Serial0.begin(1250000,SERIAL_8E1);        //  RX 44   TX  43  
    while (!Serial0) {
        delay(10);
    }
    Serial0.setPins(-1, -1, -1, Bambu_RTS_PIN);
    Serial0.setMode(UART_MODE_RS485_HALF_DUPLEX);
}


void send_bmcu_uart(const unsigned char *data, size_t length)
{
    Serial1.write(data, length);
}

void BMCU_UART_Init()
{
    Serial1.begin(1250000,SERIAL_8E1,BMCU_RX_PIN,BMCU_TX_PIN);    //  RX1  18  TX1  17
    while (!Serial1) {
        delay(10);
    }
    Serial1.setPins(-1, -1, -1, BMCU_RTS_PIN);
    Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX);
}




