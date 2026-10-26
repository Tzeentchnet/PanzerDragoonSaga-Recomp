#include "frameCadence.h"
#include "frameCompletion.h"

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
                "FAIL: %s (actual: %llu, expected: %llu)\n",
                description,
                static_cast<unsigned long long>(actual),
                static_cast<unsigned long long>(expected)
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
            std::fprintf(stderr, "%d frame timing check(s) failed.\n", mFailures);
            return 1;
        }
        std::puts("All frame timing checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

void testLongRunCadence(TestRunner& runner)
{
    pds::timing::NtscGameFramePeriod cadence;
    std::uint64_t elapsed = 0;
    for (
        std::uint32_t frame = 0;
        frame < pds::timing::kGameFramesPerThousandSeconds;
        ++frame
    )
    {
        elapsed += cadence.nextNanoseconds();
    }

    runner.expectEqual(
        "29,913 game frames span exactly 1,000 seconds",
        elapsed,
        pds::timing::kNanosecondsPerThousandSeconds
    );
    runner.expectEqual(
        "44.1 kHz audio produces 44,100,000 samples in 1,000 seconds",
        std::uint64_t{44'100} * elapsed / 1'000'000'000ull,
        std::uint64_t{44'100'000}
    );
}

void testVblankOrdering(TestRunner& runner)
{
    using pds::timing::shouldPresentFrame;

    runner.expectTrue(
        "frame is not presented before the game marks it ready",
        !shouldPresentFrame(false, 2, 2, true, 2)
    );
    runner.expectTrue(
        "frame is not presented before its second VBlank",
        !shouldPresentFrame(true, 1, 2, true, 1)
    );
    runner.expectTrue(
        "idle VDP1 permits presentation on the second VBlank",
        shouldPresentFrame(true, 2, 2, true, 2)
    );
    runner.expectTrue(
        "busy VDP1 delays presentation",
        !shouldPresentFrame(true, 3, 2, false, 3)
    );
    runner.expectTrue(
        "busy VDP1 is released after four waits",
        shouldPresentFrame(true, 4, 2, false, 4)
    );
}
}

int main()
{
    TestRunner runner;
    testLongRunCadence(runner);
    testVblankOrdering(runner);
    return runner.finish();
}
