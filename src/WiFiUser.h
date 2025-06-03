#pragma once

#include <main.h>

#include <WiFi.h>
//#include <DNSServer.h>
#include <WebServer.h>
#include <ESPmDNS.h>      //用于设备域名 MDNS.begin("esp32")
//#include <esp_wifi.h>     //用于esp_wifi_restore() 删除保存的wifi信息
#include <Update.h>
#include <pgmspace.h>
//extern const char* HOST_NAME;                 //设置设备名
//extern int connectTimeOut_s = 30;                 //WiFi连接超时时间，单位秒

extern String wifi_ssid;                    //wifi账号
extern String wifi_pass;                    //wifi密码
extern String mqtt_server;                  //mqtt服务器地址
extern int mqtt_port;                        //mqtt服务器端口
extern String mqtt_username;                 //mqtt用户名
extern String mqtt_password;                 //mqtt密码

//===========需要调用的函数===========
extern void checkConnect(bool reConnect);    //检测wifi是否已经连接
extern void restoreWiFi();                   //删除保存的wifi信息
extern void checkDNS_HTTP();                 //检测客户端DNS&HTTP请求
extern void connectToWiFi(int timeOut_s);    //连接WiFi
extern void Config_save();                  //保存配置数据
extern bool Config_read();                  //读取配置数据
extern void stopWebServer();                //停止WebServer
extern void initWebServer();                 //初始化WebServer 
extern void webtask_setup();

//===========内部函数===========
extern void handleRoot();                    //处理网站根目录的访问请求
extern void handleConfigWifi() ;             //提交数据后的提示页面
extern void handleNotFound();                //处理404情况的函数'handleNotFound'
extern void initSoftAP();                    //进入AP模式
extern void initDNS();                       //开启DNS服务器
extern bool scanWiFi();                      //扫描附近的WiFi，为了显示在配网界面
extern void wifiConfig();                    //配置配网功能

