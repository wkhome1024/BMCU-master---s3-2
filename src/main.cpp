#include <main.h>

// const char *ssid = "";       // WiFi名称
// const char *password = ""; // WiFi密码

// const char *mqtt_server1 = "192.168.10.10"; // MQTT服务器地址
// const int mqtt_port1 = 1883;                // MQTT服务器端口
const char *ha_topic = "bmcu-hub";
const char *logTopic = "bmcu-hub/log";
const char *topic[8] = {"bmcu1", "bmcu2", "bmcu3", "bmcu4", "bmcu5", "bmcu6", "bmcu7", "bmcu8"};
// const char *mqtt_username1 = "";
// const char *mqtt_password1 = "";
const char *host_name = "bmcu-hub-s3"; // 设备主机名
#define product_id "bmcu-hub"          // 产品ID
#define device_id "s3"                 // 设备ID
char mqtt_id[20];
int save_count = 0;
int postMsgId = 0;              // 消息ID初始值为0
int catch_key = 0;              // 抓包计数
bool catch_mode = false;        // 抓包模式
bool server_key = false;        // HTTP服务器开关
WiFiClient espclient;           // 创建一个WiFiClient对象
PubSubClient client(espclient); // 创建一个PubSubClient对象

Adafruit_NeoPixel SYS_leds(ledPixels, SYS_RGB, NEO_GRB + NEO_KHZ800);
void LED_init()
{
  SYS_leds.begin();
  SYS_leds.clear();
  SYS_leds.setBrightness(50);
  SYS_leds.show();
}

bool sw_send = true;
void hub_msg()
{
  // tft_print(catch_mode);
  if (sw_send)
  {
    client.publish(ha_topic, Sht30_read_mqtt().c_str());
    sw_send = !sw_send;
  }
  else
  {
    if (!client.publish(ha_topic, get_filament_map().c_str()))
      client.connect(mqtt_id, mqtt_username.c_str(), mqtt_password.c_str());
    sw_send = !sw_send;
  }
}

#define CHUNK_SIZE 128 // 每次发送的字节数
bool start_log = false;
void publishLogOverMQTT()
{
  if (!EN_log || L_data == NULL)
  {
    my_printf("(MQTT) 日志未启用或 L_data 为空");
    return;
  }

  int logLength = strlen(L_data);
  static int offset = 0;
  int remaining = 0;

  if (offset < logLength)
  {
    remaining = logLength - offset;
  }
  else
  {
    return;
  }
  if (!start_log)
  {
    client.publish(logTopic, "START");
    start_log = true;
    offset = 0;
    return;
  }
  int chunkSize = (remaining > CHUNK_SIZE) ? CHUNK_SIZE : remaining;
  char payload[chunkSize];
  snprintf(payload, sizeof(payload), L_data + offset);
  client.publish(logTopic, payload);
  offset += (chunkSize - 1);
}

