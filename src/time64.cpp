#include "main.h"
#include <esp_timer.h>

uint64_t get_time64()
{
    return (uint64_t)(esp_timer_get_time() / 1000ULL);
}

void update_time()
{
    // 时间直接来自 esp_timer，不再维护 H/L 两个变量。
}

uint32_t get_time32()
{
    return (uint32_t)(esp_timer_get_time() / 1000ULL);
}
