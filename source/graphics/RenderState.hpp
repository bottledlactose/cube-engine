#pragma once

#include <SDL3/SDL.h>

struct RenderState {
    SDL_GPUCommandBuffer *mCommandBuffer;
    SDL_GPUTexture *mSwapchainTexture;
    SDL_GPURenderPass *mRenderPass;

    SDL_GPUColorTargetInfo mColorTargetInfo;
    SDL_GPUDepthStencilTargetInfo mDepthStencilTargetInfo;
};
