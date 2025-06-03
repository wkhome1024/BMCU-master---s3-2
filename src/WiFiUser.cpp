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
void Config_save()
{
  if (!Flash_saves(&config_save, sizeof(config_save), config_addr))
    ESP_LOGE("FLASH", "wifi保存失败");

  save_count++;
}

// DNSServer dnsServer;                       //创建dnsServer实例
WebServer server(webPort); // 开启web服务, 创建TCP SERVER,参数: 端口号,最大连接数

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
/*
String config_HTML = R"(
<!DOCTYPE html>
<html lang="zh-CN">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width">
  <title>BMCU设置</title>
  <style>
    body {background:#e5e9f2;margin:0;display:grid;place-items:center;min-height:100vh;font-family:sans-serif}
    .card {width:min(90vw,300px);padding:2rem;background:#f7f7f7;border-radius:1rem;box-shadow:0 .5rem 1rem rgba(0,0,0,.3);text-align:center}
    h1 {color:#1383c6;margin-bottom:1.5rem}h1 span {color:#f26721}
    .btn {width:100%;padding:.75rem;margin:.5rem 0;background:#4d90fe;color:#fff;border:none;border-radius:.5rem;font-size:1rem;font-weight:600;cursor:pointer;position:relative;overflow:hidden}
    .btn:hover {background:#357ae8}.btn:disabled {opacity:.6;cursor:not-allowed}
    .btn.progress::after {content:'';position:absolute;bottom:0;left:0;height:3px;background:#4CAF50;width:var(--progress,0%);transition:width 2s linear}
    .btn.active {background:#4CAF50;box-shadow:0 0 10px rgba(76,175,80,.5)}
    .divider {margin:1.5rem 0;border-top:1px solid #d9d9d9}
    .status-text {font-size:.8em;color:#666;margin-top:.5rem;height:1.2em}
  </style>
</head>
<body>
  <div class="card">
    <h1>BMCU-HUB</h1>
    <form action="/upload" method="POST"><button class="btn">固件更新</button></form>
    <div class="divider"></div>
    <form action="/updatewifi" method="POST">
      <button id="wifiBtn" class="btn" disabled>更新wifi+mqtt参数</button>
      <div class="status-text" id="statusText"></div>
    </form>
  </div>
  <script>
    const wifiBtn = document.getElementById('wifiBtn');
    const statusText = document.getElementById('statusText');
    let hoverTimer;
    wifiBtn.addEventListener('mouseenter', () => {
      if (wifiBtn.disabled) {
        wifiBtn.classList.add('progress');
        wifiBtn.style.setProperty('--progress', '100%');
        statusText.textContent = '激活中...';
        hoverTimer = setTimeout(() => {
          wifiBtn.disabled = false;
          wifiBtn.classList.remove('progress');
          wifiBtn.classList.add('active');
          statusText.textContent = '按钮已激活';
        }, 2000);
      }
    });
    wifiBtn.addEventListener('mouseleave', () => {
      if (wifiBtn.disabled) {
        clearTimeout(hoverTimer);
        wifiBtn.classList.remove('progress');
        wifiBtn.style.removeProperty('--progress');
        statusText.textContent = '';
      }
    });
  </script>
</body>
</html>
)";
*/
// String ROOT_HTML_2 = "</form> </body></html>";
// String ROOT_HTML_3 = "<meta charset='UTF-8'>error, not found ssid";
// String ROOT_HTML_4 = "<meta charset='UTF-8'>error, not found ssid";

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
          <option value="close">关闭抓包</option>
          <option value="catch_mode">开启抓包模式</option>
          <option value="normal_mode">关闭抓包模式</option>
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
void handleRoot()
{
  htmlBuf[0] = '\0'; // 清空缓冲区

  if (WiFi.getMode() != WIFI_STA) // 如果没有连接wifi
  {
    strcpy_P(htmlBuf, ROOT_HTML_1);
    strcpy(htmlBuf, scanNetworksID.c_str());
    strcpy_P(htmlBuf, ROOT_HTML_2);
  }
  else if (WiFi.status() == WL_CONNECTED) // 如果已连接wifi
  {
    strcpy_P(htmlBuf, root2_html);
  }

  server.send(200, "text/html", htmlBuf);
}

/*
 * 提交数据后的提示页面
 */
