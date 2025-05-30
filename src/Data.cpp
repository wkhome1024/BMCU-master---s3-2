#include "Data.h"

char *C_data = NULL;
char *L_data = NULL;
void INIT_DATA()
{
    C_data = (char *)heap_caps_malloc(1024 * 64, MALLOC_CAP_SPIRAM);
    L_data = (char *)heap_caps_malloc(1024 * 64, MALLOC_CAP_SPIRAM);
    memset(C_data, 0, 1024 * 64);
    memset(L_data, 0, 1024 * 64);
}
void RESET_DATA(char *data)
{
    memset(data, 0, 1024 * 64);
}

void get_C_data(uint8_t *buf_X, int data_length)
{
    if (catch_key > 0)
    {
        String hexStr = ""; // 初始化空字符串
        hexStr += millis(); // 获取当前时间戳
        hexStr += " : ";
        for (int i = 0; i < data_length; i++)
        {
            char b[5];
            sprintf(b, "%02X", buf_X[i]);
            hexStr += String(b) + " "; // 在每个字节后添加空格以便区分（可选）
        }
        hexStr += "\n"; // 添加换行符
        // Serial.println(hexStr); // 打印十六进制字符串
        WriteData(hexStr.c_str());
        catch_key++;
        if (catch_key > 501) // 500个数据包后关闭抓包
        {
            catch_key = 0;
        }
    }
}

static int data_count = 0;
void WriteData(const char *data)
{
    int len = strlen(data);
    if (data_count + len >= 1024 * 64) {
        data_count = 0;
    }
    memcpy(C_data + data_count, data, len);
    data_count += len;
}

static int log_count = 0;
size_t my_log(const char *format)
{
    if (!EN_log) return 0;

    char hexStr[128]; // 固定大小缓冲区，避免 String 的开销
    sprintf(hexStr, "%lu : > %s", millis(), format);

    int len = strlen(hexStr);
    if (log_count + len >= 1024 * 64) {
        log_count = 0;
    }

    memcpy(L_data + log_count, hexStr, len);
    log_count += len;

    return len;
}
int my_printf(const char *format, ...)
{
    char buffer[128];
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

bool Flash_saves(void *buf, uint32_t length, uint32_t address)
{
    EEPROM.writeBytes(address, buf, length);
    EEPROM.commit();
    return true;
}

bool Flash_read(void *buf, uint32_t length, uint32_t address)
{

    EEPROM.readBytes(address, buf, length);
    // EEPROM.commit();

    return true;
}