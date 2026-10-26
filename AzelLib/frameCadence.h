#pragma once

#include <cstdint>

namespace pds::timing
{
inline constexpr std::uint32_t kNtscRasterRateMilliHz = 59'826;
inline constexpr std::uint32_t kVblanksPerGameFrame = 2;
inline constexpr std::uint32_t kGameFramesPerThousandSeconds =
    kNtscRasterRateMilliHz / kVblanksPerGameFrame;
inline constexpr std::uint64_t kNanosecondsPerThousandSeconds =
    1'000'000'000'000ull;

class NtscGameFramePeriod
{
public:
    constexpr std::uint64_t nextNanoseconds()
    {
        std::uint64_t interval =
            kNanosecondsPerThousandSeconds / kGameFramesPerThousandSeconds;
        mRemainder +=
            kNanosecondsPerThousandSeconds % kGameFramesPerThousandSeconds;
        if (mRemainder >= kGameFramesPerThousandSeconds)
        {
            ++interval;
            mRemainder -= kGameFramesPerThousandSeconds;
        }
        return interval;
    }

    constexpr void reset()
    {
        mRemainder = 0;
    }

private:
    std::uint64_t mRemainder = 0;
};
}
