#pragma once

#include <cstdint>
#include <limits>

namespace fixed_point_detail
{
constexpr std::int16_t fromBits16(std::uint16_t bits) noexcept
{
    if (bits <= static_cast<std::uint16_t>(std::numeric_limits<std::int16_t>::max()))
    {
        return static_cast<std::int16_t>(bits);
    }

    return static_cast<std::int16_t>(static_cast<std::int32_t>(bits) - 0x10000);
}

constexpr std::int32_t fromBits32(std::uint32_t bits) noexcept
{
    if (bits <= static_cast<std::uint32_t>(std::numeric_limits<std::int32_t>::max()))
    {
        return static_cast<std::int32_t>(bits);
    }

    return static_cast<std::int32_t>(static_cast<std::int64_t>(bits) - 0x100000000LL);
}

constexpr std::int16_t wrapNegate16(std::int16_t value) noexcept
{
    return fromBits16(static_cast<std::uint16_t>(
        std::uint32_t{0} - static_cast<std::uint16_t>(value)
    ));
}

constexpr std::int16_t wrapAdd16(std::int16_t lhs, std::int16_t rhs) noexcept
{
    return fromBits16(static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(lhs)) +
        static_cast<std::uint16_t>(rhs)
    ));
}

constexpr std::int16_t wrapSubtract16(std::int16_t lhs, std::int16_t rhs) noexcept
{
    return fromBits16(static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(lhs)) -
        static_cast<std::uint16_t>(rhs)
    ));
}

constexpr std::int16_t wrapMultiply16(std::int16_t lhs, std::int16_t rhs) noexcept
{
    return fromBits16(static_cast<std::uint16_t>(
        static_cast<std::uint32_t>(static_cast<std::uint16_t>(lhs)) *
        static_cast<std::uint16_t>(rhs)
    ));
}

constexpr std::int32_t wrapNegate32(std::int32_t value) noexcept
{
    return fromBits32(std::uint32_t{0} - static_cast<std::uint32_t>(value));
}

constexpr std::int32_t wrapAdd32(std::int32_t lhs, std::int32_t rhs) noexcept
{
    return fromBits32(
        static_cast<std::uint32_t>(lhs) + static_cast<std::uint32_t>(rhs)
    );
}

constexpr std::int32_t wrapSubtract32(std::int32_t lhs, std::int32_t rhs) noexcept
{
    return fromBits32(
        static_cast<std::uint32_t>(lhs) - static_cast<std::uint32_t>(rhs)
    );
}

constexpr std::int32_t wrapMultiply32(std::int32_t lhs, std::int32_t rhs) noexcept
{
    return fromBits32(
        static_cast<std::uint32_t>(lhs) * static_cast<std::uint32_t>(rhs)
    );
}

constexpr std::int32_t arithmeticShiftRight32(
    std::int32_t value,
    unsigned int shift
) noexcept
{
    const std::uint32_t bits = static_cast<std::uint32_t>(value);
    const bool negative = (bits & 0x80000000u) != 0;

    if (shift == 0)
    {
        return value;
    }
    if (shift >= 32)
    {
        return negative ? -1 : 0;
    }

    std::uint32_t shifted = bits >> shift;
    if (negative)
    {
        shifted |= std::numeric_limits<std::uint32_t>::max() << (32 - shift);
    }
    return fromBits32(shifted);
}

constexpr std::uint64_t arithmeticShiftRightBits64(
    std::uint64_t bits,
    unsigned int shift
) noexcept
{
    const bool negative = (bits & (std::uint64_t{1} << 63)) != 0;

    if (shift == 0)
    {
        return bits;
    }
    if (shift >= 64)
    {
        return negative ? std::numeric_limits<std::uint64_t>::max() : 0;
    }

    std::uint64_t shifted = bits >> shift;
    if (negative)
    {
        shifted |= std::numeric_limits<std::uint64_t>::max() << (64 - shift);
    }
    return shifted;
}

// SH-2 register writes keep the low 32 two's-complement bits.
constexpr std::int32_t truncateToS32(std::int64_t value) noexcept
{
    return fromBits32(static_cast<std::uint32_t>(static_cast<std::uint64_t>(value)));
}

constexpr std::int32_t truncateShiftedToS32(
    std::int64_t value,
    unsigned int shift
) noexcept
{
    const std::uint64_t shifted = arithmeticShiftRightBits64(
        static_cast<std::uint64_t>(value),
        shift
    );
    return fromBits32(static_cast<std::uint32_t>(shifted));
}

constexpr std::int32_t truncateShiftedBitsToS32(
    std::uint64_t bits,
    unsigned int shift
) noexcept
{
    return fromBits32(static_cast<std::uint32_t>(
        arithmeticShiftRightBits64(bits, shift)
    ));
}

constexpr std::int32_t divideS32(
    std::int32_t dividend,
    std::int32_t divisor
) noexcept
{
    if (divisor == 0)
    {
        return dividend < 0
            ? std::numeric_limits<std::int32_t>::min()
            : std::numeric_limits<std::int32_t>::max();
    }
    if (dividend == std::numeric_limits<std::int32_t>::min() && divisor == -1)
    {
        return std::numeric_limits<std::int32_t>::min();
    }
    return dividend / divisor;
}
}

