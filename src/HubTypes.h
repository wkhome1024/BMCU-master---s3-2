#pragma once
#include <stdint.h>

// 硬件/协议统一维度：4 路 AMS，每路 4 个托盘。
constexpr uint8_t kAmsCount = 4;
constexpr uint8_t kTrayCount = 4;
constexpr uint8_t kTrayTotal = kAmsCount * kTrayCount;

static_assert(kTrayTotal == 16, "BMCU-HUB expects 4x4 trays");
