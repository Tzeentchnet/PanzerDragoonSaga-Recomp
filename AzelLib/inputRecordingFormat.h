#pragma once

#include <array>
#include <cstdint>
#include <cstdio>

namespace pds::input_recording
{
inline constexpr std::uint32_t kMagic = 0x32534450;
inline constexpr std::uint16_t kVersion = 2;
inline constexpr std::uint16_t kHeaderSize = 64;
inline constexpr std::uint16_t kInputSize = 21;
inline constexpr std::uint16_t kFrameSize = 64;
inline constexpr std::size_t kRouteNameSize = 32;
inline constexpr std::uint32_t kMetadataFlagStateHash = 1u << 0;
inline constexpr std::uint32_t kMetadataFlagAudioSamplePosition = 1u << 1;
inline constexpr std::uint32_t kMetadataFlagFrameConnectionState = 1u << 2;
inline constexpr std::uint32_t kKnownMetadataFlags =
    kMetadataFlagStateHash |
    kMetadataFlagAudioSamplePosition |
    kMetadataFlagFrameConnectionState;
inline constexpr std::uint16_t kFrameConnectedMask = 0x3;

struct InputState
{
    std::uint8_t inputType = 0;
    std::int8_t analogX = 0;
    std::int8_t analogY = 0;
    std::int8_t analog3 = 0;
    std::int8_t analog4 = 0;
    std::array<std::uint16_t, 8> buttons{};
};

struct Metadata
{
    std::uint32_t frameCount = 0;
    std::uint32_t flags = 0;
    std::uint32_t region = 0;
    std::array<std::uint8_t, 2> controllerTypes{};
    std::uint8_t connectedMask = 0;
    std::array<char, kRouteNameSize> routeName{};
};

struct Frame
{
    std::uint32_t frameIndex = 0;
    std::array<InputState, 2> inputs{};
    std::uint64_t stateHash = 0;
    std::uint64_t audioSamplePosition = 0;
    std::uint16_t flags = 0;
};

bool writeHeader(std::FILE* file, const Metadata& metadata);
bool readHeader(std::FILE* file, Metadata& metadata);
bool writeFrame(std::FILE* file, const Frame& frame);
bool readFrame(std::FILE* file, Frame& frame);
}
