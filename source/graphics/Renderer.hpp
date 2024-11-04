#pragma once

#include "macros/singleton.hpp"

#include "MeshHandle.hpp"
#include <SDL3/SDL.h>

// TESTING ONLY for helper function
#include <glm/glm.hpp>

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

    // Temporary helper function
    void DrawCube(SDL_GPUCommandBuffer *command_buffer, SDL_GPURenderPass *render_pass, MeshHandle *mesh, glm::mat4 mvp) const {
        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
        SDL_GPUBufferBinding vertex_buffer_binding = {
            .buffer = mesh->vertex_buffer,
            .offset = 0
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
        SDL_DrawGPUPrimitives(render_pass, mesh->vertex_size, 1, 0, 0);
    }

    inline SDL_GPUDevice *GetDevice() const {
        return device;
    }
};
