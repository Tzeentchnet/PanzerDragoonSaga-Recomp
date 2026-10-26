#include "game_config.h"

#include <algorithm>
#include <charconv>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <system_error>
#include <unordered_set>
#include <vector>

#if defined(_WIN32)
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
constexpr std::string_view SupportedKeyboardKeys[] = {
    "None",
    "A", "B", "C", "D", "E", "F", "G", "H", "I", "J", "K", "L", "M",
    "N", "O", "P", "Q", "R", "S", "T", "U", "V", "W", "X", "Y", "Z",
    "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
    "Return", "Escape", "Backspace", "Tab", "Space",
    "-", "=", "[", "]", "\\", "#", ";", "'", "`", ",", ".", "/",
    "CapsLock",
    "F1", "F2", "F3", "F4", "F5", "F6",
    "F7", "F8", "F9", "F10", "F11", "F12",
    "PrintScreen", "ScrollLock", "Pause",
    "Insert", "Home", "PageUp", "Delete", "End", "PageDown",
    "Right", "Left", "Down", "Up",
    "Numlock",
    "Keypad /", "Keypad *", "Keypad -", "Keypad +", "Keypad Enter",
    "Keypad 1", "Keypad 2", "Keypad 3", "Keypad 4", "Keypad 5",
    "Keypad 6", "Keypad 7", "Keypad 8", "Keypad 9", "Keypad 0", "Keypad .",
    "Application",
    "Left Ctrl", "Left Shift", "Left Alt", "Left GUI",
    "Right Ctrl", "Right Shift", "Right Alt", "Right GUI",
};

std::string_view trim(std::string_view value)
{
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.front())))
    {
        value.remove_prefix(1);
    }
    while (!value.empty() && std::isspace(static_cast<unsigned char>(value.back())))
    {
        value.remove_suffix(1);
    }
    return value;
}

std::string lowercase(std::string_view value)
{
    std::string result(value);
    std::transform(
        result.begin(),
        result.end(),
        result.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        }
    );
    return result;
}

bool parseInteger(std::string_view value, int& result)
{
    value = trim(value);
    if (value.empty())
    {
        return false;
    }

    const char* begin = value.data();
    const char* end = begin + value.size();
    const std::from_chars_result parsed = std::from_chars(begin, end, result);
    return parsed.ec == std::errc() && parsed.ptr == end;
}

bool parseBoolean(std::string_view value, bool& result)
{
    const std::string normalized = lowercase(trim(value));
    if (normalized == "true")
    {
        result = true;
        return true;
    }
    if (normalized == "false")
    {
        result = false;
        return true;
    }
    return false;
}

bool parseBuildConfiguration(
    std::string_view value,
    BuildConfiguration& configuration
)
{
    const std::string normalized = lowercase(trim(value));
    if (normalized == "debug")
    {
        configuration = BuildConfiguration::Debug;
        return true;
    }
    if (normalized == "release")
    {
        configuration = BuildConfiguration::Release;
        return true;
    }
    if (normalized == "shipping")
    {
        configuration = BuildConfiguration::Shipping;
        return true;
    }
    return false;
}

bool parseSaturnControl(std::string_view value, SaturnControl& control)
{
    const std::string normalized = lowercase(trim(value));
    for (std::size_t i = 0; i < saturnControl_count(); ++i)
    {
        const SaturnControl candidate = static_cast<SaturnControl>(i);
        if (normalized == lowercase(saturnControl_name(candidate)))
        {
            control = candidate;
            return true;
        }
    }
    return false;
}

bool parseKeyboardKey(std::string_view value, std::string& canonicalName)
{
    const std::string normalized = lowercase(trim(value));
    for (const std::string_view candidate : SupportedKeyboardKeys)
    {
        if (normalized == lowercase(candidate))
        {
            canonicalName = candidate;
            return true;
        }
    }
    return false;
}

bool parseSaturnControlKey(std::string_view key, SaturnControl& control)
{
    for (std::size_t i = 1; i < saturnControl_count(); ++i)
    {
        const SaturnControl candidate = static_cast<SaturnControl>(i);
        if (key == saturnControl_name(candidate))
        {
            control = candidate;
            return true;
        }
    }
    return false;
}

