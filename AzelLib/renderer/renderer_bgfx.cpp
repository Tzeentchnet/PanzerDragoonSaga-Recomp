#include "PDS.h"
#include <bx/platform.h>
#include <backends/imgui_impl_sdl3.h>
#include "imguiBGFX.h"

#include "renderer_bgfx.h"

extern "C" {
void* cbSetupMetalLayer(void*);
}

SDL_Window* gWindowBGFX = nullptr;

namespace
{
#if BX_PLATFORM_WINDOWS
constexpr uint16_t MicrosoftBasicRenderDriverDeviceId = 0x008c;
#endif

bgfx::RendererType::Enum requestedRendererType(RendererBackend backend)
{
    switch (backend)
    {
    case RendererBackend::Direct3D11:
    case RendererBackend::Software:
        return bgfx::RendererType::Direct3D11;
    case RendererBackend::Direct3D12:
        return bgfx::RendererType::Direct3D12;
    case RendererBackend::Vulkan:
        return bgfx::RendererType::Vulkan;
    case RendererBackend::Auto:
#if BX_PLATFORM_WINDOWS
        return bgfx::RendererType::Direct3D12;
#elif BX_PLATFORM_OSX || BX_PLATFORM_IOS
        return bgfx::RendererType::Metal;
#else
        return bgfx::RendererType::Vulkan;
#endif
    }

    return bgfx::RendererType::Count;
}

void requestSoftwareAdapter(bgfx::Init& init)
{
#if BX_PLATFORM_WINDOWS
    // DXGI exposes WARP as the Microsoft Basic Render Driver.
    init.vendorId = BGFX_PCI_ID_MICROSOFT;
    init.deviceId = MicrosoftBasicRenderDriverDeviceId;
#else
    init.vendorId = BGFX_PCI_ID_SOFTWARE_RASTERIZER;
#endif
}

bool isSoftwareAdapter(const bgfx::Caps& caps)
{
#if BX_PLATFORM_WINDOWS
    return
        caps.vendorId == BGFX_PCI_ID_MICROSOFT &&
        caps.deviceId == MicrosoftBasicRenderDriverDeviceId;
#else
    return caps.vendorId == BGFX_PCI_ID_SOFTWARE_RASTERIZER;
#endif
}
}

backend* SDL_BGFX_backend::create(const GameConfig& config)
{
    if (init(config))
    {
        return new SDL_BGFX_backend();
    }
    return nullptr;
}

bool SDL_BGFX_backend::init(const GameConfig& config)
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD))
    {
        std::fprintf(stderr, "SDL initialization failed: %s\n", SDL_GetError());
        return false;
    }

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY;
    if (config.display.fullscreen)
    {
        flags |= SDL_WINDOW_FULLSCREEN;
    }

#ifdef __IPHONEOS__
    flags |= SDL_WINDOW_FULLSCREEN;
    flags |= SDL_WINDOW_METAL;
#endif

    int resolution[2] = {
        config.display.windowWidth,
        config.display.windowHeight,
    };

    gWindowBGFX = SDL_CreateWindow("Panzer Dragoon Saga", resolution[0], resolution[1], flags);
    if (gWindowBGFX == nullptr)
    {
        std::fprintf(stderr, "SDL window creation failed: %s\n", SDL_GetError());
        SDL_Quit();
        return false;
    }

    if (
        !SDL_GetWindowSizeInPixels(
            gWindowBGFX,
            &resolution[0],
            &resolution[1]
        )
    )
    {
        std::fprintf(
            stderr,
            "SDL could not determine the window pixel size: %s\n",
            SDL_GetError()
        );
        SDL_DestroyWindow(gWindowBGFX);
        gWindowBGFX = nullptr;
        SDL_Quit();
        return false;
    }

    SDL_PropertiesID props = SDL_GetWindowProperties(gWindowBGFX);

    bgfx::Init initparam;
    initparam.type = requestedRendererType(config.renderer.backend);
    initparam.fallback =
        config.renderer.backend == RendererBackend::Auto;
    initparam.swapChain.width = resolution[0];
    initparam.swapChain.height = resolution[1];
    initparam.reset =
        config.display.vsync ? BGFX_RESET_VSYNC : BGFX_RESET_NONE;
    if (config.renderer.backend == RendererBackend::Software)
    {
        requestSoftwareAdapter(initparam);
    }
