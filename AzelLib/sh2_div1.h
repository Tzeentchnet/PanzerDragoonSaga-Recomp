#pragma once

#include <cstdint>

namespace sh2
{
struct Div1State
{
    bool q;
    bool m;
    bool t;
};

namespace detail
{
constexpr std::int32_t signedFromBits(std::uint32_t bits) noexcept
{
    if (bits <= 0x7fffffffu)
    {
        return static_cast<std::int32_t>(bits);
    }
    return static_cast<std::int32_t>(static_cast<std::int64_t>(bits) - 0x100000000LL);
}
}

constexpr void div1(
    std::int32_t rm,
    std::int32_t& rn,
    Div1State& state
) noexcept
{
    const bool oldQ = state.q;
    const std::uint32_t rmBits = static_cast<std::uint32_t>(rm);
    std::uint32_t rnBits = static_cast<std::uint32_t>(rn);

    // Unsigned register arithmetic makes DIV1's 32-bit wrap and carry/borrow explicit.
    state.q = (rnBits & 0x80000000u) != 0;
    rnBits = (rnBits << 1) | static_cast<std::uint32_t>(state.t);

    if (!oldQ && !state.m)
    {
        const std::uint32_t before = rnBits;
        rnBits -= rmBits;
        const bool borrow = rnBits > before;
        state.q = state.q ? !borrow : borrow;
    }
    else if (!oldQ && state.m)
    {
        const std::uint32_t before = rnBits;
        rnBits += rmBits;
        const bool carry = rnBits < before;
        state.q = state.q ? carry : !carry;
    }
    else if (oldQ && !state.m)
    {
        const std::uint32_t before = rnBits;
        rnBits += rmBits;
        const bool carry = rnBits < before;
        state.q = state.q ? !carry : carry;
    }
    else
    {
        const std::uint32_t before = rnBits;
        rnBits -= rmBits;
        const bool borrow = rnBits > before;
        state.q = state.q ? borrow : !borrow;
    }

    state.t = state.q == state.m;
    rn = detail::signedFromBits(rnBits);
}
}
