#include "PDS.h"
#include "framePacing.h"
#include "game_config.h"
#include "inputRecorder.h"
#include "regressionCapture.h"

#include <cstring>
#include <cstdio>

#if (defined(__APPLE__) && (TARGET_OS_OSX))
#include <unistd.h> // for chdir
#include <libgen.h> // for dirname
#include <mach-o/dyld.h> // for _NSGetExecutablePath
#include <limits.h> // for PATH_MAX?
#endif

void azelInit();
void resetEngine();
void readInputsFromSMPC();
void updateInputs();
void loopIteration();
bool azelSdl_Init(const GameConfig& config);
void endOfFrame();

extern bool bContinue;

int main(int argc, char* argv[])
{
    GameConfig gameConfig;
    std::filesystem::path configPath = gameConfig_defaultPath();
    bool configPathWasExplicit = false;
    std::string configError;
    if (
        !gameConfig_parsePathOption(
            argc,
            argv,
            configPath,
            configPathWasExplicit,
            configError
        )
    )
    {
        std::fprintf(stderr, "%s\n", configError.c_str());
        return 2;
    }

    const GameConfigLoadResult loadResult =
        gameConfig_load(configPath, gameConfig, configError);
    if (loadResult == GameConfigLoadResult::Error)
    {
        std::fprintf(stderr, "%s\n", configError.c_str());
        return 2;
    }
    if (
        loadResult == GameConfigLoadResult::NotFound &&
        configPathWasExplicit
    )
    {
        std::fprintf(
            stderr,
            "configuration file not found: %s\n",
            configPath.string().c_str()
        );
        return 2;
    }
    if (loadResult == GameConfigLoadResult::Loaded)
    {
        std::fprintf(
            stderr,
            "Configuration loaded from %s\n",
            configPath.string().c_str()
        );
    }

    if (
        !rendererConfig_parse(
            argc,
            argv,
            gameConfig.renderer,
            configError
        )
    )
    {
        std::fprintf(stderr, "%s\n", configError.c_str());
        return 2;
    }

    if (!regressionCapture_configure(argc, argv))
    {
        return 2;
    }

#if (defined(__APPLE__) && (TARGET_OS_OSX))
    char path[PATH_MAX];
    uint32_t pathLen = sizeof(path);
    int err = _NSGetExecutablePath(path, &pathLen);
    assert(!err);
    
    // Switch to the directory of the actual binary
    chdir(dirname(path));
    // and then go up three directories to get to the folder of the .app bundle
    chdir("../Resources");
#endif
    
    if (!azelSdl_Init(gameConfig))
    {
        return 2;
    }

    azelInit();

    resetEngine();
    endOfFrame();
    readInputsFromSMPC();
    updateInputs();
    readInputsFromSMPC();
    updateInputs();

    const auto shutdown = []()
    {
        inputRecorder_stop();
        gSoloud.stopAll();
        gSoloud.deinit();
        regressionCapture_shutdown();
    };

    if (!regressionCapture_initialize())
    {
        shutdown();
        return 2;
    }

    const char* recordPath = nullptr;
    const char* replayPath = nullptr;
    for (int i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "--record") == 0)
        {
            if ((i + 1) >= argc)
            {
                std::fprintf(stderr, "--record requires a file path\n");
                shutdown();
                return 2;
            }
            recordPath = argv[++i];
        }
        else if (strcmp(argv[i], "--replay") == 0)
        {
            if ((i + 1) >= argc)
            {
                std::fprintf(stderr, "--replay requires a file path\n");
                shutdown();
                return 2;
            }
            replayPath = argv[++i];
        }
        else if (strcmp(argv[i], "--experimental-ntsc-cadence") == 0)
        {
            if (!framePacing_enableExperimentalNtscCadence())
            {
                std::fprintf(
                    stderr,
                    "--experimental-ntsc-cadence requires a build configured with "
                    "PDS_EXPERIMENTAL_NTSC_FRAME_CADENCE=ON\n"
                );
                shutdown();
                return 2;
            }
        }
    }
    if (recordPath && replayPath)
    {
        std::fprintf(stderr, "--record and --replay cannot be used together\n");
        shutdown();
        return 2;
    }
    if (
        recordPath &&
        !inputRecorder_startRecording(
            recordPath,
            regressionCapture_getRouteName(),
            regressionCapture_getRegion(),
            regressionCapture_getConnectedMask()
        )
    )
    {
        shutdown();
        return 2;
    }
    if (replayPath && !inputRecorder_startPlayback(replayPath))
    {
        shutdown();
        return 2;
    }

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(loopIteration, 0, 1);
#else
    while ((graphicEngineStatus.m4514.m0_inputDevices[0].m0_current.m8_newButtonDown & 0x100) == 0)
    {
        loopIteration();

        if (!bContinue)
            break;
    };
#endif

    shutdown();
    return inputRecorder_hasFailed() || regressionCapture_hasFailed() ? 1 : 0;
}
