#include <main.h>

// const char *ssid = "";       // WiFi名称
// const char *password = ""; // WiFi密码

// const char *mqtt_server1 = "192.168.10.10"; // MQTT服务器地址
// const int mqtt_port1 = 1883;                // MQTT服务器端口
char ha_topic[20] = "bmcu-hub-1"; // Home Assistant 发现主题
char logTopic[20] = "bmcu-hub-1/log";
// char topic[8][8] = {"bmcu1-1", "bmcu1-2", "bmcu1-3", "bmcu1-4", "bmcu1-5", "bmcu1-6", "bmcu1-7", "bmcu1-8"};
char host_name[20] = "bmcu-hub-1";                   // 设备主机名
char all_filament_topic[20] = "bmcu-hub-1/filament"; // 所有耗材信息的主题
#define product_id "bmcu-hub"                        // 产品ID
#define device_id "s3"                               // 设备ID
uint8_t hub_num = 2;                                 // 集线器编号
uint8_t F_AMS_num = 1;                              // 官方AMS数量
char mqtt_id[20];
int save_count = 0;
uint8_t mqtt_status = 0;        // MQTT连接状态
int catch_key = 0;              // 抓包计数
bool catch_mode = true;        // 抓包模式
bool server_key = false;        // HTTP服务器开关
WiFiClient espclient;           // 创建一个WiFiClient对象
PubSubClient client(espclient); // 创建一个PubSubClient对象
bool error_flag = false;
bool motor_reboot_flag = false;
#define SYS_RGB 8   // RGB灯针脚
#define ledPixels 3 // led数量
Adafruit_NeoPixel SYS_leds(ledPixels, SYS_RGB, NEO_GRB + NEO_KHZ800);
void LED_init()
{
  SYS_leds.begin();
  SYS_leds.clear();
  SYS_leds.setBrightness(50);
  SYS_leds.show();
}

void LED_setColor(uint8_t num, uint8_t r, uint8_t g, uint8_t b)
{
  SYS_leds.setPixelColor(num, r, g, b);
}

