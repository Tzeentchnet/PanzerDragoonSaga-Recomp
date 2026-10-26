#include <array>
#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <iterator>

extern "C"
{
#include "ao.h"
#include "eng_ssf/scsp.h"

unsigned char sat_ram[512 * 1024] = {};
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
                "FAIL: %s (actual: 0x%04x, expected: 0x%04x)\n",
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
            std::fprintf(stderr, "%d SCSP register check(s) failed.\n", m_failures);
            return 1;
        }

        std::puts("All SCSP register checks passed.");
#if defined(PDS_TEST_EXPECT_ASAN)
        std::puts("AddressSanitizer instrumentation is active.");
#endif
        return 0;
    }

private:
    int m_failures = 0;
};

void writeRegister(std::uint16_t address, std::uint16_t value)
{
    SCSP_0_w(address / 2, value, 0);
}

std::uint16_t readRegister(std::uint16_t address)
{
    return SCSP_0_r(address / 2, 0);
}

void testMadrsRange(TestRunner& runner)
{
    std::memset(&SCSP, 0, sizeof(SCSP));

    for (std::uint16_t index = 0; index < 32; ++index)
    {
        const std::uint16_t value = static_cast<std::uint16_t>(0x4100u + index);
        writeRegister(static_cast<std::uint16_t>(0x780u + index * 2u), value);
        runner.expectEqual("documented MADRS write", SCSP.DSP.MADRS[index], value);
    }

    const std::array<std::uint16_t, 32> madrsBefore = {
        SCSP.DSP.MADRS[0], SCSP.DSP.MADRS[1], SCSP.DSP.MADRS[2], SCSP.DSP.MADRS[3],
        SCSP.DSP.MADRS[4], SCSP.DSP.MADRS[5], SCSP.DSP.MADRS[6], SCSP.DSP.MADRS[7],
        SCSP.DSP.MADRS[8], SCSP.DSP.MADRS[9], SCSP.DSP.MADRS[10], SCSP.DSP.MADRS[11],
        SCSP.DSP.MADRS[12], SCSP.DSP.MADRS[13], SCSP.DSP.MADRS[14], SCSP.DSP.MADRS[15],
        SCSP.DSP.MADRS[16], SCSP.DSP.MADRS[17], SCSP.DSP.MADRS[18], SCSP.DSP.MADRS[19],
        SCSP.DSP.MADRS[20], SCSP.DSP.MADRS[21], SCSP.DSP.MADRS[22], SCSP.DSP.MADRS[23],
        SCSP.DSP.MADRS[24], SCSP.DSP.MADRS[25], SCSP.DSP.MADRS[26], SCSP.DSP.MADRS[27],
        SCSP.DSP.MADRS[28], SCSP.DSP.MADRS[29], SCSP.DSP.MADRS[30], SCSP.DSP.MADRS[31]
    };

    std::fill(std::begin(SCSP.DSP.MPRO), std::end(SCSP.DSP.MPRO), std::uint16_t{0x5a5a});

    for (std::uint16_t address = 0x7c0; address < 0x800; address += 2)
    {
        writeRegister(address, static_cast<std::uint16_t>(0xa000u | address));
        runner.expectEqual("reserved range reads zero", readRegister(address), std::uint16_t{0});
    }

    for (std::size_t index = 0; index < madrsBefore.size(); ++index)
    {
        runner.expectEqual(
            "reserved range preserves MADRS",
            SCSP.DSP.MADRS[index],
            madrsBefore[index]
        );
    }
    for (std::uint16_t value : SCSP.DSP.MPRO)
    {
        runner.expectEqual("reserved range preserves MPRO", value, std::uint16_t{0x5a5a});
    }

    writeRegister(0x800, 0x1357);
    runner.expectEqual("MPRO begins at 0x800", SCSP.DSP.MPRO[0], std::uint16_t{0x1357});
    runner.expectEqual("MPRO readback begins at 0x800", readRegister(0x800), std::uint16_t{0x1357});

    writeRegister(0x402, 0x0040);
    runner.expectEqual("RBP retains bit 6", SCSP.DSP.RBP, std::uint32_t{0x40});
}
}

int main()
{
    TestRunner runner;
    testMadrsRange(runner);
    return runner.finish();
}
