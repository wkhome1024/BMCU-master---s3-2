#include "WiFiUser.h"

#define HTML_BUF_SIZE 1024 * 4
static char htmlBuf[HTML_BUF_SIZE]; // 静态变量，避免栈溢出
const byte DNS_PORT = 53;           // 设置DNS端口号
const int webPort = 80;             // 设置Web端口号

const char *AP_SSID = "Bmcu-hub-AP"; // 设置AP热点名称
// const char* AP_PASS  = "";               //这里不设置设置AP热点密码

// const char *HOST_NAME = "bmcu-hub-s3"; // 设置设备名
String scanNetworksID = "";     // 用于储存扫描到的WiFi ID
int connectTimeOut_s = 30;      // WiFi连接超时时间，单位秒
IPAddress apIP(192, 168, 4, 1); // 设置AP的IP地址

String wifi_ssid = "";                // 暂时存储wifi账号密码
String wifi_pass = "";                // 暂时存储wifi账号密码
String mqtt_server = "192.168.10.10"; // 暂时存储mqtt服务器地址
int mqtt_port = 1883;                 // 暂时存储mqtt服务器端口
String mqtt_username = "11";          // 暂时存储mqtt用户名
String mqtt_password = "11";          // 暂时存储mqtt密码

// char L_data[20] = "test1234567890"; // test
// char C_data[20] = "test1234567890";
AsyncWebServer server(80);
const char *config_addr = "wificonfig";

struct alignas(4) config_struct
{
  char wifi_ssid[20];     // WiFi名称
  char wifi_password[20]; // WiFi密码
  char mqtt_server[20];   // MQTT服务器地址
  int mqtt_port = 1883;   // MQTT服务器端口
  char mqtt_username[20]; // MQTT用户名
  char mqtt_password[20]; // MQTT密码
  uint32_t version = Bambubus_version;
  bool resetcheck = true; // 是否重置
} config_save;

bool Config_read()
{
  config_struct ptr;
  if (!Flash_read(&ptr, sizeof(config_save), config_addr))
    return true;
  if (ptr.version == Bambubus_version)
  {
    memcpy(&config_save, &ptr, sizeof(config_save));

    mqtt_server = config_save.mqtt_server;
    mqtt_port = config_save.mqtt_port;
    mqtt_username = config_save.mqtt_username;
    mqtt_password = config_save.mqtt_password;
    wifi_ssid = config_save.wifi_ssid;
    wifi_pass = config_save.wifi_password;
    return false;
  }
  else
  {
    wifi_ssid = "";
    wifi_pass = "";
    return true;
  }

  // connectToWiFi(30);
  // checkConnect(config_save.resetcheck);

  return false;
}
bool wifi_needsave = false;
bool WIFI_needsave()
{
  return wifi_needsave;
}
void Config_save()
{
  if (!Flash_saves(&config_save, sizeof(config_save), config_addr))
    ESP_LOGE("FLASH", "wifi保存失败");

  wifi_needsave = false;
}

// DNSServer dnsServer;                       //创建dnsServer实例
// WebServer server(webPort); // 开启web服务, 创建TCP SERVER,参数: 端口号,最大连接数

