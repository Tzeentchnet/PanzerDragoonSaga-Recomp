#include "PDS.h"
#include "regressionCapture.h"

#include "VDP1.h"
#include "VDP2.h"
#include "inputRecorder.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <charconv>
#include <cstdio>
#include <cstring>
#include <filesystem>
#include <limits>
#include <mutex>
#include <string>
#include <system_error>
#include <vector>

namespace
{
struct StateHashes
{
    std::uint64_t game = 0;
    std::uint64_t vdp1 = 0;
    std::uint64_t vdp2 = 0;
    std::uint64_t combined = 0;
};

std::filesystem::path gCaptureDirectory;
std::string gRouteName = "route";
std::vector<std::uint32_t> gCaptureFrames;
std::uint32_t gCaptureEvery = 0;
std::uint32_t gRegion = 0;
std::uint8_t gConnectedMask = 0x3;
std::FILE* gManifest = nullptr;
std::FILE* gAudio = nullptr;
std::FILE* gRbg0Coefficients = nullptr;
bool gCaptureRbg0Coefficients = false;
std::mutex gAudioMutex;
std::mutex gErrorMutex;
std::atomic<std::uint64_t> gAudioSamples = 0;
std::atomic<bool> gAudioCaptureEnabled = false;
std::atomic<bool> gFailed = false;
char gLastError[256] = {};

void setError(const char* message)
{
    const std::lock_guard<std::mutex> lock(gErrorMutex);
    std::snprintf(gLastError, sizeof(gLastError), "%s", message);
    std::fprintf(stderr, "Regression capture: %s\n", gLastError);
    gFailed.store(true, std::memory_order_release);
}

bool parseUnsigned(const char* text, std::uint32_t& value)
{
    if (!text || text[0] == '\0')
    {
        return false;
    }

    const char* begin = text;
    int base = 10;
    if (text[0] == '0' && (text[1] == 'x' || text[1] == 'X'))
    {
        begin += 2;
        base = 16;
    }
    const char* end = text + std::strlen(text);
    const std::from_chars_result result = std::from_chars(begin, end, value, base);
    return result.ec == std::errc{} && result.ptr == end;
}

std::uint64_t hashBytes(std::uint64_t hash, const void* data, std::size_t size)
{
    const auto* bytes = static_cast<const std::uint8_t*>(data);
    for (std::size_t index = 0; index < size; ++index)
    {
        hash ^= bytes[index];
        hash *= 1099511628211ull;
    }
    return hash;
}

StateHashes computeHashes()
{
    constexpr std::uint64_t offsetBasis = 14695981039346656037ull;
    StateHashes hashes;

    hashes.game = hashBytes(offsetBasis, mainGameState.bitField, sizeof(mainGameState.bitField));
    hashes.game = hashBytes(
        hashes.game,
        mainGameState.consumables.data(),
        mainGameState.consumables.size()
    );
    hashes.game = hashBytes(hashes.game, &mainGameState.gameStats, sizeof(mainGameState.gameStats));
    hashes.game = hashBytes(hashes.game, playerDataMemoryBuffer, sizeof(playerDataMemoryBuffer));

    hashes.vdp1 = hashBytes(offsetBasis, getVdp1Pointer(0x25C00000), 0x80000);

    hashes.vdp2 = hashBytes(offsetBasis, getVdp2Vram(0), 0x80000);
    hashes.vdp2 = hashBytes(hashes.vdp2, getVdp2Cram(0), 0x1000);
    if (vdp2Controls.m4_pendingVdp2Regs)
    {
        hashes.vdp2 = hashBytes(
            hashes.vdp2,
            vdp2Controls.m4_pendingVdp2Regs,
            sizeof(*vdp2Controls.m4_pendingVdp2Regs)
        );
    }

    hashes.combined = hashBytes(offsetBasis, &hashes.game, sizeof(hashes.game));
    hashes.combined = hashBytes(hashes.combined, &hashes.vdp1, sizeof(hashes.vdp1));
    hashes.combined = hashBytes(hashes.combined, &hashes.vdp2, sizeof(hashes.vdp2));
    return hashes;
}

void putU16(std::uint8_t* output, std::uint16_t value)
{
    output[0] = static_cast<std::uint8_t>(value);
    output[1] = static_cast<std::uint8_t>(value >> 8);
}

void putU32(std::uint8_t* output, std::uint32_t value)
{
    putU16(output, static_cast<std::uint16_t>(value));
    putU16(output + 2, static_cast<std::uint16_t>(value >> 16));
}

bool writeWavHeader(std::FILE* file, std::uint32_t sampleCount)
{
    if (!file)
    {
        return false;
    }

    constexpr std::uint16_t channels = 2;
    constexpr std::uint32_t sampleRate = 44'100;
    constexpr std::uint16_t bitsPerSample = 16;
    constexpr std::uint16_t blockAlign = channels * bitsPerSample / 8;
    const std::uint32_t dataSize = sampleCount * blockAlign;
    std::array<std::uint8_t, 44> header{};
    std::memcpy(header.data(), "RIFF", 4);
    putU32(header.data() + 4, 36 + dataSize);
    std::memcpy(header.data() + 8, "WAVEfmt ", 8);
    putU32(header.data() + 16, 16);
    putU16(header.data() + 20, 1);
    putU16(header.data() + 22, channels);
    putU32(header.data() + 24, sampleRate);
    putU32(header.data() + 28, sampleRate * blockAlign);
    putU16(header.data() + 32, blockAlign);
    putU16(header.data() + 34, bitsPerSample);
    std::memcpy(header.data() + 36, "data", 4);
    putU32(header.data() + 40, dataSize);
    return std::fwrite(header.data(), header.size(), 1, file) == 1;
}

std::filesystem::path artifactPath(const char* suffix)
{
    return gCaptureDirectory / (gRouteName + suffix);
}
}

