#include "ContentManager.hpp"

#include "macros/log.hpp"

#include "Context.hpp"
#include "graphics/RenderService.hpp"

SDL_GPUShader *ContentManager::LoadShader(
    const eastl::string &inPath,
    SDL_GPUShaderStage inStage,
    Uint32 inSamplerCount,
    Uint32 inUniformBufferCount,
    Uint32 inStorageBufferCount,
    Uint32 inStorageTextureCount
) {
    auto it = mShaders.find(inPath);
    if (it != mShaders.end()) {
        return it->second;
    }

    size_t code_size;
    void *code = SDL_LoadFile((Context::Get().GetBasePath() + inPath).c_str(), &code_size);
    if (code == nullptr) {
        LOG_ERROR("Unable to load shader file: %s", SDL_GetError());
        return nullptr;
    }

    SDL_GPUShader *shader = RenderService::Get().CreateShader(
        inStage,
        (const Uint8 *)code,
        code_size,
        inSamplerCount,
        inUniformBufferCount,
        inStorageBufferCount,
        inStorageTextureCount
    );

    SDL_free(code);

    mShaders[inPath] = shader;
    return shader;
}

void ContentManager::UnloadShader(const eastl::string &inPath) {
    auto it = mShaders.find(inPath);
    if (it != mShaders.end()) {
        RenderService::Get().DestroyShader(it->second);
        mShaders.erase(it);
    }
}
