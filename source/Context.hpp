#pragma once

#include "singleton.hpp"
#include <string>

#include <SDL3/SDL.h>

class Context {
    MAKE_SINGLETON(Context)

private:
    SDL_GPUDevice *device;
    SDL_Window *window;

    std::string base_path;

public:
    bool Initialize();

    SDL_GPUShader *LoadShader(
        SDL_GPUShaderStage stage,
        const std::string &path,
        Uint32 sampler_count,
        Uint32 uniform_buffer_count,
        Uint32 storage_buffer_count,
        Uint32 storage_texture_count
    );

    inline SDL_GPUDevice *GetDevice() const {
        return device;
    }

    inline SDL_Window *GetWindow() const {
        return window;
    }

    inline std::string GetBasePath() const {
        return base_path;
    }
};
