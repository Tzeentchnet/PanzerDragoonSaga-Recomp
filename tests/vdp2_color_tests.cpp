#include "vdp2ColorCalculation.h"

#include <array>
#include <cstdint>
#include <cstdio>

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
                "FAIL: %s (actual: %u, expected: %u)\n",
                description,
                static_cast<unsigned int>(actual),
                static_cast<unsigned int>(expected)
            );
            ++mFailures;
        }
    }

    void expectTrue(const char* description, bool value)
    {
        if (!value)
        {
            std::fprintf(stderr, "FAIL: %s\n", description);
            ++mFailures;
        }
    }

    int finish() const
    {
        if (mFailures != 0)
        {
            std::fprintf(stderr, "%d VDP2 color check(s) failed.\n", mFailures);
            return 1;
        }
        std::puts("All VDP2 color checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

void testAllRatiosAndChannels(TestRunner& runner)
{
    using pds::vdp2::calculateRatioChannel;

    for (std::uint32_t ratio = 0; ratio < 32; ++ratio)
    {
        for (std::uint32_t top = 0; top < 256; ++top)
        {
            for (std::uint32_t second = 0; second < 256; ++second)
            {
                const std::uint8_t expected = static_cast<std::uint8_t>(
                    (top * (31u - ratio) + second * (ratio + 1u)) / 32u
                );
                if (
                    calculateRatioChannel(
                        static_cast<std::uint8_t>(top),
                        static_cast<std::uint8_t>(second),
                        static_cast<std::uint8_t>(ratio)
                    ) != expected
                )
                {
                    runner.expectTrue("exhaustive ratio calculation", false);
                    return;
                }
            }
        }
    }

    runner.expectEqual(
        "ratio zero keeps 31/32 of top",
        calculateRatioChannel(255, 0, 0),
        std::uint8_t{247}
    );
    runner.expectEqual(
        "ratio 31 selects second",
        calculateRatioChannel(255, 23, 31),
        std::uint8_t{23}
    );
    runner.expectEqual(
        "ratio 15 is an even blend",
        calculateRatioChannel(240, 16, 15),
        std::uint8_t{128}
    );
}

void testAdditiveMode(TestRunner& runner)
{
    using pds::vdp2::calculateAdditiveChannel;

    for (std::uint32_t top = 0; top < 256; ++top)
    {
        for (std::uint32_t second = 0; second < 256; ++second)
        {
            const std::uint8_t expected = static_cast<std::uint8_t>(
                top + second > 255 ? 255 : top + second
            );
            if (
                calculateAdditiveChannel(
                    static_cast<std::uint8_t>(top),
                    static_cast<std::uint8_t>(second)
                ) != expected
            )
            {
                runner.expectTrue("exhaustive additive calculation", false);
                return;
            }
        }
    }
}

void testRatioRegisters(TestRunner& runner)
{
    using pds::vdp2::ColorLayer;
    using pds::vdp2::ratioNumber;

    constexpr std::uint16_t ccrna = 0x1d07;
    constexpr std::uint16_t ccrr = 0x0013;
    runner.expectEqual("NBG0 uses CCRNA low bits", ratioNumber(ColorLayer::Nbg0, ccrna, ccrr), 7);
    runner.expectEqual("NBG1 uses CCRNA high bits", ratioNumber(ColorLayer::Nbg1, ccrna, ccrr), 29);
    runner.expectEqual("RBG0 uses CCRR low bits", ratioNumber(ColorLayer::Rbg0, ccrna, ccrr), 19);
}

void testTransparencyAndOrdering(TestRunner& runner)
{
    using pds::vdp2::ColorLayer;
    using pds::vdp2::priorityKey;

    runner.expectEqual("disabled layer is absent", priorityKey(ColorLayer::Nbg0, 7, false, true), -1);
    runner.expectEqual("transparent layer is absent", priorityKey(ColorLayer::Nbg0, 7, true, false), -1);

    const std::array<ColorLayer, 5> backToFront = {
        ColorLayer::Nbg3,
        ColorLayer::Nbg1,
        ColorLayer::Nbg0,
        ColorLayer::Rbg0,
        ColorLayer::Vdp1
    };
    for (std::size_t index = 1; index < backToFront.size(); ++index)
    {
        runner.expectTrue(
            "equal-priority tie break is back-to-front",
            priorityKey(backToFront[index - 1], 4, true, true) <
                priorityKey(backToFront[index], 4, true, true)
        );
    }
}
}

int main()
{
    TestRunner runner;
    testAllRatiosAndChannels(runner);
    testAdditiveMode(runner);
    testRatioRegisters(runner);
    testTransparencyAndOrdering(runner);
    return runner.finish();
}
