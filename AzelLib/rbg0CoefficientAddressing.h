#pragma once

#include <bit>
#include <cstdint>

namespace pds::vdp2
{
constexpr std::uint32_t coefficientTableBase(
    std::uint16_t ktaof,
    bool parameterB,
    std::uint32_t entrySize
)
{
    const std::uint32_t offset =
        parameterB ? (static_cast<std::uint32_t>(ktaof) >> 8) : ktaof;
    return (offset & 0x7u) * entrySize * 0x10000u;
}

constexpr std::uint32_t coefficientAddress(
    std::uint32_t tableBase,
    std::int32_t accumulator,
    std::uint32_t entrySize
)
{
    return tableBase +
        (static_cast<std::uint32_t>(accumulator) >> 16) * entrySize;
}

constexpr std::uint32_t coefficientVramAddress(
    std::uint32_t tableBase,
    std::int32_t accumulator,
    std::uint32_t entrySize
)
{
    return coefficientAddress(tableBase, accumulator, entrySize) & 0x7ffffu;
}

constexpr std::int32_t wrappingAdd(std::int32_t left, std::int32_t right)
{
    return std::bit_cast<std::int32_t>(
        static_cast<std::uint32_t>(left) + static_cast<std::uint32_t>(right)
    );
}
}
