#pragma once

#include <algorithm>
#include <cstdint>

namespace pds::vdp2
{
enum class ColorLayer : std::uint8_t
{
    Nbg3,
    Nbg1,
    Nbg0,
    Rbg0,
    Vdp1
};

constexpr std::uint8_t ratioNumber(
    ColorLayer layer,
    std::uint16_t ccrna,
    std::uint16_t ccrr
)
{
    switch (layer)
    {
    case ColorLayer::Nbg0:
        return static_cast<std::uint8_t>(ccrna & 0x1f);
    case ColorLayer::Nbg1:
        return static_cast<std::uint8_t>((ccrna >> 8) & 0x1f);
    case ColorLayer::Rbg0:
        return static_cast<std::uint8_t>(ccrr & 0x1f);
    default:
        return 0;
    }
}

constexpr std::uint8_t calculateRatioChannel(
    std::uint8_t top,
    std::uint8_t second,
    std::uint8_t ratio
)
{
    const std::uint32_t ratioNumber = ratio & 0x1f;
    return static_cast<std::uint8_t>(
        (
            static_cast<std::uint32_t>(top) * (31u - ratioNumber) +
            static_cast<std::uint32_t>(second) * (ratioNumber + 1u)
        ) >> 5
    );
}

constexpr std::uint8_t calculateAdditiveChannel(std::uint8_t top, std::uint8_t second)
{
    return static_cast<std::uint8_t>(
        std::min<std::uint32_t>(
            static_cast<std::uint32_t>(top) + static_cast<std::uint32_t>(second),
            255u
        )
    );
}

constexpr int priorityKey(
    ColorLayer layer,
    std::uint8_t priority,
    bool enabled,
    bool opaque
)
{
    if (!enabled || !opaque)
    {
        return -1;
    }

    int tieBreakRank = 0;
    switch (layer)
    {
    case ColorLayer::Nbg3:
        tieBreakRank = 0;
        break;
    case ColorLayer::Nbg1:
        tieBreakRank = 2;
        break;
    case ColorLayer::Nbg0:
        tieBreakRank = 3;
        break;
    case ColorLayer::Rbg0:
        tieBreakRank = 4;
        break;
    case ColorLayer::Vdp1:
        tieBreakRank = 5;
        break;
    }
    return static_cast<int>(priority & 0x7) * 8 + tieBreakRank;
}
}
