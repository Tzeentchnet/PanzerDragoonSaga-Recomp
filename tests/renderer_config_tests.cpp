#include "renderer/renderer_config.h"

#include <cstdio>
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

    void expectBackend(
        const char* description,
        RendererBackend actual,
        RendererBackend expected
    )
    {
        if (actual != expected)
        {
            std::fprintf(
                stderr,
                "FAIL: %s (actual: %s, expected: %s)\n",
                description,
                rendererBackend_name(actual),
                rendererBackend_name(expected)
            );
            ++mFailures;
        }
    }

    int finish() const
    {
        if (mFailures != 0)
        {
            std::fprintf(stderr, "%d renderer configuration check(s) failed.\n", mFailures);
            return 1;
        }
        std::puts("All renderer configuration checks passed.");
        return 0;
    }

private:
    int mFailures = 0;
};

void testDefault(TestRunner& runner)
{
    char program[] = "PDS.exe";
    char* argv[] = {program};
    RendererConfig config;
    std::string error;

    runner.expectTrue("default renderer parses", rendererConfig_parse(1, argv, config, error));
    runner.expectBackend("default renderer is automatic", config.backend, RendererBackend::Auto);
}

void testSupportedBackends(TestRunner& runner)
{
    struct Case
    {
        const char* value;
        RendererBackend backend;
    };

    constexpr Case cases[] = {
        {"auto", RendererBackend::Auto},
        {"d3d11", RendererBackend::Direct3D11},
        {"d3d12", RendererBackend::Direct3D12},
        {"vulkan", RendererBackend::Vulkan},
        {"software", RendererBackend::Software},
    };

    for (const Case& testCase : cases)
    {
        char program[] = "PDS.exe";
        char option[] = "--renderer";
        char value[16] = {};
        std::snprintf(value, sizeof(value), "%s", testCase.value);
        char* argv[] = {program, option, value};
        RendererConfig config;
        std::string error;

        runner.expectTrue(testCase.value, rendererConfig_parse(3, argv, config, error));
        runner.expectBackend(testCase.value, config.backend, testCase.backend);
    }
}

void testCommandLinePrecedence(TestRunner& runner)
{
    {
        char program[] = "PDS.exe";
        char* argv[] = {program};
        RendererConfig config;
        config.backend = RendererBackend::Vulkan;
        std::string error;

        runner.expectTrue(
            "saved renderer parses without an override",
            rendererConfig_parse(1, argv, config, error)
        );
        runner.expectBackend(
            "saved renderer is retained",
            config.backend,
            RendererBackend::Vulkan
        );
    }
    {
        char program[] = "PDS.exe";
        char option[] = "--renderer";
        char value[] = "d3d12";
        char* argv[] = {program, option, value};
        RendererConfig config;
        config.backend = RendererBackend::Vulkan;
        std::string error;

        runner.expectTrue(
            "command-line renderer override parses",
            rendererConfig_parse(3, argv, config, error)
        );
        runner.expectBackend(
            "command-line renderer takes precedence",
            config.backend,
            RendererBackend::Direct3D12
        );
    }
}

void testErrors(TestRunner& runner)
{
    {
        char program[] = "PDS.exe";
        char option[] = "--renderer";
        char* argv[] = {program, option};
        RendererConfig config;
        std::string error;
        runner.expectTrue(
            "missing renderer value is rejected",
            !rendererConfig_parse(2, argv, config, error) && !error.empty()
        );
    }
    {
        char program[] = "PDS.exe";
        char option[] = "--renderer";
        char value[] = "invalid";
        char* argv[] = {program, option, value};
        RendererConfig config;
        std::string error;
        runner.expectTrue(
            "unknown renderer is rejected",
            !rendererConfig_parse(3, argv, config, error) && !error.empty()
        );
    }
    {
        char program[] = "PDS.exe";
        char firstOption[] = "--renderer";
        char firstValue[] = "d3d12";
        char secondOption[] = "--renderer";
        char secondValue[] = "vulkan";
        char* argv[] = {
            program,
            firstOption,
            firstValue,
            secondOption,
            secondValue,
        };
        RendererConfig config;
        std::string error;
        runner.expectTrue(
            "duplicate renderer is rejected",
            !rendererConfig_parse(5, argv, config, error) && !error.empty()
        );
    }
}
}

int main()
{
    TestRunner runner;
    testDefault(runner);
    testSupportedBackends(runner);
    testCommandLinePrecedence(runner);
    testErrors(runner);
    return runner.finish();
}
