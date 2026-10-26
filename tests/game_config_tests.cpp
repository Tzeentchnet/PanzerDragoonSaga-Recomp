#include "game_config.h"

#include <cstdio>
#include <sstream>
#include <string>

namespace
{
class TestRunner
{
public:
    void expectTrue(const char* description, bool value)
    {
        if (!value)
        {
            std::fprintf(stderr, "FAIL: %s\n", description);
            ++mFailures;
        }
    }

    template<typename Actual, typename Expected>
    void expectEqual(
        const char* description,
        const Actual& actual,
        const Expected& expected
    )
    {
        if (actual != expected)
        {
            std::fprintf(stderr, "FAIL: %s\n", description);
            ++mFailures;
        }
    }

    int finish() const
    {
        if (mFailures != 0)
        {
            std::fprintf(
                stderr,
                "%d game configuration check(s) failed.\n",
                mFailures
            );
            return 1;
        }
        std::puts("All game configuration checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

bool parseText(
    const char* text,
    GameConfig& config,
    std::string& error
)
{
    std::istringstream input(text);
    return gameConfig_parse(input, config, error);
}

void testDefaults(TestRunner& runner)
{
    GameConfig config;

    runner.expectEqual(
        "default build is Release",
        config.build,
        BuildConfiguration::Release
    );
    runner.expectEqual(
        "default renderer is automatic",
        config.renderer.backend,
        RendererBackend::Auto
    );
    runner.expectEqual("default width", config.display.windowWidth, 1280);
    runner.expectEqual("default height", config.display.windowHeight, 960);
    runner.expectTrue("default is windowed", !config.display.fullscreen);
    runner.expectTrue("default VSync is disabled", !config.display.vsync);
    runner.expectEqual(
        "default volume",
        config.audioPacing.volumePercent,
        100
    );
    runner.expectEqual("default frame limit", config.audioPacing.frameLimit, 30);
    runner.expectEqual(
        "default keyboard A",
        config.keyboard.bindings[static_cast<std::size_t>(SaturnControl::A)],
        std::string("C")
    );
    runner.expectEqual(
        "default keyboard Start",
        config.keyboard.bindings[
            static_cast<std::size_t>(SaturnControl::Start)
        ],
        std::string("Return")
    );
    runner.expectEqual(
        "default gamepad south",
        config.gamepad.bindings[
            static_cast<std::size_t>(GamepadInput::South)
        ],
        SaturnControl::A
    );
    runner.expectEqual(
        "default gamepad left trigger",
        config.gamepad.bindings[
            static_cast<std::size_t>(GamepadInput::LeftTrigger)
        ],
        SaturnControl::L
    );
}

void testFullConfiguration(TestRunner& runner)
{
    constexpr const char* text = R"ini(
; User settings
[config]
version=1

[launcher]
build=Shipping

[graphics]
renderer=vulkan
window_width=1920
window_height=1080
fullscreen=true
vsync=true

[audio_pacing]
volume_percent=65
frame_limit=0

[input.keyboard]
a=J
b=K
c=L
start=Space
up=W
down=S
left=A
right=D
x=U
y=I
l=Q
r=E

[input.gamepad]
south=b
east=a
west=y
north=x
back=none
start=start
left_stick=l
right_stick=r
left_shoulder=l
right_shoulder=r
dpad_up=up
dpad_down=down
dpad_left=left
dpad_right=right
left_trigger=y
right_trigger=c
left_stick_directions=false

[experimental]
gpu_vdp1=false
combined_dual_plane=false
smooth_gouraud=false
direct_ray_rendering=false
)ini";

    GameConfig config;
    std::string error;
    runner.expectTrue(
        "full configuration parses",
        parseText(text, config, error)
    );
    runner.expectEqual(
        "configured build",
        config.build,
        BuildConfiguration::Shipping
    );
    runner.expectEqual(
        "configured renderer",
        config.renderer.backend,
        RendererBackend::Vulkan
    );
    runner.expectEqual("configured width", config.display.windowWidth, 1920);
    runner.expectEqual("configured height", config.display.windowHeight, 1080);
    runner.expectTrue("configured fullscreen", config.display.fullscreen);
    runner.expectTrue("configured VSync", config.display.vsync);
    runner.expectEqual(
        "configured volume",
        config.audioPacing.volumePercent,
        65
    );
    runner.expectEqual(
        "configured unlimited frame rate",
        config.audioPacing.frameLimit,
        0
    );
    runner.expectEqual(
        "configured keyboard A",
        config.keyboard.bindings[static_cast<std::size_t>(SaturnControl::A)],
        std::string("J")
    );
    runner.expectEqual(
        "configured south button",
        config.gamepad.bindings[
            static_cast<std::size_t>(GamepadInput::South)
        ],
        SaturnControl::B
    );
    runner.expectTrue(
        "configured left stick directions",
        !config.gamepad.leftStickDirections
    );
    runner.expectTrue(
        "configured experimental settings",
        !config.experimental.gpuVdp1 &&
        !config.experimental.combinedDualPlane &&
        !config.experimental.smoothGouraud &&
        !config.experimental.directRayRendering
    );
}

void expectRejected(
    TestRunner& runner,
    const char* description,
    const char* text
)
{
    GameConfig config;
    std::string error;
    runner.expectTrue(
        description,
        !parseText(text, config, error) && !error.empty()
    );
}

void testErrors(TestRunner& runner)
{
    expectRejected(
        runner,
        "missing version is rejected",
        "[graphics]\nrenderer=auto\n"
    );
    expectRejected(
        runner,
        "unsupported version is rejected",
        "[config]\nversion=2\n"
    );
    expectRejected(
        runner,
        "unknown setting is rejected",
        "[config]\nversion=1\n[graphics]\nbackend=vulkan\n"
    );
    expectRejected(
        runner,
        "duplicate setting is rejected",
        "[config]\nversion=1\nversion=1\n"
    );
    expectRejected(
        runner,
        "bad boolean is rejected",
        "[config]\nversion=1\n[graphics]\nfullscreen=yes\n"
    );
    expectRejected(
        runner,
        "bad resolution is rejected",
        "[config]\nversion=1\n[graphics]\nwindow_width=320\n"
    );
    expectRejected(
        runner,
        "bad frame limit is rejected",
        "[config]\nversion=1\n[audio_pacing]\nframe_limit=241\n"
    );
    expectRejected(
        runner,
        "unknown keyboard key is rejected",
        "[config]\nversion=1\n[input.keyboard]\na=Banana\n"
    );
    expectRejected(
        runner,
        "duplicate keyboard key is rejected",
        "[config]\nversion=1\n[input.keyboard]\na=J\nb=J\n"
    );
    expectRejected(
        runner,
        "unknown gamepad action is rejected",
        "[config]\nversion=1\n[input.gamepad]\nsouth=menu\n"
    );
}

void testRoundTrip(TestRunner& runner)
{
    GameConfig source;
    source.renderer.backend = RendererBackend::Software;
    source.display.windowWidth = 1600;
    source.audioPacing.volumePercent = 42;
    source.keyboard.bindings[
        static_cast<std::size_t>(SaturnControl::Start)
    ] = "Space";
    source.gamepad.bindings[
        static_cast<std::size_t>(GamepadInput::Back)
    ] = SaturnControl::Y;
    source.experimental.smoothGouraud = false;

    std::ostringstream output;
    std::string error;
    runner.expectTrue(
        "configuration serializes",
        gameConfig_write(output, source, error)
    );

    GameConfig parsed;
    runner.expectTrue(
        "serialized configuration parses",
        parseText(output.str().c_str(), parsed, error)
    );
    runner.expectEqual(
        "round-trip renderer",
        parsed.renderer.backend,
        RendererBackend::Software
    );
    runner.expectEqual(
        "round-trip width",
        parsed.display.windowWidth,
        1600
    );
    runner.expectEqual(
        "round-trip volume",
        parsed.audioPacing.volumePercent,
        42
    );
    runner.expectEqual(
        "round-trip Start key",
        parsed.keyboard.bindings[
            static_cast<std::size_t>(SaturnControl::Start)
        ],
        std::string("Space")
    );
    runner.expectEqual(
        "round-trip gamepad Back",
        parsed.gamepad.bindings[
            static_cast<std::size_t>(GamepadInput::Back)
        ],
        SaturnControl::Y
    );
    runner.expectTrue(
        "round-trip experimental toggle",
        !parsed.experimental.smoothGouraud
    );
}

void testCommandLinePath(TestRunner& runner)
{
    {
        char program[] = "PDS.exe";
        char option[] = "--config";
        char value[] = "custom.ini";
        char* argv[] = {program, option, value};
        std::filesystem::path path = "default.ini";
        bool explicitPath = false;
        std::string error;

        runner.expectTrue(
            "explicit config path parses",
            gameConfig_parsePathOption(
                3,
                argv,
                path,
                explicitPath,
                error
            )
        );
        runner.expectTrue("config path is explicit", explicitPath);
        runner.expectEqual(
            "config path is retained",
            path,
            std::filesystem::path("custom.ini")
        );
    }
    {
        char program[] = "PDS.exe";
        char option[] = "--config";
        char* argv[] = {program, option};
        std::filesystem::path path;
        bool explicitPath = false;
        std::string error;

        runner.expectTrue(
            "missing config path is rejected",
            !gameConfig_parsePathOption(
                2,
                argv,
                path,
                explicitPath,
                error
            ) &&
            !error.empty()
        );
    }
}
}

int main()
{
    TestRunner runner;
    testDefaults(runner);
    testFullConfiguration(runner);
    testErrors(runner);
    testRoundTrip(runner);
    testCommandLinePath(runner);
    return runner.finish();
}
