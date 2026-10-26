#include "fixedPoint.h"

std::int32_t FP_GetIntegerPortion(fixedPoint& FP)
{
    return fixed_point_detail::arithmeticShiftRight32(FP.asS32(), 16);
}

fixedPoint FP_Div(std::int32_t dividend, fixedPoint divisor)
{
    const std::int32_t divisorValue = divisor.asS32();
    if (divisorValue == 0)
    {
        return dividend < 0
            ? std::numeric_limits<std::int32_t>::min()
            : std::numeric_limits<std::int32_t>::max();
    }

    const std::int64_t numerator = static_cast<std::int64_t>(dividend) * 0x10000;
    return fixedPoint::fromS32(
        fixed_point_detail::truncateToS32(numerator / divisorValue)
    );
}

std::int64_t gDivident;
std::int64_t gDivisor;

void asyncDivStart(std::int32_t r4, fixedPoint r5)
{
    gDivident = static_cast<std::int64_t>(r4) * 0x10000;
    gDivisor = r5.asS32();
}

void asyncDivStart_integer(std::int32_t r4, std::int32_t r5)
{
    gDivident = r4;
    gDivisor = r5;
}

fixedPoint asyncDivEnd()
{
    if (gDivisor == 0)
    {
        return gDivident < 0
            ? std::numeric_limits<std::int32_t>::min()
            : std::numeric_limits<std::int32_t>::max();
    }
    if (gDivident == std::numeric_limits<std::int64_t>::min() && gDivisor == -1)
    {
        return 0;
    }

    return fixedPoint::fromS32(
        fixed_point_detail::truncateToS32(gDivident / gDivisor)
    );
}

fixedPoint intDivide(fixedPoint divisor, fixedPoint dividend)
{
    return fixedPoint::fromS32(
        fixed_point_detail::divideS32(dividend.asS32(), divisor.asS32())
    );
}

std::int64_t MUL_FP(const fixedPoint& A, const fixedPoint& B)
{
    return static_cast<std::int64_t>(A.asS32()) * B.asS32();
}

fixedPoint MTH_Mul_5_6(fixedPoint a, fixedPoint b, fixedPoint c)
{
    return MTH_Mul(a, MTH_Mul(b, c));
}

fixedPoint MTH_Mul(fixedPoint a, fixedPoint b)
{
    const std::int64_t product =
        static_cast<std::int64_t>(a.asS32()) * b.asS32();
    return fixedPoint::fromS32(
        fixed_point_detail::truncateShiftedToS32(product, 16)
    );
}

fixedPoint FP_Pow2(fixedPoint r4)
{
    const std::int64_t product =
        static_cast<std::int64_t>(r4.asS32()) * r4.asS32();
    return fixedPoint::fromS32(
        fixed_point_detail::truncateShiftedToS32(product, 16)
    );
}
