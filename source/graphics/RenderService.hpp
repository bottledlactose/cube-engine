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

    // Start Render State
    SDL_GPUSampleCount mSampleCount;

    SDL_GPUTexture *mDepthTexture;
    SDL_GPUTexture *mMSAATexture;
    SDL_GPUTexture *mResolveTexture;
    // End Render State

    eastl::unordered_map<eastl::string, SDL_GPUGraphicsPipeline *> mPipelines;

public:
    bool Initialize(SDL_Window *inWindow);
    void Shutdown();

    bool CreatePipeline(
        const eastl::string &inName,
        SDL_GPUShader *inVertexShader,
        SDL_GPUShader *inFragmentShader
    );
    void DestroyPipeline(const eastl::string &inName);
    void UsePipeline(SDL_GPURenderPass *inRenderPass, const eastl::string &inName) const;

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

    SDL_GPUTexture *CreateDepthTexture(u32 inWidth, u32 inHeight);
    SDL_GPUTexture *CreateMSAATexture(u32 inWidth, u32 inHeight);
    SDL_GPUTexture *CreateResolveTexture(u32 inWidth, u32 inHeight);
    void DestroyTexture(SDL_GPUTexture *inTexture) const;

    MeshHandle *CreateMesh(
        void *inVertexData, u32 inVertexSize, u32 inVertexCount,
        void *inIndexData, u32 inIndexSize, u32 inIndexCount
    ) const;
    void DestroyMesh(MeshHandle *inMesh) const;
    void DrawMesh(SDL_GPURenderPass *inRenderPass, MeshHandle *inMesh) const;

    inline SDL_GPUDevice *GetDevice() const {
        return mDevice;
    }

    inline SDL_GPUTexture *GetDepthTexture() const {
        return mDepthTexture;
    }

    inline SDL_GPUTexture *GetMSAATexture() const {
        return mMSAATexture;
    }

    inline SDL_GPUTexture *GetResolveTexture() const {
        return mResolveTexture;
    }
};
