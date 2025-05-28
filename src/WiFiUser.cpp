#include "WiFiUser.h"

const byte DNS_PORT = 53; // 设置DNS端口号
const int webPort = 80;   // 设置Web端口号

const char *AP_SSID = "Bmcu-hub-AP"; // 设置AP热点名称
// const char* AP_PASS  = "";               //这里不设置设置AP热点密码

//const char *HOST_NAME = "bmcu-hub-s3"; // 设置设备名
String scanNetworksID = "";            // 用于储存扫描到的WiFi ID
int connectTimeOut_s = 30;             // WiFi连接超时时间，单位秒
IPAddress apIP(192, 168, 4, 1);        // 设置AP的IP地址

String wifi_ssid = "";     // 暂时存储wifi账号密码
String wifi_pass = "";     // 暂时存储wifi账号密码
String mqtt_server = "";   // 暂时存储mqtt服务器地址
int mqtt_port = 0;         // 暂时存储mqtt服务器端口
String mqtt_username = ""; // 暂时存储mqtt用户名
String mqtt_password = ""; // 暂时存储mqtt密码

//char L_data[20] = "test1234567890"; // test
//char C_data[20] = "test1234567890";

#define config_addr ((uint32_t)0x0900)

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
  config_struct *ptr = (config_struct *)(EEPROM.getDataPtr() + config_addr);
  // flash_save_struct ptr;
  // Flash_read(&ptr,sizeof(ptr),use_flash_addr);
  if (ptr->version == Bambubus_version)
  {
    memcpy(&config_save, ptr, sizeof(config_save));
    mqtt_server = config_save.mqtt_server;
    mqtt_port = config_save.mqtt_port;
    mqtt_username = config_save.mqtt_username;
    mqtt_password = config_save.mqtt_password;
  }
  wifi_ssid = "";
  wifi_pass = "";
  // checkConnect(config_save.resetcheck);

  if (wifi_ssid != "") // wifi_ssid不为空，意味着从网页读取到wifi
  {
    // 使用局部变量存储 wifi_ssid 和 wifi_pass
    const char *ssid = wifi_ssid.c_str();
    const char *pass = wifi_pass.c_str();

    // Serial.println("用web配置信息连接.");
    WiFi.begin(ssid, pass); // 使用局部变量的指针
    wifi_ssid = "";
    wifi_pass = "";
  }

  return config_save.resetcheck;
}
void Config_save()
{
  Flash_saves(&config_save, sizeof(config_save), config_addr);
}

// DNSServer dnsServer;                       //创建dnsServer实例
WebServer server(webPort); // 开启web服务, 创建TCP SERVER,参数: 端口号,最大连接数

