#include <array>
#include <bit>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <limits>

#include "fixedPoint.h"
#include "sh2_div1.h"

#if defined(PDS_TEST_EXPECT_ASAN) && !defined(__SANITIZE_ADDRESS__)
#error "The AddressSanitizer configuration must compile this test with instrumentation."
#endif

namespace
{
class TestRunner
{
public:
    void expectEqual(const char* description, std::int32_t actual, std::int32_t expected)
    {
        if (actual != expected)
        {
            std::fprintf(
                stderr,
                "FAIL: %s (actual: %d, expected: %d)\n",
                description,
                actual,
                expected
            );
            ++m_failures;
        }
    }

    void expectEqual64(
        const char* description,
        std::int64_t actual,
        std::int64_t expected
    )
    {
        if (actual != expected)
        {
            std::fprintf(
                stderr,
                "FAIL: %s (actual: %lld, expected: %lld)\n",
                description,
                static_cast<long long>(actual),
                static_cast<long long>(expected)
            );
            ++m_failures;
        }
    }

    void expectNear(const char* description, float actual, float expected)
    {
        if (std::fabs(actual - expected) > 0.00001f)
        {
            std::fprintf(
                stderr,
                "FAIL: %s (actual: %.6f, expected: %.6f)\n",
                description,
                static_cast<double>(actual),
                static_cast<double>(expected)
            );
            ++m_failures;
        }
    }

    void expectDiv1(
        std::uint32_t rm,
        std::uint32_t initialRn,
        bool initialQ,
        bool initialM,
        bool initialT,
        std::int32_t actualRn,
        const sh2::Div1State& actualState,
        std::int32_t expectedRn,
        bool expectedQ,
        bool expectedT
    )
    {
        if (actualRn == expectedRn &&
            actualState.q == expectedQ &&
            actualState.m == initialM &&
            actualState.t == expectedT)
        {
            return;
        }

        std::fprintf(
            stderr,
            "FAIL: DIV1 rm=%08x rn=%08x QMT=%d%d%d "
            "(actual rn/QMT=%08x/%d%d%d, expected=%08x/%d%d%d)\n",
            static_cast<unsigned int>(rm),
            static_cast<unsigned int>(initialRn),
            initialQ,
            initialM,
            initialT,
            static_cast<unsigned int>(std::bit_cast<std::uint32_t>(actualRn)),
            actualState.q,
            actualState.m,
            actualState.t,
            static_cast<unsigned int>(std::bit_cast<std::uint32_t>(expectedRn)),
            expectedQ,
            initialM,
            expectedT
        );
        ++m_failures;
    }

    int finish() const
    {
        if (m_failures != 0)
        {
            std::fprintf(stderr, "%d fixed-point check(s) failed.\n", m_failures);
            return 1;
        }

        std::puts("All fixed-point and DIV1 checks passed.");
#if defined(PDS_TEST_EXPECT_ASAN)
        std::puts("AddressSanitizer instrumentation is active.");
#endif
        return 0;
    }

private:
    int m_failures = 0;
};

void testConversions(TestRunner& runner)
{
    fixedPoint zero;
    runner.expectEqual("default construction", zero.asS32(), 0);
    runner.expectEqual("zero integer conversion", fixedPoint::fromInteger(0).asS32(), 0);
    runner.expectEqual("+1 integer conversion", fixedPoint::fromInteger(1).asS32(), 0x10000);
    runner.expectEqual("-1 integer conversion", fixedPoint::fromInteger(-1).asS32(), -0x10000);

    fixedPoint integer = fixedPoint::fromInteger(3);
    runner.expectEqual("integer raw representation", integer.asS32(), 0x00030000);
    runner.expectEqual("integer extraction", integer.getInteger(), 3);
    runner.expectNear("integer float conversion", integer.toFloat(), 3.0f);

    fixedPoint fractional = fixedPoint::fromS32(0x00018000);
    runner.expectEqual("fractional integer extraction", fractional.toInteger(), 1);
    runner.expectNear("fractional float conversion", fractional.toFloat(), 1.5f);

    fixedPoint negativeFractional = fixedPoint::fromS32(-0x00018000);
    runner.expectEqual(
        "negative arithmetic integer extraction",
        negativeFractional.getInteger(),
        -2
    );
    runner.expectEqual(
        "static negative integer extraction",
        fixedPoint::toInteger(negativeFractional),
        -2
    );

    runner.expectEqual(
        "minimum 16-bit integer conversion",
        fixedPoint::fromInteger(std::numeric_limits<std::int16_t>::min()).asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "maximum 16-bit integer conversion",
        fixedPoint::fromInteger(std::numeric_limits<std::int16_t>::max()).asS32(),
        0x7fff0000
    );
    runner.expectEqual(
        "minimum raw value preservation",
        fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min()).asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "maximum raw value preservation",
        fixedPoint::fromS32(std::numeric_limits<std::int32_t>::max()).asS32(),
        std::numeric_limits<std::int32_t>::max()
    );

