// AS5600.cpp
#include "AS5600.h"

#define AS5600_SDA_PIN 7
#define AS5600_SCL_PIN 6

AS5600::AS5600() : rawAngleState(RAW_ANGLE_IDLE) {}

void AS5600::begin()
{
    Wire1.begin(AS5600_SDA_PIN, AS5600_SCL_PIN, 100000);
}

uint16_t AS5600::readRawAngle()
{
    return readRegister16(REG_RAW_ANGLE_H);
}

uint16_t AS5600::readAngle()
{
    return readRegister16(REG_ANGLE_H);
}

uint8_t AS5600::readStatus()
{
    return readRegister8(REG_STATUS);
}

uint8_t AS5600::readAGC()
{
    return readRegister8(REG_AGC);
}

uint16_t AS5600::readMagnitude()
{
    return readRegister16(REG_MAGNITUDE_H);
}

int AS5600::getMagnetStatus()
{
    uint8_t status = readStatus();
    if (status == 0xFF) // I2C 读失败
        return -2;
    if (status & 0x20)
    {
        return 1; // 磁铁太强
    }
    else if (status & 0x10)
    {
        return -1; // 磁铁太弱
    }
    else if (status & 0x08)
    {
        return 0; // 磁铁正常
    }
    return -2; // 未检测到磁铁
}

bool AS5600::isConnected()
{
    Wire1.beginTransmission(AS5600_ADDRESS);
    return (Wire1.endTransmission() == 0);
}

void AS5600::setZeroPosition(uint16_t position)
{
    writeRegister16(REG_ZPOS_H, REG_ZPOS_L, position & 0x0FFF);
}

uint16_t AS5600::getZeroPosition()
{
    return readRegister16(REG_ZPOS_H) & 0x0FFF;
}

void AS5600::setMaximumAngle(uint16_t angle)
{
    writeRegister16(REG_MANG_H, REG_MANG_L, angle & 0x0FFF);
}

float AS5600::getAngleDegrees()
{
    uint16_t angle = readAngle();
    return (angle * 360.0) / 4096.0;
}

float AS5600::getAngleRadians()
{
    uint16_t angle = readAngle();
    return (angle * 2.0 * 3.14159265358979323846) / 4096.0;
}

uint8_t AS5600::readRegister8(uint8_t reg)
{
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(reg);
    if (Wire1.endTransmission(false) != 0)
        return 0;
    if (Wire1.requestFrom(AS5600_ADDRESS, (uint8_t)1) != 1)
        return 0;
    return Wire1.read();
}

uint16_t AS5600::readRegister16(uint8_t regH)
{
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regH);
    if (Wire1.endTransmission(false) != 0)
        return 0;
    // 高低字节必须在同一次 I2C 事务中读取，避免电机转动时角度撕裂。
    if (Wire1.requestFrom(AS5600_ADDRESS, (uint8_t)2) != 2)
        return 0;
    uint8_t highByte = Wire1.read();
    uint8_t lowByte = Wire1.read();
    return ((uint16_t)highByte << 8) | lowByte;
}

void AS5600::writeRegister16(uint8_t regH, uint8_t regL, uint16_t value)
{
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regH);
    Wire1.write((value >> 8) & 0x0F);
    Wire1.endTransmission();

    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regL);
    Wire1.write(value & 0xFF);
    Wire1.endTransmission();
}

// 异步 readRawAngle 相关方法
bool AS5600::updateRawAngleAsync()
{
    if (rawAngleState == RAW_ANGLE_IDLE)
    {
        requestRegister(REG_RAW_ANGLE_H, 2); // 一次请求高低两个字节
        rawAngleState = RAW_ANGLE_READING;
        return false;
    }

    if (Wire1.available() < 2)
    {
        while (Wire1.available())
            Wire1.read(); // 丢弃不完整数据，避免下次拼错
        rawAngleState = RAW_ANGLE_IDLE; // 读取失败/不完整，下次重新请求
        return false;
    }

    rawAngleHighByte = Wire1.read();
    rawAngleLowByte = Wire1.read();
    rawAngleResult = ((uint16_t)rawAngleHighByte << 8) | rawAngleLowByte;
    rawAngleState = RAW_ANGLE_IDLE;
    return true; // 返回 true 表示读取完成
}
uint16_t AS5600::getRawAngleResult()
{
    return rawAngleResult;
}
// 封装公共 I2C 请求逻辑
void AS5600::requestRegister(uint8_t regAddr, uint8_t length)
{
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regAddr);
    if (Wire1.endTransmission(false) != 0)
        return;
    Wire1.requestFrom(AS5600_ADDRESS, length);
}