// 上下两段HTML代码
String ROOT_HTML_1 = R"(
<!DOCTYPE html>
<html>
<head>
  <meta charset="UTF-8">
  <meta http-equiv="content-type" content="text/html; charset=UTF-8" />
  <title>WIFI设置页面</title>
  <style>
    #content, .login, .login-card a, .login-card h1, .login-help { text-align: center }
    body, html { margin: 0; padding: 0; width: 100%; height: 100%; display: table }
    #content { font-family: 'Source Sans Pro', sans-serif; -webkit-background-size: cover; -moz-background-size: cover; -o-background-size: cover; background-size: cover; display: table-cell; vertical-align: middle }
    .login-card { padding: 40px; width: 274px; background-color: #F7F7F7; margin: 0 auto 10px; border-radius: 20px; box-shadow: 8px 8px 15px rgba(0,0,0,.3); overflow: hidden }
    .login-card h1 { font-weight: 400; font-size: 2.3em; color: #1383c6 }
    .login-card h1 span { color: #f26721 }
    .login-card img { width: 70%; height: 70% }
    .login-card input[type=submit] { width: 100%; display: block; margin-bottom: 10px; position: relative }
    .login-card input[type=text], input[type=password] { height: 44px; font-size: 16px; width: 100%; margin-bottom: 10px; -webkit-appearance: none; background: #fff; border: 1px solid #d9d9d9; border-top: 1px solid silver; padding: 0 8px; box-sizing: border-box; -moz-box-sizing: border-box }
    .login-card input[type=text]:hover, input[type=password]:hover { border: 1px solid #b9b9b9; border-top: 1px solid #a0a0a0; -moz-box-shadow: inset 0 1px 2px rgba(0,0,0,.1); -webkit-box-shadow: inset 0 1px 2px rgba(0,0,0,.1); box-shadow: inset 0 1px 2px rgba(0,0,0,.1) }
    .login { font-size: 14px; font-family: Arial,sans-serif; font-weight: 700; height: 36px; padding: 0 8px }
    .login-submit { -webkit-appearance: none; -moz-appearance: none; appearance: none; border: 0; color: #fff; text-shadow: 0 1px rgba(0,0,0,.1); background-color: #4d90fe }
    .login-submit:disabled { opacity: .6 }
    .login-submit:hover { border: 0; text-shadow: 0 1px rgba(0,0,0,.3); background-color: #357ae8 }
    .login-card a { text-decoration: none; color: #666; font-weight: 400; display: inline-block; opacity: .6; transition: opacity ease .5s }
    .login-card a:hover { opacity: 1 }
    .login-help { width: 100%; font-size: 12px }
    .list { list-style-type: none; padding: 0 }
    .list__item { margin: 0 0 .7rem; padding: 0 }
    label { display: -webkit-box; display: -webkit-flex; display: -ms-flexbox; display: flex; -webkit-box-align: center; -webkit-align-items: center; -ms-flex-align: center; align-items: center; text-align: left; font-size: 14px; }
    input[type=checkbox] { -webkit-box-flex: 0; -webkit-flex: none; -ms-flex: none; flex: none; margin-right: 10px; float: left }
    .error { font-size: 14px; font-family: Arial,sans-serif; font-weight: 700; height: 25px; padding: 0 8px; padding-top: 10px; -webkit-appearance: none; -moz-appearance: none; appearance: none; border: 0; color: #fff; text-shadow: 0 1px rgba(0,0,0,.1); background-color: #ff1215 }
    @media screen and (max-width:450px) {
      .login-card { width: 70%!important }
      .login-card img { width: 30%; height: 30% }
    }
  </style>
</head>
<body style="background-color: #e5e9f2">
  <div id="content">
    <form name='input' action='/configwifi' method='POST'>
      <div class="login-card">
        <h1>WiFi+MQTT</h1>
        <form name="login_form" method="post" action="$PORTAL_ACTION$">
          <input type="text" name="ssid" placeholder="请选择 WiFi 名称" id="auth_user" list="data-list" style="border-radius: 10px">
          <datalist id="data-list">
)";

String ROOT_HTML_2 = R"(
          <input type="password" name="password" placeholder="请输入 WiFi 密码" id="auth_pass" style="border-radius: 10px">
          <input type="text" name="mqtt_server" placeholder="请输入 MQTT IP" id="auth_mqtt_ip" style="border-radius: 10px">
          <input type="text" name="mqtt_port" placeholder="请输入 MQTT 端口" id="auth_mqtt_port" style="border-radius: 10px">
          <input type="text" name="mqtt_username" placeholder="请输入 MQTT 用户名" id="auth_mqtt_id" style="border-radius: 10px">
          <input type="text" name="mqtt_password" placeholder="请输入 MQTT 密码" id="auth_mqtt_pd" style="border-radius: 10px">
          <div class="login-help">
            <ul class="list">
              <li class="list__item"></li>
            </ul>
          </div>
          <input type="submit" class="login login-submit" value="确 定 连 接" id="login" disabled style="border-radius: 15px">
        </form>
      </div>
    </form>
  </div>
</body>
</html>
)";

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
// String ROOT_HTML_2 = "</form> </body></html>";
// String ROOT_HTML_3 = "<meta charset='UTF-8'>error, not found ssid";
// String ROOT_HTML_4 = "<meta charset='UTF-8'>error, not found ssid";

String root2_html = R"(
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
    <form action="/config" method="POST"><div class="form-group"><button type="submit">HUB设置</button></div></form>
  </div>
</body>
</html>
)";

String upload_html = R"(
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
        .upload-area {border:2px dashed #ddd;border-radius:8px;padding:30px;margin-bottom:20px;transition:all .3s}
        .upload-area:hover {border-color:#4d90fe;background:#f8faff}
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
      <div class="upload-area" id="dropZone">
        <p>拖放更新固件到此处或</p>
        <input type="file" id="fileInput" name="file" style="display: none;" accept=".bin" maxSize="1500000">
        <label for="fileInput" class="upload-btn">选择固件文件</label>
        <div class="file-info" id="fileName">未选择文件</div>
      </div>
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
    const dropZone = document.getElementById('dropZone');
    const uploadProgress = document.getElementById('uploadProgress');
    fileInput.addEventListener('change', (e) => {
      if(e.target.files.length) {
        const fileSizeInKB = (e.target.files[0].size / 1024).toFixed(2);
        fileName.textContent = `已选择: ${e.target.files[0].name}，文件大小 ${fileSizeInKB} KB`;
      }
    });
    dropZone.addEventListener('dragover', (e) => {
      e.preventDefault();
      dropZone.style.borderColor = '#4d90fe';
      dropZone.style.backgroundColor = '#f8faff';
    });
    dropZone.addEventListener('dragleave', () => {
      dropZone.style.borderColor = '#ddd';
      dropZone.style.backgroundColor = 'transparent';
    });
    dropZone.addEventListener('drop', (e) => {
      e.preventDefault();
      dropZone.style.borderColor = '#ddd';
      dropZone.style.backgroundColor = 'transparent';
      if(e.dataTransfer.files.length) {
        fileInput.files = e.dataTransfer.files;
        fileName.textContent = `已选择: ${e.dataTransfer.files[0].name}`;
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
)";

/*
 * 处理网站根目录的访问请求
 */
void handleRoot()
{
  if (WiFi.getMode() != WIFI_STA) // 如果没有连接wifi
  {
    server.send(200, "text/html", ROOT_HTML_1 + scanNetworksID + ROOT_HTML_2); // scanNetWprksID是扫描到的wifi
  }
  else if (WiFi.status() == WL_CONNECTED) // 如果连接wifi
  {
    server.send(200, "text/html", root2_html);
  }
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

  delay(2000);
  WiFi.softAPdisconnect(true); // 参数设置为true，设备将直接关闭接入点模式，即关闭设备所建立的WiFi网络。
  server.close();              // 关闭web服务
  WiFi.softAPdisconnect();     // 在不输入参数的情况下调用该函数,将关闭接入点模式,并将当前配置的AP热点网络名和密码设置为空值.
  // Serial.println("WiFi Connect SSID:" + wifi_ssid + "  PASS:" + wifi_pass);
  Config_save();                     // 保存配置
  if (WiFi.status() != WL_CONNECTED) // wifi没有连接成功
  {
    // Serial.println("开始调用连接函数connectToWiFi()..");
    connectToWiFi(connectTimeOut_s);
  }
  else
  {
    config_save.resetcheck = false; // 如果wifi连接成功，则将resetcheck设置为false
    // Serial.println("提交的配置信息自动连接成功..");
  }
  // Config_save(); //保存配置
}

void handleUpdateWifi() // 返回http状态
{
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
  }
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
  server.send(200, "text/html", config_HTML);
}

void handleUpload()
{
  server.send(200, "text/html", upload_html);
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
  // Serial.println("日志数据");
  // String str = L_data;
  // str.replace("\n", "<br />");
  server.send(200, "text/html", "<!DOCTYPE html> <html lang=\"en\"> <head> <meta charset=\"UTF-8\"> <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\"> <title>日志</title> </head> <body>" + String(L_data) + "</body> </html>");
  // server.sendContent("<meta charset='UTF-8'>LOG:<br />"  + String(L_data) + "</body> </html>");
  // server.sendContent(L_data);
  // server.sendContent("</body> </html>");
}

void handleData()
{
  if (server.hasArg("catchkey")) // 判断是否有抓包参数
  {
    String catchkey = server.arg("catchkey"); // 获取html表单输入框name名为"catchkey"的内容
    //catchkey.trim();                          // 去除前后空格
    if (catchkey == "open")                   // 如果抓包参数为"开启抓包"
    {
      catch_key = 1; //开启抓包
      my_printf("(http) Bambu-hub开启抓包");
      server.send(200, "text/plain", "open catch");
      return;
    }
    else if (catchkey == "close") // 如果抓包参数为"关闭抓包"
    {
      catch_key = 0; //关闭抓包
      my_printf("(http) Bambu-hub关闭抓包");
      server.send(200, "text/plain", "close catch");
      return;
    }
    else if (catchkey == "catch_mode") // 如果抓包参数为"catch_mode"
    {
      catch_mode = true; //设置为抓包模式
      my_printf("(http) Bambu-hub设置为抓包模式--屏蔽输出");
      server.send(200, "text/plain", "catch mode");
      return;
    }
    else if (catchkey == "normal_mode") // 如果抓包参数为"normal_mode"
    {
      catch_mode = false; //设置为normal模式
      my_printf("(http) Bambu-hub设置为normal模式--抓包数据包含bmcu数据");
      server.send(200, "text/plain", "normal mode");
      return;
    }
    //server.send(200, "text/plain", catchkey); // 返回错误页面
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

  // 必须添加第二个参数HTTP_GET，以下面这种格式去写，否则无法强制门户
  if (WiFi.getMode() == WIFI_AP)
  {
    server.on("/", HTTP_GET, handleRoot);
  }
  else
  {
    server.on("/", HTTP_POST, handleRoot);
  }
  // server.on("/", HTTP_GET, handleRoot);                      //  当浏览器请求服务器根目录(网站首页)时调用自定义函数handleRoot处理，设置主页回调函数，必须添加第二个参数HTTP_GET，否则无法强制门户
  server.on("/configwifi", HTTP_POST, handleConfigWifi); //  当浏览器请求服务器/configwifi(表单字段)目录时调用自定义函数handleConfigWifi处理
  server.on("/config", HTTP_POST, handleConfig);
  server.on("/data", HTTP_POST, handleData);
  server.on("/log", HTTP_POST, handlelog);
  // server.on("/update", HTTP_POST, handleUpdate);
  server.on("/upload", HTTP_POST, handleUpload);
  server.on("/updatewifi", HTTP_POST, handleUpdateWifi); //  当浏览器请求服务器/updatewifi(表单字段)目录时调用自定义函数handleUpdateWifi处理
  server.onNotFound(handleNotFound);                     // 当浏览器请求的网络资源无法在服务器找到时调用自定义函数handleNotFound处理

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
    for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      // Serial.print(i + 1);
      // Serial.print(": ");
      // Serial.print(WiFi.SSID(i));
      // Serial.print(" (");
      // Serial.print(WiFi.RSSI(i));
      // Serial.print(")");
      // Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
      scanNetworksID += "<option>" + WiFi.SSID(i) + "</option>";
      delay(10);
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

  if (wifi_ssid != "") // wifi_ssid不为空，意味着从网页读取到wifi
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
    WiFi.begin(); // begin()不传入参数，默认连接上一次连接成功的wifi
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
      // Serial.println("WIFI autoconnect fail, start AP for webconfig now...");
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
                                    // Config_save();                  //保存配置

    MDNS.end(); // 停止DNS服务器
    // server.stop();                            //停止开发板所建立的网络服务器。
  }
}

/*
 * 配置配网功能
 */
void wifiConfig()
{
  initSoftAP();
  initDNS();
  initWebServer();
  scanWiFi();
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
  if (WiFi.status() != WL_CONNECTED) // wifi连接失败
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