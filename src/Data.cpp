#include "Data.h"

#define BUFFER_SIZE (1024 * 64)

Preferences preferences;
TextBuffer C_capture;
TextBuffer L_log;
bool EN_catch = false;

bool TextBuffer::init(size_t capacity, bool psram)
{
    if (!mutex_)
        mutex_ = xSemaphoreCreateMutex();
    buffer_ = (char *)heap_caps_malloc(capacity, psram ? MALLOC_CAP_SPIRAM : MALLOC_CAP_8BIT);
    capacity_ = buffer_ ? capacity : 0;
    length_ = 0;
    if (buffer_)
        memset(buffer_, 0, capacity_);
    return buffer_ != nullptr;
}

bool TextBuffer::append(const char *data, size_t length)
{
    if (!buffer_ || capacity_ == 0 || !data || length == 0)
        return false;
    if (length >= capacity_)
        length = capacity_ - 1;
    if (mutex_)
        xSemaphoreTake(mutex_, portMAX_DELAY);
    bool ok = (length_ + length + 1 <= capacity_);
    if (ok)
    {
        memcpy(buffer_ + length_, data, length);
        length_ += length;
        buffer_[length_] = '\0';
    }
    if (mutex_)
        xSemaphoreGive(mutex_);
    return ok;
}

size_t TextBuffer::copy_to(char *dst, size_t max_len) const
{
    if (!buffer_ || !dst || max_len == 0)
        return 0;
    if (mutex_)
        xSemaphoreTake(mutex_, portMAX_DELAY);
    size_t n = (length_ < max_len - 1) ? length_ : (max_len - 1);
    memcpy(dst, buffer_, n);
    dst[n] = '\0';
    if (mutex_)
        xSemaphoreGive(mutex_);
    return n;
}

void TextBuffer::clear()
{
    if (!buffer_ || capacity_ == 0)
        return;
    if (mutex_)
        xSemaphoreTake(mutex_, portMAX_DELAY);
    memset(buffer_, 0, capacity_);
    length_ = 0;
    if (mutex_)
        xSemaphoreGive(mutex_);
}

void INIT_DATA()
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_LOGE("(ERROR)", "Failed to nvs_flash_init");
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    const size_t c_size = catch_mode ? (BUFFER_SIZE * 64) : BUFFER_SIZE;
    bool c_ok = C_capture.init(c_size, true);
    bool l_ok = L_log.init(BUFFER_SIZE, true);
    EN_catch = catch_mode && c_ok;

    if (!c_ok || !l_ok)
    {
        ESP_LOGE("(ERROR)", "Failed to allocate memory for buffers");
        EN_catch = false;
    }
}

void RESET_DATA(char *data)
{
    if (!data)
        return;
    if (data == C_capture.data())
        C_capture.clear();
    else if (data == L_log.data())
        L_log.clear();
}

void get_C_data(uint8_t *buf_X, int data_length)
{
    if (catch_key == 0)
        return;

    if (buf_X == nullptr || data_length <= 0)
    {
        catch_key = 0;
        return;
    }

    const int maxBufSize = 256;
    char logBuffer[maxBufSize];
    int offset = 0;

    uint32_t timestamp = millis();
    offset += snprintf(logBuffer + offset, maxBufSize - offset, "%08lu : ", (unsigned long)timestamp);

    for (int i = 0; i < data_length && offset < maxBufSize - 4; i++)
    {
        snprintf(logBuffer + offset, maxBufSize - offset, "%02X ", buf_X[i]);
        offset += 3;
    }

    if (offset < maxBufSize - 2)
    {
        logBuffer[offset++] = '\n';
        logBuffer[offset] = '\0';
    }

    WriteData(logBuffer);

    catch_key++;
    if ((catch_key > 501 && !EN_catch) || catch_key > 40000)
    {
        my_printf("(http) Bambu-hub抓包结束");
        catch_key = 0;
    }
}

void WriteData(const char *data)
{
    if (!data)
        return;
    size_t len = strlen(data);
    if (!C_capture.append(data, len))
    {
        C_capture.clear();
        C_capture.append(data, len);
    }
}

size_t my_log(const char *format)
{
    if (!EN_log || !L_log.data())
        return 0;

    char hexStr[256];
    int n = snprintf(hexStr, sizeof(hexStr), "<br />%010lu : > %s\n", (unsigned long)millis(), format);
    if (n < 0)
        return 0;
    size_t len = (size_t)n;
    if (len >= sizeof(hexStr))
        len = sizeof(hexStr) - 1;

    if (!L_log.append(hexStr, len))
    {
        L_log.clear();
        L_log.append(hexStr, len);
    }
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
        return 0;
    return 0;
}

static SemaphoreHandle_t flash_mutex = xSemaphoreCreateMutex();

bool Flash_saves(void *buf, uint16_t length, const char *address)
{
    xSemaphoreTake(flash_mutex, portMAX_DELAY);
    preferences.begin("storage", false);
    int len = preferences.putBytes(address, buf, length);
    preferences.end();
    xSemaphoreGive(flash_mutex);
    return len == length;
}

bool Flash_read(void *buf, uint16_t length, const char *address)
{
    if (!buf || !address || length == 0)
    {
        ESP_LOGE("(Flash)", "Invalid parameters in Flash_read");
        return false;
    }

    preferences.begin("storage", false);
    if (!preferences.isKey(address))
    {
        preferences.end();
        ESP_LOGE("(Flash)", "Key %s not found", address);
        return false;
    }

    size_t bytesRead = preferences.getBytes(address, buf, length);
    preferences.end();

    return bytesRead == length;
}