void setup()
{

  INIT_DATA();
  BambuBus_init();
  Switch_init();
  Sht30_init();
  LED_init();
  // tft_init();
  IO_init();
  // 检查是否有保存的Wi-Fi配置信息

  // WiFi.setHostname(host_name);
  //  WiFi.begin(ssid, password); // 尝试自动连接上次保存的Wi-Fi
  //   Serial.println("尝试连接已保存的WiFi...");
  checkConnect(Config_read()); // 检查配置Wi-Fi连接
  // 等待连接成功

  if (WiFi.status() == WL_CONNECTED)
  {
    uint8_t mac_ad[6];
    WiFi.macAddress(mac_ad);
    sprintf(mqtt_id, "%s-%02X%02X", host_name, mac_ad[4], mac_ad[5]);
    client.setServer(mqtt_server.c_str(), mqtt_port); // 设置MQTT服务器地址和端口
    client.connect(mqtt_id, mqtt_username.c_str(), mqtt_password.c_str());
    client.publish(ha_topic, "Hi, I'm ESP32 ^^");
    my_printf("(wifi) WiFi连接成功");
    my_printf("(wifi) WiFi名称: %s", WiFi.SSID().c_str());
    my_printf("(wifi) WiFi IP地址: %s", WiFi.localIP().toString().c_str());
    my_printf("(wifi) MQTT服务器: %s", mqtt_server.c_str());
    my_printf("(wifi) MQTT端口: %d", mqtt_port);
    my_printf("(wifi) MQTT ID: %s", mqtt_id);
    my_printf("(wifi) MQTT连接成功");
  }

  RS485_init();
  // my_printf("(flash) SPIFFS总大小: %d, SPIFFS已使用大小: %d, Flash size: %d", LittleFS.totalBytes(), LittleFS.usedBytes(), ESP.getFlashChipSize());
  my_printf("(memory) RAM可使用大小: %d", ESP.getFreeHeap());
  my_printf("(memory) PSRAM可使用大小: %d", ESP.getFreePsram());
  webtask_setup();
}
uint32_t error_time = 0;
uint32_t offline_time = 0;
uint32_t mqtt_time = 0;
uint32_t led_time = 0;
uint32_t server_time = 0;
uint32_t save_time = 0;
uint32_t switch_time = 0;
void loop()
{
  package_type stu = BambuBus_stu();
  //package_type stu = BambuBus_run();
  //  Bmcu_readuart();
  //   int stu =-1;
  uint32_t time_now = get_time32();

  if (stu == BambuBus_package_ERROR) // offline
  {
    // SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
    SYS_leds.clear();
    if (error_time < (time_now - 1000))
    {
      error_time = time_now + 1000;
    }
    else if (error_time > time_now)
      SYS_leds.setPixelColor(0, 0x30, 0x00, 0x00);
    else if (error_time < time_now)
      SYS_leds.setPixelColor(2, 0x30, 0x00, 0x00);

    if (WiFi.status() == WL_CONNECTED)
    {
      SYS_leds.setPixelColor(1, 0x10, 0xD0, 0x30);
      if (mqtt_time < time_now)
      {
        mqtt_time = time_now + 20000;
        hub_msg();
        my_printf("(bambus) bambus连接中...");
      }
    }
  }
  else if (stu == BambuBus_package_heartbeat) // have data
  {

    SYS_leds.clear();
    if (error_time < (time_now - 2000))
    {
      error_time = time_now + 2000;
      // Sht30_read();
    }
    else if (error_time > time_now)
    {
      SYS_leds.setPixelColor(0, 0x10, 0xD0, 0x30);
    }
    else if (error_time < time_now)
    {
      SYS_leds.setPixelColor(0, 0x00, 0x00, 0x00);
    }

    if (WiFi.status() == WL_CONNECTED)
    {
      SYS_leds.setPixelColor(1, 0x10, 0xD0, 0x30);

      if (mqtt_time < time_now)
      {
        mqtt_time = time_now + 5000; // 5秒延迟
        uint8_t ams_num = postMsgId / 4;
        uint8_t tay_num = postMsgId % 4;
        // ESP_LOGE("memory", "RAM可使用大小: %d", ESP.getFreeHeap());
        String temp;
        if (tay_num == 0)
          temp = ("{\"tay1\":" + Bmcu_set_json(ams_num, tay_num) + "}");
        if (tay_num == 1)
          temp = ("{\"tay2\":" + Bmcu_set_json(ams_num, tay_num) + "}");
        if (tay_num == 2)
          temp = ("{\"tay3\":" + Bmcu_set_json(ams_num, tay_num) + "}");
        if (tay_num == 3)
          temp = ("{\"tay4\":" + Bmcu_set_json(ams_num, tay_num) + "}");
        client.publish(topic[ams_num], temp.c_str());
        postMsgId++;
        if (postMsgId > ((get_AMS_num_max() * 4) - 1))
        {
          postMsgId = 0;
          hub_msg();
          my_printf("(mqtt) 发送数据成功");
          SYS_leds.setPixelColor(2, 0x00, 0x00, 0x30);
        }
        else
        {
          SYS_leds.setPixelColor(2, 0x10, 0xD0, 0x30);
        }
      }
    }
    else if (!BambuBus_if_on_print() && offline_time < time_now)
    {

      offline_time = time_now + 300000; // 300秒后重连
      if (WiFi.reconnect())
      {
        client.connect(mqtt_id, mqtt_username.c_str(), mqtt_password.c_str());
      }
    }
    // SYS_leds.show();

    if (Switch_need_refresh())
    {
      if (switch_time == 0)
        switch_time = time_now + 2000; // 强制刷新耗材信息
      else if (switch_time < time_now && switch_time != 0)
      {
        Switch_set_refresh(false);
        switch_time = 0;
      }
    }
  }
  if (save_time < time_now)
  {
    if (save_time != 0)
    {
      if (!enable_24())
      {
        set_24(true);
      }
      set_fan(30); // 30度开启风扇
      publishLogOverMQTT();
      if (Switch_need_to_save())
      {
        // tft_print(catch_mode);
        Switch_save();
      }
      else if (WIFI_needsave())
      {
        Config_save();
      }
    }
    save_time = time_now + 60000; // 60 秒一次
    if (save_count >= 40)
    {
      Bambubus_set_need_to_save();
      save_count = 0;
    }
    save_count++;
  }
  if (server_time == 0 && !server_key) // 如果WebServer未开启
  {
    server_time = time_now + 20000; // 20秒后开启WebServer
    // initWebServer();                  // 开启WebServer
    // my_printf("(web) WebServer已开启");
  }
  else if (server_time < time_now && server_time != 1)
  {
    server_key = true;
    // stopWebServer();    // 关闭WebServer
    my_printf("(web) WebServer已开启");
    server_time = 1; // 防止重复执行
  }
  if (led_time < time_now)
  {
    led_time = time_now + 500;
    if (SYS_leds.canShow())
    {
      SYS_leds.show();
    }
  }

  vTaskDelay(pdMS_TO_TICKS(5)); // 控速
  // delay(1);
}
