#include "RenderService.hpp"

#include "macros/log.hpp"

#include "Context.hpp"
#include "vertices/PositionNormalTextureVertex.hpp"

#include <EASTL/vector.h>

#include <SDL_gpu_shadercross.h>

// Shader source code
#include "shaders/basic_triangle.vert.h"
#include "shaders/basic_triangle.frag.h"
#include "shaders/light_source.vert.h"
#include "shaders/light_source.frag.h"

#include "ShaderCreateInfo.hpp"
#include "PipelineCreateInfo.hpp"

#include "RenderState.hpp"

bool RenderService::Initialize(SDL_Window *inWindow) {
    assert(inWindow != nullptr);
    mWindow = inWindow;

    // Get window dimensions
    int window_width, window_height;
    SDL_GetWindowSize(mWindow, &window_width, &window_height);

    mDevice = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);
    if (mDevice == nullptr) {
        LOG_ERROR("Unable to create GPU device: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(mDevice, mWindow)) {
        LOG_ERROR("Unable to claim window for GPU device: %s", SDL_GetError());
        return false;
    }

    // Determine the renderer's sample count
    mSampleCount = SDL_GPU_SAMPLECOUNT_1;
    if (SDL_GPUTextureSupportsSampleCount(
        mDevice,
        SDL_GetGPUSwapchainTextureFormat(mDevice, mWindow),
        SDL_GPU_SAMPLECOUNT_4)) {
            // If the renderer supports 4x MSAA, use it
            mSampleCount = SDL_GPU_SAMPLECOUNT_4;
        }

    mDepthTexture = CreateDepthTexture(window_width, window_height);
    if (mDepthTexture == nullptr) {
        return false;
    }

    if (mSampleCount != SDL_GPU_SAMPLECOUNT_1) {
        // We don't need to error check these because MSAA is optional
        mMSAATexture = CreateMSAATexture(window_width, window_height);
        mResolveTexture = CreateResolveTexture(window_width, window_height);
    }

    SDL_GPUShader *basic_triangle_vert = RenderService::Get().CreateShader(
        SDL_GPU_SHADERSTAGE_VERTEX,
        (const Uint8 *)BASIC_TRIANGLE_VERT_SHADER,
        BASIC_TRIANGLE_VERT_SHADER_SIZE,
        0, 1, 0, 0
    );

    SDL_GPUShader *basic_triangle_frag = RenderService::Get().CreateShader(
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        (const Uint8 *)BASIC_TRIANGLE_FRAG_SHADER,
        BASIC_TRIANGLE_FRAG_SHADER_SIZE,
        0, 2, 0, 0
    );

    CreatePipeline("default_mesh", basic_triangle_vert, basic_triangle_frag);

    SDL_ReleaseGPUShader(mDevice, basic_triangle_vert);
    SDL_ReleaseGPUShader(mDevice, basic_triangle_frag);

    SDL_GPUShader *light_source_vert = RenderService::Get().CreateShader(
        SDL_GPU_SHADERSTAGE_VERTEX,
        (const Uint8 *)LIGHT_SOURCE_VERT_SHADER,
        LIGHT_SOURCE_VERT_SHADER_SIZE,
        0, 1, 0, 0
    );

    SDL_GPUShader *light_source_frag = RenderService::Get().CreateShader(
        SDL_GPU_SHADERSTAGE_FRAGMENT,
        (const Uint8 *)LIGHT_SOURCE_FRAG_SHADER,
        LIGHT_SOURCE_FRAG_SHADER_SIZE,
        0, 0, 0, 0
    );

    CreatePipeline("light_source", light_source_vert, light_source_frag);

    SDL_ReleaseGPUShader(mDevice, light_source_vert);
    SDL_ReleaseGPUShader(mDevice, light_source_frag);

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
        LOG_ERROR("Pipeline not found: %s", inName.c_str());
        return;
    }

    SDL_BindGPUGraphicsPipeline(inRenderPass, it->second);
}

