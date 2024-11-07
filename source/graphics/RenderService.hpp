#pragma once

#include "macros/singleton.hpp"

#include "MeshHandle.hpp"
#include <SDL3/SDL.h>

// TESTING ONLY for helper function
#include <glm/glm.hpp>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

class RenderService {
MAKE_SINGLETON(RenderService)
private:
    SDL_GPUDevice *mDevice;
    SDL_Window *mWindow;

    // TODO: Add map with multiple graphics pipelines
    SDL_GPUGraphicsPipeline *default_pipeline;

    eastl::unordered_map<eastl::string, SDL_GPUGraphicsPipeline *> mPipelines;

public:
    bool Initialize(SDL_Window *inWindow);
    void Shutdown();

    bool CreateDefaultPipeline(
        SDL_GPUShader *inVertexShader,
        SDL_GPUShader *inFragmentShader
    );
    void DestroyDefaultPipeline();
    void UseDefaultPipeline(SDL_GPURenderPass *inRenderPass) const;

    SDL_GPUShader *CreateShader(
        SDL_GPUShaderStage inStage,
        const Uint8 *inCode,
        size_t inCodeSize,
        u32 inSamplerCount,
        u32 inUniformBufferCount,
        u32 inStorageBufferCount,
        u32 inStorageTextureCount
    ) const;
    void DestroyShader(SDL_GPUShader *inShader) const;

    SDL_GPUTexture *CreateDepthStencil(u32 inWidth, u32 inHeight);
    void DestroyDepthStencil(SDL_GPUTexture *inDepthStencil) const;

    MeshHandle *CreateMesh(
        void *inVertexData, u32 inVertexSize,
        void *inIndexData, u32 inIndexSize
    ) const;
    void DestroyMesh(MeshHandle *inMesh) const;

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
        return mDevice;
    }
};
