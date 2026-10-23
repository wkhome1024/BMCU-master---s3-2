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
MyRingBuffer rxBuffer0(RX_BUFFER_SIZE);
MyRingBuffer rxBuffer1(RX_BUFFER_SIZE);

#define RS485_TX_QUEUE_LEN 12
#define RS485_MAX_FRAME 512
enum Rs485Port : uint8_t
{
    RS485_PORT_BAMBU = 0,
    RS485_PORT_BMCU = 1
};
struct Rs485Frame
{
    uint8_t port;
    uint16_t length;
    uint8_t data[RS485_MAX_FRAME];
};
static QueueHandle_t rs485_tx_queue = nullptr;

static bool rs485_enqueue(uint8_t port, const unsigned char *data, size_t length)
{
    if (!rs485_tx_queue || !data || length == 0 || length > RS485_MAX_FRAME)
        return false;
    Rs485Frame frame;
    frame.port = port;
    frame.length = (uint16_t)length;
    memcpy(frame.data, data, length);
    return xQueueSend(rs485_tx_queue, &frame, pdMS_TO_TICKS(2)) == pdTRUE;
}

// 唯一的 RS485 发送任务：串口写入、CRC 之后的帧都从这里发出。
static void rs485TxTask(void *parameter)
{
    Rs485Frame frame;
    for (;;)
    {
        if (xQueueReceive(rs485_tx_queue, &frame, portMAX_DELAY) != pdTRUE)
            continue;
        if (frame.port == RS485_PORT_BAMBU)
        {
            if (catch_mode || Switch_need_refresh())
                continue;
            Serial0.write(frame.data, frame.length);
            vTaskDelay(pdMS_TO_TICKS(1));
        }
        else
        {
            Serial1.write(frame.data, frame.length);
        }
        if (catch_key > 200 && !catch_mode)
            get_C_data(frame.data, frame.length);
    }
}

void serialTask(void *parameter)
{
    for (;;)
    {
        while (rxBuffer0.available())
        {
            uint8_t c = rxBuffer0.read();
            RX_IRQ(c);
        }
        BambuBus_run();
        vTaskDelay(pdMS_TO_TICKS(1));
        while (rxBuffer1.available())
        {
            uint8_t d = rxBuffer1.read();
            RX_BMCU(d);
        }
        vTaskDelay(pdMS_TO_TICKS(1)); // 每1ms调用一次BambuBus_run()
    }
}
void bmcuTask(void *parameter)
{
    for (;;)
    {
        package_type stu = BambuBus_run();
        if (stu != BambuBus_package_NONE)
        {
            // ESP_LOGE("BambuBus", "Processing package type: %d", stu);
        }

        vTaskDelay(pdMS_TO_TICKS(1)); // 每2ms调用一次
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
    rs485_enqueue(RS485_PORT_BAMBU, data, length);
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
    rs485_enqueue(RS485_PORT_BMCU, data, length);
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
    /*
    BaseType_t bmcuResult = xTaskCreate(bmcuTask, "Bmcu Task", TASK_STACK_SIZE, NULL, 2, NULL);
    if (bmcuResult != pdPASS)
    {
        ESP_LOGE("(bmcu)", "Failed to create Bmcu  Task");
    }
    */    
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

    rs485_tx_queue = xQueueCreate(RS485_TX_QUEUE_LEN, sizeof(Rs485Frame));
    if (!rs485_tx_queue)
    {
        ESP_LOGE("(rs485)", "Failed to create TX queue");
    }
    else
    {
        BaseType_t txResult = xTaskCreate(rs485TxTask, "RS485 TX", 4096, NULL, 5, NULL);
        if (txResult != pdPASS)
        {
            ESP_LOGE("(rs485)", "Failed to create TX task");
        }
    }
    start_rs485_task();
}

void bmcu_485_task(void *parameter)
{
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(100));
        bmcusend_for_Heart();
        vTaskDelay(pdMS_TO_TICKS(100));
        bmcusend_for_motion();
    }
}

void bmcu_485_init()
{
    BaseType_t bmcuResult = xTaskCreate(bmcu_485_task, "Bmcu 485 Task", TASK_STACK_SIZE, NULL, 2, NULL);
    if (bmcuResult != pdPASS)
    {
        ESP_LOGE("(bmcu)", "Failed to create Bmcu 485 Task");
    }
}