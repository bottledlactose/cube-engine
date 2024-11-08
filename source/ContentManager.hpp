#pragma once

#include "macros/types.hpp"

#include <EASTL/unordered_map.h>
#include <EASTL/string.h>

#include <SDL3/SDL_gpu.h>

class ContentManager {
private:
    eastl::unordered_map<eastl::string, SDL_GPUShader *> mShaders;

public:
    ContentManager() = default;
    ~ContentManager() = default;

    SDL_GPUShader *LoadShader(
        const eastl::string &inPath,
        SDL_GPUShaderStage inStage,
        Uint32 inSamplerCount,
        Uint32 inUniformBufferCount,
        Uint32 inStorageBufferCount,
        Uint32 inStorageTextureCount
    );
    void UnloadShader(const eastl::string &inPath);
};
