#include "inputRecordingFormat.h"

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstring>

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
            std::fprintf(stderr, "%d input recording check(s) failed.\n", mFailures);
            return 1;
        }
        std::puts("All input recording checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

void testRoundTrip(TestRunner& runner)
{
    using namespace pds::input_recording;

    std::FILE* file = std::tmpfile();
    runner.expectTrue("temporary recording opened", file != nullptr);
    if (!file)
    {
        return;
    }

    Metadata metadata;
    metadata.frameCount = 1;
    metadata.flags = kKnownMetadataFlags;
    metadata.region = 4;
    metadata.controllerTypes = {2, 1};
    metadata.connectedMask = 1;
    std::snprintf(metadata.routeName.data(), metadata.routeName.size(), "%s", "name-entry");

    Frame frame;
    frame.frameIndex = 0;
    frame.inputs[0].inputType = 2;
    frame.inputs[0].analogX = -128;
    frame.inputs[0].analogY = 127;
    frame.inputs[0].analog3 = -1;
    frame.inputs[0].analog4 = 1;
    frame.inputs[0].buttons = {0, 1, 0x1234, 0xffff, 4, 5, 6, 7};
    frame.inputs[1].inputType = 1;
    frame.inputs[1].buttons = {8, 9, 10, 11, 12, 13, 14, 15};
    frame.stateHash = 0x0123456789abcdef;
    frame.audioSamplePosition = 0xfedcba9876543210;
    frame.flags = 3;

    runner.expectTrue("header write", writeHeader(file, metadata));
    runner.expectTrue("frame write", writeFrame(file, frame));
    runner.expectEqual("stable file size", std::ftell(file), static_cast<long>(kHeaderSize + kFrameSize));
    runner.expectEqual("rewind", std::fseek(file, 0, SEEK_SET), 0);

    Metadata decodedMetadata;
    Frame decodedFrame;
    runner.expectTrue("header read", readHeader(file, decodedMetadata));
    runner.expectTrue("frame read", readFrame(file, decodedFrame));
    runner.expectEqual("frame count", decodedMetadata.frameCount, metadata.frameCount);
    runner.expectEqual("region", decodedMetadata.region, metadata.region);
    runner.expectEqual("connected mask", decodedMetadata.connectedMask, metadata.connectedMask);
    runner.expectTrue(
        "route name",
        std::strcmp(decodedMetadata.routeName.data(), metadata.routeName.data()) == 0
    );
    runner.expectEqual("frame index", decodedFrame.frameIndex, frame.frameIndex);
    runner.expectEqual("negative analog value", decodedFrame.inputs[0].analogX, frame.inputs[0].analogX);
    runner.expectEqual("positive analog value", decodedFrame.inputs[0].analogY, frame.inputs[0].analogY);
    runner.expectEqual("button bits", decodedFrame.inputs[0].buttons[3], frame.inputs[0].buttons[3]);
    runner.expectEqual("state hash", decodedFrame.stateHash, frame.stateHash);
    runner.expectEqual(
        "audio sample position",
        decodedFrame.audioSamplePosition,
        frame.audioSamplePosition
    );
    runner.expectEqual("frame flags", decodedFrame.flags, frame.flags);

    std::fclose(file);
}

void testRejectsUnknownVersion(TestRunner& runner)
{
    using namespace pds::input_recording;

    std::FILE* file = std::tmpfile();
    runner.expectTrue("invalid-version file opened", file != nullptr);
    if (!file)
    {
        return;
    }

    Metadata metadata;
    runner.expectTrue("invalid-version header seed", writeHeader(file, metadata));
    runner.expectEqual("version seek", std::fseek(file, 4, SEEK_SET), 0);
    const std::array<std::uint8_t, 2> invalidVersion = {0xff, 0xff};
    runner.expectTrue(
        "invalid version write",
        std::fwrite(invalidVersion.data(), invalidVersion.size(), 1, file) == 1
    );
    runner.expectEqual("invalid-version rewind", std::fseek(file, 0, SEEK_SET), 0);

    Metadata decoded;
    runner.expectTrue("unknown version rejected", !readHeader(file, decoded));
    std::fclose(file);
}

void testControllerTransitions(TestRunner& runner)
{
    using namespace pds::input_recording;

    std::FILE* file = std::tmpfile();
    runner.expectTrue("controller-transition file opened", file != nullptr);
    if (!file)
    {
        return;
    }

    Metadata metadata;
    metadata.frameCount = 5;
    metadata.flags = kMetadataFlagFrameConnectionState;
    metadata.controllerTypes = {1, 0};
    metadata.connectedMask = 0;
    runner.expectTrue("controller-transition header write", writeHeader(file, metadata));

    std::array<Frame, 5> frames{};
    for (std::size_t index = 0; index < frames.size(); ++index)
    {
        frames[index].frameIndex = static_cast<std::uint32_t>(index);
    }

    frames[1].inputs[0].inputType = 1;
    frames[1].flags = 0x1;
    frames[1].inputs[0].buttons[0] = 0x0001;
    frames[1].inputs[0].buttons[1] = 0x0001;
    frames[1].inputs[0].buttons[3] = 0x0001;

    frames[2].inputs[0].inputType = 1;
    frames[2].flags = 0x1;
    frames[2].inputs[0].buttons[0] = 0x0001;

    frames[3].inputs[0].inputType = 2;
    frames[3].flags = 0x1;
    frames[3].inputs[0].analogX = -128;
    frames[3].inputs[0].analogY = 127;
    frames[3].inputs[0].analog3 = 0;
    frames[3].inputs[0].analog4 = -1;

    for (const Frame& frame : frames)
    {
        runner.expectTrue("controller-transition frame write", writeFrame(file, frame));
    }
    runner.expectEqual("controller-transition rewind", std::fseek(file, 0, SEEK_SET), 0);

    Metadata decodedMetadata;
    runner.expectTrue("controller-transition header read", readHeader(file, decodedMetadata));
    runner.expectEqual("initial disconnected mask", decodedMetadata.connectedMask, std::uint8_t{0});

    std::array<Frame, 5> decodedFrames{};
    for (Frame& frame : decodedFrames)
    {
        runner.expectTrue("controller-transition frame read", readFrame(file, frame));
    }

    runner.expectEqual("disconnected input type", decodedFrames[0].inputs[0].inputType, std::uint8_t{0});
    runner.expectEqual("digital reconnect type", decodedFrames[1].inputs[0].inputType, std::uint8_t{1});
    runner.expectEqual("digital reconnect flag", decodedFrames[1].flags, std::uint16_t{1});
    runner.expectEqual("one-frame press", decodedFrames[1].inputs[0].buttons[1], std::uint16_t{1});
    runner.expectEqual("one-frame press clears", decodedFrames[2].inputs[0].buttons[1], std::uint16_t{0});
    runner.expectEqual("held input remains", decodedFrames[2].inputs[0].buttons[0], std::uint16_t{1});
    runner.expectEqual("analog mode transition", decodedFrames[3].inputs[0].inputType, std::uint8_t{2});
    runner.expectEqual("analog negative extreme", decodedFrames[3].inputs[0].analogX, std::int8_t{-128});
    runner.expectEqual("analog positive extreme", decodedFrames[3].inputs[0].analogY, std::int8_t{127});
    runner.expectEqual("analog neutral", decodedFrames[3].inputs[0].analog3, std::int8_t{0});
    runner.expectEqual("analog signed byte", decodedFrames[3].inputs[0].analog4, std::int8_t{-1});
    runner.expectEqual("disconnect transition", decodedFrames[4].inputs[0].inputType, std::uint8_t{0});
    runner.expectEqual("disconnect flag", decodedFrames[4].flags, std::uint16_t{0});

    std::fclose(file);
}

void testRejectsInvalidMetadata(TestRunner& runner)
{
    using namespace pds::input_recording;

    std::FILE* file = std::tmpfile();
    runner.expectTrue("invalid-metadata file opened", file != nullptr);
    if (!file)
    {
        return;
    }

    Metadata metadata;
    metadata.flags = 1u << 31;
    runner.expectTrue("invalid-metadata header seed", writeHeader(file, metadata));
    runner.expectEqual("invalid-metadata rewind", std::fseek(file, 0, SEEK_SET), 0);

    Metadata decoded;
    runner.expectTrue("unknown metadata flag rejected", !readHeader(file, decoded));
    std::fclose(file);

    file = std::tmpfile();
    runner.expectTrue("invalid-mask file opened", file != nullptr);
    if (!file)
    {
        return;
    }

    metadata = {};
    metadata.connectedMask = 0x4;
    runner.expectTrue("invalid-mask header seed", writeHeader(file, metadata));
    runner.expectEqual("invalid-mask rewind", std::fseek(file, 0, SEEK_SET), 0);
    runner.expectTrue("invalid controller mask rejected", !readHeader(file, decoded));
    std::fclose(file);

    file = std::tmpfile();
    runner.expectTrue("invalid-frame-flags file opened", file != nullptr);
    if (!file)
    {
        return;
    }

    Frame frame;
    frame.flags = 0x4;
    runner.expectTrue("invalid-frame-flags seed", writeFrame(file, frame));
    runner.expectEqual("invalid-frame-flags rewind", std::fseek(file, 0, SEEK_SET), 0);
    Frame decodedFrame;
    runner.expectTrue("unknown frame flag rejected", !readFrame(file, decodedFrame));
    std::fclose(file);
}
}

int main()
{
    TestRunner runner;
    testRoundTrip(runner);
    testRejectsUnknownVersion(runner);
    testControllerTransitions(runner);
    testRejectsInvalidMetadata(runner);
    return runner.finish();
}
