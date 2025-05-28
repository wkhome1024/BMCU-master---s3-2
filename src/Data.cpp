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
    free(data);
    data = (char *)heap_caps_malloc(1024 * 64, MALLOC_CAP_SPIRAM);
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

    if (data_count < 1024 * 64)
    {
        memcpy(C_data + data_count, data, strlen(data));
        data_count += strlen(data);
    }
    else
    {
        data_count = 0;
        //RESET_DATA(C_data);
        memcpy(C_data + data_count, data, strlen(data));
        data_count += strlen(data);
    }
}

static int log_count = 0;
size_t my_log(const char *format)
{
    if (!EN_log)
    {
        return 0; // 日志开关关闭
    }
    // char buffer[128];
    // int len = snprintf(buffer, sizeof(buffer), format);
    String hexStr = ""; // 初始化空字符串

    // 将日志输出到文件
    if (memcmp(format, "(", 1) == 0)
    {
        if (log_count != 0)
        {
            hexStr += "<br />"; // 添加换行符
        }
        hexStr += millis();     // 获取当前时间戳
        hexStr += " : > ";      // 添加时间戳和分隔符
        hexStr += format;
    }
    else
    {
        hexStr += format; // 添加日志内容
    }

    if (log_count < 1024 * 64)
    {
        memcpy(L_data + log_count, hexStr.c_str(), hexStr.length());
        log_count += hexStr.length();
    }
    else
    {
        log_count = 0;
        memcpy(L_data + log_count, hexStr.c_str(), hexStr.length());
        log_count += hexStr.length();
    }
    return hexStr.length();
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