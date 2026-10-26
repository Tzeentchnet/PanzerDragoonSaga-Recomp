#pragma once

#include <string>
#include <string_view>

enum class RendererBackend
{
    Auto,
    Direct3D11,
    Direct3D12,
    Vulkan,
    Software,
};

struct RendererConfig
{
    RendererBackend backend = RendererBackend::Auto;
};

const char* rendererBackend_name(RendererBackend backend);
bool rendererBackend_parse(std::string_view value, RendererBackend& backend);
bool rendererConfig_parse(
    int argc,
    char* argv[],
    RendererConfig& config,
    std::string& error
);