// 上下两段HTML代码
const char ROOT_HTML_1[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>WIFI设置页面</title>
  <style>
   :root{--primary:#1383c6;--secondary:#f26721;--bg-color:#e5e9f2;--card-bg:#F7F7F7;--input-border:#d9d9d9;--btn-color:#4d90fe;--btn-hover:#357ae8}
   body,html{margin:0;padding:0;width:100%;height:100%;display:table;background-color:var(--bg-color);font-family:'Source Sans Pro',Arial,sans-serif}#content{display:table-cell;vertical-align:middle;text-align:center}
   .login-card{padding:2.5rem;width:22rem;background-color:var(--card-bg);margin:0 auto 1rem;border-radius:1.25rem;box-shadow:0.5rem 0.5rem 1rem rgba(0,0,0,0.15);transition:transform 0.3s ease}
   .login-card:hover{transform:translateY(-0.25rem)}
   .login-card h1{font-weight:400;font-size:2rem;color:var(--primary);margin-bottom:1.5rem}
   input[type="text"],input[type="password"],input[type="submit"]{width:100%;padding:0.75rem;margin-bottom:1rem;border-radius:0.625rem;border:1px solid var(--input-border);font-size:1rem;box-sizing:border-box}
   input[type="submit"]{background-color:var(--btn-color);color:white;font-weight:600;cursor:pointer;border:none;transition:background 0.2s}
   input[type="submit"]:hover{background-color:var(--btn-hover)}
   input[type="submit"]:disabled{opacity:0.6;cursor:not-allowed}
   @media (max-width:480px){.login-card{width:85%;padding:1.5rem}}
  </style>
</head>
<body>
  <div id="content">
    <div class="login-card">
      <h1>WiFi+MQTT</h1>
      <form name="login_form" method="post" action="/configwifi">
        <input type="text" name="ssid" placeholder="请选择 WiFi 名称" list="data-list" required>
        <datalist id="data-list">
)rawliteral";

const char ROOT_HTML_2[] PROGMEM = R"rawliteral(
        <input type="password" name="password" placeholder="请输入 WiFi 密码" required>
        <input type="text" name="mqtt_server" placeholder="请输入 MQTT IP" value="192.168.10.10">
        <input type="text" name="mqtt_port" placeholder="请输入 MQTT 端口" value="1883">
        <input type="text" name="mqtt_username" placeholder="请输入 MQTT 用户名">
        <input type="password" name="mqtt_password" placeholder="请输入 MQTT 密码">
        <input type="submit" value="确 定 连 接">
      </form>
    </div>
  </div>
</body>
</html>
)rawliteral";

