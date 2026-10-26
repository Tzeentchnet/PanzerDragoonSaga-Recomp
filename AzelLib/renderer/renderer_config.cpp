#include "renderer_config.h"

#include <cstring>

const char* rendererBackend_name(RendererBackend backend)
{
    switch (backend)
    {
    case RendererBackend::Auto:
        return "auto";
    case RendererBackend::Direct3D11:
        return "d3d11";
    case RendererBackend::Direct3D12:
        return "d3d12";
    case RendererBackend::Vulkan:
        return "vulkan";
    case RendererBackend::Software:
        return "software";
    }

    return "unknown";
}

bool rendererBackend_parse(std::string_view value, RendererBackend& backend)
{
    if (value == "auto")
    {
        backend = RendererBackend::Auto;
    }
    else if (value == "d3d11")
    {
        backend = RendererBackend::Direct3D11;
    }
    else if (value == "d3d12")
    {
        backend = RendererBackend::Direct3D12;
    }
    else if (value == "vulkan")
    {
        backend = RendererBackend::Vulkan;
    }
    else if (value == "software")
    {
        backend = RendererBackend::Software;
    }
    else
    {
        return false;
    }

    return true;
}

bool rendererConfig_parse(
    int argc,
    char* argv[],
    RendererConfig& config,
    std::string& error
)
{
    bool rendererSeen = false;

    for (int i = 1; i < argc; ++i)
    {
        if (std::strcmp(argv[i], "--renderer") != 0)
        {
            continue;
        }

        if (rendererSeen)
        {
            error = "--renderer may only be specified once";
            return false;
        }
        if ((i + 1) >= argc)
        {
            error = "--renderer requires auto, d3d11, d3d12, vulkan, or software";
            return false;
        }

        rendererSeen = true;
        const char* value = argv[++i];
        if (!rendererBackend_parse(value, config.backend))
        {
            error = "unknown --renderer value: ";
            error += value;
            return false;
        }
    }

    return true;
}