void handleConfigWifi() // 返回http状态
{
  if (server.hasArg("ssid")) // 判断是否有账号参数
  {
    // Serial.print("got ssid:");
    wifi_ssid = server.arg("ssid");                                       // 获取html表单输入框name名为"ssid"的内容
    wifi_ssid.trim();                                                     // 去除前后空格
    memcpy(config_save.wifi_ssid, wifi_ssid.c_str(), wifi_ssid.length()); // 将wifi_ssid的内容复制到config_save.wifi_ssid中
    // Serial.println(wifi_ssid);
  }
  else // 没有参数
  {
    // Serial.println("error, not found ssid");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found ssid"); // 返回错误页面
    return;
  }
  // 密码与账号同理
  if (server.hasArg("password"))
  {
    // Serial.print("got password:");
    wifi_pass = server.arg("password");                                       // 获取html表单输入框name名为"password"的内容
    wifi_pass.trim();                                                         // 去除前后空格
    memcpy(config_save.wifi_password, wifi_pass.c_str(), wifi_pass.length()); // 将wifi_pass的内容复制到config_save.wifi_password中
    // Serial.println(wifi_pass);
  }
  else
  {
    // Serial.println("error, not found password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found password");
    return;
  }

  if (server.hasArg("mqtt_server"))
  {
    // Serial.print("got mqtt_server:");
    mqtt_server = server.arg("mqtt_server");                                    // 获取html表单输入框name名为"mqtt_server"的内容
    mqtt_server.trim();                                                         // 去除前后空格
    memcpy(config_save.mqtt_server, mqtt_server.c_str(), mqtt_server.length()); // 将mqtt_server的内容复制到config_save.mqtt_server中
    // Serial.println(mqtt_server);
  }
  else
  {
    // Serial.println("error, not found mqtt_server");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_server");
    return;
  }

  if (server.hasArg("mqtt_port"))
  {
    // Serial.print("got mqtt_port:");
    String s_port = server.arg("mqtt_port"); // 获取html表单输入框name名为"mqtt_port"的内容
    s_port.trim();                           // 去除前后空格
    mqtt_port = s_port.toInt();              // 将字符串转换为整数
    config_save.mqtt_port = mqtt_port;       // 将mqtt_port的内容复制到config_save.mqtt_port中
    // Serial.println(mqtt_port);
  }
  else
  {
    // Serial.println("error, not found mqtt_port");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_port");
    return;
  }
  if (server.hasArg("mqtt_username"))
  {
    // Serial.print("got mqtt_username:");
    mqtt_username = server.arg("mqtt_username");                                      // 获取html表单输入框name名为"mqtt_username"的内容
    mqtt_username.trim();                                                             // 去除前后空格
    memcpy(config_save.mqtt_username, mqtt_username.c_str(), mqtt_username.length()); // 将mqtt_username的内容复制到config_save.mqtt_username中
    // Serial.println(mqtt_username);
  }
  else
  {
    // Serial.println("error, not found mqtt_username");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_username");
    return;
  }
  if (server.hasArg("mqtt_password"))
  {
    // Serial.print("got mqtt_password:");
    mqtt_password = server.arg("mqtt_password");                                      // 获取html表单输入框name名为"mqtt_password"的内容
    mqtt_password.trim();                                                             // 去除前后空格
    memcpy(config_save.mqtt_password, mqtt_password.c_str(), mqtt_password.length()); // 将mqtt_password的内容复制到config_save.mqtt_password中
    // Serial.println(mqtt_password);
  }
  else
  {
    // Serial.println("error, not found mqtt_password");
    server.send(200, "text/html", "<meta charset='UTF-8'>error, not found mqtt_password");
    return;
  }
  server.send(200, "text/html", "<meta charset='UTF-8'>SSID:" + wifi_ssid + "<br />password:" + wifi_pass + "<br />mqtt_server:" + mqtt_server + "<br />mqtt_port:" + String(mqtt_port) + "<br />mqtt_username:" + mqtt_username + "<br />mqtt_password:" + mqtt_password + "<br />已取得WiFi信息,正在尝试连接,请手动关闭此页面。"); // 返回保存成功页面
  config_save.resetcheck = false;
  Config_save(); // 保存配置
  delay(1000);
  if (WiFi.status() == WL_CONNECTED && WiFi.getMode() == WIFI_STA)
  {
    WiFi.disconnect(false, true);
    WiFi.mode(WIFI_STA);
    WiFi.begin(wifi_ssid.c_str(), wifi_pass.c_str());
  }
  else
  {
    WiFi.softAPdisconnect(true); // 参数设置为true，设备将直接关闭接入点模式，即关闭设备所建立的WiFi网络。
    server.close();              // 关闭web服务
    WiFi.softAPdisconnect();     // 在不输入参数的情况下调用该函数,将关闭接入点模式,并将当前配置的AP热点网络名和密码设置为空值.
    // Serial.println("WiFi Connect SSID:" + wifi_ssid + "  PASS:" + wifi_pass);
  }

  if (WiFi.status() != WL_CONNECTED) // wifi没有连接成功
  {
    // Serial.println("开始调用连接函数connectToWiFi()..");
    connectToWiFi(connectTimeOut_s);
  }

  // Config_save(); //保存配置
}