    fixedPoint minimum = fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min());
    fixedPoint maximum = fixedPoint::fromS32(std::numeric_limits<std::int32_t>::max());
    runner.expectEqual("minimum raw integer portion", FP_GetIntegerPortion(minimum), -32768);
    runner.expectEqual("maximum raw integer portion", FP_GetIntegerPortion(maximum), 32767);
}

void testWrappingOperations(TestRunner& runner)
{
    fixedPoint value = fixedPoint::fromS32(0x00010000);
    value += fixedPoint::fromS32(0x00008000);
    runner.expectEqual("addition assignment", value.asS32(), 0x00018000);

    value -= fixedPoint::fromS32(0x00004000);
    runner.expectEqual("subtraction assignment", value.asS32(), 0x00014000);
    runner.expectEqual("unary negation", (-value).asS32(), -0x00014000);
    runner.expectEqual("absolute value", (-value).getAbs().asS32(), 0x00014000);

    fixedPoint addWrap = fixedPoint::fromS32(std::numeric_limits<std::int32_t>::max());
    addWrap += fixedPoint::fromS32(1);
    runner.expectEqual(
        "32-bit wrapping addition",
        addWrap.asS32(),
        std::numeric_limits<std::int32_t>::min()
    );

    fixedPoint subtractWrap =
        fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min());
    subtractWrap -= fixedPoint::fromS32(1);
    runner.expectEqual(
        "32-bit wrapping subtraction",
        subtractWrap.asS32(),
        std::numeric_limits<std::int32_t>::max()
    );

    fixedPoint multiplyWrap =
        fixedPoint::fromS32(std::numeric_limits<std::int32_t>::max());
    multiplyWrap *= fixedPoint::fromS32(2);
    runner.expectEqual("32-bit wrapping multiplication", multiplyWrap.asS32(), -2);

    fixedPoint minimum =
        fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min());
    runner.expectEqual(
        "minimum unary negation wraps",
        (-minimum).asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "minimum absolute value wraps",
        minimum.getAbs().asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
}