bool sw_send = true;
void hub_msg()
{
  static uint8_t postMsgId = 0; // 消息ID初始值为0
  // tft_print(catch_mode);
  if (sw_send)
  {
    if (mqtt_status < 3)
    {
      if (!client.publish(ha_topic, Sht30_read_mqtt().c_str()))
      {
        client.connect(mqtt_id, mqtt_username.c_str(), mqtt_password.c_str());
        mqtt_status++;
        my_printf("(mqtt) MQTT发布失败,正在重连...尝试次数: %d", mqtt_status);
      }
    }
    else if (mqtt_status > 8)
    {
      my_printf("(sensor) 电机输出: %d", motor_pwm);
      my_printf("(sensor) pull+online+H_pwm: %s", Motion_get_status().c_str());
      mqtt_status ++;
      if (mqtt_status > 15)
      {
        mqtt_status = 0; // 重置状态以尝试重新连接
      }
    }
    sw_send = !sw_send;
    // my_printf("{\"Pull_Voltage\":%.2f,\"Online_Voltage\":%.2f,\"Buf_PWM\":%.2f}",pull_voltage, online_voltage, Buf_pwm_read());
    // my_printf("(sensor) 拉力传感器电压: %.2f V", MC_PULL_stu_raw);
    // my_printf("(sensor) 在线传感器电压: %.2f V", MC_ONLINE_key_stu_raw);
    // my_printf("(sensor) 缓冲PWM状态: %.2f", H_PULL_stu_raw);
    // my_printf("(sensor) 电机输出: %d", motor_pwm);
  }
  else
  {
    uint8_t ams_num = postMsgId / 4;
    uint8_t tay_num = postMsgId % 4;
    String all_filament_data = "{";
    if (mqtt_status < 3)
    {
      // 为每个AMS创建一个对象
      all_filament_data += "\"tay" + String(postMsgId + 1) + "\":";
      all_filament_data += Bmcu_set_json(ams_num, tay_num);
      all_filament_data += "}";
      // 发布到统一的耗材主题
      client.publish(all_filament_topic, all_filament_data.c_str());
    }
    postMsgId++;
    if (postMsgId > ((get_AMS_num_max() * 4) - 1))
    {
      postMsgId = 0;
      // my_printf("(mqtt) 发送数据成功");
      // my_printf("(sensor) 送料距离: %.2f mm", last_total_distance);
      my_printf("(sensor) 电机输出: %d", motor_pwm);
      my_printf("(sensor) pull+online+H_pwm: %s", Motion_get_status().c_str());
      SYS_leds.setPixelColor(1, 0x00, 0x00, 0x30); // 发送数据成功后变为蓝色
      if (SYS_leds.canShow())
      {
        SYS_leds.show();
      }
    }
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
  else if (offset > 63 * 1024 && logLength < 1024) // 如果偏移量超过64KB且日志长度小于1KB
  {
    my_printf("(LOG) 日志缓冲已重置，重置偏移量");
    offset = 0; // 重置偏移量
    remaining = logLength;
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
  host_name[9] += (hub_num - 1); // 修改主机名以包含集线器编号
  checkConnect(Config_read());   // 检查配置Wi-Fi连接
  // 等待连接成功

  if (WiFi.status() == WL_CONNECTED)
  {
    uint8_t mac_ad[6];
    WiFi.macAddress(mac_ad);
    sprintf(mqtt_id, "%s-%02X%02X", host_name, mac_ad[4], mac_ad[5]);
    ha_topic[9] += (hub_num - 1);
    logTopic[9] += (hub_num - 1);
    all_filament_topic[9] += (hub_num - 1);
    client.setServer(mqtt_server.c_str(), mqtt_port); // 设置MQTT服务器地址和端口
    client.connect(mqtt_id, mqtt_username.c_str(), mqtt_password.c_str());
    my_printf("(wifi) WiFi连接成功");
    my_printf("(wifi) WiFi名称: %s", WiFi.SSID().c_str());
    my_printf("(wifi) WiFi IP地址: %s", WiFi.localIP().toString().c_str());
    my_printf("(wifi) MQTT服务器: %s", mqtt_server.c_str());
    my_printf("(wifi) MQTT端口: %d", mqtt_port);
    my_printf("(wifi) MQTT ID: %s", mqtt_id);
    if (client.publish(ha_topic, "Hi, I'm ESP32 ^^"))
        my_printf("(wifi) MQTT连接成功");
  }

  RS485_init();
  Motion_control_init();
  // my_printf("(flash) SPIFFS总大小: %d, SPIFFS已使用大小: %d, Flash size: %d", LittleFS.totalBytes(), LittleFS.usedBytes(), ESP.getFlashChipSize());
  my_printf("(memory) RAM可使用大小: %d", ESP.getFreeHeap());
  my_printf("(memory) PSRAM可使用大小: %d", ESP.getFreePsram());
  webtask_setup();
  //setup_motor_task();
}
uint64_t error_time = 0;
uint64_t offline_time = 0;
uint64_t mqtt_time = 0;
uint64_t led_time = 0;
uint64_t server_time = 0;
uint64_t save_time = 0;
uint64_t switch_time = 0;
void loop()
{
  package_type stu = BambuBus_stu();
  // package_type stu = BambuBus_run();
  //   Bmcu_readuart();
  //    int stu =-1;
  uint64_t time_now = get_time64();

  if (stu == BambuBus_package_ERROR) // offline
  {
    // SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
    // SYS_leds.clear();
    error_flag = true;
    if (WiFi.status() == WL_CONNECTED)
    {
      SYS_leds.setPixelColor(1, 0x10, 0xD0, 0x30);
      if (mqtt_time < time_now)
      {
        mqtt_time = time_now + 10000;
        hub_msg();
        my_printf("(bambus) bambus连接中...");
      }
    }
  }
  else if (stu == BambuBus_package_heartbeat) // have data
  {

    // SYS_leds.clear();
    error_flag = false;
    if (WiFi.status() == WL_CONNECTED)
    {
      SYS_leds.setPixelColor(1, 0x10, 0xD0, 0x30); // 绿色常亮表示在线

      if (mqtt_time < time_now)
      {
        mqtt_time = time_now + 10000; // 10秒延迟
        hub_msg();
      }
    }
    else if (BambuBus_not_on_print() && offline_time < time_now)
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
      {
        Motor_reboot();
        switch_time = time_now + 8000; // 强制刷新
        my_printf("(hub) AMS数据刷新成功");
      }
      else if (switch_time < time_now && switch_time != 0)
      {
        Switch_set_refresh(false);
        p2s_reset_startup_seq();
        switch_time = 0;
      }
    }
  }
  if (save_time < time_now)
  {
    if (save_time != 0)
    {
      Set_fan_t(30); // 30度开启风扇
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
    // motor_test = false;
    save_time = time_now + 60000; // 60 秒一次
    if (save_count >= 40)
    {
      if (bambubus_save_flag)
      {
        Bambubus_set_need_to_save();
        bambubus_save_flag = false;
      }
      save_count = 0;
    }
    else if (save_count == 30)
    {
      if (motor_reboot_flag)
      {
          Motor_reboot();
          send_reset();  //打印完成 耗材复位
          motor_reboot_flag = false;      
      }
    }
    save_count++;
  }
  if (server_time == 0) // 如果WebServer未开启
  {
    server_time = time_now + 20000; // 20秒后开启WebServer
    // initWebServer();                  // 开启WebServer
    // my_printf("(web) WebServer已开启");
  }
  else if (server_time == 1 && !enable_24())
  {
    server_time = time_now + 20000;
  }
  else if (server_time < time_now && server_time != 1)
  {
    if (!server_key)
    {
      server_key = true;
      my_printf("(web) WebServer已开启");
    }
    if (!enable_24())
    {
      Set_24(true);
      my_printf("(power) 从机24v已开启");
    }
    server_time = 1; // 防止重复执行
  }
  if (led_time < time_now)
  {
    led_time = time_now + 500;
    // hub_msg();
    if (SYS_leds.canShow())
    {
      SYS_leds.show();
    }
  }
  // Motion_control_run(error_flag);
  vTaskDelay(pdMS_TO_TICKS(4)); // 控速
  // delay(1);
}