const char root2_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width,initial-scale=1.0">
  <title>BMCU-HUB 控制中心</title>
  <style>
    :root{--p:#1383c6;--s:#4d90fe;--a:#f26721;--bg:#e5e9f2;--cbg:#f7f7f7;--td:#333;--tl:#666;--b:#d9d9d9}
    body{background:#bg;min-height:100vh;display:flex;justify-content:center;align-items:center;font-family:system-ui,sans-serif}
    .card{width:min(90vw,320px);padding:2rem;background:#cbg;border-radius:.75rem;box-shadow:0 .5rem 1rem rgba(0,0,0,.1);text-align:center}
    h1{color:var(--p);font-weight:500;margin-bottom:1rem}h1 span{color:var(--a)}
    .form-group{margin-bottom:1rem}select,button{width:100%;padding:.75rem;border-radius:.5rem;border:1px solid var(--b);font-size:1rem}
    select{background:#fff;margin-bottom:.5rem}button{background:var(--s);color:#fff;border:0;font-weight:600;cursor:pointer}
    button:hover{background:#357ae8}button:disabled{opacity:.6;cursor:not-allowed}.divider{margin:1rem 0;border-top:1px solid var(--b)}
    .status-text {font-size:.8em;color:#666;margin-top:.5rem;height:1.2em}
  </style>
</head>
<body>
  <div class="card">
    <h1>BMCU-HUB</h1>
    <form id="captureForm" action="/data" method="POST">
      <div class="form-group">
        <select id="catchkey" name="catchkey" required>
          <option disabled selected>选择抓包参数</option>
          <option value="data">输出抓包数据</option>
          <option value="open">开启抓包500个包</option>
          <option value="close">关闭抓包--断开从机</option>
          <option value="catch_mode">开启抓包模式</option>
          <option value="normal_mode">关闭抓包模式</option>
          <option value="refresh">强制刷新</option>
        </select>
        <button type="submit">抓包设置</button>
      </div>
    </form>
    <div class="divider"></div>
    <form action="/log" method="POST"><div class="form-group"><button type="submit">查看系统日志</button></div></form>
    <div class="divider"></div>
    <form action="/upload" method="POST"><div class="form-group"><button type="submit">HUB固件更新</button></div></form>
    <div class="divider"></div>
    <form action="/updatewifi" method="POST"><div class="form-group"><button type="submit" id="wifiBtn" disabled>更新wifi+mqtt参数</button></div><div class="status-text" id="statusText"></div></form>
  </div>
  <script>
    const wifiBtn = document.getElementById('wifiBtn');
    const statusText = document.getElementById('statusText');
    let hoverTimer;
    wifiBtn.addEventListener('mouseenter', () => {
      if (wifiBtn.disabled) {
        statusText.textContent = '激活中...';
        hoverTimer = setTimeout(() => {
          wifiBtn.disabled = false;
          statusText.textContent = '按钮已激活';
        }, 2000);
      }
    });
    wifiBtn.addEventListener('mouseleave', () => {
      if (wifiBtn.disabled) {
        clearTimeout(hoverTimer);
        statusText.textContent = '';
      }
    });
  </script>
</body>
</html>
)rawliteral";

const char upload_html[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>BMCU-HUB更新</title>
    <style>
        body {background:#e5e9f2;margin:0;padding:0;height:100vh;display:flex;justify-content:center;align-items:center;font-family:Arial,sans-serif}
        .upload-container {background:#fff;width:350px;padding:30px;border-radius:15px;box-shadow:0 10px 25px rgba(0,0,0,0.1);text-align:center}
        h1 {color:#1383c6;margin-bottom:25px;font-weight:500}
        .upload-btn {background:#4d90fe;color:#fff;border:none;padding:12px 25px;border-radius:8px;font-size:16px;cursor:pointer;transition:background .3s}
        .upload-btn:hover {background:#357ae8}
        .file-info {margin-top:15px;font-size:14px;color:#666}
        .progress-bar {height:8px;background:#eee;border-radius:4px;margin-top:20px;overflow:hidden}
        .progress {height:100%;background:#4d90fe;width:0%;transition:width .3s}
    </style>
</head>
<body>
  <div class="upload-container">
    <h1>BMCU-HUB</h1>
    <form id="uploadForm" action="/update" method="POST" enctype="multipart/form-data">
        <input type="file" id="fileInput" name="file" style="display: none;" accept=".bin" maxSize="1000000">
        <label for="fileInput" class="upload-btn">选择固件文件</label>
        <div class="file-info" id="fileName">未选择文件</div>
      <div class="progress-bar">
        <div class="progress" id="uploadProgress"></div>
      </div>
      <button type="submit" class="upload-btn" style="margin-top: 20px;">开始更新</button>
    </form>
  </div>
  <script src="https://code.jquery.com/jquery-3.6.0.min.js"></script>
  <script>
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const fileSize = document.getElementById('fileSize');
    const uploadProgress = document.getElementById('uploadProgress');
    fileInput.addEventListener('change', (e) => {
      if(e.target.files.length) {
        const fileSizeInKB = (e.target.files[0].size / 1024).toFixed(2);
        fileName.textContent = `已选择: ${e.target.files[0].name}，文件大小 ${fileSizeInKB} KB`;
      }
    });
    $('#uploadForm').submit(function(e){
      e.preventDefault();
      if(!fileInput.files.length) return;
      var formData = new FormData(this);
      $.ajax({
        url: '/update',
        type: 'POST',
        data: formData,
        contentType: false,
        processData: false,
        xhr: function() {
          var xhr = new window.XMLHttpRequest();
          xhr.upload.addEventListener('progress', function(evt) {
            if (evt.lengthComputable) {
              var percent = Math.round((evt.loaded / evt.total) * 100);
              uploadProgress.style.width = percent + '%';
            }
          }, false);
          return xhr;
        },
        success: function(d, s) {
          alert('文件上传成功！');
          uploadProgress.style.width = '0%';
          fileName.textContent = '未选择文件';
          fileInput.value = '';
        },
        error: function(a, b, c) {
          alert('上传失败: ' + c);
          uploadProgress.style.width = '0%';
          fileName.textContent = '未选择文件';
          fileInput.value = '';
        }
      });
    });
  </script>
</body>
</html>
)rawliteral";

/*
 * 处理网站根目录的访问请求
 */

void WebHandler::handleRoot(AsyncWebServerRequest *request)
{
  htmlBuf[0] = '\0'; // 清空缓冲区

  if (WiFi.getMode() != WIFI_STA)
  {
    strcpy_P(htmlBuf, ROOT_HTML_1);
    strcat(htmlBuf, scanNetworksID.c_str());
    strcat_P(htmlBuf, ROOT_HTML_2);
  }
  else if (WiFi.status() == WL_CONNECTED)
  {
    strcpy_P(htmlBuf, root2_html);
  }

  request->send(200, "text/html", htmlBuf);
}

void WebHandler::handleConfigWifi(AsyncWebServerRequest *request)
{

  if (request->hasParam("ssid", true))
  {
    wifi_ssid = request->getParam("ssid", true)->value();
    wifi_ssid.trim();
    memcpy(config_save.wifi_ssid, wifi_ssid.c_str(), wifi_ssid.length());
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid");
    return;
  }

  if (request->hasParam("password", true))
  {
    wifi_pass = request->getParam("password", true)->value();
    wifi_pass.trim();
    memcpy(config_save.wifi_password, wifi_pass.c_str(), wifi_pass.length());
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }

  if (request->hasParam("mqtt_server", true))
  {
    mqtt_server = request->getParam("mqtt_server", true)->value();
    mqtt_server.trim();
    memcpy(config_save.mqtt_server, mqtt_server.c_str(), mqtt_server.length());
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_server");
    return;
  }

  if (request->hasParam("mqtt_port", true))
  {
    mqtt_port = request->getParam("mqtt_port", true)->value().toInt();
    config_save.mqtt_port = mqtt_port;
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_port");
    return;
  }

  if (request->hasParam("mqtt_username", true))
  {
    mqtt_username = request->getParam("mqtt_username", true)->value();
    mqtt_username.trim();
    memcpy(config_save.mqtt_username, mqtt_username.c_str(), mqtt_username.length());
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_username");
    return;
  }

  if (request->hasParam("mqtt_password", true))
  {
    mqtt_password = request->getParam("mqtt_password", true)->value();
    mqtt_password.trim();
    memcpy(config_save.mqtt_password, mqtt_password.c_str(), mqtt_password.length());
  }
  else
  {
    request->send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_password");
    return;
  }

  String response = "<meta charset='UTF-8'>SSID:" + wifi_ssid +
                    "<br />password:" + wifi_pass +
                    "<br />mqtt_server:" + mqtt_server +
                    "<br />mqtt_port:" + String(mqtt_port) +
                    "<br />mqtt_username:" + mqtt_username +
                    "<br />mqtt_password:" + mqtt_password +
                    "<br />已取得WiFi信息,正在尝试连接,请手动关闭此页面。";

  config_save.resetcheck = false;
  wifi_needsave = true;
  request->send(200, "text/html", response);
  delay(100);
  if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA)
  {
    WiFi.disconnect(false, true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  }
  else
  {
    WiFi.softAPdisconnect(true); // 参数设置为true，设备将直接关闭接入点模式，即关闭设备所建立的WiFi网络。
    server.end();                // 关闭web服务
    WiFi.softAPdisconnect();     // 在不输入参数的情况下调用该函数,将关闭接入点模式,并将当前配置的AP热点网络名和密码设置为空值.
    // Serial.println("WiFi Connect SSID:" + wifi_ssid + "  PASS:" + wifi_pass);
  }

  if (WiFi.status() != WL_CONNECTED) // wifi没有连接成功
  {
    // Serial.println("开始调用连接函数connectToWiFi()..");
    connectToWiFi(connectTimeOut_s);
  }
}

void WebHandler::handleUpdateWifi(AsyncWebServerRequest *request)
{
  scanWiFi();
  htmlBuf[0] = '\0'; // 清空缓冲区

  strcpy_P(htmlBuf, ROOT_HTML_1);
  strcat(htmlBuf, scanNetworksID.c_str());
  strcat_P(htmlBuf, ROOT_HTML_2);

  request->send(200, "text/html", htmlBuf);
}

void WebHandler::handleData(AsyncWebServerRequest *request)
{
  if (!(request->hasParam("catchkey", true)))
  {
    request->send(200, "text/plain", "unknown command");
    return;
  }

  String catchkey = request->getParam("catchkey", true)->value();
  if (catchkey == "open")
  {
    catch_key = 1;
    my_printf("(http) Bambu-hub开启抓包");
    request->send(200, "text/plain", "open catch");
  }
  else if (catchkey == "close")
  {
    catch_key = 0;
    Set_24(false);
    my_printf("(http) Bambu-hub关闭抓包--断开从机电源20s");
    request->send(200, "text/plain", "close catch");
  }
  else if (catchkey == "catch_mode")
  {
    catch_mode = true;
    my_printf("(http) Bambu-hub设置为抓包模式--屏蔽输出");
    request->send(200, "text/plain", "catch mode");
  }
  else if (catchkey == "normal_mode")
  {
    catch_mode = false;
    my_printf("(http) Bambu-hub设置为normal模式--抓包数据包含bmcu数据");
    request->send(200, "text/plain", "normal mode");
  }
  else if (catchkey == "refresh")
  {
    if (!Bambus_onflush())
    {
      Switch_set_refresh(true);
      request->send(200, "text/plain", "success");
    }
    else
      request->send(200, "text/plain", "error, print is onuse");
  }

  if (C_data[0] == '\0')
  {
    request->send(200, "text/plain", "no catch data");
    return;
  }
  else if (!EN_catch)
    request->send(200, "text/plain", C_data);
  else if (EN_catch)
  {
    char *dataPtr = C_data;
    int dataLen = strlen(C_data);
    // 使用 chunked response 发送数据
    AsyncWebServerResponse *response = request->beginChunkedResponse("text/plain", [dataPtr, dataLen](uint8_t *buffer, size_t maxLen, size_t index) -> size_t
                                                                     {
        size_t bytesToSend = min(maxLen, (size_t)(dataLen - index));
        if (bytesToSend > 0) {
            memcpy(buffer, dataPtr + index, bytesToSend);
        }
        return bytesToSend; });

    // 生成带时间戳的文件名
    char filename[64];
    time_t now = time(nullptr);
    strftime(filename, sizeof(filename), "bmcu_capture_%Y%m%d_%H%M%S.txt", localtime(&now));

    // 添加下载相关的HTTP头
    response->addHeader("Content-Disposition", String("attachment; filename=\"") + filename + "\"");
    response->addHeader("Cache-Control", "no-cache");

    request->send(response);

    // 记录日志
    my_printf("(http) C_data downloaded, size: %d bytes", dataLen);
  }
}

void WebHandler::handleLog(AsyncWebServerRequest *request)
{
  if (L_data[0] == '\0')
  {
    request->send(200, "text/plain", "no log data");
    return;
  }

  char *p = L_data;
  int len = strlen(L_data);
  if (len > 1024 * 48)
    p += 1024 * 48;
  else if (len > 1024 * 32)
    p += 1024 * 32;
  else if (len > 1024 * 16)
    p += 1024 * 16;

  String html = "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>日志</title></head><body>" + String(p) + "</body></html>";
  request->send(200, "text/html", html);
}

void WebHandler::handleUpload(AsyncWebServerRequest *request)
{
  htmlBuf[0] = '\0'; // 清空缓冲区
  strcpy_P(htmlBuf, upload_html);
  request->send(200, "text/html", htmlBuf);
}

void WebHandler::handleUpdate(AsyncWebServerRequest *request)
{
  AsyncWebServerResponse *response = request->beginResponse(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
  response->addHeader("Connection", "close");
  request->send(response);
  delay(1000);
  ESP.restart();
}

void WebHandler::handleOTA(AsyncWebServerRequest *request)
{
  request->send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>OTA Update</title></head><body><h1>OTA Update Page</h1><p>OTA update online</p></body></html>");
}

void WebHandler::handleNotFound(AsyncWebServerRequest *request)
{
  handleRoot(request); // 返回根页面
}

void WebHandler::registerRoutes(AsyncWebServer &server)
{
  server.on("/", HTTP_GET, WebHandler::handleRoot);
  server.on("/configwifi", HTTP_POST, WebHandler::handleConfigWifi);
  server.on("/updatewifi", HTTP_POST, WebHandler::handleUpdateWifi);
  server.on("/data", HTTP_POST, WebHandler::handleData);
  server.on("/log", HTTP_POST, WebHandler::handleLog);
  server.on("/upload", HTTP_POST, WebHandler::handleUpload);
  // server.on("/update", HTTP_POST, WebHandler::handleUpdate);
  // server.on("/ota", HTTP_GET, WebHandler::handleOTA);
  server.onNotFound(WebHandler::handleNotFound);
  // 处理静态资源请求
  server.on("/.*\\.ico", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(404, "text/plain", "Not Found"); });
  // OTA 文件上传
  server.on("/update", HTTP_POST, [](AsyncWebServerRequest *request)
            {
            request->send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
            delay(1000);
            ESP.restart(); }, [](AsyncWebServerRequest *request, const String &filename, size_t index, uint8_t *data, size_t len, bool final)
            {
            if (!index) {
                Serial.printf("Update: %s\n", filename.c_str());
                if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
                    Update.printError(Serial);
                }
            }

            if (len) {
                if (Update.write(data, len) != len) {
                    Update.printError(Serial);
                }
            }

            if (final) {
                if (Update.end(true)) {
                    Serial.println("OTA Success");
                } else {
                    Update.printError(Serial);
                }
            } });
}
/*
 * 进入AP模式
 */
void initSoftAP()
{
  WiFi.mode(WIFI_AP);                                         // 配置为AP模式
  WiFi.softAPConfig(apIP, apIP, IPAddress(255, 255, 255, 0)); // 设置AP热点IP和子网掩码
  if (WiFi.softAP(AP_SSID))                                   // 开启AP热点,如需要密码则添加第二个参数
  {
    // 打印相关信息
    // Serial.println("ESP-32S SoftAP is right.");
    // Serial.print("Soft-AP IP address = ");
    // Serial.println(WiFi.softAPIP());                                                //接入点ip
    // Serial.println(String("MAC address = ")  + WiFi.softAPmacAddress().c_str());    //接入点mac
  }
  else // 开启AP热点失败
  {
    // Serial.println("WiFiAP Failed");
    // delay(1000);
    // Serial.println("restart now...");
    // ESP.restart();                                      //重启复位esp32
  }
}

/*
 * 开启DNS服务器
 */
void initDNS()
{
  MDNS.begin(host_name); // 开启MDNS服务，HOST_NAME是设备名
}

/*
 * 初始化WebServer
 */

void stopWebServer()
{
  server.end(); // 停止web服务
  // ESP.restart();                                          //重启设备
  // dnsServer.stop();                                        //停止dns服务
  // Serial.println("WebServer stopped!");
}
void startwebServer()
{
  server.begin();
  // Serial.println("WebServer started!");
}
void stopDNS()
{
  MDNS.end(); // 停止dns服务
  // Serial.println("DNS stopped!");
}
/*
 * 扫描附近的WiFi，为了显示在配网界面
 */
bool scanWiFi()
{
  // Serial.println("scan start");
  // Serial.println("--------->");
  //  扫描附近WiFi
  int n = WiFi.scanNetworks();
  // Serial.println("scan done");
  if (n == 0)
  {
    // Serial.println("no networks found");
    scanNetworksID += "<option>no networks found</option>";
    return false;
  }
  else
  {
    // Serial.print(n);
    // Serial.println(" networks found");
    for (int i = 0; i < min(n, 5); ++i)
    {
      scanNetworksID += "<option>" + WiFi.SSID(i) + "</option>";
    }
    scanNetworksID += "</datalist>";
    return true;
  }
}

/*
 * 连接WiFi
 */
void connectToWiFi(int timeOut_s)
{
  WiFi.setHostname(host_name); // 设置设备名
  // Serial.println("进入connectToWiFi()函数");
  WiFi.mode(WIFI_STA);       // 设置为STA模式并连接WIFI
  WiFi.setAutoConnect(true); // 设置自动连接

  if (wifi_ssid != "") // wifi_ssid不为空，意味着从网页或存储读取到wifi
  {
    // 使用局部变量存储 wifi_ssid 和 wifi_pass
    const char *ssid = wifi_ssid.c_str();
    const char *pass = wifi_pass.c_str();

    // Serial.println("用web配置信息连接.");
    WiFi.begin(ssid, pass); // 使用局部变量的指针
    wifi_ssid = "";
    wifi_pass = "";
  }
  else // 未从网页读取到wifi
  {
    // Serial.println("用nvs保存的信息连接.");
    WiFi.begin();
    ESP_LOGE("WIFI", "WiFi_connect_id : default");
  }

  int Connect_time = 0;                 // 用于连接计时，如果长时间连接不成功，复位设备
  while (WiFi.status() != WL_CONNECTED) // 等待WIFI连接成功
  {
    // Serial.print(".");

    delay(500);
    Connect_time++;

    if (Connect_time > 2 * timeOut_s) // 长时间连接不上，重新进入配网页面
    {

      // Serial.println("");                     //主要目的是为了换行符
      ESP_LOGE("WIFI", "connect fail, check id and psd start AP for webconfig now...");
      wifiConfig(); // 开始配网功能
      return;       // 跳出 防止无限初始化
    }
  }

  if (WiFi.status() == WL_CONNECTED) // 如果连接成功
  {
    /*
    Serial.println("WIFI connect Success");
    Serial.printf("SSID:%s", WiFi.SSID().c_str());
    Serial.printf(", PSW:%s\r\n", WiFi.psk().c_str());
    Serial.print("LocalIP:");
    Serial.print(WiFi.localIP());
    Serial.print(" ,GateIP:");
    Serial.println(WiFi.gatewayIP());
    Serial.print("WIFI status is:");
    Serial.print(WiFi.status());

    */
    // my_log("<br />(http) Bambu-hub配置WiFi成功");
    config_save.resetcheck = false; // 如果wifi连接成功，则将resetcheck设置为false
    // Config_save();
    if (millis() > 600000 && server_key)
      initWebServer();
    // MDNS.end(); // 停止DNS服务器
    //  server.stop();                            //停止开发板所建立的网络服务器。
  }
}

/*
 * 配置配网功能
 */

void initWebServer()
{
  server.reset();
  WebHandler::registerRoutes(server);
  server.begin();
  // Serial.println("WebServer started!");
}

void webServerTask(void *parameter)
{
  bool task_flag = true;
  while (true)
  {
    if (server_key)
    {
      if (task_flag)
      {
        task_flag = false;
        initWebServer();
      }
    }
    else
    {
      if (!task_flag)
      {
        task_flag = true;
        server.end();
      }
    }

    vTaskDelay(pdMS_TO_TICKS(500));
  }
}
// 在 setup() 中启动任务
void webtask_setup()
{
  // server.reset();
  // initWebServer();
  xTaskCreate(webServerTask, "WebServer", 8192, NULL, 0, NULL);
}

void wifiConfig()
{
  initSoftAP();
  initDNS();
  initWebServer();
  scanWiFi();
  server_key = true;
}

/*
 * 删除保存的wifi信息，这里的删除是删除存储在flash的信息。删除后wifi读不到上次连接的记录，需重新配网
 */
void restoreWiFi()
{
  delay(500);
  // esp_wifi_restore();  //删除保存的wifi信息
  Serial.println("连接信息已清空,准备重启设备..");
  delay(10);
}

/*
 * 检查wifi是否已经连接
 */
void checkConnect(bool reConnect)
{

  if (reConnect == true && WiFi.getMode() != WIFI_AP && WiFi.getMode() != WIFI_AP_STA)
  {
    // Serial.println("WIFI未连接.");
    // Serial.println("WiFi Mode:");
    // Serial.println(WiFi.getMode());
    // Serial.println("正在连接WiFi...");
    wifiConfig(); // 开始配网功能
    return;       // 跳出 防止无限初始化
  }
  if (reConnect == false)
  {
    connectToWiFi(connectTimeOut_s); // 连接wifi函数
  }

  // wifi连接成功
}

/*
 * 检测客户端DNS&HTTP请求
 */
