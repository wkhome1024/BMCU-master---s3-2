#include "Data.h"

#define BUFFER_SIZE (1024 * 64)

Preferences preferences;
char *C_data = NULL;
char *L_data = NULL;


void INIT_DATA()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGE("(ERROR)", "Failed to nvs_flash_init");
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);
    //if (!LittleFS.begin()) {
    //    my_log("无法挂载LittleFS");
    //    return;
    //}
    //my_log("LittleFS 初始化成功");
    C_data = (char *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM);
    L_data = (char *)heap_caps_malloc(BUFFER_SIZE, MALLOC_CAP_SPIRAM);

    if (!C_data || !L_data) {
        ESP_LOGE("(ERROR)", "Failed to allocate memory for buffers");
        //while (1) {} // 死循环，防止继续运行
    }
    memset(C_data, 0, BUFFER_SIZE);
    memset(L_data, 0, BUFFER_SIZE);

}
void RESET_DATA(char *data)
{
    memset(data, 0, BUFFER_SIZE);
}

void get_C_data(uint8_t *buf_X, int data_length)
{
    if (catch_key == 0)
        return;

    // 边界检查
    if (buf_X == nullptr || data_length <= 0)
    {
        catch_key = 0;
        return;
    }

    // 静态缓冲区用于构建日志内容（根据实际需求调整大小）
    const int maxBufSize = 256;
    char logBuffer[maxBufSize];
    int offset = 0;

    // 写入时间戳
    uint32_t timestamp = millis();
    offset += snprintf(logBuffer + offset, maxBufSize - offset, "%08d : ", timestamp);

    // 构建十六进制字符串
    for (int i = 0; i < data_length && offset < maxBufSize - 4; i++)
    {
        snprintf(logBuffer + offset, maxBufSize - offset, "%02X ", buf_X[i]);
        offset += 3; // 占用了两个字符和一个空格
    }

    // 添加换行符
    if (offset < maxBufSize - 2)
    {
        logBuffer[offset++] = '\n';
        logBuffer[offset] = '\0';
    }

    // 写入数据
    WriteData(logBuffer);

    catch_key++;
    if (catch_key > 501) // 500个数据包后关闭抓包
    {
        my_printf("(http) Bambu-hub抓包结束");
        catch_key = 0;
    }
}

static int data_count = 0;
void WriteData(const char *data)
{
    if (!data || !C_data) return;

    int len = strlen(data);
    if (data_count + len >= BUFFER_SIZE)
    {
        data_count = 0;
    }
    memcpy(C_data + data_count, data, len);
    data_count += len;
}

static int log_count = 0;
size_t my_log(const char *format)
{
    if (!EN_log || !L_data) return 0;

    char hexStr[256]; // 固定大小缓冲区，避免 String 的开销
    sprintf(hexStr, "<br />%08d : > %s\n", millis(), format);

    int len = strlen(hexStr);
    if (log_count + len >= BUFFER_SIZE)
    {
        log_count = 0;
        memset(L_data, 0, BUFFER_SIZE);
    }

    memcpy(L_data + log_count, hexStr, len);
    log_count += len;

    return len;
}
int my_printf(const char *format, ...)
{
    char buffer[256];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    my_log(buffer);
    return len;
}

int esplog_printf()
{
    if (!EN_log)
    {
        return 0; // 日志开关关闭
    }
    return 0;
}


static SemaphoreHandle_t flash_mutex = xSemaphoreCreateMutex();

bool Flash_saves(void *buf, uint16_t length, const char *address)
{
    xSemaphoreTake(flash_mutex, portMAX_DELAY);
    preferences.begin("storage", false); // 打开命名空间"storage"，false表示读写模式
    int len = preferences.putBytes(address, buf, length); // 写入二进制数据
    preferences.end();
    xSemaphoreGive(flash_mutex);
    return len == length;
}

bool Flash_read(void *buf, uint16_t length, const char *address)
{
    if (!buf || !address || length == 0) {
        ESP_LOGE("(Flash)", "Invalid parameters in Flash_read");
        return false;
    }

    preferences.begin("storage", false);
    if (!preferences.isKey(address)) {
        preferences.end();
        ESP_LOGE("(Flash)", "Key %s not found", address);
        return false;
    }

    size_t bytesRead = preferences.getBytes(address, buf, length);
    preferences.end();


    return bytesRead == length;
}