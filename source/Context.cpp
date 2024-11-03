#include "Context.hpp"

#include "PositionColorVertex.hpp"
#include "graphics/Renderer.hpp"
#include <cstdio>

#include <SDL_gpu_shadercross.h>

bool Context::Initialize() {
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
        return false;
    }

    window = SDL_CreateWindow("SDL App", 800, 600, 0);
    if (window == nullptr) {
        fprintf(stderr, "Unable to create window: %s", SDL_GetError());
        return false;
    }

    Renderer::Get().Initialize(window);

    return true;
}

bool Context::CreateDefaultPipeline(SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader) {

    SDL_GPUDevice *device = Renderer::Get().GetDevice();
    
    SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, window)
    };

    SDL_GPUVertexBufferDescription vertex_buffer_description = {
        .slot = 0,
        .pitch = sizeof(PositionColorVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };

    SDL_GPUVertexAttribute vertex_attributes[3] = {
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0
        },
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = sizeof(float) * 3
        },
        {
            .location = 2,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM,
            .offset = sizeof(float) * 6
        }
    };

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info = {
        .vertex_shader = vertex_shader,
        .fragment_shader = fragment_shader,
        .vertex_input_state = {
            .vertex_buffer_descriptions = &vertex_buffer_description,
            .num_vertex_buffers = 1,
            .vertex_attributes = vertex_attributes,
            .num_vertex_attributes = 3,
        },
        .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
        .depth_stencil_state = {
            .compare_op = SDL_GPU_COMPAREOP_LESS,
            .enable_depth_test = true,
            .enable_depth_write = true,
        },
        .target_info = {
            .color_target_descriptions = &color_target_description,
            .num_color_targets = 1,
            .depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
            .has_depth_stencil_target = true
        },
    };

    default_pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
    if (default_pipeline == nullptr) {
        fprintf(stderr, "Unable to create graphics pipeline: %s", SDL_GetError());
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
    SDL_GPUDevice *device = Renderer::Get().GetDevice();

    // TODO: Move actual file loading to a separate function, something like an asset loader?
    size_t code_size;
    void *code = SDL_LoadFile((GetBasePath() + path).c_str(), &code_size);
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
    SDL_GPUDevice *device = Renderer::Get().GetDevice();

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
