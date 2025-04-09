#include "Flash_saves.h"






bool Flash_saves(void *buf, uint32_t length, uint32_t address)
{

    EEPROM.writeBytes(address,buf,length);
    EEPROM.commit();


    return true;
}

bool Flash_read(void *buf, uint32_t length, uint32_t address)
{

    EEPROM.readBytes(address,buf,length);
    //EEPROM.commit();


    return true;
}