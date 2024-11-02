#include <stdio.h>
#include <string>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

// Used for cross-platform shader loading
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#include <SDL_gpu_shadercross.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "PositionColorVertex.hpp"
#include "Context.hpp"

static SDL_GPUDevice *device = nullptr;
static SDL_Window *window = nullptr;
static SDL_GPUBuffer *vertex_buffer = nullptr;

static SDL_GPUTexture *depth_texture = nullptr;

static glm::mat4 projection_matrix = glm::mat4(1.0f);
static glm::mat4 view_matrix = glm::mat4(1.0f);
static glm::mat4 model_matrix = glm::mat4(1.0f);

static PositionColorVertex vertices[36] = {
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},

    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},

    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255},
    
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},

    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},

    {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255},
    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},

    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},

    {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255},
    {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
    {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255},

    {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255},
    {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255},
    {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255},
};

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize()) {
        return SDL_APP_FAILURE;
    }

    // testing only
    // TODO: Handle window resizing
    projection_matrix = glm::perspective(glm::radians(45.0f), 800.0f / 600.0f, 0.1f, 100.0f);
    view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    device = Context::Get().GetDevice();
    window = Context::Get().GetWindow();

    // TODO: Move shader loading to context initialization
    SDL_GPUShader *vertex_shader = Context::Get().LoadShader(SDL_GPU_SHADERSTAGE_VERTEX, "shaders/basic_triangle.vert.spv", 0, 1, 0, 0);
    if (vertex_shader == nullptr) {
        return SDL_APP_FAILURE;
    }

    SDL_GPUShader *fragment_shader = Context::Get().LoadShader(SDL_GPU_SHADERSTAGE_FRAGMENT, "shaders/basic_triangle.frag.spv", 0, 0, 0, 0);
    if (fragment_shader == nullptr) {
        return SDL_APP_FAILURE;
    }

    // TODO: Handle window resizing
    depth_texture = Context::Get().CreateDepthStencil(800, 600);
    if (depth_texture == nullptr) {
        return SDL_APP_FAILURE;
    }

    if (!Context::Get().CreateDefaultPipeline(vertex_shader, fragment_shader)) {
        return SDL_APP_FAILURE;
    }

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

    SDL_GPUBufferCreateInfo buffer_create_info;
    SDL_zero(buffer_create_info);
    buffer_create_info.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    buffer_create_info.size = sizeof(PositionColorVertex) * 36;

    vertex_buffer = SDL_CreateGPUBuffer(device, &buffer_create_info);
    if (vertex_buffer == nullptr) {
        fprintf(stderr, "Unable to create vertex buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTransferBufferCreateInfo transfer_buffer_create_info;
    SDL_zero(transfer_buffer_create_info);
    transfer_buffer_create_info.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transfer_buffer_create_info.size = sizeof(PositionColorVertex) * 36;

    SDL_GPUTransferBuffer *transfer_buffer = SDL_CreateGPUTransferBuffer(device, &transfer_buffer_create_info);
    if (transfer_buffer == nullptr) {
        fprintf(stderr, "Unable to create transfer buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    PositionColorVertex *transfer_data = (PositionColorVertex *)SDL_MapGPUTransferBuffer(device, transfer_buffer, false);
    if (transfer_data == nullptr) {
        fprintf(stderr, "Unable to map transfer buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }



    SDL_memcpy(transfer_data, vertices, sizeof(PositionColorVertex) * 36);
    SDL_UnmapGPUTransferBuffer(device, transfer_buffer);

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    SDL_GPUCopyPass *copy_pass = SDL_BeginGPUCopyPass(command_buffer);

    SDL_GPUTransferBufferLocation transfer_buffer_location;
    SDL_zero(transfer_buffer_location);
    transfer_buffer_location.transfer_buffer = transfer_buffer;
    transfer_buffer_location.offset = 0;

    SDL_GPUBufferRegion buffer_region;
    SDL_zero(buffer_region);
    buffer_region.buffer = vertex_buffer;
    buffer_region.offset = 0;
    buffer_region.size = sizeof(PositionColorVertex) * 36;

    SDL_UploadToGPUBuffer(copy_pass, &transfer_buffer_location, &buffer_region, false);

    SDL_EndGPUCopyPass(copy_pass);
    SDL_SubmitGPUCommandBuffer(command_buffer);
    SDL_ReleaseGPUTransferBuffer(device, transfer_buffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(device);
    if (command_buffer == nullptr) {
        fprintf(stderr, "Unable to acquire GPU command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_texture;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, window, &swapchain_texture, nullptr, nullptr)) {
        fprintf(stderr, "Unable to acquire GPU swapchain texture: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (swapchain_texture != nullptr) {
        SDL_GPURenderPass *render_pass;
        SDL_GPUColorTargetInfo color_target_info;
        SDL_zero(color_target_info);
        color_target_info.texture = swapchain_texture;
        color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
        color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        color_target_info.store_op = SDL_GPU_STOREOP_STORE;

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info;
        SDL_zero(depth_stencil_target_info);
        depth_stencil_target_info.clear_depth = 1.0f;
        depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.texture = depth_texture;
        depth_stencil_target_info.cycle = true;

        render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
        //render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);

        SDL_GPUBufferBinding vertex_buffer_binding;
        SDL_zero(vertex_buffer_binding);
        vertex_buffer_binding.buffer = vertex_buffer;
        vertex_buffer_binding.offset = 0;

        SDL_BindGPUGraphicsPipeline(render_pass, Context::Get().GetDefaultPipeline());

        // Rotate the model matrix
        model_matrix = glm::rotate(model_matrix, glm::radians(0.5f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
        SDL_DrawGPUPrimitives(render_pass, 36, 1, 0, 0);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    SDL_ReleaseGPUTexture(device, depth_texture);
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(device, Context::Get().GetDefaultPipeline());
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    
    SDL_Quit();
}
