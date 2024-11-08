#include "RenderService.hpp"

#include "macros/log.hpp"

// Testing only
#include <cstdio>
#include <cassert>

#include "Context.hpp"
#include "vertices/PositionNormalColorVertex.hpp"

#include <EASTL/vector.h>

#include <SDL_gpu_shadercross.h>

// Testing
struct ShaderCreateInfo {
    const eastl::string &mPath;
    SDL_GPUShaderStage mStage;
    Uint32 mSamplerCount;
    Uint32 mUniformBufferCount;
    Uint32 mStorageBufferCount;
    Uint32 mStorageTextureCount;
};

struct PipelineCreateInfo {
    eastl::string mName;
    ShaderCreateInfo mVertexShader;
    ShaderCreateInfo mFragmentShader;
};

bool RenderService::Initialize(SDL_Window *inWindow) {
    assert(inWindow != nullptr);
    mWindow = inWindow;

    mDevice = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);
    if (mDevice == nullptr) {
        fprintf(stderr, "Unable to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(mDevice, mWindow)) {
        fprintf(stderr, "Unable to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    eastl::vector<PipelineCreateInfo> pipeline_info = {
        {
            "default_mesh",
            {
                "shaders/basic_triangle.vert.spv",
                SDL_GPU_SHADERSTAGE_VERTEX,
                0, 1, 0, 0
            },
            {
                "shaders/basic_triangle.frag.spv",
                SDL_GPU_SHADERSTAGE_FRAGMENT,
                0, 0, 0, 0
            }
        }
    };

    for (auto &info : pipeline_info) {
        //PipelineCreateInfo &info = pair.second;

        SDL_GPUShader *vertex_shader = Context::Get().GetContent().LoadShader(
            info.mVertexShader.mPath,
            info.mVertexShader.mStage,
            info.mVertexShader.mSamplerCount,
            info.mVertexShader.mUniformBufferCount,
            info.mVertexShader.mStorageBufferCount,
            info.mVertexShader.mStorageTextureCount
        );
        if (vertex_shader == nullptr) {
            return false;
        }

        SDL_GPUShader *fragment_shader = Context::Get().GetContent().LoadShader(
            info.mFragmentShader.mPath,
            info.mFragmentShader.mStage,
            info.mFragmentShader.mSamplerCount,
            info.mFragmentShader.mUniformBufferCount,
            info.mFragmentShader.mStorageBufferCount,
            info.mFragmentShader.mStorageTextureCount
        );
        if (fragment_shader == nullptr) {
            return false;
        }

        CreatePipeline(info.mName, vertex_shader, fragment_shader);

        Context::Get().GetContent().UnloadShader(info.mVertexShader.mPath);
        Context::Get().GetContent().UnloadShader(info.mFragmentShader.mPath);
    }





    // // TODO: Make shader loading easier
    // SDL_GPUShader *vertex_shader = Context::Get().GetContent().LoadShader(
    //     "shaders/basic_triangle.vert.spv",
    //     SDL_GPU_SHADERSTAGE_VERTEX,
    //     0, 1, 0, 0);
    // if (vertex_shader == nullptr) {
    //     return false;
    // }

    // SDL_GPUShader *fragment_shader = Context::Get().GetContent().LoadShader(
    //     "shaders/basic_triangle.frag.spv",
    //     SDL_GPU_SHADERSTAGE_FRAGMENT,
    //     0, 0, 0, 0);
    // if (fragment_shader == nullptr) {
    //     return false;
    // }

    // CreateDefaultPipeline(vertex_shader, fragment_shader);

    // Context::Get().GetContent().UnloadShader("shaders/basic_triangle.vert.spv");
    // Context::Get().GetContent().UnloadShader("shaders/basic_triangle.frag.spv");

    return true;
}

void RenderService::DestroyPipeline(const eastl::string &inName) {
    auto it = mPipelines.find(inName);
    if (it != mPipelines.end()) {
        SDL_ReleaseGPUGraphicsPipeline(mDevice, it->second);
        mPipelines.erase(it);
    }
}

void RenderService::UsePipeline(SDL_GPURenderPass *inRenderPass, const eastl::string &inName) const {

    auto it = mPipelines.find(inName);
    if (it == mPipelines.end()) {
        //fprintf(stderr, "Pipeline not found: %s", inName.c_str());
        LOG_ERROR("Pipeline not found: %s", inName.c_str());
        return;
    }

    SDL_BindGPUGraphicsPipeline(inRenderPass, it->second);
}

void RenderService::Shutdown() {

    for (auto &pair : mPipelines) {
        SDL_ReleaseGPUGraphicsPipeline(mDevice, pair.second);
    }

    if (mDevice != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(mDevice, mWindow);
        SDL_DestroyGPUDevice(mDevice);
        mDevice = nullptr;
    }
}

bool RenderService::CreatePipeline(const eastl::string &inName, SDL_GPUShader *inVertexShader, SDL_GPUShader *inFragmentShader) {

    SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(mDevice, Context::Get().GetWindow())
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
        .vertex_shader = inVertexShader,
        .fragment_shader = inFragmentShader,
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

    SDL_GPUGraphicsPipeline *pipeline = SDL_CreateGPUGraphicsPipeline(mDevice, &pipeline_create_info);
    if (pipeline == nullptr) {
        fprintf(stderr, "Unable to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    mPipelines[inName] = pipeline;
    return true;
}

SDL_GPUShader *RenderService::CreateShader(
    SDL_GPUShaderStage inStage,
    const Uint8 *inCode,
    size_t inCodeSize,
    u32 inSamplerCount,
    u32 inUniformBufferCount,
    u32 inStorageBufferCount,
    u32 inStorageTextureCount
) const {

    SDL_GPUShaderCreateInfo create_info = {
        .code_size = inCodeSize,
        .code = inCode,
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = inStage,
        .num_samplers = inSamplerCount,
        .num_storage_textures = inStorageTextureCount,
        .num_storage_buffers = inStorageBufferCount,
        .num_uniform_buffers = inUniformBufferCount,
    };

    SDL_GPUShader *shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(mDevice, &create_info);
    if (shader == nullptr) {
        LOG_ERROR("Unable to create shader: %s", SDL_GetError());
        return nullptr;
    }

    return shader;
}

void RenderService::DestroyShader(SDL_GPUShader *inShader) const {
    if (inShader != nullptr) {
        SDL_ReleaseGPUShader(mDevice, inShader);
    }
}

SDL_GPUTexture *RenderService::CreateDepthStencil(u32 inWidth, u32 inHeight) {
    SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = inWidth,
        .height = inHeight,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };

    SDL_GPUTexture *texture = SDL_CreateGPUTexture(mDevice, &create_info);
    if (texture == nullptr) {
        fprintf(stderr, "Unable to create depth texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}

void RenderService::DestroyDepthStencil(SDL_GPUTexture *depth_texture) const {
    if (depth_texture != nullptr) {
        SDL_ReleaseGPUTexture(mDevice, depth_texture);
    }
}

MeshHandle *RenderService::CreateMesh(
    void *inVertexData, u32 inVertexSize,
    void *inIndexData, u32 inIndexSize
) const {
    MeshHandle *mesh = (MeshHandle *)SDL_malloc(sizeof(MeshHandle));
    if (mesh == nullptr) {
        fprintf(stderr, "Unable to allocate memory for mesh");
        return nullptr;
    }

    mesh->vertex_size = inVertexSize;

    SDL_GPUBufferCreateInfo buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = inVertexSize
    };

    mesh->vertex_buffer = SDL_CreateGPUBuffer(mDevice, &buffer_create_info);
    if (mesh->vertex_buffer == nullptr) {
        fprintf(stderr, "Unable to create vertex buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    // TODO: Handle index buffer creation

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = inVertexSize
    };

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(mDevice, &transfer_buffer_create_info);
    if (transfer_buffer == nullptr) {
        fprintf(stderr, "Unable to create transfer buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    void *transfer_data = SDL_MapGPUTransferBuffer(mDevice, transfer_buffer, false);
    if (transfer_data == nullptr) {
        fprintf(stderr, "Unable to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(mDevice, transfer_buffer);
        DestroyMesh(mesh);
        return nullptr;
    }

    SDL_memcpy(transfer_data, inVertexData, inVertexSize);
    SDL_UnmapGPUTransferBuffer(mDevice, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(mDevice);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTransferBufferLocation transfer_buffer_location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };

    SDL_GPUBufferRegion buffer_region = {
        .buffer = mesh->vertex_buffer,
        .offset = 0,
        .size = inVertexSize
    };

    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(mDevice, transfer_buffer);

    return mesh;
}

void RenderService::DestroyMesh(MeshHandle *inMesh) const {
    if (inMesh != nullptr) {
        if (inMesh->vertex_buffer != nullptr) {
            SDL_ReleaseGPUBuffer(mDevice, inMesh->vertex_buffer);
        }

        SDL_free(inMesh);
    }
}
