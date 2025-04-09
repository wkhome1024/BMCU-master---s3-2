#include <Arduino.h>
#include <SPI.h>
#include <WiFi.h>
#include "stdio.h"
#include "485.h"
#include "BambuBus.h"
#include "Flash_saves.h"
#include "switch.h"

void setup() {

  EEPROM.begin(4096);  //申请存储空间
  BambuBus_init();
  //Switch_init();
  LED_Init();


}
unsigned char error_times = 0;
void loop() 
{
  while (1)
  {
      package_type stu = BambuBus_run();

      // int stu =-1;
      if (stu!=BambuBus_package_NONE)//have data/offline
      {
          Bmcu_readuart();
          if (stu == BambuBus_package_ERROR)//offline
          {
              //SYS_RGB.set_RGB(0x30, 0x00, 0x00, 0);
              error_times++;
              error_times = error_times % 100;
              if(error_times == 99)
                  
              delayMicroseconds(1000);
          }
          else//have data
          {
              error_times = 0;
              if (stu == BambuBus_package_heartbeat)
              {

              }
              
              if(Switch_need_to_save())
                  Switch_save();
              if(Switch_need_to_delay())
              {
                  Switch_set_not_to_delay();
                  delay(5000);
              }
          }
      }
  }

}