void handleUpdateWifi() // 返回http状态
{
  scanWiFi();
  htmlBuf[0] = '\0'; // 清空缓冲区
  strcpy_P(htmlBuf, ROOT_HTML_1);
  strcpy(htmlBuf, scanNetworksID.c_str());
  strcpy_P(htmlBuf, ROOT_HTML_2);
  server.send(200, "text/html", htmlBuf);

  // server.send(200, "text/html", ROOT_HTML_1 + scanNetworksID + ROOT_HTML_2); // scanNetWprksID是扫描到的wifi

  // scanWiFi();
  // server.send(200, "text/html", ROOT_HTML_1 + scanNetworksID + ROOT_HTML_2);
  /*
  server.send(200, "text/html", "<meta charset='UTF-8'>即将断开连接,请手动切换到bmcu-hub-ap进行wifi配置,配置模式持续120s"); // 返回保存成功页面
  WiFi.disconnect();                                                                                                        // 断开当前连接的WiFi
  delay(500);
  wifiConfig();                                             // 调用wifiConfig函数，配置WiFi信息
  int count = 0;                                            // 计数器
  while (WiFi.status() != WL_CONNECTED && count < 120 * 10) // 等待WiFi连接成功或超时120s
  {
    delay(100);
    count++;
    // Serial.println("等待WiFi连接成功...");
    checkDNS_HTTP(); // 检查DNS和HTTP请求
  }
  if (WiFi.status() == WL_CONNECTED) // 如果WiFi连接成功
  {
    // server.send(200, "text/html", "<meta charset='UTF-8'>WiFi连接成功!<br />IP地址: " + WiFi.localIP().toString()); //返回连接成功页面
    my_printf("(http) Bambu-hub更新wifi+mqtt参数成功");
  }
  else // 如果WiFi连接失败
  {
    // server.send(200, "text/html", "<meta charset='UTF-8'>WiFi连接失败，请检查配置参数是否正确。"); //返回连接失败页面
    my_printf("(http) Bambu-hub更新wifi+mqtt参数失败");
    WiFi.softAPdisconnect();
    WiFi.mode(WIFI_STA);                                          // 设置WiFi为STA模式
    WiFi.begin(config_save.wifi_ssid, config_save.wifi_password); // 重新开始WiFi连接
  }*/
  // ESP.restart();                   //重启设备
}

/*
 * 处理404情况的函数'handleNotFound'
 */
void handleNotFound() // 当浏览器请求的网络资源无法在服务器找到时通过此自定义函数处理
{
  handleRoot(); // 访问不存在目录则返回配置页面
  //   server.send(404, "text/plain", "404: Not found");
}

void handleConfig()
{
  server.send(200, "text/html", "config_HTML");
}

void handleUpload()
{
  htmlBuf[0] = '\0'; // 清空缓冲区
  strcpy_P(htmlBuf, upload_html);
  server.send(200, "text/html", htmlBuf);
  // server.send(200, "text/html", upload_html);
}

void handleUpdate()
{
  server.send(200, "text/html", "update_html");
}

void handleota()
{
  server.send(200, "text/html", "<!DOCTYPE html><html><head><meta charset='UTF-8'><title>OTA Update</title></head><body><h1>OTA Update Page</h1><p>OTA update online</p></body></html>");
  // Serial.println("OTA update page requested, but not implemented yet.");
  // OTA_key = true; // 设置OTA开关为true
  // my_log("<br />(http) Bambu-hub开启OTA");
}

void handlelog()
{
  if (L_data[0] == '\0')
  {
    server.send(200, "text/plain", "no log data");
    return;
  }
  char *p = L_data;
  int len = strlen(L_data);
  if (len > 1024 * 48)
  {
    p = p + 1024 * 48;
  }
  else if (len > 1024 * 32)
  {
    p = p + 1024 * 32;
  }
  else if (len > 1024 * 16)
  {
    p = p + 1024 * 16;
  }

  server.send(200, "text/html", "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>日志</title> </head> <body>" + String(p) + "</body> </html>");
  // server.sendContent("<meta charset='UTF-8'>LOG:<br />"  + String(L_data) + "</body> </html>");
  // server.sendContent(L_data);
  // server.sendContent("</body> </html>");
}

