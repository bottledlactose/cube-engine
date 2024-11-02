#pragma once

#include "macros/singleton.hpp"
#include "macros/types.hpp"

#include <string>

#include <SDL3/SDL.h>

class Context {
    MAKE_SINGLETON(Context)

private:
    SDL_GPUDevice *device;
    SDL_Window *window;

    SDL_GPUGraphicsPipeline *default_pipeline;

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

    SDL_GPUTexture *CreateDepthStencil(Uint32 width, Uint32 height);
    bool CreateDefaultPipeline(SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader);

    SDL_GPUBuffer *CreateMesh(void *data, u32 size);
    bool DestroyMesh(SDL_GPUBuffer *buffer);

    SDL_GPUGraphicsPipeline *GetDefaultPipeline() const {
        return default_pipeline;
    }
    
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