void testDivision(TestRunner& runner)
{
    fixedPoint quotient = fixedPoint::fromS32(-7);
    quotient /= fixedPoint::fromS32(3);
    runner.expectEqual("negative compound division truncates to zero", quotient.asS32(), -2);

    quotient = fixedPoint::fromS32(7);
    quotient /= fixedPoint::fromS32(-3);
    runner.expectEqual("mixed-sign compound division", quotient.asS32(), -2);

    quotient = fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min());
    quotient /= fixedPoint::fromS32(-1);
    runner.expectEqual(
        "compound division overflow wraps",
        quotient.asS32(),
        std::numeric_limits<std::int32_t>::min()
    );

    quotient = fixedPoint::fromS32(1);
    quotient /= fixedPoint::fromS32(0);
    runner.expectEqual(
        "positive compound division by zero",
        quotient.asS32(),
        std::numeric_limits<std::int32_t>::max()
    );

    quotient = fixedPoint::fromS32(-1);
    quotient /= fixedPoint::fromS32(0);
    runner.expectEqual(
        "negative compound division by zero",
        quotient.asS32(),
        std::numeric_limits<std::int32_t>::min()
    );

    runner.expectEqual(
        "negative fixed-point division truncates to zero",
        FP_Div(-1, fixedPoint::fromS32(3)).asS32(),
        -21845
    );
    runner.expectEqual(
        "mixed-sign fixed-point division",
        FP_Div(1, fixedPoint::fromS32(-3)).asS32(),
        -21845
    );
    runner.expectEqual(
        "double-negative fixed-point division",
        FP_Div(-1, fixedPoint::fromS32(-3)).asS32(),
        21845
    );
    runner.expectEqual(
        "fixed-point quotient truncates to register width",
        FP_Div(
            std::numeric_limits<std::int32_t>::max(),
            fixedPoint::fromS32(1)
        ).asS32(),
        -0x10000
    );
    runner.expectEqual(
        "fixed-point minimum divided by negative one truncates",
        FP_Div(
            std::numeric_limits<std::int32_t>::min(),
            fixedPoint::fromS32(-1)
        ).asS32(),
        0
    );
    runner.expectEqual(
        "positive fixed-point division by zero",
        FP_Div(1, fixedPoint::fromS32(0)).asS32(),
        std::numeric_limits<std::int32_t>::max()
    );
    runner.expectEqual(
        "negative fixed-point division by zero",
        FP_Div(-1, fixedPoint::fromS32(0)).asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "zero fixed-point division by zero",
        FP_Div(0, fixedPoint::fromS32(0)).asS32(),
        std::numeric_limits<std::int32_t>::max()
    );

    runner.expectEqual(
        "integer division truncates to zero",
        intDivide(fixedPoint::fromS32(3), fixedPoint::fromS32(-7)).asS32(),
        -2
    );
    runner.expectEqual(
        "integer division overflow wraps",
        intDivide(
            fixedPoint::fromS32(-1),
            fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min())
        ).asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "integer division by zero saturates",
        intDivide(fixedPoint::fromS32(0), fixedPoint::fromS32(1)).asS32(),
        std::numeric_limits<std::int32_t>::max()
    );
    runner.expectEqual("performDivision ordering", performDivision(3, -7), -2);
    runner.expectEqual(
        "performDivision zero edge",
        performDivision(0, -1),
        std::numeric_limits<std::int32_t>::min()
    );

    asyncDivStart(-1, fixedPoint::fromS32(3));
    runner.expectEqual("async fixed-point division", asyncDivEnd().asS32(), -21845);
    asyncDivStart_integer(std::numeric_limits<std::int32_t>::min(), -1);
    runner.expectEqual(
        "async integer division overflow truncates",
        asyncDivEnd().asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
    asyncDivStart_integer(-1, 0);
    runner.expectEqual(
        "async negative division by zero",
        asyncDivEnd().asS32(),
        std::numeric_limits<std::int32_t>::min()
    );
}

void testFixedPointProducts(TestRunner& runner)
{
    runner.expectEqual(
        "negative sub-LSB product uses arithmetic truncation",
        MTH_Mul(fixedPoint::fromS32(-1), fixedPoint::fromS32(0x8000)).asS32(),
        -1
    );
    runner.expectEqual(
        "negative fractional product",
        MTH_Mul(
            fixedPoint::fromS32(-0x18000),
            fixedPoint::fromS32(0x8000)
        ).asS32(),
        -0xc000
    );
    runner.expectEqual(
        "negative multiplier product",
        MTH_Mul(
            fixedPoint::fromS32(0x18000),
            fixedPoint::fromS32(-0x8000)
        ).asS32(),
        -0xc000
    );
    runner.expectEqual(
        "negative square",
        FP_Pow2(fixedPoint::fromS32(-0x18000)).asS32(),
        0x24000
    );
    runner.expectEqual64(
        "widened raw product",
        MUL_FP(
            fixedPoint::fromS32(std::numeric_limits<std::int32_t>::min()),
            fixedPoint::fromS32(-1)
        ),
        0x80000000LL
    );
}

