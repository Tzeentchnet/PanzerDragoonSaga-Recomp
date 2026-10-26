#include "PDS.h"
#include "inputRecorder.h"
#include "inputRecordingFormat.h"

#include <array>
#include <cstdio>
#include <cstring>

void (*gOnInputFinalized)() = nullptr;
extern bool bContinue;

enum class eMode { Off, Recording, Playback };

static eMode gMode = eMode::Off;
static FILE* gFile = nullptr;
static u32 gFrame = 0;
static u32 gExpectedFrames = 0;
static bool gLegacyPlayback = false;
static bool gFailed = false;
static char gPath[256] = "input_recording.bin";
static char gLastError[256] = {};
static pds::input_recording::Metadata gMetadata;
static InputRecorderFrameHashCallback gFrameHashCallback = nullptr;
static InputRecorderAudioPositionCallback gAudioPositionCallback = nullptr;

static constexpr u32 kLegacyMagic = 0x52534450;

using sInput = s_graphicEngineStatus_4514_inputStatus;

static void setError(const char* message)
{
    std::snprintf(gLastError, sizeof(gLastError), "%s: %s", gPath, message);
    std::fprintf(stderr, "Input recorder: %s\n", gLastError);
    gFailed = true;
    bContinue = false;
}

static void closeFile()
{
    if (gFile)
    {
        fclose(gFile);
        gFile = nullptr;
    }
}

static pds::input_recording::InputState encodeInput(const sInput& input)
{
    pds::input_recording::InputState encoded;
    encoded.inputType = input.m0_inputType;
    encoded.analogX = input.m2_analogX;
    encoded.analogY = input.m3_analogY;
    encoded.analog3 = input.m4;
    encoded.analog4 = input.m5;
    encoded.buttons = {
        input.m6_buttonDown,
        input.m8_newButtonDown,
        input.mA,
        input.mC_newButtonDown2,
        input.mE,
        input.m10,
        input.m12,
        input.m14
    };
    return encoded;
}

static void decodeInput(const pds::input_recording::InputState& encoded, sInput& input)
{
    input = {};
    input.m0_inputType = encoded.inputType;
    input.m2_analogX = encoded.analogX;
    input.m3_analogY = encoded.analogY;
    input.m4 = encoded.analog3;
    input.m5 = encoded.analog4;
    input.m6_buttonDown = encoded.buttons[0];
    input.m8_newButtonDown = encoded.buttons[1];
    input.mA = encoded.buttons[2];
    input.mC_newButtonDown2 = encoded.buttons[3];
    input.mE = encoded.buttons[4];
    input.m10 = encoded.buttons[5];
    input.m12 = encoded.buttons[6];
    input.m14 = encoded.buttons[7];
}

void inputRecorder_stop()
{
    if (gMode == eMode::Recording && gFile)
    {
        gMetadata.frameCount = gFrame;
        if (
            std::fseek(gFile, 0, SEEK_SET) != 0 ||
            !pds::input_recording::writeHeader(gFile, gMetadata) ||
            std::fflush(gFile) != 0
        )
        {
            setError("failed to finalize recording header");
        }
    }
    closeFile();
    gMode = eMode::Off;
}

bool inputRecorder_startRecording(
    const char* path,
    const char* routeName,
    std::uint32_t region,
    std::uint8_t connectedMask
)
{
    if (path && path[0])
    {
        std::snprintf(gPath, sizeof(gPath), "%s", path);
    }
    inputRecorder_stop();
    gFailed = false;
    gLastError[0] = '\0';
    gFile = std::fopen(gPath, "wb");
    if (!gFile)
    {
        setError("failed to open recording for writing");
        return false;
    }

    gMetadata = {};
    if (gFrameHashCallback)
    {
        gMetadata.flags |= pds::input_recording::kMetadataFlagStateHash;
    }
    if (gAudioPositionCallback)
    {
        gMetadata.flags |= pds::input_recording::kMetadataFlagAudioSamplePosition;
    }
    gMetadata.flags |= pds::input_recording::kMetadataFlagFrameConnectionState;
    gMetadata.region = region;
    gMetadata.connectedMask = connectedMask;
    gMetadata.controllerTypes = {
        graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m0_inputType,
        graphicEngineStatus.m4514.m0_inputDevices[1].m0_current.m0_inputType
    };
    if (routeName)
    {
        std::snprintf(gMetadata.routeName.data(), gMetadata.routeName.size(), "%s", routeName);
    }
    if (!pds::input_recording::writeHeader(gFile, gMetadata))
    {
        setError("failed to write recording header");
        closeFile();
        return false;
    }

    gFrame = 0;
    gExpectedFrames = 0;
    gLegacyPlayback = false;
    gMode = eMode::Recording;
    return true;
}

