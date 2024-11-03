#include "Renderer.hpp"

// Testing only
#include <cstdio>

#include "Context.hpp"

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

    return true;
}

void Renderer::Shutdown() {
    if (device != nullptr) {
        SDL_DestroyGPUDevice(device);
        device = nullptr;
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