bool regressionCapture_configure(int argc, char* argv[])
{
    gFailed.store(false, std::memory_order_relaxed);
    gLastError[0] = '\0';
    gAudioSamples.store(0, std::memory_order_relaxed);

    for (int index = 1; index < argc; ++index)
    {
        const char* argument = argv[index];
        if (std::strcmp(argument, "--capture-dir") == 0)
        {
            if (++index >= argc)
            {
                setError("--capture-dir requires a path");
                return false;
            }
            gCaptureDirectory = argv[index];
        }
        else if (std::strcmp(argument, "--capture-frame") == 0)
        {
            std::uint32_t frame = 0;
            if (++index >= argc || !parseUnsigned(argv[index], frame))
            {
                setError("--capture-frame requires an unsigned frame number");
                return false;
            }
            gCaptureFrames.push_back(frame);
        }
        else if (std::strcmp(argument, "--capture-every") == 0)
        {
            if (++index >= argc || !parseUnsigned(argv[index], gCaptureEvery) || gCaptureEvery == 0)
            {
                setError("--capture-every requires a positive frame interval");
                return false;
            }
        }
        else if (std::strcmp(argument, "--route") == 0)
        {
            if (++index >= argc || argv[index][0] == '\0')
            {
                setError("--route requires a non-empty name");
                return false;
            }
            gRouteName = argv[index];
        }
        else if (std::strcmp(argument, "--region") == 0)
        {
            if (++index >= argc || !parseUnsigned(argv[index], gRegion))
            {
                setError("--region requires an unsigned value");
                return false;
            }
        }
        else if (std::strcmp(argument, "--controller-mask") == 0)
        {
            std::uint32_t mask = 0;
            if (++index >= argc || !parseUnsigned(argv[index], mask) || mask > 0x3)
            {
                setError("--controller-mask requires a value from 0 through 3");
                return false;
            }
            gConnectedMask = static_cast<std::uint8_t>(mask);
        }
        else if (std::strcmp(argument, "--capture-rbg0-coefficients") == 0)
        {
#if defined(PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING)
            gCaptureRbg0Coefficients = true;
#else
            setError(
                "--capture-rbg0-coefficients requires "
                "PDS_EXPERIMENTAL_RBG0_COEFFICIENT_ADDRESSING"
            );
            return false;
#endif
        }
    }

    std::sort(gCaptureFrames.begin(), gCaptureFrames.end());
    gCaptureFrames.erase(std::unique(gCaptureFrames.begin(), gCaptureFrames.end()), gCaptureFrames.end());
    return true;
}

