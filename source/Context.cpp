#include "Context.hpp"

#include <cstdio>

#include <SDL_gpu_shadercross.h>

bool Context::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    base_path = SDL_GetBasePath();

    device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);
    if (device == nullptr) {
        fprintf(stderr, "Unable to create GPU device: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("SDL App", 800, 600, 0);
    if (window == nullptr) {
        fprintf(stderr, "Unable to create window: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        fprintf(stderr, "Unable to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    return true;
}

SDL_GPUShader *Context::LoadShader(
    SDL_GPUShaderStage stage,
    const std::string &path,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count
) {
    // TODO: Move actual file loading to a separate function, something like an asset loader?
    size_t code_size;
    void *code = SDL_LoadFile((base_path + path).c_str(), &code_size);
    if (code == nullptr) {
        fprintf(stderr, "Unable to load shader file: %s", SDL_GetError());
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
        fprintf(stderr, "Unable to create shader: %s", SDL_GetError());
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

SDL_GPUTexture *Context::CreateDepthStencil(Uint32 width, Uint32 height) {
    SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = width,
        .height = height,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };

    SDL_GPUTexture *texture = SDL_CreateGPUTexture(device, &create_info);
    if (texture == nullptr) {
        fprintf(stderr, "Unable to create depth texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}
