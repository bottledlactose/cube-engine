#include <stdio.h>
#include <string>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

// Used for cross-platform shader loading
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#include <SDL_gpu_shadercross.h>

static SDL_GPUDevice *device = nullptr;
static SDL_Window *window = nullptr;

static SDL_GPUShader *LoadShader(
    SDL_GPUDevice *device,
    SDL_GPUShaderStage stage,
    const std::string &path,
    Uint32 sampler_count,
    Uint32 uniform_buffer_count,
    Uint32 storage_buffer_count,
    Uint32 storage_texture_count
) {

    size_t code_size;
    void *code = SDL_LoadFile(path.c_str(), &code_size);
    if (code == nullptr) {
        fprintf(stderr, "Unable to load shader file: %s", SDL_GetError());
        return nullptr;
    }

    SDL_GPUShaderCreateInfo create_info;
    SDL_zero(create_info);
    create_info.code = (const Uint8 *)code;
    create_info.code_size = code_size;
    create_info.entrypoint = "main";
    create_info.format = SDL_GPU_SHADERFORMAT_SPIRV;
    create_info.stage = stage;
    create_info.num_samplers = sampler_count;
    create_info.num_uniform_buffers = uniform_buffer_count;
    create_info.num_storage_buffers = storage_buffer_count;
    create_info.num_storage_textures = storage_texture_count;

    SDL_GPUShader *shader = SDL_ShaderCross_CompileGraphicsShaderFromSPIRV(device, &create_info);
    if (shader == nullptr) {
        fprintf(stderr, "Unable to create shader: %s", SDL_GetError());
        SDL_free(code);
        return nullptr;
    }

    SDL_free(code);
    return shader;
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Unable to initialize SDL: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    device = SDL_CreateGPUDevice(SDL_ShaderCross_GetSPIRVShaderFormats(), true, nullptr);
    if (device == nullptr) {
        fprintf(stderr, "Unable to create GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    window = SDL_CreateWindow("SDL App", 800, 600, 0);
    if (window == nullptr) {
        fprintf(stderr, "Unable to create window: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    if (!SDL_ClaimWindowForGPUDevice(device, window)) {
        fprintf(stderr, "Unable to claim window for GPU device: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    const char *base_path = SDL_GetBasePath();

    std::string vertex_shader_path = base_path;
    vertex_shader_path += "shaders/basic_triangle.vert.spv";

    SDL_GPUShader *vertex_shader = LoadShader(device, SDL_GPU_SHADERSTAGE_VERTEX, vertex_shader_path.c_str(), 0, 0, 0, 0);
    if (vertex_shader == nullptr) {
        return SDL_APP_FAILURE;
    }

    std::string fragment_shader_path = base_path;
    fragment_shader_path += "shaders/basic_triangle.frag.spv";

    SDL_GPUShader *fragment_shader = LoadShader(device, SDL_GPU_SHADERSTAGE_FRAGMENT, fragment_shader_path.c_str(), 0, 0, 0, 0);
    if (fragment_shader == nullptr) {
        return SDL_APP_FAILURE;
    }

    // TODO: Set up pipeline

    SDL_ReleaseGPUShader(device, vertex_shader);
    SDL_ReleaseGPUShader(device, fragment_shader);

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

        render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);
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
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    
    SDL_Quit();
}