bool regressionCapture_initialize()
{
    inputRecorder_setFrameHashCallback(regressionCapture_computeStateHash);
    inputRecorder_setAudioPositionCallback(regressionCapture_getAudioSamplePosition);

    if (gCaptureDirectory.empty())
    {
        if (gCaptureRbg0Coefficients)
        {
            setError("--capture-rbg0-coefficients requires --capture-dir");
            return false;
        }
        return true;
    }

    std::error_code error;
    std::filesystem::create_directories(gCaptureDirectory, error);
    if (error)
    {
        setError("failed to create capture directory");
        return false;
    }

    gManifest = std::fopen(artifactPath("_frames.csv").string().c_str(), "wb");
    if (!gManifest)
    {
        setError("failed to create frame manifest");
        return false;
    }
    if (std::fprintf(gManifest, "frame,game_state,vdp1,vdp2,audio_samples\n") < 0)
    {
        setError("failed to write frame manifest header");
        return false;
    }

    gAudio = std::fopen(artifactPath("_audio.wav").string().c_str(), "wb");
    if (!gAudio || !writeWavHeader(gAudio, 0))
    {
        setError("failed to create audio capture");
        return false;
    }
    gAudioCaptureEnabled.store(true, std::memory_order_release);

    if (gCaptureRbg0Coefficients)
    {
        gRbg0Coefficients =
            std::fopen(artifactPath("_rbg0_coefficients.csv").string().c_str(), "wb");
        if (
            !gRbg0Coefficients ||
            std::fprintf(
                gRbg0Coefficients,
                "frame,scanline,pixel,parameter,per_pixel,accumulator,"
                "table_base,vram_address\n"
            ) < 0
        )
        {
            setError("failed to create RBG0 coefficient trace");
            return false;
        }
    }
    return true;
}

void regressionCapture_shutdown()
{
    inputRecorder_setFrameHashCallback(nullptr);
    inputRecorder_setAudioPositionCallback(nullptr);

    gAudioCaptureEnabled.store(false, std::memory_order_release);
    {
        std::lock_guard<std::mutex> lock(gAudioMutex);
        if (gAudio)
        {
            const std::uint64_t samples = gAudioSamples.load(std::memory_order_relaxed);
            if (
                samples > (std::numeric_limits<std::uint32_t>::max() - 36u) / 4u ||
                std::fseek(gAudio, 0, SEEK_SET) != 0 ||
                !writeWavHeader(gAudio, static_cast<std::uint32_t>(samples)) ||
                std::fclose(gAudio) != 0
            )
            {
                setError("failed to finalize audio capture");
            }
            gAudio = nullptr;
        }
    }

    if (gManifest)
    {
        if (std::fclose(gManifest) != 0)
        {
            setError("failed to finalize frame manifest");
        }
        gManifest = nullptr;
    }

    if (gRbg0Coefficients)
    {
        if (std::fclose(gRbg0Coefficients) != 0)
        {
            setError("failed to finalize RBG0 coefficient trace");
        }
        gRbg0Coefficients = nullptr;
    }
}

bool regressionCapture_hasFailed()
{
    return gFailed.load(std::memory_order_acquire);
}

const char* regressionCapture_getLastError()
{
    return gLastError;
}

const char* regressionCapture_getRouteName()
{
    return gRouteName.c_str();
}

std::uint32_t regressionCapture_getRegion()
{
    return gRegion;
}

std::uint8_t regressionCapture_getConnectedMask()
{
    return gConnectedMask;
}

std::uint64_t regressionCapture_computeStateHash()
{
    return computeHashes().combined;
}

std::uint64_t regressionCapture_getAudioSamplePosition()
{
    return gAudioSamples.load(std::memory_order_relaxed);
}

void regressionCapture_appendAudioSample(std::int16_t left, std::int16_t right)
{
    gAudioSamples.fetch_add(1, std::memory_order_relaxed);
    if (!gAudioCaptureEnabled.load(std::memory_order_acquire))
    {
        return;
    }

    std::lock_guard<std::mutex> lock(gAudioMutex);
    if (!gAudioCaptureEnabled.load(std::memory_order_relaxed) || !gAudio)
    {
        return;
    }

    std::array<std::uint8_t, 4> sample{};
    putU16(sample.data(), static_cast<std::uint16_t>(left));
    putU16(sample.data() + 2, static_cast<std::uint16_t>(right));
    if (std::fwrite(sample.data(), sample.size(), 1, gAudio) != 1)
    {
        gAudioCaptureEnabled.store(false, std::memory_order_release);
        setError("failed to append audio sample");
    }
}