void RenderService::Shutdown() {
    SDL_ReleaseGPUTexture(mDevice, mDepthTexture);
    SDL_ReleaseGPUTexture(mDevice, mMSAATexture);
    SDL_ReleaseGPUTexture(mDevice, mResolveTexture);

    for (auto &pair : mPipelines) {
        SDL_ReleaseGPUGraphicsPipeline(mDevice, pair.second);
    }

    if (mDevice != nullptr) {
        SDL_ReleaseWindowFromGPUDevice(mDevice, mWindow);
        SDL_DestroyGPUDevice(mDevice);
        mDevice = nullptr;
    }
}

void RenderService::SetViewport(Uint32 inWidth, Uint32 inHeight) {
    if (mDepthTexture != nullptr) {
        DestroyTexture(mDepthTexture);
        mDepthTexture = CreateDepthTexture(inWidth, inHeight);
    }

    if (mMSAATexture != nullptr) {
        DestroyTexture(mMSAATexture);
        mMSAATexture = CreateMSAATexture(inWidth, inHeight);
    }

    if (mResolveTexture != nullptr) {
        DestroyTexture(mResolveTexture);
        mResolveTexture = CreateResolveTexture(inWidth, inHeight);
    }
}

bool RenderService::CreatePipeline(const eastl::string &inName, SDL_GPUShader *inVertexShader, SDL_GPUShader *inFragmentShader) {

    SDL_GPUColorTargetDescription color_target_description = {
        .format = SDL_GetGPUSwapchainTextureFormat(mDevice, Context::Get().GetWindow())
    };

    SDL_GPUVertexBufferDescription vertex_buffer_description = {
        .slot = 0,
        .pitch = sizeof(PositionNormalTextureVertex),
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
            .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,
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
        .multisample_state = {
            .sample_count = mSampleCount
        },
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
        LOG_ERROR("Unable to create graphics pipeline: %s", SDL_GetError());
        return false;
    }

    mPipelines[inName] = pipeline;
    return true;
}

