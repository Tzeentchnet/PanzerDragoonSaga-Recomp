#pragma once

#include <cstdint>

// Call once per frame after input is finalized, before runTasks()
void updateInputRecorder();

bool inputRecorder_startRecording(
    const char* path,
    const char* routeName = nullptr,
    std::uint32_t region = 0,
    std::uint8_t connectedMask = 0x3
);
bool inputRecorder_startPlayback(const char* path);
void inputRecorder_stop();
bool inputRecorder_hasFailed();
const char* inputRecorder_getLastError();
std::uint32_t inputRecorder_getFrame();

using InputRecorderFrameHashCallback = std::uint64_t (*)();
using InputRecorderAudioPositionCallback = std::uint64_t (*)();
void inputRecorder_setFrameHashCallback(InputRecorderFrameHashCallback callback);
void inputRecorder_setAudioPositionCallback(InputRecorderAudioPositionCallback callback);

// Null when not installed
extern void (*gOnInputFinalized)();
