#pragma once

#include "macros/singleton.hpp"

#include "MeshHandle.hpp"
#include <SDL3/SDL.h>

class Renderer {
MAKE_SINGLETON(Renderer)

private:
    SDL_GPUDevice *device;
    SDL_Window *window;

    // TODO: Add map with multiple graphics pipelines
    SDL_GPUGraphicsPipeline *default_pipeline;

public:
    bool Initialize(SDL_Window *window);
    void Shutdown();

    bool CreateDefaultPipeline(
        SDL_GPUShader *vertex_shader,
        SDL_GPUShader *fragment_shader
    );
    void DestroyDefaultPipeline();
    void UseDefaultPipeline(SDL_GPURenderPass *render_pass) const;

    SDL_GPUTexture *CreateDepthStencil(u32 width, u32 height);
    void DestroyDepthStencil(SDL_GPUTexture *depth_texture) const;

    MeshHandle *CreateMesh(
        void *vertex_data, u32 vertex_size,
        void *index_data, u32 index_size
    ) const;
    void DestroyMesh(MeshHandle *mesh) const;

    inline SDL_GPUDevice *GetDevice() const {
        return device;
    }
};