void handleData()
{
  if (server.hasArg("catchkey")) // 判断是否有抓包参数
  {
    String catchkey = server.arg("catchkey"); // 获取html表单输入框name名为"catchkey"的内容
    // catchkey.trim();                          // 去除前后空格
    if (catchkey == "open") // 如果抓包参数为"开启抓包"
    {
      catch_key = 1; // 开启抓包
      my_printf("(http) Bambu-hub开启抓包");
      server.send(200, "text/plain", "open catch");
      return;
    }
    else if (catchkey == "close") // 如果抓包参数为"关闭抓包"
    {
      catch_key = 0; // 关闭抓包
      my_printf("(http) Bambu-hub关闭抓包");
      server.send(200, "text/plain", "close catch");
      return;
    }
    else if (catchkey == "catch_mode") // 如果抓包参数为"catch_mode"
    {
      catch_mode = true; // 设置为抓包模式
      my_printf("(http) Bambu-hub设置为抓包模式--屏蔽输出");
      server.send(200, "text/plain", "catch mode");
      return;
    }
    else if (catchkey == "normal_mode") // 如果抓包参数为"normal_mode"
    {
      catch_mode = false; // 设置为normal模式
      my_printf("(http) Bambu-hub设置为normal模式--抓包数据包含bmcu数据");
      server.send(200, "text/plain", "normal mode");
      return;
    }
    // server.send(200, "text/plain", catchkey); // 返回错误页面
  }

  if (C_data[0] == '\0')
  {
    server.send(200, "text/plain", "no catch data");
    return;
  }
  else
    server.send(200, "text/plain", C_data);
  // server.sendContent("<br />");
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
void initWebServer()
{

  server.on("/", HTTP_POST, handleRoot);

  // server.on("/", HTTP_GET, handleRoot);                      //  当浏览器请求服务器根目录(网站首页)时调用自定义函数handleRoot处理，设置主页回调函数，必须添加第二个参数HTTP_GET，否则无法强制门户
  server.on("/configwifi", HTTP_POST, handleConfigWifi); //  当浏览器请求服务器/configwifi(表单字段)目录时调用自定义函数handleConfigWifi处理
  // server.on("/config", HTTP_POST, handleConfig);
  server.on("/data", HTTP_POST, handleData);
  server.on("/log", HTTP_POST, handlelog);
  // server.on("/update", HTTP_POST, handleUpdate);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/updatewifi", HTTP_POST, handleUpdateWifi); //  当浏览器请求服务器/updatewifi(表单字段)目录时调用自定义函数handleUpdateWifi处理
  server.onNotFound(handleNotFound);                     // 当浏览器请求的网络资源无法在服务器找到时调用自定义函数handleNotFound处理
  server.on("/.*\\.ico", HTTP_GET, []()
            {
              server.send(404, "text/plain", "Not Found"); // 忽略所有静态资源请求
            });
  server.on("/update", HTTP_POST, []()
            {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    delay(1000);
    ESP.restart(); }, []()
            {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      ESP_LOGE("Update","Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        ESP_LOGE("Update","Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    } });

  server.begin(); // 启动TCP SERVER

  // Serial.println("WebServer started!");
}

void stopWebServer()
{
  server.stop(); // 停止web服务
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
  WiFi.mode(WIFI_STA); // 设置为STA模式并连接WIFI
  // WiFi.setAutoConnect(true); // 设置自动连接

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
    if (millis() > 60000 && server_key)
        server.begin();
    // MDNS.end(); // 停止DNS服务器
    //  server.stop();                            //停止开发板所建立的网络服务器。
  }
}

/*
 * 配置配网功能
 */
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
      server.handleClient();
    }
    else
    {
      if (!task_flag)
      {
        task_flag = true;
        server.stop();
      }
    }


    vTaskDelay(pdMS_TO_TICKS(200));
  }
}

// 在 setup() 中启动任务
void webtask_setup()
{
  //initWebServer();
  xTaskCreatePinnedToCore(webServerTask, "WebServer", 8192 * 2, NULL, 0, NULL, 0);
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
void checkDNS_HTTP()
{

  // dnsServer.processNextRequest();   //检查客户端DNS请求
  server.handleClient(); // 检查客户端(浏览器)http请求
}