void testSaturnNormalization(TestRunner& runner)
{
    runner.expectEqual(
        "zero normalization",
        fixedPoint::fromS32(0).normalized().asS32(),
        0
    );
    runner.expectEqual(
        "positive 28-bit normalization boundary",
        fixedPoint::fromS32(0x07ffffff).normalized().asS32(),
        0x07ffffff
    );
    runner.expectEqual(
        "negative 28-bit normalization boundary",
        fixedPoint::fromS32(0x08000000).normalized().asS32(),
        std::bit_cast<std::int32_t>(0xf8000000u)
    );
    runner.expectEqual(
        "negative 28-bit maximum",
        fixedPoint::fromS32(0x0fffffff).normalized().asS32(),
        -1
    );
    runner.expectEqual(
        "upper bits cleared for positive values",
        fixedPoint::fromS32(0x60765432).normalized().asS32(),
        0x00765432
    );
    runner.expectEqual(
        "upper bits ignored at positive boundary",
        fixedPoint::fromS32(std::bit_cast<std::int32_t>(0xf7ffffffu))
            .normalized()
            .asS32(),
        0x07ffffff
    );
    runner.expectEqual(
        "upper bits ignored at negative boundary",
        fixedPoint::fromS32(0x18000000).normalized().asS32(),
        std::bit_cast<std::int32_t>(0xf8000000u)
    );
}

void testFp16Wrapping(TestRunner& runner)
{
    runner.expectEqual("positive fp16 widening", fp16(3).toFP32(), 0x00030000);
    runner.expectEqual("negative fp16 widening", fp16(-2).toFP32(), -0x00020000);
    runner.expectEqual(
        "minimum fp16 widening",
        fp16(std::numeric_limits<std::int16_t>::min()).toFP32(),
        std::numeric_limits<std::int32_t>::min()
    );
    runner.expectEqual(
        "maximum fp16 widening",
        fp16(std::numeric_limits<std::int16_t>::max()).toFP32(),
        0x7fff0000
    );

    fp16 value(5);
    value += fp16(2);
    value -= fp16(1);
    runner.expectEqual("fp16 value operations", static_cast<std::int16_t>(value), 6);

    fp16 addWrap(std::numeric_limits<std::int16_t>::max());
    addWrap += fp16(1);
    runner.expectEqual(
        "16-bit wrapping addition",
        static_cast<std::int16_t>(addWrap),
        std::numeric_limits<std::int16_t>::min()
    );

    fp16 subtractWrap(std::numeric_limits<std::int16_t>::min());
    subtractWrap -= fp16(1);
    runner.expectEqual(
        "16-bit wrapping subtraction",
        static_cast<std::int16_t>(subtractWrap),
        std::numeric_limits<std::int16_t>::max()
    );

    fp16 multiplyWrap(std::numeric_limits<std::int16_t>::max());
    multiplyWrap *= fp16(2);
    runner.expectEqual(
        "16-bit wrapping multiplication",
        static_cast<std::int16_t>(multiplyWrap),
        -2
    );
    runner.expectEqual(
        "minimum fp16 negation wraps",
        static_cast<std::int16_t>(-fp16(std::numeric_limits<std::int16_t>::min())),
        std::numeric_limits<std::int16_t>::min()
    );
}

struct Div1Reference
{
    std::int32_t rn;
    bool q;
    bool t;
    bool carryOrBorrow;
};