bool parseGamepadInputKey(std::string_view key, GamepadInput& input)
{
    for (std::size_t i = 0; i < gamepadInput_count(); ++i)
    {
        const GamepadInput candidate = static_cast<GamepadInput>(i);
        if (key == gamepadInput_name(candidate))
        {
            input = candidate;
            return true;
        }
    }
    return false;
}

std::string lineError(
    std::size_t lineNumber,
    std::string_view message
)
{
    std::ostringstream stream;
    stream << "configuration line " << lineNumber << ": " << message;
    return stream.str();
}

bool applySetting(
    const std::string& section,
    const std::string& key,
    std::string_view value,
    GameConfig& config,
    bool& versionSeen,
    std::string& error
)
{
    if (section == "config" && key == "version")
    {
        if (!parseInteger(value, config.version))
        {
            error = "version must be an integer";
            return false;
        }
        versionSeen = true;
        return true;
    }

    if (section == "launcher" && key == "build")
    {
        if (!parseBuildConfiguration(value, config.build))
        {
            error = "build must be Debug, Release, or Shipping";
            return false;
        }
        return true;
    }

    if (section == "graphics" && key == "renderer")
    {
        const std::string normalized = lowercase(trim(value));
        if (!rendererBackend_parse(normalized, config.renderer.backend))
        {
            error = "renderer must be auto, d3d11, d3d12, vulkan, or software";
            return false;
        }
        return true;
    }
    if (section == "graphics" && key == "window_width")
    {
        if (
            !parseInteger(value, config.display.windowWidth) ||
            config.display.windowWidth < 640 ||
            config.display.windowWidth > 7680
        )
        {
            error = "window_width must be between 640 and 7680";
            return false;
        }
        return true;
    }
    if (section == "graphics" && key == "window_height")
    {
        if (
            !parseInteger(value, config.display.windowHeight) ||
            config.display.windowHeight < 480 ||
            config.display.windowHeight > 4320
        )
        {
            error = "window_height must be between 480 and 4320";
            return false;
        }
        return true;
    }
    if (section == "graphics" && key == "fullscreen")
    {
        if (!parseBoolean(value, config.display.fullscreen))
        {
            error = "fullscreen must be true or false";
            return false;
        }
        return true;
    }
    if (section == "graphics" && key == "vsync")
    {
        if (!parseBoolean(value, config.display.vsync))
        {
            error = "vsync must be true or false";
            return false;
        }
        return true;
    }

    if (section == "audio_pacing" && key == "volume_percent")
    {
        if (
            !parseInteger(value, config.audioPacing.volumePercent) ||
            config.audioPacing.volumePercent < 0 ||
            config.audioPacing.volumePercent > 100
        )
        {
            error = "volume_percent must be between 0 and 100";
            return false;
        }
        return true;
    }
    if (section == "audio_pacing" && key == "frame_limit")
    {
        if (
            !parseInteger(value, config.audioPacing.frameLimit) ||
            (
                config.audioPacing.frameLimit != 0 &&
                (
                    config.audioPacing.frameLimit < 1 ||
                    config.audioPacing.frameLimit > 240
                )
            )
        )
        {
            error = "frame_limit must be 0 (unlimited) or between 1 and 240";
            return false;
        }
        return true;
    }

    if (section == "input.keyboard")
    {
        SaturnControl control;
        if (!parseSaturnControlKey(key, control))
        {
            error = "unknown keyboard binding: " + key;
            return false;
        }

        std::string canonicalName;
        if (!parseKeyboardKey(value, canonicalName))
        {
            error = "unsupported keyboard key for " + key;
            return false;
        }
        config.keyboard.bindings[static_cast<std::size_t>(control)] =
            std::move(canonicalName);
        return true;
    }

    if (section == "input.gamepad" && key == "left_stick_directions")
    {
        if (!parseBoolean(value, config.gamepad.leftStickDirections))
        {
            error = "left_stick_directions must be true or false";
            return false;
        }
        return true;
    }
    if (section == "input.gamepad")
    {
        GamepadInput input;
        if (!parseGamepadInputKey(key, input))
        {
            error = "unknown gamepad input: " + key;
            return false;
        }

        SaturnControl control;
        if (!parseSaturnControl(value, control))
        {
            error = "unknown Saturn control for " + key;
            return false;
        }
        config.gamepad.bindings[static_cast<std::size_t>(input)] = control;
        return true;
    }

    if (section == "experimental" && key == "gpu_vdp1")
    {
        if (!parseBoolean(value, config.experimental.gpuVdp1))
        {
            error = "gpu_vdp1 must be true or false";
            return false;
        }
        return true;
    }
    if (section == "experimental" && key == "combined_dual_plane")
    {
        if (!parseBoolean(value, config.experimental.combinedDualPlane))
        {
            error = "combined_dual_plane must be true or false";
            return false;
        }
        return true;
    }
    if (section == "experimental" && key == "smooth_gouraud")
    {
        if (!parseBoolean(value, config.experimental.smoothGouraud))
        {
            error = "smooth_gouraud must be true or false";
            return false;
        }
        return true;
    }
    if (section == "experimental" && key == "direct_ray_rendering")
    {
        if (!parseBoolean(value, config.experimental.directRayRendering))
        {
            error = "direct_ray_rendering must be true or false";
            return false;
        }
        return true;
    }

    error = "unknown setting [" + section + "] " + key;
    return false;
}