#if BX_PLATFORM_LINUX || BX_PLATFORM_BSD
    initparam.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_X11_DISPLAY_POINTER, NULL);
    initparam.swapChain.nwh = (void*)(uintptr_t)SDL_GetNumberProperty(props, SDL_PROP_WINDOW_X11_WINDOW_NUMBER, 0);
#elif BX_PLATFORM_OSX
    initparam.swapChain.ndt = NULL;
    initparam.swapChain.nwh = cbSetupMetalLayer(SDL_GetPointerProperty(props, SDL_PROP_WINDOW_COCOA_WINDOW_POINTER, NULL));
#elif BX_PLATFORM_IOS
    SDL_SetHint(SDL_HINT_RENDER_DRIVER, "metal");
    SDL_Renderer* pRenderer = SDL_CreateRenderer(gWindowBGFX, NULL);
    initparam.swapChain.ndt = NULL;
    initparam.swapChain.nwh = SDL_GetRenderMetalLayer(pRenderer);
    initparam.type = bgfx::RendererType::Metal;
#elif BX_PLATFORM_WINDOWS
    initparam.swapChain.ndt = NULL;
    initparam.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_WIN32_HWND_POINTER, NULL);
#elif BX_PLATFORM_STEAMLINK
    initparam.swapChain.ndt = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_VIVANTE_DISPLAY_POINTER, NULL);
    initparam.swapChain.nwh = SDL_GetPointerProperty(props, SDL_PROP_WINDOW_VIVANTE_WINDOW_POINTER, NULL);
#endif // BX_PLATFORM_

    if (!bgfx::init(initparam))
    {
        std::fprintf(
            stderr,
            "bgfx initialization failed for requested renderer '%s'\n",
            rendererBackend_name(config.renderer.backend)
        );
        SDL_DestroyWindow(gWindowBGFX);
        gWindowBGFX = nullptr;
        SDL_Quit();
        return false;
    }

    const bgfx::RendererType::Enum selectedType = bgfx::getRendererType();
    const bgfx::Caps* caps = bgfx::getCaps();
    std::fprintf(
        stderr,
        "Renderer requested=%s selected=%s vendor=0x%04x device=0x%04x "
        "window=%ux%u fullscreen=%s vsync=%s\n",
        rendererBackend_name(config.renderer.backend),
        bgfx::getRendererName(selectedType),
        caps->vendorId,
        caps->deviceId,
        initparam.swapChain.width,
        initparam.swapChain.height,
        config.display.fullscreen ? "true" : "false",
        config.display.vsync ? "true" : "false"
    );

    if (
        config.renderer.backend == RendererBackend::Software &&
        !isSoftwareAdapter(*caps)
    )
    {
        std::fprintf(stderr, "The selected renderer is not a software adapter.\n");
        bgfx::shutdown();
        SDL_DestroyWindow(gWindowBGFX);
        gWindowBGFX = nullptr;
        SDL_Quit();
        return false;
    }

    imguiCreate();

    ImGuiIO& io = ImGui::GetIO();
    //io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;
    if ((bgfx::getRendererType() == bgfx::RendererType::Direct3D11) || (bgfx::getRendererType() == bgfx::RendererType::Direct3D12)) {
        ImGui_ImplSDL3_InitForD3D(gWindowBGFX);
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::Metal) {
        ImGui_ImplSDL3_InitForMetal(gWindowBGFX);
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::OpenGL) {
        ImGui_ImplSDL3_InitForOpenGL(gWindowBGFX, SDL_GL_GetCurrentContext());
    }
    else
    if (bgfx::getRendererType() == bgfx::RendererType::Vulkan) {
        ImGui_ImplSDL3_InitForVulkan(gWindowBGFX);
    }
    else {
        ImGui_ImplSDL3_InitForOther(gWindowBGFX);
    }

    //bgfx::setDebug(BGFX_DEBUG_TEXT | BGFX_DEBUG_STATS);

    return true;
}

void SDL_BGFX_backend::bindBackBuffer()
{
}

void SDL_BGFX_backend::ImGUI_NewFrame()
{
    if (!isShipping())
    {
        //ImGui_ImplOpenGL3_NewFrame();
    }
}

void SDL_BGFX_backend::ImGUI_RenderDrawData(ImDrawData* pDrawData)
{
    if (!isShipping())
    {
        //ImGui_ImplOpenGL3_RenderDrawData(pDrawData);
    }
}
