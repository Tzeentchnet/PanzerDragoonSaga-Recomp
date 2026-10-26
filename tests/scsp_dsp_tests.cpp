#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <vector>

#include "audio/SCSPTiming.h"

extern "C"
{
#include "ao.h"
#include "eng_ssf/scsp.h"
}

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
            ++m_failures;
        }
    }

    int finish() const
    {
        if (m_failures != 0)
        {
            std::fprintf(stderr, "%d SCSP DSP check(s) failed.\n", m_failures);
            return 1;
        }

        std::puts("All SCSP DSP checks passed.");
#if defined(PDS_TEST_EXPECT_ASAN)
        std::puts("AddressSanitizer instrumentation is active.");
#endif
        return 0;
    }

private:
    int m_failures = 0;
};

struct Step
{
    std::uint32_t tra = 0;
    std::uint32_t twt = 0;
    std::uint32_t twa = 0;
    std::uint32_t xsel = 0;
    std::uint32_t ysel = 0;
    std::uint32_t ira = 0;
    std::uint32_t iwt = 0;
    std::uint32_t iwa = 0;
    std::uint32_t table = 0;
    std::uint32_t mwt = 0;
    std::uint32_t mrd = 0;
    std::uint32_t ewt = 0;
    std::uint32_t ewa = 0;
    std::uint32_t adrl = 0;
    std::uint32_t frcl = 0;
    std::uint32_t shift = 0;
    std::uint32_t yrl = 0;
    std::uint32_t negb = 0;
    std::uint32_t zero = 0;
    std::uint32_t bsel = 0;
    std::uint32_t nofl = 0;
    std::uint32_t coef = 0;
    std::uint32_t masa = 0;
    std::uint32_t adreb = 0;
    std::uint32_t nxadr = 0;
};

class DspFixture
{
public:
    DspFixture()
        : ram(0x40000)
    {
        reset();
    }

    void reset()
    {
        std::fill(ram.begin(), ram.end(), std::uint16_t{0});
        SCSPDSP_Init(&dsp);
        dsp.SCSPRAM = ram.data();
        dsp.SCSPRAM_LENGTH = static_cast<UINT32>(ram.size());
        dsp.TEMP[0] = 0x123400;
        dsp.MADRS[0] = 0x100;
    }

    void put(std::size_t index, const Step& step)
    {
        const std::uint16_t words[4] = {
            static_cast<std::uint16_t>((step.tra << 8) | (step.twt << 7) | step.twa),
            static_cast<std::uint16_t>(
                (step.xsel << 15) | (step.ysel << 13) | (step.ira << 6) |
                (step.iwt << 5) | step.iwa
            ),
            static_cast<std::uint16_t>(
                (step.table << 15) | (step.mwt << 14) | (step.mrd << 13) |
                (step.ewt << 12) | (step.ewa << 8) | (step.adrl << 7) |
                (step.frcl << 6) | (step.shift << 4) | (step.yrl << 3) |
                (step.negb << 2) | (step.zero << 1) | step.bsel
            ),
            static_cast<std::uint16_t>(
                (step.coef << 9) | (step.nofl << 8) | (step.masa << 2) |
                (step.adreb << 1) | step.nxadr
            )
        };
        std::memcpy(&dsp.MPRO[index * 4], words, sizeof(words));
    }

    void start()
    {
        SCSPDSP_Start(&dsp);
    }

    void sample()
    {
        SCSPDSP_Step(&dsp);
    }

    _SCSPDSP dsp{};
    std::vector<std::uint16_t> ram;
};

void testDelayedAccesses(TestRunner& runner)
{
    DspFixture fixture;
    fixture.put(2, {.table = 1, .mwt = 1, .nofl = 1});
    fixture.start();
    fixture.sample();
    runner.expectEqual("even-step write reaches sound RAM", fixture.ram[0x100], std::uint16_t{0x1234});

    fixture.reset();
    fixture.ram[0x100] = 0x5678;
    fixture.put(0, {.table = 1, .mrd = 1, .nofl = 1});
    fixture.put(1, {.iwt = 1, .iwa = 1});
    fixture.put(2, {.iwt = 1, .iwa = 2});
    fixture.start();
    fixture.sample();
    runner.expectEqual("read is not visible one step later", fixture.dsp.MEMS[1], 0);
    runner.expectEqual("read is visible two steps later", fixture.dsp.MEMS[2], 0x567800);
}

void testPersistentState(TestRunner& runner)
{
    DspFixture fixture;
    fixture.put(0, {.ewt = 1});
    fixture.start();
    fixture.sample();
    runner.expectEqual("first sample starts with a clear accumulator", fixture.dsp.EFREG[0], std::int16_t{0});
    fixture.sample();
    runner.expectEqual(
        "accumulator persists between samples",
        fixture.dsp.EFREG[0],
        std::int16_t{0x1234}
    );

    std::memset(fixture.dsp.MPRO, 0, sizeof(fixture.dsp.MPRO));
    fixture.start();
    fixture.sample();
    runner.expectEqual(
        "EFREG persists when no step writes it",
        fixture.dsp.EFREG[0],
        std::int16_t{0x1234}
    );
}

void testAccumulatorWidth(TestRunner& runner)
{
    DspFixture fixture;
    fixture.dsp.TEMP[0] = 0x7fffff;
    fixture.dsp.COEF[0] = 0x7ff8;
    for (std::size_t step = 1; step <= 5; ++step)
    {
        fixture.put(step, {.ysel = 1, .bsel = 1});
    }
    fixture.put(6, {.ewt = 1});
    fixture.start();
    fixture.sample();
    runner.expectEqual(
        "accumulator wraps to 26 bits before clamping",
        fixture.dsp.EFREG[0],
        static_cast<std::int16_t>(0x8000)
    );
}

void testLastStepAndRbp(TestRunner& runner)
{
    DspFixture fixture;
    fixture.put(127, {.table = 1, .mwt = 1, .nofl = 1});
    fixture.start();
    fixture.sample();
    runner.expectEqual("step 127 defers its write", fixture.ram[0x100], std::uint16_t{0});
    fixture.sample();
    runner.expectEqual("all 128 steps execute", fixture.ram[0x100], std::uint16_t{0x1234});

    fixture.reset();
    fixture.ram[0x100] = 0x5678;
    fixture.dsp.RBP = 0x40;
    fixture.put(0, {.table = 1, .mrd = 1, .nofl = 1});
    fixture.put(2, {.iwt = 1, .iwa = 1});
    fixture.start();
    fixture.sample();
    runner.expectEqual("RBP bit 6 addresses past sound RAM", fixture.dsp.MEMS[1], 0);
}

void testSoundClockRatio(TestRunner& runner)
{
    runner.expectEqual("SCSP sample rate", kScspSampleRate, std::uint32_t{44'100});
    runner.expectEqual(
        "M68000 cycles per SCSP sample",
        kScspM68kCyclesPerSample,
        std::uint32_t{256}
    );
    runner.expectEqual("derived sound CPU clock", kScspM68kClock, std::uint32_t{11'289'600});
}
}

int main()
{
    TestRunner runner;
    testDelayedAccesses(runner);
    testPersistentState(runner);
    testAccumulatorWidth(runner);
    testLastStepAndRbp(runner);
    testSoundClockRatio(runner);
    return runner.finish();
}