static_assert(sizeof(std::int16_t) == 2);
static_assert(sizeof(std::int32_t) == 4);
static_assert(sizeof(std::int64_t) == 8);
static_assert(std::numeric_limits<std::int32_t>::min() == -0x7fffffff - 1);

class fp16
{
public:
    std::int16_t m_value;

    fp16() : m_value(0)
    {
    }

    fp16(std::int16_t newValue) : m_value(newValue)
    {
    }

    fp16 operator - () const
    {
        return fp16(fixed_point_detail::wrapNegate16(m_value));
    }

    fp16 operator += (const fp16& otherValue)
    {
        m_value = fixed_point_detail::wrapAdd16(m_value, otherValue.m_value);
        return *this;
    }

    fp16 operator -= (const fp16& otherValue)
    {
        m_value = fixed_point_detail::wrapSubtract16(m_value, otherValue.m_value);
        return *this;
    }

    fp16 operator *= (const fp16& otherValue)
    {
        m_value = fixed_point_detail::wrapMultiply16(m_value, otherValue.m_value);
        return *this;
    }

    std::int32_t toFP32() const
    {
        const std::uint32_t bits =
            static_cast<std::uint32_t>(static_cast<std::uint16_t>(m_value)) << 16;
        return fixed_point_detail::fromBits32(bits);
    }

    operator std::int16_t() const
    {
        return m_value;
    }

    operator std::int16_t()
    {
        return m_value;
    }
};

class fixedPoint
{
public:
    std::int32_t m_value;

    fixedPoint() : m_value(0)
    {
    }

    fixedPoint(std::int32_t newValue) : m_value(newValue)
    {
    }

    fixedPoint operator - () const
    {
        return fixedPoint(fixed_point_detail::wrapNegate32(m_value));
    }

    fixedPoint operator += (const fixedPoint& otherValue)
    {
        m_value = fixed_point_detail::wrapAdd32(m_value, otherValue.m_value);
        return *this;
    }

    fixedPoint operator -= (const fixedPoint& otherValue)
    {
        m_value = fixed_point_detail::wrapSubtract32(m_value, otherValue.m_value);
        return *this;
    }

    fixedPoint operator *= (const fixedPoint& otherValue)
    {
        m_value = fixed_point_detail::wrapMultiply32(m_value, otherValue.m_value);
        return *this;
    }

    fixedPoint operator /= (const fixedPoint& otherValue)
    {
        m_value = fixed_point_detail::divideS32(m_value, otherValue.m_value);
        return *this;
    }

    std::int32_t asS32() const
    {
        return m_value;
    }

    std::int32_t getInteger() const
    {
        return fixed_point_detail::arithmeticShiftRight32(m_value, 16);
    }

    static fixedPoint fromS32(std::int32_t input)
    {
        return fixedPoint(input);
    }

    static fixedPoint fromInteger(std::int16_t integer)
    {
        fixedPoint newFixedPoint;
        newFixedPoint.setFromInteger(integer);
        return newFixedPoint;
    }

    static std::int32_t toInteger(const fixedPoint& fp)
    {
        return fixed_point_detail::arithmeticShiftRight32(fp.asS32(), 16);
    }

    std::int32_t toInteger()
    {
        return toInteger(*this);
    }

    fixedPoint normalized() const
    {
        std::uint32_t normalizedBits =
            static_cast<std::uint32_t>(m_value) & 0x0fffffffu;
        if ((normalizedBits & 0x08000000u) != 0)
        {
            normalizedBits |= 0xf0000000u;
        }
        return fixedPoint(fixed_point_detail::fromBits32(normalizedBits));
    }

    fixedPoint getAbs()
    {
        return m_value >= 0 ? *this : -(*this);
    }

    operator std::int32_t() const
    {
        return m_value;
    }

    float toFloat() const
    {
        return static_cast<float>(m_value) / 65536.0f;
    }

    void setFromInteger(std::int16_t inputInteger)
    {
        const std::uint32_t bits =
            static_cast<std::uint32_t>(static_cast<std::uint16_t>(inputInteger)) << 16;
        m_value = fixed_point_detail::fromBits32(bits);
    }
};

fixedPoint MTH_Mul_5_6(fixedPoint a, fixedPoint b, fixedPoint c);
fixedPoint MTH_Mul(fixedPoint a, fixedPoint b);
std::int64_t MUL_FP(const fixedPoint& A, const fixedPoint& B);

std::int32_t FP_GetIntegerPortion(fixedPoint& FP);
fixedPoint FP_Div(std::int32_t dividend, fixedPoint divisor);
fixedPoint intDivide(fixedPoint divisor, fixedPoint dividend);
inline std::int32_t performDivision(std::int32_t divisor, std::int32_t dividend)
{
    return fixed_point_detail::divideS32(dividend, divisor);
}
std::int32_t atan2_FP(std::int32_t y, std::int32_t x);
std::int32_t atan2(std::int32_t y, std::int32_t x);
fixedPoint FP_Pow2(fixedPoint r4);

void asyncDivStart(std::int32_t r4, fixedPoint r5);
void asyncDivStart_integer(std::int32_t r4, std::int32_t r5);
fixedPoint asyncDivEnd();

int shiftLeft32(int param_1, int param_2);
