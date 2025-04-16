#include <main.h>

const char *ssid = "WKhome";       // WiFi名称
const char *password = "44332211"; // WiFi密码

const char *mqtt_server = "192.168.10.10"; // MQTT服务器地址
const int mqtt_port = 1883;                // MQTT服务器端口
const char *topic[8] = {"bmcu1", "bmcu2", "bmcu3", "bmcu4", "bmcu5", "bmcu6", "bmcu7", "bmcu8"};
const char *mqtt_username = "wk";
const char *mqtt_password = "iloveyou12";

#define product_id "bmcu-hub" // 产品ID
#define device_id "01"        // 设备ID

int postMsgId = 0; // 消息ID初始值为0
int mqtt_count[8];

WiFiClient espclient;           // 创建一个WiFiClient对象
PubSubClient client(espclient); // 创建一个PubSubClient对象

void setup()
{

  EEPROM.begin(4096); // 申请存储空间
  BambuBus_init();
  Switch_init();
  LED_Init();

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

    client.setServer(mqtt_server, mqtt_port); // 设置MQTT服务器地址和端口
    client.connect(product_id, mqtt_username, mqtt_password);
    client.publish(topic[0], "Hi, I'm ESP32 ^^");
  }
}
uint64_t error_time = 0;
void loop()
{
  Bambu_readuart();
  package_type stu = BambuBus_run();
  Bmcu_readuart();
  // int stu =-1;
  uint64_t time_now = get_time64();
  if (stu != BambuBus_package_NONE) // have data/offline
  {
    if (stu == BambuBus_package_ERROR) // offline
    {
      // SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
      digitalWrite(13, LOW);
      if (error_time == 0 || error_time < (time_now - 1000))
        error_time = time_now + 1000;
      else if (error_time > time_now)
        digitalWrite(12, HIGH);
      else if (error_time < time_now)
        digitalWrite(12, LOW);
      // delayMicroseconds(1000);
    }
    else // have data
    {
      if (stu == BambuBus_package_heartbeat)
      {
        if (error_time == 0 || error_time < (time_now - 2000))
          error_time = time_now + 2000;
        else if (error_time > time_now)
        {
          digitalWrite(12, HIGH);
          digitalWrite(13, LOW);
        }
        else if (error_time < time_now)
        {
          digitalWrite(12, LOW);
          digitalWrite(13, HIGH);
        }
        if (WiFi.status() == WL_CONNECTED)
        {

          if (mqtt_count[postMsgId] == 0)
            mqtt_count[postMsgId] = time_now + 500;
          if (mqtt_count[postMsgId] < time_now)
          {
            client.publish(topic[postMsgId], Bmcu_set_josn(postMsgId));
            postMsgId++;
            if (postMsgId > (get_AMS_num_max() - 1))
            {
              postMsgId = 0;
              for (size_t i = 0; i < 8; i++)
              {
                mqtt_count[i] = 0;
              }
            }
          }
        }
      }
      if (Switch_need_to_save())
        Switch_save();
      if (Switch_need_to_delay())
      {
        Switch_set_not_to_delay();
        delay(5000);
      }
    }
  }

  // delay(1);
}
