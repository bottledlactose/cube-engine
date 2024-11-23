#pragma once

#include <SDL3/SDL.h>
#include <EASTL/string.h>

struct ShaderCreateInfo {
    const eastl::string &mPath;
    SDL_GPUShaderStage mStage;
    Uint32 mSamplerCount;
    Uint32 mUniformBufferCount;
    Uint32 mStorageBufferCount;
    Uint32 mStorageTextureCount;
};
