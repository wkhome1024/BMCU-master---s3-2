#ifndef AS5600_H
#define AS5600_H

#include <Wire.h>

// 异步读取原始角度的状态枚举
enum RawAngleState {
    RAW_ANGLE_IDLE,
    RAW_ANGLE_READING_HIGH,
    RAW_ANGLE_READING_LOW,
    RAW_ANGLE_COMPLETE
};

class AS5600 {
private:
    static const uint8_t AS5600_ADDRESS = 0x36;
    
    // Register addresses
    static const uint8_t REG_ZMCO = 0x00;
    static const uint8_t REG_ZPOS_H = 0x01;
    static const uint8_t REG_ZPOS_L = 0x02;
    static const uint8_t REG_MPOS_H = 0x03;
    static const uint8_t REG_MPOS_L = 0x04;
    static const uint8_t REG_MANG_H = 0x05;
    static const uint8_t REG_MANG_L = 0x06;
    static const uint8_t REG_CONF_H = 0x07;
    static const uint8_t REG_CONF_L = 0x08;
    static const uint8_t REG_RAW_ANGLE_H = 0x0C;
    static const uint8_t REG_RAW_ANGLE_L = 0x0D;
    static const uint8_t REG_ANGLE_H = 0x0E;
    static const uint8_t REG_ANGLE_L = 0x0F;
    static const uint8_t REG_STATUS = 0x0B;
    static const uint8_t REG_AGC = 0x1A;
    static const uint8_t REG_MAGNITUDE_H = 0x1B;
    static const uint8_t REG_MAGNITUDE_L = 0x1C;
    // 异步读取原始角度相关变量
    RawAngleState rawAngleState = RAW_ANGLE_IDLE;
    uint8_t rawAngleHighByte = 0;
    uint8_t rawAngleLowByte = 0;
    uint16_t rawAngleResult = 0;

    // 私有辅助方法
    uint8_t readRegister8(uint8_t reg);
    uint16_t readRegister16(uint8_t regH, uint8_t regL);
    void writeRegister16(uint8_t regH, uint8_t regL, uint16_t value);
    void requestRegister(uint8_t regAddr);

public:
    AS5600();
    void begin();

    // 同步读取方法
    uint16_t readRawAngle();
    uint16_t readAngle();
    uint8_t readStatus();
    uint8_t readAGC();
    uint16_t readMagnitude();

    // 状态检查方法
    int getMagnetStatus();
    bool isConnected();

    // 配置方法
    void setZeroPosition(uint16_t position);
    uint16_t getZeroPosition();
    void setMaximumAngle(uint16_t angle);

    // 角度获取方法
    float getAngleDegrees();
    float getAngleRadians();

    // 异步读取原始角度方法
    bool updateRawAngleAsync();
    uint16_t getRawAngleResult();
};

#endif