#pragma once

#include "macros/singleton.hpp"

#include <SDL3/SDL.h>

#include <EASTL/string.h>
#include <EASTL/unordered_map.h>

#include "MeshHandle.hpp"
#include "RenderState.hpp"

class RenderService {
MAKE_SINGLETON(RenderService)
private:
    SDL_GPUDevice *mDevice;
    SDL_Window *mWindow;

    SDL_GPUTexture *mDepthTexture;
    SDL_GPUSampleCount mSampleCount;
    SDL_GPUTexture *mMSAATexture;
    SDL_GPUTexture *mResolveTexture;

    eastl::unordered_map<eastl::string, SDL_GPUGraphicsPipeline *> mPipelines;

public:
    bool Initialize(SDL_Window *inWindow);
    void Shutdown();

    void SetViewport(Uint32 inWidth, Uint32 inHeight);

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
        Uint32 inSamplerCount,
        Uint32 inUniformBufferCount,
        Uint32 inStorageBufferCount,
        Uint32 inStorageTextureCount
    ) const;
    void DestroyShader(SDL_GPUShader *inShader) const;

    SDL_GPUTexture *CreateDepthTexture(Uint32 inWidth, Uint32 inHeight);
    SDL_GPUTexture *CreateMSAATexture(Uint32 inWidth, Uint32 inHeight);
    SDL_GPUTexture *CreateResolveTexture(Uint32 inWidth, Uint32 inHeight);
    void DestroyTexture(SDL_GPUTexture *inTexture) const;

    MeshHandle *CreateMesh(
        void *inVertexData, Uint32 inVertexSize, Uint32 inVertexCount,
        void *inIndexData, Uint32 inIndexSize, Uint32 inIndexCount
    ) const;
    void DestroyMesh(MeshHandle *inMesh) const;
    void DrawMesh(SDL_GPURenderPass *inRenderPass, MeshHandle *inMesh) const;

    RenderState *BeginPass();
    void EndPass(RenderState *inState);

    inline SDL_GPUDevice *GetDevice() const {
        return mDevice;
    }

    inline SDL_GPUSampleCount GetSampleCount() const {
        return mSampleCount;
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