bool validateKeyboardBindings(const GameConfig& config, std::string& error)
{
    std::unordered_set<std::string> usedKeys;
    for (std::size_t i = 1; i < saturnControl_count(); ++i)
    {
        const std::string& key = config.keyboard.bindings[i];
        if (lowercase(key) == "none")
        {
            continue;
        }
        if (!usedKeys.insert(lowercase(key)).second)
        {
            error = "keyboard key '" + key + "' is assigned more than once";
            return false;
        }
    }
    return true;
}

const char* booleanName(bool value)
{
    return value ? "true" : "false";
}
}

const char* buildConfiguration_name(BuildConfiguration configuration)
{
    switch (configuration)
    {
    case BuildConfiguration::Debug:
        return "Debug";
    case BuildConfiguration::Release:
        return "Release";
    case BuildConfiguration::Shipping:
        return "Shipping";
    }
    return "Unknown";
}

const char* saturnControl_name(SaturnControl control)
{
    switch (control)
    {
    case SaturnControl::None:
        return "none";
    case SaturnControl::A:
        return "a";
    case SaturnControl::B:
        return "b";
    case SaturnControl::C:
        return "c";
    case SaturnControl::Start:
        return "start";
    case SaturnControl::Up:
        return "up";
    case SaturnControl::Down:
        return "down";
    case SaturnControl::Left:
        return "left";
    case SaturnControl::Right:
        return "right";
    case SaturnControl::X:
        return "x";
    case SaturnControl::Y:
        return "y";
    case SaturnControl::L:
        return "l";
    case SaturnControl::R:
        return "r";
    case SaturnControl::Count:
        break;
    }
    return "unknown";
}

const char* gamepadInput_name(GamepadInput input)
{
    switch (input)
    {
    case GamepadInput::South:
        return "south";
    case GamepadInput::East:
        return "east";
    case GamepadInput::West:
        return "west";
    case GamepadInput::North:
        return "north";
    case GamepadInput::Back:
        return "back";
    case GamepadInput::Start:
        return "start";
    case GamepadInput::LeftStick:
        return "left_stick";
    case GamepadInput::RightStick:
        return "right_stick";
    case GamepadInput::LeftShoulder:
        return "left_shoulder";
    case GamepadInput::RightShoulder:
        return "right_shoulder";
    case GamepadInput::DpadUp:
        return "dpad_up";
    case GamepadInput::DpadDown:
        return "dpad_down";
    case GamepadInput::DpadLeft:
        return "dpad_left";
    case GamepadInput::DpadRight:
        return "dpad_right";
    case GamepadInput::LeftTrigger:
        return "left_trigger";
    case GamepadInput::RightTrigger:
        return "right_trigger";
    case GamepadInput::Count:
        break;
    }
    return "unknown";
}

std::uint16_t saturnControl_mask(SaturnControl control)
{
    switch (control)
    {
    case SaturnControl::None:
        return 0;
    case SaturnControl::A:
        return 0x0001;
    case SaturnControl::B:
        return 0x0002;
    case SaturnControl::C:
        return 0x0004;
    case SaturnControl::Start:
        return 0x0008;
    case SaturnControl::Up:
        return 0x0010;
    case SaturnControl::Down:
        return 0x0020;
    case SaturnControl::Left:
        return 0x0040;
    case SaturnControl::Right:
        return 0x0080;
    case SaturnControl::X:
        return 0x0800;
    case SaturnControl::R:
        return 0x1000;
    case SaturnControl::Y:
        return 0x4000;
    case SaturnControl::L:
        return 0x8000;
    case SaturnControl::Count:
        break;
    }
    return 0;
}

