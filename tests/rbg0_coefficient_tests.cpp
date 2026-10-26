#include "rbg0CoefficientAddressing.h"

#include <cstdint>
#include <cstdio>
#include <limits>

#if defined(PDS_TEST_EXPECT_ASAN) && !defined(__SANITIZE_ADDRESS__)
#error "The AddressSanitizer configuration must compile this test with instrumentation."
#endif

namespace
{
class TestRunner
{
public:
    template <typename Actual, typename Expected>
    void expectEqual(const char* description, Actual actual, Expected expected)
    {
        if (actual != expected)
        {
            std::fprintf(
                stderr,
                "FAIL: %s (actual: 0x%08x, expected: 0x%08x)\n",
                description,
                static_cast<unsigned int>(actual),
                static_cast<unsigned int>(expected)
            );
            ++mFailures;
        }
    }

    int finish() const
    {
        if (mFailures != 0)
        {
            std::fprintf(stderr, "%d RBG0 coefficient check(s) failed.\n", mFailures);
            return 1;
        }
        std::puts("All RBG0 coefficient checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

void testTableBases(TestRunner& runner)
{
    using pds::vdp2::coefficientTableBase;

    runner.expectEqual(
        "parameter A uses low KTAOF bits",
        coefficientTableBase(0x0503, false, 4),
        std::uint32_t{0x0c0000}
    );
    runner.expectEqual(
        "parameter B uses high KTAOF bits",
        coefficientTableBase(0x0503, true, 4),
        std::uint32_t{0x140000}
    );
    runner.expectEqual(
        "two-byte entries scale the table base",
        coefficientTableBase(0x0503, true, 2),
        std::uint32_t{0x0a0000}
    );
}

void testAddressing(TestRunner& runner)
{
    using pds::vdp2::coefficientAddress;
    using pds::vdp2::coefficientVramAddress;

    runner.expectEqual(
        "fractional accumulator does not advance entry",
        coefficientAddress(0x40000, 0x0000ffff, 4),
        std::uint32_t{0x40000}
    );
    runner.expectEqual(
        "integer accumulator advances entry",
        coefficientAddress(0x40000, 0x00010000, 4),
        std::uint32_t{0x40004}
    );
    runner.expectEqual(
        "table address wraps to VDP2 VRAM",
        coefficientVramAddress(0x140000, 0x00010000, 4),
        std::uint32_t{0x40004}
    );
    runner.expectEqual(
        "negative accumulator traverses backward with hardware wrapping",
        coefficientVramAddress(0, -0x00010000, 4),
        std::uint32_t{0x3fffc}
    );
}

void testAccumulatorWrapping(TestRunner& runner)
{
    using pds::vdp2::wrappingAdd;

    runner.expectEqual(
        "positive fractional carry",
        wrappingAdd(0x0000ffff, 2),
        std::int32_t{0x00010001}
    );
    runner.expectEqual(
        "negative delta traverses opposite direction",
        wrappingAdd(0x00010000, -0x00008000),
        std::int32_t{0x00008000}
    );
    runner.expectEqual(
        "signed overflow wraps explicitly",
        wrappingAdd(std::numeric_limits<std::int32_t>::max(), 1),
        std::numeric_limits<std::int32_t>::min()
    );
}
}

int main()
{
    TestRunner runner;
    testTableBases(runner);
    testAddressing(runner);
    testAccumulatorWrapping(runner);
    return runner.finish();
}
