#pragma once

#include "macros/singleton.hpp"
#include "macros/types.hpp"

#include <string>

#include <SDL3/SDL.h>

struct ContextCreateInfo {
    const char *mTitle;
    i32 mWidth;
    i32 mHeight;
};

class Context {
MAKE_SINGLETON(Context)
private:
    SDL_Window *mWindow;

public:
    bool Initialize(const ContextCreateInfo &inCreateInfo);
    void Shutdown();

    SDL_GPUShader *LoadShader(
        SDL_GPUShaderStage stage,
        const std::string &path,
        Uint32 sampler_count,
        Uint32 uniform_buffer_count,
        Uint32 storage_buffer_count,
        Uint32 storage_texture_count
    );

    inline SDL_Window *GetWindow() const {
        return mWindow;
    }

    inline std::string GetBasePath() const {
        return SDL_GetBasePath();
    }
};