bool gameConfig_parse(
    std::istream& input,
    GameConfig& config,
    std::string& error
)
{
    GameConfig parsedConfig;
    std::string section;
    std::unordered_set<std::string> seenSettings;
    bool versionSeen = false;
    std::string line;
    std::size_t lineNumber = 0;

    while (std::getline(input, line))
    {
        ++lineNumber;
        std::string_view current = trim(line);
        if (
            current.empty() ||
            current.front() == ';' ||
            current.front() == '#'
        )
        {
            continue;
        }

        if (current.front() == '[')
        {
            if (current.size() < 3 || current.back() != ']')
            {
                error = lineError(lineNumber, "invalid section header");
                return false;
            }
            section = lowercase(trim(current.substr(1, current.size() - 2)));
            if (section.empty())
            {
                error = lineError(lineNumber, "section name cannot be empty");
                return false;
            }
            continue;
        }

        if (section.empty())
        {
            error = lineError(lineNumber, "setting appears before a section");
            return false;
        }

        const std::size_t separator = current.find('=');
        if (separator == std::string_view::npos)
        {
            error = lineError(lineNumber, "expected key=value");
            return false;
        }

        const std::string key = lowercase(trim(current.substr(0, separator)));
        const std::string_view value = trim(current.substr(separator + 1));
        if (key.empty() || value.empty())
        {
            error = lineError(lineNumber, "key and value cannot be empty");
            return false;
        }

        const std::string identity = section + '\n' + key;
        if (!seenSettings.insert(identity).second)
        {
            error = lineError(lineNumber, "duplicate setting [" + section + "] " + key);
            return false;
        }

        std::string settingError;
        if (
            !applySetting(
                section,
                key,
                value,
                parsedConfig,
                versionSeen,
                settingError
            )
        )
        {
            error = lineError(lineNumber, settingError);
            return false;
        }
    }

    if (input.bad())
    {
        error = "failed while reading configuration";
        return false;
    }
    if (!versionSeen)
    {
        error = "configuration is missing [config] version";
        return false;
    }
    if (parsedConfig.version != PdsGameConfigVersion)
    {
        error =
            "unsupported configuration version " +
            std::to_string(parsedConfig.version) +
            "; expected " +
            std::to_string(PdsGameConfigVersion);
        return false;
    }
    if (!validateKeyboardBindings(parsedConfig, error))
    {
        return false;
    }

    config = std::move(parsedConfig);
    return true;
}

bool gameConfig_write(
    std::ostream& output,
    const GameConfig& config,
    std::string& error
)
{
    output
        << "[config]\n"
        << "version=" << PdsGameConfigVersion << "\n\n"
        << "[launcher]\n"
        << "build=" << buildConfiguration_name(config.build) << "\n\n"
        << "[graphics]\n"
        << "renderer=" << rendererBackend_name(config.renderer.backend) << '\n'
        << "window_width=" << config.display.windowWidth << '\n'
        << "window_height=" << config.display.windowHeight << '\n'
        << "fullscreen=" << booleanName(config.display.fullscreen) << '\n'
        << "vsync=" << booleanName(config.display.vsync) << "\n\n"
        << "[audio_pacing]\n"
        << "volume_percent=" << config.audioPacing.volumePercent << '\n'
        << "frame_limit=" << config.audioPacing.frameLimit << "\n\n"
        << "[input.keyboard]\n";

    for (std::size_t i = 1; i < saturnControl_count(); ++i)
    {
        const SaturnControl control = static_cast<SaturnControl>(i);
        output
            << saturnControl_name(control)
            << '='
            << config.keyboard.bindings[i]
            << '\n';
    }

    output << "\n[input.gamepad]\n";
    for (std::size_t i = 0; i < gamepadInput_count(); ++i)
    {
        const GamepadInput input = static_cast<GamepadInput>(i);
        output
            << gamepadInput_name(input)
            << '='
            << saturnControl_name(config.gamepad.bindings[i])
            << '\n';
    }
    output
        << "left_stick_directions="
        << booleanName(config.gamepad.leftStickDirections)
        << "\n\n"
        << "[experimental]\n"
        << "gpu_vdp1=" << booleanName(config.experimental.gpuVdp1) << '\n'
        << "combined_dual_plane="
        << booleanName(config.experimental.combinedDualPlane)
        << '\n'
        << "smooth_gouraud="
        << booleanName(config.experimental.smoothGouraud)
        << '\n'
        << "direct_ray_rendering="
        << booleanName(config.experimental.directRayRendering)
        << '\n';

    if (!output.good())
    {
        error = "failed while writing configuration";
        return false;
    }
    return true;
}

