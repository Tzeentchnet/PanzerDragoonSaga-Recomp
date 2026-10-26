#include "PDS.h"
#include "framePacing.h"

#include "frameCadence.h"

namespace
{
bool gExperimentalNtscCadence = false;
}

bool framePacing_enableExperimentalNtscCadence()
{
#if defined(PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE)
    gExperimentalNtscCadence = true;
    return true;
#else
    return false;
#endif
}

bool framePacing_isExperimentalNtscCadenceEnabled()
{
    return gExperimentalNtscCadence;
}

void framePacing_waitForNextFrame(int frameLimit)
{
    static bool wasExperimental = false;
    static Uint64 lastFrame = SDL_GetTicksNS();
    static Uint64 nextExperimentalFrame = 0;
    static pds::timing::NtscGameFramePeriod experimentalPeriod;

    if (gExperimentalNtscCadence)
    {
        const Uint64 now = SDL_GetTicksNS();
        if (!wasExperimental || nextExperimentalFrame == 0)
        {
            nextExperimentalFrame = now;
            experimentalPeriod.reset();
        }

        const Uint64 interval = experimentalPeriod.nextNanoseconds();
        nextExperimentalFrame += interval;
        if (now < nextExperimentalFrame)
        {
            SDL_DelayPrecise(nextExperimentalFrame - now);
        }
        else if (now - nextExperimentalFrame > interval * 4)
        {
            nextExperimentalFrame = now;
            experimentalPeriod.reset();
        }

        wasExperimental = true;
        lastFrame = SDL_GetTicksNS();
        return;
    }

    wasExperimental = false;
    nextExperimentalFrame = 0;
    if (frameLimit <= 0)
    {
        lastFrame = SDL_GetTicksNS();
        return;
    }

    const Uint64 now = SDL_GetTicksNS();
    const Uint64 interval = 1'000'000'000ull / static_cast<Uint64>(frameLimit);
    const Uint64 elapsed = now - lastFrame;
    if (elapsed < interval)
    {
        SDL_DelayPrecise(interval - elapsed);
    }
    lastFrame = SDL_GetTicksNS();
}