bool inputRecorder_startPlayback(const char* path)
{
    if (path && path[0])
    {
        std::snprintf(gPath, sizeof(gPath), "%s", path);
    }
    inputRecorder_stop();
    gFailed = false;
    gLastError[0] = '\0';
    gFile = std::fopen(gPath, "rb");
    if (!gFile)
    {
        setError("failed to open recording for playback");
        return false;
    }

    u32 magic = 0;
    if (std::fread(&magic, sizeof(magic), 1, gFile) != 1)
    {
        setError("failed to read recording magic");
        closeFile();
        return false;
    }

    if (magic == pds::input_recording::kMagic)
    {
        if (
            std::fseek(gFile, 0, SEEK_SET) != 0 ||
            !pds::input_recording::readHeader(gFile, gMetadata)
        )
        {
            setError("invalid versioned recording header");
            closeFile();
            return false;
        }
        gExpectedFrames = gMetadata.frameCount;
        gLegacyPlayback = false;
    }
    else if (magic == kLegacyMagic)
    {
        if (std::fread(&gExpectedFrames, sizeof(gExpectedFrames), 1, gFile) != 1)
        {
            setError("invalid legacy recording header");
            closeFile();
            return false;
        }
        gMetadata = {};
        gMetadata.frameCount = gExpectedFrames;
        gLegacyPlayback = true;
    }
    else
    {
        setError("unrecognized recording format");
        closeFile();
        return false;
    }

    gFrame = 0;
    gMode = eMode::Playback;
    return true;
}

void inputRecorder_setFrameHashCallback(InputRecorderFrameHashCallback callback)
{
    gFrameHashCallback = callback;
}

void inputRecorder_setAudioPositionCallback(InputRecorderAudioPositionCallback callback)
{
    gAudioPositionCallback = callback;
}

bool inputRecorder_hasFailed()
{
    return gFailed;
}

const char* inputRecorder_getLastError()
{
    return gLastError;
}

std::uint32_t inputRecorder_getFrame()
{
    return gFrame;
}

void updateInputRecorder()
{
    if (gMode != eMode::Off && gFile)
    {
        sInput& dev0 = graphicEngineStatus.m4514.m0_inputDevices[0].m0_current;
        sInput& dev1 = graphicEngineStatus.m4514.m0_inputDevices[1].m0_current;

        if (gMode == eMode::Recording)
        {
            pds::input_recording::Frame frame;
            frame.frameIndex = gFrame;
            frame.inputs[0] = encodeInput(dev0);
            frame.inputs[1] = encodeInput(dev1);
            frame.stateHash = gFrameHashCallback ? gFrameHashCallback() : 0;
            frame.audioSamplePosition = gAudioPositionCallback ? gAudioPositionCallback() : 0;
            if (dev0.m0_inputType != 0)
            {
                frame.flags |= 0x1;
            }
            if (dev1.m0_inputType != 0)
            {
                frame.flags |= 0x2;
            }
            if (
                !pds::input_recording::writeFrame(gFile, frame) ||
                std::fflush(gFile) != 0
            )
            {
                setError("failed to write frame");
                inputRecorder_stop();
            }
            else
            {
                gFrame++;
            }
        }
        else if (!gLegacyPlayback && gFrame >= gExpectedFrames)
        {
            inputRecorder_stop();
        }
        else if (gLegacyPlayback && gExpectedFrames != 0 && gFrame >= gExpectedFrames)
        {
            inputRecorder_stop();
        }
        else if (gLegacyPlayback)
        {
            if (
                std::fread(&dev0, sizeof(sInput), 1, gFile) != 1 ||
                std::fread(&dev1, sizeof(sInput), 1, gFile) != 1
            )
            {
                if (std::feof(gFile) == 0)
                {
                    setError("failed to read legacy frame");
                }
                inputRecorder_stop();
            }
            else
            {
                gFrame++;
            }
        }
        else
        {
            pds::input_recording::Frame frame;
            if (!pds::input_recording::readFrame(gFile, frame))
            {
                setError("failed to read versioned frame");
                inputRecorder_stop();
            }
            else if (frame.frameIndex != gFrame)
            {
                setError("recording frame index mismatch");
                inputRecorder_stop();
            }
            else
            {
                if (
                    (gMetadata.flags & pds::input_recording::kMetadataFlagStateHash) != 0 &&
                    !gFrameHashCallback
                )
                {
                    setError("state hash verifier is unavailable");
                    inputRecorder_stop();
                }
                else if (
                    (gMetadata.flags & pds::input_recording::kMetadataFlagStateHash) != 0 &&
                    frame.stateHash != gFrameHashCallback()
                )
                {
                    setError("deterministic state hash mismatch");
                    inputRecorder_stop();
                }
                else if (
                    (gMetadata.flags & pds::input_recording::kMetadataFlagAudioSamplePosition) != 0 &&
                    !gAudioPositionCallback
                )
                {
                    setError("audio position verifier is unavailable");
                    inputRecorder_stop();
                }
                else if (
                    (gMetadata.flags & pds::input_recording::kMetadataFlagAudioSamplePosition) != 0 &&
                    frame.audioSamplePosition != gAudioPositionCallback()
                )
                {
                    setError("deterministic audio position mismatch");
                    inputRecorder_stop();
                }
                else
                {
                    if (
                        (gMetadata.flags &
                            pds::input_recording::kMetadataFlagFrameConnectionState) != 0
                    )
                    {
                        if ((frame.flags & 0x1) != 0)
                        {
                            decodeInput(frame.inputs[0], dev0);
                        }
                        else
                        {
                            dev0 = {};
                        }

                        if ((frame.flags & 0x2) != 0)
                        {
                            decodeInput(frame.inputs[1], dev1);
                        }
                        else
                        {
                            dev1 = {};
                        }
                    }
                    else
                    {
                        decodeInput(frame.inputs[0], dev0);
                        decodeInput(frame.inputs[1], dev1);
                    }
                    gFrame++;
                }
            }
        }
    }

    if (gOnInputFinalized)
        gOnInputFinalized();
}