SDL_GPUShader *RenderService::CreateShader(
    SDL_GPUShaderStage inStage,
    const Uint8 *inCode,
    size_t inCodeSize,
    Uint32 inSamplerCount,
    Uint32 inUniformBufferCount,
    Uint32 inStorageBufferCount,
    Uint32 inStorageTextureCount
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

SDL_GPUTexture *RenderService::CreateDepthTexture(Uint32 inWidth, Uint32 inHeight) {
    SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GPU_TEXTUREFORMAT_D16_UNORM,
        .usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
        .width = inWidth,
        .height = inHeight,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = mSampleCount,
    };

    SDL_GPUTexture *texture = SDL_CreateGPUTexture(mDevice, &create_info);
    if (texture == nullptr) {
        LOG_ERROR("Unable to create depth texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}

SDL_GPUTexture *RenderService::CreateMSAATexture(Uint32 inWidth, Uint32 inHeight) {
    SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GetGPUSwapchainTextureFormat(mDevice, Context::Get().GetWindow()),
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET,
        .width = inWidth,
        .height = inHeight,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = mSampleCount,
    };

    SDL_GPUTexture *texture = SDL_CreateGPUTexture(mDevice, &create_info);
    if (texture == nullptr) {
        LOG_ERROR("Unable to create MSAA texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}

SDL_GPUTexture *RenderService::CreateResolveTexture(Uint32 inWidth, Uint32 inHeight) {
    SDL_GPUTextureCreateInfo create_info = {
        .type = SDL_GPU_TEXTURETYPE_2D,
        .format = SDL_GetGPUSwapchainTextureFormat(mDevice, Context::Get().GetWindow()),
        .usage = SDL_GPU_TEXTUREUSAGE_COLOR_TARGET | SDL_GPU_TEXTUREUSAGE_SAMPLER,
        .width = inWidth,
        .height = inHeight,
        .layer_count_or_depth = 1,
        .num_levels = 1,
        .sample_count = SDL_GPU_SAMPLECOUNT_1,
    };

    SDL_GPUTexture *texture = SDL_CreateGPUTexture(mDevice, &create_info);
    if (texture == nullptr) {
        LOG_ERROR("Unable to create resolve texture: %s", SDL_GetError());
        return nullptr;
    }

    return texture;
}

void RenderService::DestroyTexture(SDL_GPUTexture *depth_texture) const {
    if (depth_texture != nullptr) {
        SDL_ReleaseGPUTexture(mDevice, depth_texture);
    }
}

MeshHandle *RenderService::CreateMesh(
    void *inVertexData, Uint32 inVertexSize, Uint32 inVertextCount,
    void *inIndexData, Uint32 inIndexSize, Uint32 inIndexCount
) const {
    MeshHandle *mesh = (MeshHandle *)SDL_malloc(sizeof(MeshHandle));
    if (mesh == nullptr) {
        LOG_ERROR("Unable to allocate memory for mesh");
        return nullptr;
    }

    mesh->mVertexSize = inVertexSize;
    mesh->mVertexCount = inVertextCount;

    mesh->mIndexSize = inIndexSize;
    mesh->mIndexCount = inIndexCount;

    SDL_GPUBufferCreateInfo vertex_buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = inVertexSize
    };

    mesh->mVertexBuffer = SDL_CreateGPUBuffer(mDevice, &vertex_buffer_create_info);
    if (mesh->mVertexBuffer == nullptr) {
        LOG_ERROR("Unable to create vertex buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    SDL_GPUBufferCreateInfo index_buffer_create_info = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = inIndexSize
    };

    mesh->mIndexBuffer = SDL_CreateGPUBuffer(mDevice, &index_buffer_create_info);
    if (mesh->mIndexBuffer == nullptr) {
        LOG_ERROR("Unable to create index buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = inVertexSize + inIndexSize
    };

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(mDevice, &transfer_buffer_create_info);
    if (transfer_buffer == nullptr) {
        LOG_ERROR("Unable to create transfer buffer: %s", SDL_GetError());
        DestroyMesh(mesh);
        return nullptr;
    }

    void *transfer_data = SDL_MapGPUTransferBuffer(mDevice, transfer_buffer, false);
    if (transfer_data == nullptr) {
        LOG_ERROR("Unable to map transfer buffer: %s", SDL_GetError());
        SDL_ReleaseGPUTransferBuffer(mDevice, transfer_buffer);
        DestroyMesh(mesh);
        return nullptr;
    }

    SDL_memcpy(transfer_data, inVertexData, inVertexSize);
    SDL_memcpy((Uint8 *)transfer_data + inVertexSize, inIndexData, inIndexSize);

    SDL_UnmapGPUTransferBuffer(mDevice, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(mDevice);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTransferBufferLocation vertex_transfer_buffer_location = {
        .transfer_buffer = transfer_buffer,
        .offset = 0
    };

    SDL_GPUBufferRegion vertex_buffer_region = {
        .buffer = mesh->mVertexBuffer,
        .offset = 0,
        .size = inVertexSize
    };

    SDL_UploadToGPUBuffer(copy_pass, &vertex_transfer_buffer_location, &vertex_buffer_region, false);

    SDL_GPUTransferBufferLocation index_transfer_buffer_location = {
        .transfer_buffer = transfer_buffer,
        .offset = inVertexSize
    };

    SDL_GPUBufferRegion index_buffer_region = {
        .buffer = mesh->mIndexBuffer,
        .offset = 0,
        .size = inIndexSize
    };

    SDL_UploadToGPUBuffer(copy_pass, &index_transfer_buffer_location, &index_buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(mDevice, transfer_buffer);

    return mesh;
}

void RenderService::DestroyMesh(MeshHandle *inMesh) const {
    if (inMesh != nullptr) {
        if (inMesh->mVertexBuffer != nullptr) {
            SDL_ReleaseGPUBuffer(mDevice, inMesh->mVertexBuffer);
        }

        if (inMesh->mIndexBuffer != nullptr) {
            SDL_ReleaseGPUBuffer(mDevice, inMesh->mIndexBuffer);
        }

        SDL_free(inMesh);
    }
}

void RenderService::DrawMesh(SDL_GPURenderPass *inRenderPass, MeshHandle *inMesh) const {

    SDL_GPUBufferBinding vertex_buffer_binding = {
        .buffer = inMesh->mVertexBuffer,
        .offset = 0
    };
    SDL_BindGPUVertexBuffers(inRenderPass, 0, &vertex_buffer_binding, 1);

    if (inMesh->mIndexBuffer == nullptr) {
        SDL_DrawGPUPrimitives(inRenderPass, inMesh->mVertexSize, 1, 0, 0);
    } else {
        SDL_GPUBufferBinding index_buffer_binding = {
            .buffer = inMesh->mIndexBuffer,
            .offset = 0
        };

        SDL_BindGPUIndexBuffer(inRenderPass, &index_buffer_binding, SDL_GPU_INDEXELEMENTSIZE_16BIT);
        SDL_DrawGPUIndexedPrimitives(inRenderPass, inMesh->mIndexCount, 1, 0, 0, 0);
    }
}

RenderState *RenderService::BeginPass() {
    RenderState *state = (RenderState *)SDL_malloc(sizeof(RenderState));
    if (state == nullptr) {
        LOG_ERROR("Unable to allocate memory for render state: %s", SDL_GetError());
        return nullptr;
    }

    state->mCommandBuffer = SDL_AcquireGPUCommandBuffer(mDevice);
    if (state->mCommandBuffer == nullptr) {
        LOG_ERROR("Unable to acquire GPU command buffer: %s", SDL_GetError());
        SDL_free(state);
        return nullptr;
    }

    state->mSwapchainTexture = nullptr;
    if (!SDL_AcquireGPUSwapchainTexture(state->mCommandBuffer, Context::Get().GetWindow(), &state->mSwapchainTexture, nullptr, nullptr)) {
        LOG_ERROR("Unable to acquire GPU swapchain texture: %s", SDL_GetError());
        SDL_free(state);
        return nullptr;
    }

    if (state->mSwapchainTexture != nullptr) {
        SDL_zero(state->mColorTargetInfo);
        state->mColorTargetInfo.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

        // Check is MSAA is enabled
        if (mSampleCount != SDL_GPU_SAMPLECOUNT_1) {
            state->mColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            state->mColorTargetInfo.store_op = SDL_GPU_STOREOP_RESOLVE;
            state->mColorTargetInfo.texture = mMSAATexture;
            state->mColorTargetInfo.resolve_texture = mResolveTexture;
            state->mColorTargetInfo.cycle = true;
            state->mColorTargetInfo.cycle_resolve_texture = true;
        } else {
            state->mColorTargetInfo.texture = state->mSwapchainTexture;
            state->mColorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
            state->mColorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        }

        // Set up depth buffer info
        SDL_zero(state->mDepthStencilTargetInfo);
        state->mDepthStencilTargetInfo = {
            .texture = mDepthTexture,
            .clear_depth = 1.0f,
            .load_op = SDL_GPU_LOADOP_CLEAR,
            .store_op = SDL_GPU_STOREOP_DONT_CARE,
            .stencil_load_op = SDL_GPU_LOADOP_DONT_CARE,
            .stencil_store_op = SDL_GPU_STOREOP_DONT_CARE,
            .cycle = true
        };

        // Begin render pass
        state->mRenderPass = SDL_BeginGPURenderPass(
            state->mCommandBuffer,
            &state->mColorTargetInfo, 1,
            &state->mDepthStencilTargetInfo
        );

        return state;
    }

    SDL_free(state);
    return nullptr;
}

void RenderService::EndPass(RenderState *inState) {
    SDL_EndGPURenderPass(inState->mRenderPass);

    // Check if MSAA is enabled
    if (mSampleCount != SDL_GPU_SAMPLECOUNT_1) {
        // Blit the resolved MSAA texture to the swapchain texture
        SDL_GPUBlitInfo blit_info = {
            .source = {
                .texture = mResolveTexture,
                .w = static_cast<Uint32>(Context::Get().GetWindowWidth()),
                .h = static_cast<Uint32>(Context::Get().GetWindowHeight())
            },
            .destination = {
                .texture = inState->mSwapchainTexture,
                .w = static_cast<Uint32>(Context::Get().GetWindowWidth()),
                .h = static_cast<Uint32>(Context::Get().GetWindowHeight())
            },
            .load_op = SDL_GPU_LOADOP_DONT_CARE
        };

        SDL_BlitGPUTexture(inState->mCommandBuffer, &blit_info);
    }
}
