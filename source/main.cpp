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

#include "graphics/PositionNormalColorVertex.hpp"
#include "Context.hpp"
#include "graphics/Renderer.hpp"

static SDL_GPUDevice *device = nullptr;
static SDL_Window *window = nullptr;
//static SDL_GPUBuffer *vertex_buffer = nullptr;
static MeshHandle *mesh_handle = nullptr;

static SDL_GPUTexture *depth_texture = nullptr;

static glm::mat4 projection_matrix = glm::mat4(1.0f);
static glm::mat4 view_matrix = glm::mat4(1.0f);
static glm::mat4 model_matrix_a = glm::mat4(1.0f);
static glm::mat4 model_matrix_b = glm::mat4(1.0f);

static PositionNormalColorVertex vertices[36] = {
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

    device = Renderer::Get().GetDevice();
    window = Context::Get().GetWindow();

    // TODO: Handle window resizing
    depth_texture = Context::Get().CreateDepthStencil(800, 600);
    if (depth_texture == nullptr) {
        return SDL_APP_FAILURE;
    }

    mesh_handle = Renderer::Get().CreateMesh(vertices, sizeof(PositionNormalColorVertex) * 36, nullptr, 0);
    if (mesh_handle == nullptr) {
        return SDL_APP_FAILURE;
    }

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
        
        Renderer::Get().UseDefaultPipeline(render_pass);

        // Rotate the model matrix
        model_matrix_a = glm::rotate(model_matrix_a, glm::radians(0.5f), glm::vec3(0.5f, 1.0f, 0.0f));
        glm::mat4 mvp_a = projection_matrix * view_matrix * model_matrix_a;

        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp_a, sizeof(glm::mat4));
        SDL_GPUBufferBinding vertex_buffer_binding = {
            .buffer = mesh_handle->vertex_buffer,
            .offset = 0
        };
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
        SDL_DrawGPUPrimitives(render_pass, mesh_handle->vertex_size, 1, 0, 0);

        // Rotate the model matrix
        model_matrix_b = glm::rotate(model_matrix_b, glm::radians(0.5f), glm::vec3(1.0f, 0.5f, 0.0f));
        glm::mat4 mvp_b = projection_matrix * view_matrix * model_matrix_b;

        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp_b, sizeof(glm::mat4));
        SDL_BindGPUVertexBuffers(render_pass, 0, &vertex_buffer_binding, 1);
        SDL_DrawGPUPrimitives(render_pass, mesh_handle->vertex_size, 1, 0, 0);

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
    //SDL_ReleaseGPUBuffer(device, vertex_buffer);
    Renderer::Get().DestroyMesh(mesh_handle);
    //SDL_ReleaseWindowFromGPUDevice(device, window);
    //SDL_DestroyGPUDevice(device);
    Renderer::Get().Shutdown(Context::Get().GetWindow());
    SDL_DestroyWindow(window);
    
    SDL_Quit();
}
