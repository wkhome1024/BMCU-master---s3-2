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
int mqtt_count[32];

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

  Serial.onReceive(Bambu_readuart);    //串口回调；
  Serial1.onReceive(Bmcu_readuart);    //串口回调；
}
uint64_t error_time = 0;
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
      digitalWrite(LED_BUILTIN2, LOW);
      if (error_time == 0 || error_time < (time_now - 1000))
        error_time = time_now + 1000;
      else if (error_time > time_now)
        digitalWrite(LED_BUILTIN1, HIGH);
      else if (error_time < time_now)
        digitalWrite(LED_BUILTIN1, LOW);
      // delayMicroseconds(1000);
      /*if (WiFi.status() != WL_CONNECTED)
        {

          if (WiFi.reconnect())
          {
            client.connect(product_id, mqtt_username, mqtt_password);
          }
          
        }
        if (WiFi.status() == WL_CONNECTED)
        {

          if (mqtt_count[postMsgId] == 0)
            mqtt_count[postMsgId] = time_now + 10000;
          if (mqtt_count[postMsgId] < time_now)
          {
            //String tay[4] = {"11","22","33","44"};
            String temp = ("{\"tay1\":" +Bmcu_set_json(postMsgId,0) +", \"tay2\":" +Bmcu_set_json(postMsgId,1) +",\"tay3\":" +Bmcu_set_json(postMsgId,2) +",\"tay4\":" +Bmcu_set_json(postMsgId,3) +"}");
            client.publish(topic[postMsgId], temp.c_str());
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
        }*/



    }
    else // have data
    {
      if (stu == BambuBus_package_heartbeat)
      {
        if (error_time == 0 || error_time < (time_now - 2000))
          error_time = time_now + 2000;
        else if (error_time > time_now)
        {
          digitalWrite(LED_BUILTIN1, HIGH);
          digitalWrite(LED_BUILTIN2, LOW);
        }
        else if (error_time < time_now)
        {
          digitalWrite(LED_BUILTIN1, LOW);
          digitalWrite(LED_BUILTIN2, HIGH);
        }
      }
      if (WiFi.status() == WL_CONNECTED && stu == BambuBus_long_package_MC_online)
      {

        if (mqtt_count[postMsgId] == 0)
          mqtt_count[postMsgId] = time_now + 10000;
        if (mqtt_count[postMsgId] < time_now)
        {
          uint8_t ams_num = postMsgId /4;
          uint8_t tay_num = postMsgId %4;
          String temp;
          //String tay[4] = {"11","22","33","44"};
          //String temp = ("{\"tay1\":" +Bmcu_set_json(postMsgId,0) +", \"tay2\":" +Bmcu_set_json(postMsgId,1) +",\"tay3\":" +Bmcu_set_json(postMsgId,2) +",\"tay4\":" +Bmcu_set_json(postMsgId,3) +"}");
          //client.publish(topic[postMsgId], temp.c_str());

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
            for (size_t i = 0; i < 32; i++)
            {
              mqtt_count[i] = 0;
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