GameConfigLoadResult gameConfig_load(
    const std::filesystem::path& path,
    GameConfig& config,
    std::string& error
)
{
    std::error_code filesystemError;
    const bool exists = std::filesystem::exists(path, filesystemError);
    if (filesystemError)
    {
        error =
            "could not inspect configuration file '" +
            path.string() +
            "': " +
            filesystemError.message();
        return GameConfigLoadResult::Error;
    }
    if (!exists)
    {
        return GameConfigLoadResult::NotFound;
    }

    std::ifstream input(path);
    if (!input)
    {
        error = "could not open configuration file '" + path.string() + "'";
        return GameConfigLoadResult::Error;
    }
    if (!gameConfig_parse(input, config, error))
    {
        error = path.string() + ": " + error;
        return GameConfigLoadResult::Error;
    }
    return GameConfigLoadResult::Loaded;
}

bool gameConfig_save(
    const std::filesystem::path& path,
    const GameConfig& config,
    std::string& error
)
{
    const std::filesystem::path parent = path.parent_path();
    if (!parent.empty())
    {
        std::error_code filesystemError;
        std::filesystem::create_directories(parent, filesystemError);
        if (filesystemError)
        {
            error =
                "could not create configuration directory '" +
                parent.string() +
                "': " +
                filesystemError.message();
            return false;
        }
    }

    std::ofstream output(path, std::ios::trunc);
    if (!output)
    {
        error = "could not create configuration file '" + path.string() + "'";
        return false;
    }
    if (!gameConfig_write(output, config, error))
    {
        error = path.string() + ": " + error;
        return false;
    }
    return true;
}

std::filesystem::path gameConfig_defaultPath()
{
#if defined(_WIN32)
    const DWORD required =
        GetEnvironmentVariableW(L"LOCALAPPDATA", nullptr, 0);
    if (required > 1)
    {
        std::vector<wchar_t> buffer(required);
        const DWORD written = GetEnvironmentVariableW(
            L"LOCALAPPDATA",
            buffer.data(),
            static_cast<DWORD>(buffer.size())
        );
        if (written > 0 && written < buffer.size())
        {
            return
                std::filesystem::path(buffer.data()) /
                L"PanzerDragoonSaga-Recomp" /
                L"settings.ini";
        }
    }
#else
    if (const char* configHome = std::getenv("XDG_CONFIG_HOME"))
    {
        if (*configHome != '\0')
        {
            return
                std::filesystem::path(configHome) /
                "panzer-dragoon-saga-recomp" /
                "settings.ini";
        }
    }
    if (const char* home = std::getenv("HOME"))
    {
        if (*home != '\0')
        {
            return
                std::filesystem::path(home) /
                ".config" /
                "panzer-dragoon-saga-recomp" /
                "settings.ini";
        }
    }
#endif

    std::error_code filesystemError;
    const std::filesystem::path current =
        std::filesystem::current_path(filesystemError);
    if (filesystemError)
    {
        return "pds-settings.ini";
    }
    return current / "pds-settings.ini";
}

bool gameConfig_parsePathOption(
    int argc,
    char* argv[],
    std::filesystem::path& path,
    bool& pathWasExplicit,
    std::string& error
)
{
    bool pathSeen = false;
    pathWasExplicit = false;

    for (int i = 1; i < argc; ++i)
    {
        if (std::string_view(argv[i]) != "--config")
        {
            continue;
        }
        if (pathSeen)
        {
            error = "--config may only be specified once";
            return false;
        }
        if ((i + 1) >= argc)
        {
            error = "--config requires a file path";
            return false;
        }

        pathSeen = true;
        pathWasExplicit = true;
        path = argv[++i];
        if (path.empty())
        {
            error = "--config requires a non-empty file path";
            return false;
        }
    }

    return true;
}
