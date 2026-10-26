#pragma once

#include <cstdint>

inline constexpr std::uint32_t kScspSampleRate = 44'100;
inline constexpr std::uint32_t kScspM68kCyclesPerSample = 256;
inline constexpr std::uint32_t kScspM68kClock = kScspSampleRate * kScspM68kCyclesPerSample;
