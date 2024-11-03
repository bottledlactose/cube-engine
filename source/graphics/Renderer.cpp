#include "Renderer.hpp"

// Testing only
#include <cstdio>

#include "Context.hpp"
#include "vertices/PositionNormalColorVertex.hpp"

#include <SDL_gpu_shadercross.h>

bool Renderer::Initialize(SDL_Window *window) {
    if (window == nullptr) {
        fprintf(stderr, "Window is null");
        return false;
    }

    

    device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);
    if (device == nullptr) {
        fprintf(stderr, "Unable to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        fprintf(stderr, "Unable to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    // TODO: Make shader loading easier
    SDL_GPUShader *vertex_shader = Context::Get().LoadShader(
        SDL_GPU_SHADERSTAGE_VERTEX,
        "shaders/basic_triangle.vert.spv",
        0, 1, 0, 0);
    if (vertex_shader == nullptr) {
        return false;
    }

    SDL_GPUShader *fragment_shader = Context::Get().LoadShader(
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        "shaders/basic_triangle.frag.spv",
        0, 0, 0, 0);
    if (fragment_shader == nullptr) {
        return false;
    }

    CreateDefaultPipeline(vertex_shader, fragment_shader);

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    this->window = window;
    return true;
}

void Renderer::DestroyDefaultPipeline() {
    if (default_pipeline != nullptr) {
        SDL_ReleaseGPUGraphicsPipeline(device, default_pipeline);
        default_pipeline = nullptr;
    }
}

void Renderer::UseDefaultPipeline(SDL_GPURenderPass *render_pass) const {
    SDL_BindGPUGraphicsPipeline(render_pass, default_pipeline);
}

void Renderer::Shutdown() {
    DestroyDefaultPipeline();

    if (device != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(device, window);
        SDL_DestroyGPUDevice(device);
        device = nullptr;
    }
}

bool Renderer::CreateDefaultPipeline(SDL_GPUShader *vertex_shader, SDL_GPUShader *fragment_shader) {

    SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(device, Context::Get().GetWindow())
    };

    SDL_GPUVertexBufferDescription vertex_buffer_description = {
        .slot = 0,
        .pitch = sizeof(PositionNormalColorVertex),
        .input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX,
        .instance_step_rate = 0,
    };

    SDL_GPUVertexAttribute vertex_attributes[3] = {
        // Position
        {
            .location = 0,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = 0
        },
        // Normal
        {
            .location = 1,
            .buffer_slot = 0,
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3,
            .offset = sizeof(float) * 3
        },
        // Color
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

SDL_GPUTexture *Renderer::CreateDepthStencil(u32 width, u32 height) {
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

void Renderer::DestroyDepthStencil(SDL_GPUTexture *depth_texture) const {
    if (depth_texture != nullptr) {
        SDL_ReleaseGPUTexture(device, depth_texture);
    }
}

MeshHandle *Renderer::CreateMesh(
    void *vertex_data, u32 vertex_size,
    void *index_data, u32 index_size
) const {
    MeshHandle *mesh = (MeshHandle *)SDL_malloc(sizeof(MeshHandle));
    if (mesh == nullptr) {
        fprintf(stderr, "Unable to allocate memory for mesh");
        return nullptr;
    }

    mesh->vertex_size = vertex_size;

    SDL_GPUBufferCreateInfo buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = vertex_size
    };

    mesh->vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    if (mesh->vertex_buffer == nullptr) {
        fprintf(stderr, "Unable to create vertex buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    // TODO: Handle index buffer creation

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = vertex_size
    };

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    if (transfer_buffer == nullptr) {
        fprintf(stderr, "Unable to create transfer buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    void *transfer_data = SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (transfer_data == nullptr) {
        fprintf(stderr, "Unable to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);
        DestroyMesh(mesh);
        return nullptr;
    }

    SDL_memcpy(transfer_data, vertex_data, vertex_size);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTransferBufferLocation transfer_buffer_location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };

    SDL_GPUBufferRegion buffer_region = {
        .buffer = mesh->vertex_buffer,
        .offset = 0,
        .size = vertex_size
    };

    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    return mesh;
}

void Renderer::DestroyMesh(MeshHandle *mesh) const {
    if (mesh != nullptr) {
        if (mesh->vertex_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(device, mesh->vertex_buffer);
        }

        SDL_free(mesh);
    }
}
