#include "inputRecordingFormat.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace pds::input_recording
{
namespace
{
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

void putU64(std::uint8_t* output, std::uint64_t value)
{
    putU32(output, static_cast<std::uint32_t>(value));
    putU32(output + 4, static_cast<std::uint32_t>(value >> 32));
}

std::uint16_t getU16(const std::uint8_t* input)
{
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(input[0]) |
        static_cast<std::uint16_t>(input[1]) << 8
    );
}

std::uint32_t getU32(const std::uint8_t* input)
{
    return static_cast<std::uint32_t>(getU16(input)) |
        static_cast<std::uint32_t>(getU16(input + 2)) << 16;
}

std::uint64_t getU64(const std::uint8_t* input)
{
    return static_cast<std::uint64_t>(getU32(input)) |
        static_cast<std::uint64_t>(getU32(input + 4)) << 32;
}

std::int8_t getS8(std::uint8_t input)
{
    if (input <= 0x7f)
    {
        return static_cast<std::int8_t>(input);
    }
    return static_cast<std::int8_t>(static_cast<int>(input) - 0x100);
}

void encodeInput(std::uint8_t* output, const InputState& input)
{
    output[0] = input.inputType;
    output[1] = static_cast<std::uint8_t>(input.analogX);
    output[2] = static_cast<std::uint8_t>(input.analogY);
    output[3] = static_cast<std::uint8_t>(input.analog3);
    output[4] = static_cast<std::uint8_t>(input.analog4);
    for (std::size_t index = 0; index < input.buttons.size(); ++index)
    {
        putU16(output + 5 + index * 2, input.buttons[index]);
    }
}

void decodeInput(const std::uint8_t* input, InputState& output)
{
    output.inputType = input[0];
    output.analogX = getS8(input[1]);
    output.analogY = getS8(input[2]);
    output.analog3 = getS8(input[3]);
    output.analog4 = getS8(input[4]);
    for (std::size_t index = 0; index < output.buttons.size(); ++index)
    {
        output.buttons[index] = getU16(input + 5 + index * 2);
    }
}
}

bool writeHeader(std::FILE* file, const Metadata& metadata)
{
    if (!file)
    {
        return false;
    }

    std::array<std::uint8_t, kHeaderSize> bytes{};
    putU32(bytes.data(), kMagic);
    putU16(bytes.data() + 4, kVersion);
    putU16(bytes.data() + 6, kHeaderSize);
    putU32(bytes.data() + 8, metadata.frameCount);
    putU16(bytes.data() + 12, kFrameSize);
    putU32(bytes.data() + 16, metadata.flags);
    putU32(bytes.data() + 20, metadata.region);
    bytes[24] = metadata.controllerTypes[0];
    bytes[25] = metadata.controllerTypes[1];
    bytes[26] = metadata.connectedMask;
    std::memcpy(bytes.data() + 28, metadata.routeName.data(), metadata.routeName.size());
    return std::fwrite(bytes.data(), bytes.size(), 1, file) == 1;
}

bool readHeader(std::FILE* file, Metadata& metadata)
{
    if (!file)
    {
        return false;
    }

    std::array<std::uint8_t, kHeaderSize> bytes{};
    if (std::fread(bytes.data(), bytes.size(), 1, file) != 1)
    {
        return false;
    }
    if (
        getU32(bytes.data()) != kMagic ||
        getU16(bytes.data() + 4) != kVersion ||
        getU16(bytes.data() + 6) != kHeaderSize ||
        getU16(bytes.data() + 12) != kFrameSize
    )
    {
        return false;
    }

    metadata = {};
    metadata.frameCount = getU32(bytes.data() + 8);
    metadata.flags = getU32(bytes.data() + 16);
    metadata.region = getU32(bytes.data() + 20);
    metadata.controllerTypes[0] = bytes[24];
    metadata.controllerTypes[1] = bytes[25];
    metadata.connectedMask = bytes[26];
    if ((metadata.flags & ~kKnownMetadataFlags) != 0 || metadata.connectedMask > 0x3)
    {
        return false;
    }
    std::memcpy(metadata.routeName.data(), bytes.data() + 28, metadata.routeName.size());
    metadata.routeName.back() = '\0';
    return true;
}

bool writeFrame(std::FILE* file, const Frame& frame)
{
    if (!file)
    {
        return false;
    }

    std::array<std::uint8_t, kFrameSize> bytes{};
    putU32(bytes.data(), frame.frameIndex);
    encodeInput(bytes.data() + 4, frame.inputs[0]);
    encodeInput(bytes.data() + 4 + kInputSize, frame.inputs[1]);
    putU64(bytes.data() + 46, frame.stateHash);
    putU64(bytes.data() + 54, frame.audioSamplePosition);
    putU16(bytes.data() + 62, frame.flags);
    return std::fwrite(bytes.data(), bytes.size(), 1, file) == 1;
}

bool readFrame(std::FILE* file, Frame& frame)
{
    if (!file)
    {
        return false;
    }

    std::array<std::uint8_t, kFrameSize> bytes{};
    if (std::fread(bytes.data(), bytes.size(), 1, file) != 1)
    {
        return false;
    }

    frame = {};
    frame.frameIndex = getU32(bytes.data());
    decodeInput(bytes.data() + 4, frame.inputs[0]);
    decodeInput(bytes.data() + 4 + kInputSize, frame.inputs[1]);
    frame.stateHash = getU64(bytes.data() + 46);
    frame.audioSamplePosition = getU64(bytes.data() + 54);
    frame.flags = getU16(bytes.data() + 62);
    return (frame.flags & ~kFrameConnectedMask) == 0;
}
}
