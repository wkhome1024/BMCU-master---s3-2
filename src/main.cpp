#include <main.h>

const char *ssid = "WKhome";       // WiFi名称
const char *password = "44332211"; // WiFi密码

const char *mqtt_server = "192.168.10.10"; // MQTT服务器地址
const int mqtt_port = 1883;                // MQTT服务器端口
const char *topic[8] = {"bmcu1", "bmcu2", "bmcu3", "bmcu4", "bmcu5", "bmcu6", "bmcu7", "bmcu8"};
const char *mqtt_username = "wk";
const char *mqtt_password = "iloveyou12";

#define product_id "bmcu-hub" // 产品ID
#define device_id "s3"        // 设备ID

int postMsgId = 0; // 消息ID初始值为0

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




char mqtt_id[20];

void setup()
{

  EEPROM.begin(4096); // 申请存储空间
  BambuBus_init();
  Switch_init();
  Sht30_init();
  LED_init();
  tft_init();
  // 检查是否有保存的Wi-Fi配置信息
  if (WiFi.status() != WL_CONNECTED)
  {
    WiFi.begin(ssid, password); // 尝试自动连接上次保存的Wi-Fi
    // Serial.println("尝试连接已保存的WiFi...");

    // 等待连接成功
    int attempts = 0;
    while (WiFi.status() != WL_CONNECTED && attempts < 20)
    {
      delay(500);
      // Serial.print(".");
      attempts++;
    }
    uint8_t mac_ad[6];
    WiFi.macAddress(mac_ad);
    sprintf(mqtt_id, "%s-%s-%02X%02X", product_id, device_id, mac_ad[4], mac_ad[5]);
    client.setServer(mqtt_server, mqtt_port); // 设置MQTT服务器地址和端口
    client.connect(mqtt_id, mqtt_username, mqtt_password);
    client.publish(topic[0], "Hi, I'm ESP32 ^^");
  }

  Serial0.onReceive(Bambu_readuart);    //串口回调；
  Serial1.onReceive(Bmcu_readuart);    //串口回调；
}
uint64_t error_time = 0;
uint64_t offline_time = 0;
uint64_t mqtt_time = 0;
uint64_t led_time = 0;
void loop()
{
  //Bambu_readuart();
  
  package_type stu = BambuBus_run();
  //Bmcu_readuart();
  // int stu =-1;
  uint64_t time_now = get_time64();
  if (stu != BambuBus_package_NONE) // have data/offline
  {
    if (stu == BambuBus_package_ERROR) // offline
    {
      // SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
      SYS_leds.clear();
      if (error_time < (time_now - 1000))
      {
        error_time = time_now + 1000;
        tft_print();
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
          mqtt_time = time_now + 5000; // 5秒延迟
          client.publish(topic[0], Sht30_read_mqtt().c_str());
        }
      }


    }
    else // have data
    {
      if (stu == BambuBus_package_heartbeat)
      { 
        SYS_leds.clear(); 
        if (error_time < (time_now - 2000))
        {
          error_time = time_now + 2000;
          Sht30_read();
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
          uint8_t ams_num = postMsgId /4;
          uint8_t tay_num = postMsgId %4;
          String temp;

          if (tay_num == 0)
              temp = ("{\"tay1\":" +Bmcu_set_json(ams_num,tay_num) +"}");
          if (tay_num == 1)
              temp = ("{\"tay2\":" +Bmcu_set_json(ams_num,tay_num) +"}");
          if (tay_num == 2)
              temp = ("{\"tay3\":" +Bmcu_set_json(ams_num,tay_num) +"}");
          if (tay_num == 3)
              temp = ("{\"tay4\":" +Bmcu_set_json(ams_num,tay_num) +"}");

          client.publish(topic[ams_num], temp.c_str());
          postMsgId++;
          if (postMsgId > ((get_AMS_num_max() *4) - 1))
          {
            postMsgId = 0;
            client.publish(topic[0], Sht30_read_mqtt().c_str());
            tft_print();
            SYS_leds.setPixelColor(2, 0x00, 0x00, 0x30);
          }
          else
          {
            SYS_leds.setPixelColor(2, 0x10, 0xD0, 0x30);
          }
        }
        }
        else if (!Bambu_onprint() && offline_time < time_now)
        {    

            offline_time = time_now + 300000; // 300秒后重连
            if (WiFi.reconnect())
            {
               client.connect(mqtt_id, mqtt_username, mqtt_password);
            }
                
        }
        //SYS_leds.show();
      }

      if (Switch_need_to_save())
      {
        tft_print();
        Switch_save();
      }
      if (Switch_need_to_delay())
      {
        Switch_set_not_to_delay();
        delay(5000);
      }
    }
    if (led_time < time_now)
    {
      led_time = time_now + 500;
      if (SYS_leds.canShow())
      {
        SYS_leds.show();
      }
    }
  }

  // delay(1);
}
