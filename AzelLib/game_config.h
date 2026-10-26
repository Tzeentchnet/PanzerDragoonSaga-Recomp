#pragma once

#include "renderer/renderer_config.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <iosfwd>
#include <string>
#include <string_view>

constexpr int PdsGameConfigVersion = 1;

enum class BuildConfiguration
{
    Debug,
    Release,
    Shipping,
};

enum class SaturnControl : std::uint8_t
{
    None,
    A,
    B,
    C,
    Start,
    Up,
    Down,
    Left,
    Right,
    X,
    Y,
    L,
    R,
    Count,
};

enum class GamepadInput : std::uint8_t
{
    South,
    East,
    West,
    North,
    Back,
    Start,
    LeftStick,
    RightStick,
    LeftShoulder,
    RightShoulder,
    DpadUp,
    DpadDown,
    DpadLeft,
    DpadRight,
    LeftTrigger,
    RightTrigger,
    Count,
};

constexpr std::size_t saturnControl_count()
{
    return static_cast<std::size_t>(SaturnControl::Count);
}

constexpr std::size_t gamepadInput_count()
{
    return static_cast<std::size_t>(GamepadInput::Count);
}

struct DisplayConfig
{
    int windowWidth = 1280;
    int windowHeight = 960;
    bool fullscreen = false;
    bool vsync = false;
};

struct AudioPacingConfig
{
    int volumePercent = 100;
    int frameLimit = 30;
};

struct KeyboardConfig
{
    std::array<std::string, saturnControl_count()> bindings = {
        "None",
        "C",
        "X",
        "Z",
        "Return",
        "Up",
        "Down",
        "Left",
        "Right",
        "A",
        "S",
        "Q",
        "W",
    };
};

struct GamepadConfig
{
    std::array<SaturnControl, gamepadInput_count()> bindings = {
        SaturnControl::A,
        SaturnControl::B,
        SaturnControl::X,
        SaturnControl::C,
        SaturnControl::None,
        SaturnControl::Start,
        SaturnControl::None,
        SaturnControl::None,
        SaturnControl::L,
        SaturnControl::R,
        SaturnControl::Up,
        SaturnControl::Down,
        SaturnControl::Left,
        SaturnControl::Right,
        SaturnControl::L,
        SaturnControl::R,
    };
    bool leftStickDirections = true;
};

struct ExperimentalRenderingConfig
{
    bool gpuVdp1 = true;
    bool combinedDualPlane = true;
    bool smoothGouraud = true;
    bool directRayRendering = true;
};

struct GameConfig
{
    int version = PdsGameConfigVersion;
    BuildConfiguration build = BuildConfiguration::Release;
    RendererConfig renderer;
    DisplayConfig display;
    AudioPacingConfig audioPacing;
    KeyboardConfig keyboard;
    GamepadConfig gamepad;
    ExperimentalRenderingConfig experimental;
};

enum class GameConfigLoadResult
{
    Loaded,
    NotFound,
    Error,
};

const char* buildConfiguration_name(BuildConfiguration configuration);
const char* saturnControl_name(SaturnControl control);
const char* gamepadInput_name(GamepadInput input);
std::uint16_t saturnControl_mask(SaturnControl control);

bool gameConfig_parse(
    std::istream& input,
    GameConfig& config,
    std::string& error
);
bool gameConfig_write(
    std::ostream& output,
    const GameConfig& config,
    std::string& error
);
GameConfigLoadResult gameConfig_load(
    const std::filesystem::path& path,
    GameConfig& config,
    std::string& error
);
bool gameConfig_save(
    const std::filesystem::path& path,
    const GameConfig& config,
    std::string& error
);
std::filesystem::path gameConfig_defaultPath();
bool gameConfig_parsePathOption(
    int argc,
    char* argv[],
    std::filesystem::path& path,
    bool& pathWasExplicit,
    std::string& error
);
