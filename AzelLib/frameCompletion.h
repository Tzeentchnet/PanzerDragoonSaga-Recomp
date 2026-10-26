#pragma once

#include <cstdint>

namespace pds::timing
{
constexpr bool shouldPresentFrame(
    bool frameReady,
    std::uint32_t vsyncsSincePresent,
    std::uint32_t vsyncsPerFrame,
    bool vdp1Idle,
    std::uint32_t vsyncsWaitingForVdp1
)
{
    return
        frameReady &&
        vsyncsSincePresent >= vsyncsPerFrame &&
        (vdp1Idle || vsyncsWaitingForVdp1 > 3);
}
}
