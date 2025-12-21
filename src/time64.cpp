#include "main.h"
uint64_t _time64_time_H = 0;
uint32_t _time64_time_L = 0;
uint32_t _time64_temp = 0;
uint64_t get_time64()
{
    // millis() should return the system uptime in milliseconds as a uint32_t; ensure it is defined elsewhere.
    //uint32_t T = millis();
    return (_time64_time_H + (uint64_t)_time64_time_L);
}
void update_time()
{
    _time64_temp = millis();
    if (_time64_temp < _time64_time_L)
    {
        _time64_time_H += 0x100000000;
    }
    _time64_time_L = _time64_temp;
}
uint32_t get_time32()
{
    // This function returns the current time in milliseconds as a uint32_t.
    return _time64_time_L;
}   