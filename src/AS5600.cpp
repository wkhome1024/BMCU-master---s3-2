// AS5600.cpp
#include "AS5600.h"

#define AS5600_SDA_PIN 7
#define AS5600_SCL_PIN 6

AS5600::AS5600() {}

void AS5600::begin() {
    Wire1.begin(AS5600_SDA_PIN, AS5600_SCL_PIN, 100000);
}

uint16_t AS5600::readRawAngle() {
    return readRegister16(REG_RAW_ANGLE_H, REG_RAW_ANGLE_L);
}

uint16_t AS5600::readAngle() {
    return readRegister16(REG_ANGLE_H, REG_ANGLE_L);
}

uint8_t AS5600::readStatus() {
    return readRegister8(REG_STATUS);
}

uint8_t AS5600::readAGC() {
    return readRegister8(REG_AGC);
}

uint16_t AS5600::readMagnitude() {
    return readRegister16(REG_MAGNITUDE_H, REG_MAGNITUDE_L);
}

int AS5600::getMagnetStatus() {
    uint8_t status = readStatus();
    if (status & 0x20) {
        return 1;  // 磁铁太强
    } else if (status & 0x10) {
        return -1; // 磁铁太弱
    } else if (status & 0x08) {
        return 0;  // 磁铁正常
    }
    return -2;     // 未检测到磁铁
}

bool AS5600::isConnected() {
    Wire1.beginTransmission(AS5600_ADDRESS);
    return (Wire1.endTransmission() == 0);
}

void AS5600::setZeroPosition(uint16_t position) {
    writeRegister16(REG_ZPOS_H, REG_ZPOS_L, position & 0x0FFF);
}

uint16_t AS5600::getZeroPosition() {
    return readRegister16(REG_ZPOS_H, REG_ZPOS_L) & 0x0FFF;
}

void AS5600::setMaximumAngle(uint16_t angle) {
    writeRegister16(REG_MANG_H, REG_MANG_L, angle & 0x0FFF);
}

float AS5600::getAngleDegrees() {
    uint16_t angle = readAngle();
    return (angle * 360.0) / 4096.0;
}

float AS5600::getAngleRadians() {
    uint16_t angle = readAngle();
    return (angle * 2.0 * 3.14159265358979323846) / 4096.0;
}

uint8_t AS5600::readRegister8(uint8_t reg) {
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(reg);
    Wire1.endTransmission(false);
    Wire1.requestFrom(AS5600_ADDRESS, (uint8_t)1);
    return Wire1.read();
}

uint16_t AS5600::readRegister16(uint8_t regH, uint8_t regL) {
    uint8_t highByte = readRegister8(regH);
    uint8_t lowByte = readRegister8(regL);
    return (highByte << 8) | lowByte;
}

void AS5600::writeRegister16(uint8_t regH, uint8_t regL, uint16_t value) {
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regH);
    Wire1.write((value >> 8) & 0x0F);
    Wire1.endTransmission();
    
    Wire1.beginTransmission(AS5600_ADDRESS);
    Wire1.write(regL);
    Wire1.write(value & 0xFF);
    Wire1.endTransmission();
}