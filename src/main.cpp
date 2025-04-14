#include <main.h>


void setup()
{

  EEPROM.begin(4096); // 申请存储空间
  BambuBus_init();
  Switch_init();
  LED_Init();
}
uint64_t error_time = 0;
void loop()
{
  
    package_type stu = BambuBus_run();
    Bmcu_readuart();
    // int stu =-1;
    uint64_t time_now = get_time64();
    if (stu != BambuBus_package_NONE) // have data/offline
    {
      if (stu == BambuBus_package_ERROR) // offline
      {
        // SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
        digitalWrite(13, LOW);
       if (error_time == 0 || error_time < (time_now - 1000))
          error_time = time_now + 1000;
        else if (error_time > time_now)
          digitalWrite(12, HIGH);
        else if (error_time < time_now)
          digitalWrite(12, LOW);
       // delayMicroseconds(1000);
      }
      else // have data
      {
        if (stu == BambuBus_package_heartbeat)
        {
          if (error_time == 0 || error_time < (time_now - 2000))
              error_time = time_now + 2000;
          else if (error_time > time_now)
          {
              digitalWrite(12, HIGH);
              digitalWrite(13, LOW); 
          }
          else if (error_time < time_now)
          {
              digitalWrite(12, LOW);
              digitalWrite(13, HIGH);
          }
        }
        if (Switch_need_to_save())
          Switch_save();
        if (Switch_need_to_delay())
        {
          Switch_set_not_to_delay();
          delay(5000);
        }
      }
    }


    //delay(1);
    


  
}
