#pragma once

#include "game_config.h"
#include "renderer.h"

class SDL_BGFX_backend : public backend
{
private:
    static bool init(const GameConfig& config);
public:
    static backend* create(const GameConfig& config);

    virtual void bindBackBuffer() override;

    virtual void ImGUI_NewFrame() override;
    virtual void ImGUI_RenderDrawData(ImDrawData* pDrawData) override;
};