Div1Reference referenceDiv1(
    std::uint32_t rm,
    std::uint32_t rn,
    bool oldQ,
    bool m,
    bool t
)
{
    const bool shiftedOut = (rn & 0x80000000u) != 0;
    const std::uint32_t shifted = (rn << 1) | static_cast<std::uint32_t>(t);

    std::uint32_t result;
    bool carryOrBorrow;
    bool q;
    if (oldQ == m)
    {
        carryOrBorrow = shifted < rm;
        result = shifted - rm;
        q = oldQ
            ? (shiftedOut ? carryOrBorrow : !carryOrBorrow)
            : (shiftedOut ? !carryOrBorrow : carryOrBorrow);
    }
    else
    {
        const std::uint64_t sum =
            static_cast<std::uint64_t>(shifted) + static_cast<std::uint64_t>(rm);
        carryOrBorrow = sum > std::numeric_limits<std::uint32_t>::max();
        result = static_cast<std::uint32_t>(sum);
        q = oldQ
            ? (shiftedOut ? !carryOrBorrow : carryOrBorrow)
            : (shiftedOut ? carryOrBorrow : !carryOrBorrow);
    }

    return {
        std::bit_cast<std::int32_t>(result),
        q,
        q == m,
        carryOrBorrow
    };
}

void testDiv1(TestRunner& runner)
{
    constexpr std::array<std::uint32_t, 5> divisors{
        0x00000000u,
        0x00000001u,
        0x7fffffffu,
        0x80000000u,
        0xffffffffu
    };
    constexpr std::array<std::uint32_t, 8> dividends{
        0x00000000u,
        0x00000001u,
        0x3fffffffu,
        0x40000000u,
        0x7fffffffu,
        0x80000000u,
        0xbfffffffu,
        0xffffffffu
    };

    std::uint32_t stateCoverage = 0;
    std::uint32_t transitionCoverage = 0;
    for (int initialQ = 0; initialQ <= 1; ++initialQ)
    {
        for (int initialM = 0; initialM <= 1; ++initialM)
        {
            for (int initialT = 0; initialT <= 1; ++initialT)
            {
                stateCoverage |=
                    1u << ((initialQ << 2) | (initialM << 1) | initialT);

                for (const std::uint32_t rm : divisors)
                {
                    for (const std::uint32_t initialRn : dividends)
                    {
                        const Div1Reference expected = referenceDiv1(
                            rm,
                            initialRn,
                            initialQ != 0,
                            initialM != 0,
                            initialT != 0
                        );

                        std::int32_t rn = std::bit_cast<std::int32_t>(initialRn);
                        sh2::Div1State state{
                            initialQ != 0,
                            initialM != 0,
                            initialT != 0
                        };
                        sh2::div1(std::bit_cast<std::int32_t>(rm), rn, state);

                        runner.expectDiv1(
                            rm,
                            initialRn,
                            initialQ != 0,
                            initialM != 0,
                            initialT != 0,
                            rn,
                            state,
                            expected.rn,
                            expected.q,
                            expected.t
                        );

                        const unsigned int transition =
                            (static_cast<unsigned int>(initialQ) << 3) |
                            (static_cast<unsigned int>(initialM) << 2) |
                            (((initialRn >> 31) & 1u) << 1) |
                            static_cast<unsigned int>(expected.carryOrBorrow);
                        transitionCoverage |= 1u << transition;
                    }
                }
            }
        }
    }

    runner.expectEqual(
        "all Q/M/T input states covered",
        static_cast<std::int32_t>(stateCoverage),
        0xff
    );
    runner.expectEqual(
        "all DIV1 carry/borrow transitions covered",
        static_cast<std::int32_t>(transitionCoverage),
        0xffff
    );
}
}

int main()
{
    TestRunner runner;
    testConversions(runner);
    testWrappingOperations(runner);
    testDivision(runner);
    testFixedPointProducts(runner);
    testSaturnNormalization(runner);
    testFp16Wrapping(runner);
    testDiv1(runner);
    return runner.finish();
}