void regressionCapture_recordFrame(std::uint32_t frame)
{
    if (!gManifest)
    {
        return;
    }

    const StateHashes hashes = computeHashes();
    if (
        std::fprintf(
            gManifest,
            "%u,%016llx,%016llx,%016llx,%llu\n",
            frame,
            static_cast<unsigned long long>(hashes.game),
            static_cast<unsigned long long>(hashes.vdp1),
            static_cast<unsigned long long>(hashes.vdp2),
            static_cast<unsigned long long>(regressionCapture_getAudioSamplePosition())
        ) < 0 ||
        std::fflush(gManifest) != 0
    )
    {
        setError("failed to append frame manifest");
    }
}

bool regressionCapture_shouldCaptureImage(std::uint32_t frame)
{
    if (gCaptureDirectory.empty())
    {
        return false;
    }
    if (gCaptureEvery != 0 && frame % gCaptureEvery == 0)
    {
        return true;
    }
    return std::binary_search(gCaptureFrames.begin(), gCaptureFrames.end(), frame);
}

bool regressionCapture_makeImagePath(
    std::uint32_t frame,
    const char* label,
    char* output,
    std::size_t outputSize
)
{
    if (!output || outputSize == 0 || !label || gCaptureDirectory.empty())
    {
        return false;
    }

    const std::filesystem::path path =
        gCaptureDirectory /
        (gRouteName + "_f" + std::to_string(frame) + "_" + label);
    const std::string pathString = path.string();
    if (pathString.size() + 1 > outputSize)
    {
        setError("capture image path is too long");
        return false;
    }
    std::memcpy(output, pathString.c_str(), pathString.size() + 1);
    return true;
}

bool regressionCapture_writeRgbaTga(
    std::uint32_t frame,
    const char* label,
    const std::uint32_t* pixels,
    std::uint16_t width,
    std::uint16_t height
)
{
    if (!pixels || !regressionCapture_shouldCaptureImage(frame))
    {
        return true;
    }

    char path[512];
    if (!regressionCapture_makeImagePath(frame, label, path, sizeof(path)))
    {
        return false;
    }
    std::strncat(path, ".tga", sizeof(path) - std::strlen(path) - 1);

    std::FILE* file = std::fopen(path, "wb");
    if (!file)
    {
        setError("failed to create layer image");
        return false;
    }

    std::array<std::uint8_t, 18> header{};
    header[2] = 2;
    putU16(header.data() + 12, width);
    putU16(header.data() + 14, height);
    header[16] = 32;
    header[17] = 0x28;
    bool success = std::fwrite(header.data(), header.size(), 1, file) == 1;
    for (std::size_t index = 0; success && index < static_cast<std::size_t>(width) * height; ++index)
    {
        const std::uint32_t rgba = pixels[index];
        const std::array<std::uint8_t, 4> bgra = {
            static_cast<std::uint8_t>(rgba >> 16),
            static_cast<std::uint8_t>(rgba >> 8),
            static_cast<std::uint8_t>(rgba),
            static_cast<std::uint8_t>(rgba >> 24)
        };
        success = std::fwrite(bgra.data(), bgra.size(), 1, file) == 1;
    }
    success = std::fclose(file) == 0 && success;
    if (!success)
    {
        setError("failed to write layer image");
    }
    return success;
}

bool regressionCapture_isRbg0CoefficientTraceEnabled()
{
    return gRbg0Coefficients != nullptr;
}

void regressionCapture_recordRbg0Coefficient(
    std::uint32_t frame,
    std::uint32_t scanline,
    std::uint32_t pixel,
    bool parameterB,
    bool perPixel,
    std::int32_t accumulator,
    std::uint32_t tableBase,
    std::uint32_t vramAddress
)
{
    if (!gRbg0Coefficients)
    {
        return;
    }

    if (
        std::fprintf(
            gRbg0Coefficients,
            "%u,%u,%u,%c,%u,%d,%08x,%05x\n",
            frame,
            scanline,
            pixel,
            parameterB ? 'B' : 'A',
            perPixel ? 1u : 0u,
            accumulator,
            tableBase,
            vramAddress
        ) < 0
    )
    {
        setError("failed to append RBG0 coefficient trace");
    }
}
