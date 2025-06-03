
---

## 🚀 快速开始
# BMCU
#### BMCU介绍           设计师：4061N     [https://oshwhub.com/bamboo-shoot-xmcu-pcb-team/bmcu](https://oshwhub.com/bamboo-shoot-xmcu-pcb-team/bmcu)

BMCU以四通道为一个单位，目前以CH32单片机为主控设计。其设计所需资料均参考于网络公开资料及个人测试，程序基于Platform IO平台下CH32单片机的Arduino支持库设计，调用了robtillaart的CRC库。
 **注意:本项目遵循GPL2.0开源协议，但需要额外补充的是本项目禁止商业用途。** 





# BMCU-HUB 嵌入式固件项目 基于bmcu代码  

这是一个基于 ESP32S3 的嵌入式 Hub 固件项目，支持 WiFi 配网、Web 管理界面、OTA 升级、数据抓包等功能。


## 📋 主要功能

- ✅ 多bmcu连接--通道重定向实现16选4   bmcu从机需刷入相应的固件
- 🔧 Soft-AP 模式提供本地配网服务  
- 🖥️ WebServer 提供网页配置入口  
- 🔄 OTA 固件升级  
- 📤 数据上报与日志查看  
- 🌡️ SHT30 温湿度传感器驱动
- 🔧 风扇接口、五通前耗材在线接口、从bmcu电源管理（测试）
  

## 🛠️ 开发环境

