#pragma once

#include <cstddef>
#include <cstdint>

bool regressionCapture_configure(int argc, char* argv[]);
bool regressionCapture_initialize();
void regressionCapture_shutdown();
bool regressionCapture_hasFailed();
const char* regressionCapture_getLastError();

const char* regressionCapture_getRouteName();
std::uint32_t regressionCapture_getRegion();
std::uint8_t regressionCapture_getConnectedMask();

std::uint64_t regressionCapture_computeStateHash();
std::uint64_t regressionCapture_getAudioSamplePosition();
void regressionCapture_appendAudioSample(std::int16_t left, std::int16_t right);
void regressionCapture_recordFrame(std::uint32_t frame);

bool regressionCapture_shouldCaptureImage(std::uint32_t frame);
bool regressionCapture_makeImagePath(
    std::uint32_t frame,
    const char* label,
    char* output,
    std::size_t outputSize
);
bool regressionCapture_writeRgbaTga(
    std::uint32_t frame,
    const char* label,
    const std::uint32_t* pixels,
    std::uint16_t width,
    std::uint16_t height
);

bool regressionCapture_isRbg0CoefficientTraceEnabled();
void regressionCapture_recordRbg0Coefficient(
    std::uint32_t frame,
    std::uint32_t scanline,
    std::uint32_t pixel,
    bool parameterB,
    bool perPixel,
    std::int32_t accumulator,
    std::uint32_t tableBase,
    std::uint32_t vramAddress
);
