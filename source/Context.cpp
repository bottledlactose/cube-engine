#include "Context.hpp"

#include "macros/log.hpp"

#include "graphics/RenderService.hpp"

// Keep this here until we move shader stuff outside of the Context class
#include <SDL_gpu_shadercross.h>

bool Context::Initialize(const ContextCreateInfo &inCreateInfo) {
    // Initialize the SDL video subsystem, needed for window creation and rendering
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        LOG_ERROR("Unable to initialize SDL video subsystem: %s", SDL_GetError());
        return false;
    }

    // Create the main window for the game
    mWindow = SDL_CreateWindow(inCreateInfo.mTitle, inCreateInfo.mWidth, inCreateInfo.mHeight, 0);
    if (mWindow == nullptr) {
        LOG_ERROR("Unable to create window: %s", SDL_GetError());
        return false;
    }

    // Begin initializating services here
    if (!RenderService::Get().Initialize(mWindow)) {
        return false;
    }

    return true;
}

void Context::Shutdown() {
    RenderService::Get().Shutdown();

    if (mWindow != nullptr) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

SDL_GPUShader *Context::LoadShader(
    SDL_GPUShaderStage stage,
    const std::string &path,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count
) {
    SDL_GPUDevice *device = RenderService::Get().GetDevice();

    // TODO: Move actual file loading to a separate function, something like an asset loader?
    size_t code_size;
    void *code = SDL_LoadFile((GetBasePath() + path).c_str(), &code_size);
    if (code == nullptr) {
        LOG_ERROR("Unable to load shader file: %s", SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = code_size,
        .code = (const Uint8 *)code,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = stage,
        .num_samplers = sampler_count,
        .num_storage_textures = storage_texture_count,
        .num_storage_buffers = storage_buffer_count,
        .num_uniform_buffers = uniform_buffer_count,
    };

    SDL_GPUShader *shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &create_info);
    if (shader == nullptr) {
        LOG_ERROR("Unable to create shader: %s", SDL_GetError());
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);
    return shader;
}