- 平台：ESP-IDF / Arduino IDE / PlatformIO
- 设备：ESP32 系列开发板
- 库依赖：
  - [robtillaart/CRC@^1.0.3](https://github.com/robtillaart/CRC)
  - [knolleary/PubSubClient@^2.8](https://github.com/knolleary/PubSubClient)
  - [bodmer/TFT_eSPI@^2.5.43](https://github.com/Bodmer/TFT_eSPI)
  - [adafruit/Adafruit NeoPixel@^1.13.0](https://github.com/adafruit/Adafruit_NeoPixel)
  - [adafruit/Adafruit ST7735 and ST7789 Library@^1.11.0](https://github.com/adafruit/Adafruit-ST7735-Library)

---

## 🧱 系统架构与模块划分

### 📦 模块划分

| 模块 | 文件 | 功能描述 |
|------|------|----------|
| WiFi 连接管理 | `WiFiUser.cpp/h` | 支持 STA/AP 模式切换、自动重连、配网流程 |
| WebServer | `WiFiUser.cpp/h` | 提供 HTTP 接口用于配置和控制 |
| 数据管理 | `Data.cpp/h` | 实现全局缓冲区写入逻辑 |
| OTA 更新 | [WiFiUser.cpp] | 通过 `/update` 接口实现固件远程升级 |
| 传感器驱动 | `sht30.cpp/h` | SHT30 温湿度传感器读取 |
| 时间处理 | `time64.cpp/h` | 时间戳转换与 UTC 处理 |
| RS485 通信 | `485.cpp/h` | 实现 RS485 总线通信 |
| 总线协议 | `BambuBus.cpp/h` | 封装设备间通信协议 |
| 通道重定向 | `Switch.cpp/h` | 控制保存、读取、切换通道 |
| 主程序入口 | `main.cpp/h` | 初始化流程、主循环逻辑 |

---

## ⚙️ 首次启动流程

1. 上电后，设备尝试连接上次保存的 WiFi。
2. 若连接失败，进入 Soft-AP 模式，热点名称为 `Bmcu-hub-AP`（默认值可在 [WIFIUser.cpp]中修改）。
3. 用户通过手机或电脑连接该热点。
4. 浏览器访问 `http://bmcu-hub-s3.local` 或 `192.168.4.1`，进入配网页面。
5. 输入目标 WiFi 的 SSID 和密码，提交后设备将尝试连接。
6. 若连接成功，设备返回 STA 模式并获取 IP 地址。

### 📎 配置文件说明

主要配置参数（可在 [main.h] 修改）：

| 参数 | 默认值 | 描述 |
|------|--------|------|
| [host_name] | `"bmcu-hub-s3"` | mDNS 主机名 |
| `topic[8]` | `bmcu1` | mqtt 主题名称 |
| `catch_num` | `500` | 抓包数量 |
| `server_key` | `true` | web 服务开关 |

---

## 🔁 强制恢复出厂设置

- web 界面设置进入强制配网模式  悬停2秒激活按键


---

## 🔄 运行流程

1. 初始化硬件与串口
2. 加载 NVS 中保存的 WiFi 配置
3. 尝试连接 WiFi（若失败进入 AP 模式）
4. 启动 WebServer 提供配置页面
5. 用户提交配置后保存并尝试重新连接
6. 若成功连接，保持运行并响应客户端请求
7. 支持 OTA 升级与数据上报

---

## 💡 特定耗材名TPU-ams设置

### 📈 触发的命令列表

| 命令 | 描述 |
|------|------|
| `sw2 == 0xD1` | 重置耗材里程（白色） |
| `sw2 == 0xD3` | 设置电机退料时间（棕色） |
| `sw2 == 0xD5` | 设置电机 PWM 零点（岩石灰） |
| `sw2 == 0xD7` | 电机 PWM 自动标定（灰色） |
| `sw2 == 0xD9` | 选中激活为 On Use（黑色） |
| `sw2 == 0x0D` | 重定向重置为以选定通道为bmcu_num的4个通道（黄色） | 
任意颜色触发所有bmcu耗材位置重置（短回抽耗材回位）
沙漠黄 触发断开后端bmcu从机电源120s


### 🧾 MQTT示例调用逻辑
var msg1 = {
  "payload":
    {
    "print":
    {
        "sequence_id": "0",
        "command": "ams_filament_setting",
        "ams_id": 0,
        "tray_id": 0,                                  //tray_id 为配置通道
        "tray_info_idx": "GFU02",
        "tray_color": "406100FF",                      //tray_color 为 HEX 格式，其中第6位为设置的重定向通道
        "nozzle_temp_min": 220,
        "nozzle_temp_max": 240,
        "tray_type": "TPU-AMS"
    }
    }
}   /// 发送到Bambu机器mqtt接口


### 📊 使用场景

- 当打印机发送特定命令（如设置耗材颜色、PWM 参数等）时，系统会调用 `Switch_set_filament()` 函数进行解析并执行对应操作。
- 可结合 Web UI 或 MQTT 发送自定义命令来远程控制耗材状态。
  mqtt发布主题为"bmcu-hub","bmcu1", "bmcu2", "bmcu3", "bmcu4", "bmcu5", "bmcu6", "bmcu7", "bmcu8"
  示例 {"Temp":"34.1","Humidity":"27.9"}


---

## 📡 WebServer API 接口文档

> 所有接口默认使用 `application/x-www-form-urlencoded` 格式提交表单数据。
> 浏览器访问 `http://bmcu-hub-s3/`

### 🔧 接口列表

| 接口路径 | 方法 | 描述 |
|----------|------|------|
| [/]| GET / POST | 首页展示或处理登录请求 |
| `/configwifi` | POST | 处理用户提交的 WiFi 配置 |
| `/config` | POST | 更新设备通用配置 |
| `/data` | POST | 接收客户端发送的数据 |
| `/log` | POST | 获取系统日志信息 |
| `/upload` | POST | 文件上传接口（可用于配置文件上传） |
| `/updatewifi` | POST | 更新 WiFi 配置并重启连接 |
| `/update` | POST | 固件 OTA 升级接口 |
| `/notfound` | 任意 | 页面未找到响应 |
| `/switch` | POST | 控制 LED 颜色切换（新增） |

---

## 📦 数据管理与日志功能

### 📥 数据写入 (`Data.cpp/h`)
- 全局缓冲区 [C_data]（大小为 64KB）
- 使用 [WriteData(const char *data)] 写入数据
- 若超出容量则清空缓冲区并重新写入

### 📋 日志功能
- 通过 `/log` 接口返回日志内容
- 日志信息包括：
  - WiFi 连接状态
  - 固件更新结果
  - 系统运行状态
  - 切换记录等

---

## 🔄 OTA 固件更新流程

1. 通过 `/update` 接口接收 `.bin` 文件
2. 使用 `Update.write()` 写入 Flash
3. 成功后重启设备

---

## 📁 部署与烧录指南

### 💻 开发环境准备

#### 硬件要求：
- ESP32-S3 开发板（BMCU-HUB PCB）
- 1.44tft  屏幕 ST7735接口
- USB-TTL 转换器（如 CP2102、CH340）
- 连接线、电源

#### 软件工具：
- ESP-IDF（推荐 v4.4+）
- Arduino IDE
- PlatformIO（推荐）

### 💾 编译与烧录（以 PlatformIO 为例）

```bash
# 安装插件
platformio run

# 编译
platformio run

# 烧录
platformio run --target upload

# 查看串口输出
platformio device monitor
```

---

## 📋 常见问题排查手册

### ❓ 无法连接 WiFi
- 检查输入的 SSID 和密码是否正确
- 确保路由器信号强度足够
- 查看串口输出是否有错误提示

### 📶 无法访问 Web 配置页面
- 确保已连接到 Soft-AP 热点
- 尝试访问 `http://192.168.4.1`
- 检查浏览器是否屏蔽了 HTTP 请求

### 🔌 OTA 升级失败
- 检查固件大小是否超过 Flash 容量
- 确保固件兼容当前硬件平台
- 查看串口输出是否有错误代码

---

