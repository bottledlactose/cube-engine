#pragma once

#include <EASTL/unordered_map.h>
#include <EASTL/string.h>

#include <SDL3/SDL_gpu.h>

#include "graphics/MeshHandle.hpp"

class ContentManager {
private:
    eastl::unordered_map<eastl::string, SDL_GPUShader *> mShaders;
    eastl::unordered_map<eastl::string, MeshHandle *> mMeshes;

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

    MeshHandle *LoadMesh(const eastl::string &inPath);
    void UnloadMesh(const eastl::string &inPath);

    void Unload();
};
