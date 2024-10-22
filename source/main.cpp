#include <stdio.h>
#include <string>

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL.h>

// Used for cross-platform shader loading
#define SDL_GPU_SHADERCROSS_IMPLEMENTATION
#include <SDL_gpu_shadercross.h>

struct PositionColorVertex {
    float x, y, z;
    float nx, ny, nz;
    Uint8 r, g, b, a; // TODO: Ensure this is the correct format
};

static SDL_GPUDevice *device = nullptr;
static SDL_Window *window = nullptr;
static SDL_GPUGraphicsPipeline *pipeline = nullptr;
static SDL_GPUBuffer *vertex_buffer = nullptr;

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
    SDL_GPUColorTargetDescription color_target_description;
    SDL_zero(color_target_description);
    color_target_description.format = SDL_GetGPUSwapchainTextureFormat(device, window);

    SDL_GPUVertexBufferDescription vertex_buffer_description;
    SDL_zero(vertex_buffer_description);
    vertex_buffer_description.slot = 0;
    vertex_buffer_description.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertex_buffer_description.instance_step_rate = 0;
    vertex_buffer_description.pitch = sizeof(PositionColorVertex);

    SDL_GPUVertexAttribute vertex_attributes[3];
    SDL_zero(vertex_attributes[0]);
    vertex_attributes[0].buffer_slot = 0;
    vertex_attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[0].location = 0;
    vertex_attributes[0].offset = 0;

    SDL_zero(vertex_attributes[1]);
    vertex_attributes[1].buffer_slot = 0;
    vertex_attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertex_attributes[1].location = 1;
    vertex_attributes[1].offset = sizeof(float) * 3;

    SDL_zero(vertex_attributes[2]);
    vertex_attributes[2].buffer_slot = 0;
    vertex_attributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_UBYTE4_NORM;
    vertex_attributes[2].location = 2;
    vertex_attributes[2].offset = sizeof(float) * 6;

    SDL_GPUVertexInputState vertex_input_state;
    SDL_zero(vertex_input_state);
    vertex_input_state.num_vertex_buffers = 1;
    vertex_input_state.vertex_buffer_descriptions = &vertex_buffer_description;
    vertex_input_state.num_vertex_attributes = 3;
    vertex_input_state.vertex_attributes = vertex_attributes;

    SDL_GPUGraphicsPipelineCreateInfo pipeline_create_info;
    SDL_zero(pipeline_create_info);

    pipeline_create_info.target_info.num_color_targets = 1;
    pipeline_create_info.target_info.color_target_descriptions = &color_target_description;
    pipeline_create_info.vertex_input_state = vertex_input_state;
    
    pipeline_create_info.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipeline_create_info.vertex_shader = vertex_shader;
    pipeline_create_info.fragment_shader = fragment_shader;

    pipeline = SDL_CreateGPUGraphicsPipeline(device, &pipeline_create_info);
    if (pipeline == nullptr) {
        fprintf(stderr, "Unable to create graphics pipeline: %s", SDL_GetError());
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

    transfer_data[0] = {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255};
    transfer_data[1] = {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255};
    transfer_data[2] = {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255};

    transfer_data[3] = {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255};
    transfer_data[4] = {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255};
    transfer_data[5] = {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255};

    transfer_data[6] = {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255};
    transfer_data[7] = {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255};
    transfer_data[8] = {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255};

    transfer_data[9] = {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255};
    transfer_data[10] = {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255};
    transfer_data[11] = {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255};

    transfer_data[12] = {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255};
    transfer_data[13] = {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255};
    transfer_data[14] = {-0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 255, 0, 0, 255};

    transfer_data[15] = {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255};
    transfer_data[16] = {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255};
    transfer_data[17] = {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255};

    transfer_data[18] = {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255};
    transfer_data[19] = {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255};
    transfer_data[20] = {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255};

    transfer_data[21] = {0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 255, 255, 0, 255};
    transfer_data[22] = {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255};
    transfer_data[23] = {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255};

    transfer_data[24] = {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255};
    transfer_data[25] = {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255};
    transfer_data[26] = {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255};

    transfer_data[27] = {0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0, 0, 0, 255};
    transfer_data[28] = {-0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 1.0f, 255, 255, 255, 255};
    transfer_data[29] = {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255};

    transfer_data[30] = {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255};
    transfer_data[31] = {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255};
    transfer_data[32] = {-0.5f, -0.5f, 0.5f, 0.0f, 1.0f, 1.0f, 0, 255, 255, 255};

    transfer_data[33] = {-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0, 255, 0, 255};
    transfer_data[34] = {0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 1.0f, 0, 0, 255, 255};
    transfer_data[35] = {0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 1.0f, 255, 0, 255, 255};

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

        render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, nullptr);

        SDL_GPUBufferBinding vertex_buffer_binding;
        SDL_zero(vertex_buffer_binding);
        vertex_buffer_binding.buffer = vertex_buffer;
        vertex_buffer_binding.offset = 0;

        SDL_BindGPUGraphicsPipeline(render_pass, pipeline);
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
    SDL_ReleaseGPUBuffer(device, vertex_buffer);
    SDL_ReleaseGPUGraphicsPipeline(device, pipeline);
    SDL_ReleaseWindowFromGPUDevice(device, window);
    SDL_DestroyGPUDevice(device);
    SDL_DestroyWindow(window);
    
    SDL_Quit();
}
