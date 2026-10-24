#include "485.h"
#define s3v3 true   // 默认为v2

#define Bambu_RX_PIN 44
#define Bambu_TX_PIN 43
#define Bambu_BAUD_RATE 1250000
#ifdef s3v3
#define Bambu_RTS_PIN 2
#define BMCU_RX_PIN 15
#define BMCU_TX_PIN 16
#define BMCU_RTS_PIN 1
#else
#define Bambu_RTS_PIN 15
#define BMCU_RX_PIN 18
#define BMCU_TX_PIN 17
#define BMCU_RTS_PIN 16
#endif // version

#define TASK_STACK_SIZE (8192)
#define RX_BUFFER_SIZE 1024
MyRingBuffer<RX_BUFFER_SIZE> rxBuffer0;
MyRingBuffer<RX_BUFFER_SIZE> rxBuffer1;

// 单一通信任务：接收、协议处理、Bambu/BMCU 发送、BMCU 周期帧都在这里完成。
void serialTask(void *parameter)
{
    uint32_t next_bmcu = millis() + 100;
    bool send_heart = true;
    for (;;)
    {
        while (rxBuffer0.available())
        {
            uint8_t c = rxBuffer0.read();
            RX_IRQ(c);
        }
        BambuBus_run();

        uint32_t now = millis();
        if ((int32_t)(now - next_bmcu) >= 0)
        {
            if (send_heart)
                bmcusend_for_Heart();
            else
                bmcusend_for_motion();
            send_heart = !send_heart;
            next_bmcu = now + 100;
        }

        while (rxBuffer1.available())
        {
            uint8_t d = rxBuffer1.read();
            RX_BMCU(d);
        }
        vTaskDelay(pdMS_TO_TICKS(1));
    }
}
void Readuart()
{
    while (Serial0.available())
    {
        uint8_t c = Serial0.read();
        rxBuffer0.write(c);
        //RX_IRQ(c);
    }
    while (Serial1.available())
    {
        uint8_t d = Serial1.read();
        rxBuffer1.write(d);
        //RX_BMCU(d);
    }
}

void send_bambu_uart(const unsigned char *data, size_t length)
{
    if (catch_mode || Switch_need_refresh())
        return;
    Serial0.write(data, length);
    vTaskDelay(pdMS_TO_TICKS(1)); // Bambu RS485 换向间隔
    if (catch_key > 200 && !catch_mode)
        get_C_data((uint8_t *)data, length);
}

void BambuBUS_UART_Init()
{
    Serial0.begin(1250000, SERIAL_8E1); //  RX 44   TX  43
    while (!Serial0)
    {
        delay(10);
    }
    Serial0.setPins(-1, -1, -1, Bambu_RTS_PIN);
    Serial0.setMode(UART_MODE_RS485_HALF_DUPLEX);
    // pinMode(Bambu_RTS_PIN, OUTPUT);
    // digitalWrite(Bambu_RTS_PIN, LOW); // 设置RTS引脚为低
    Serial0.onReceive(Readuart); // 串口回调；

    /*
    Serial0.onReceive([]()
                      {
    while (Serial0.available()) {
        uint8_t c = Serial0.read();
        rxBuffer0.write(c);
        //RX_IRQ(c);
    } });    
    */
}

void send_bmcu_uart(const unsigned char *data, size_t length)
{
    Serial1.write(data, length);
    if (catch_key > 200 && !catch_mode)
        get_C_data((uint8_t *)data, length);
}

void BMCU_UART_Init()
{
    Serial1.begin(512000, SERIAL_8E1, BMCU_RX_PIN, BMCU_TX_PIN); //  RX1  18  TX1  17
    while (!Serial1)
    {
        delay(10);
    }
    Serial1.setPins(-1, -1, -1, BMCU_RTS_PIN);
    Serial1.setMode(UART_MODE_RS485_HALF_DUPLEX);

    /*
    Serial1.onReceive([]()
                      {
    while (Serial1.available()) {
        uint8_t c = Serial1.read();
        rxBuffer1.write(c);
        //RX_BMCU(c);
    } });    
    */

}

void start_rs485_task()
{
    BaseType_t serialResult = xTaskCreate(serialTask, "Serial Task", TASK_STACK_SIZE, NULL, 4, NULL);
    if (serialResult != pdPASS)
    {
        ESP_LOGE("(rs485)", "Failed to create Serial Task");
    }
}
void RS485_init()
{
    BMCU_UART_Init();
    BambuBUS_UART_Init();
    // Serial0.onReceive(Bambu_readuart); // 串口回调；
    // Serial1.onReceive(Bmcu_readuart);  // 串口回调；
    delay(100);
    start_rs485_task();
}