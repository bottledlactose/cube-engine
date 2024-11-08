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
#include <glm/gtc/quaternion.hpp>

#include "graphics/vertices/PositionNormalColorVertex.hpp"
#include "Context.hpp"
#include "graphics/RenderService.hpp"
#include "physics/PhysicsService.hpp"

// TODO: Clean up these weird definitions for EASTL

// testing
void* __cdecl operator new[](size_t size, const char* name, int flags, unsigned debugFlags, const char* file, int line)
{
	return new uint8_t[size];
}

// testing even more
void* __cdecl operator new[](size_t size, size_t alignment, size_t alignmentOffset, const char* file, int line, unsigned int debugFlags, const char* name, int flags) {
    // Implement a simple aligned memory allocation if needed
    // For simplicity, assuming default alignment
    return ::operator new(size);
}

#include <EASTL/vector.h>

static MeshHandle *mesh_handle = nullptr;
static SDL_GPUTexture *depth_texture = nullptr;

static glm::mat4 projection_matrix = glm::mat4(1.0f);
static glm::mat4 view_matrix = glm::mat4(1.0f);

static glm::vec3 light_position = glm::vec3(-1.0f, 1.0f, 1.2f);
static glm::vec3 light_color = glm::vec3(1.0f, 1.0f, 1.0f);

static eastl::vector<JPH::Vec3> box_positions = {
    JPH::Vec3(0.0f, 0.0f, 0.0f),
    JPH::Vec3(1.0f, 0.0f, 0.0f),
    JPH::Vec3(0.0f, 1.0f, 0.0f),
    JPH::Vec3(0.0f, -1.0f, 0.0f),
    JPH::Vec3(0.0f, 0.0f, 1.0f),
    JPH::Vec3(0.0f, 0.0f, -1.0f),
    JPH::Vec3(0.55f, 5.0f, 0.0f),
};

static eastl::vector<JPH::BodyID> bodies;

static PositionNormalColorVertex vertices[] = {
    { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.5f, 0.0f },

    { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.5f, 0.0f },

    { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },

    {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.5f, 0.0f },

    { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.5f, 0.0f },

    { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    {  0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f },
    { -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.5f, 0.0f }
};


static JPH::BodyID floor_id;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize({ "boomblox", 1270, 720 })) {
        return SDL_APP_FAILURE;
    }

    // testing only, move to camera class
    // TODO: Handle window resizing
    projection_matrix = glm::perspective(glm::radians(45.0f), 1280.0f / 720.0f, 0.1f, 100.0f);
    view_matrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 5.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    // TODO: Handle window resizing
    depth_texture = RenderService::Get().CreateDepthStencil(1280, 720);
    if (depth_texture == nullptr) {
        return SDL_APP_FAILURE;
    }

    mesh_handle = RenderService::Get().CreateMesh(vertices, sizeof(PositionNormalColorVertex) * 36, nullptr, 0);
    if (mesh_handle == nullptr) {
        return SDL_APP_FAILURE;
    }

    floor_id = PhysicsService::Get().CreateBox(JPH::Vec3(0.0f, -2.0f, 0.0f), JPH::Vec3(100.0f, 0.1f, 100.0f));

    for (const JPH::Vec3 &position : box_positions) {
        JPH::BodyID box_id = PhysicsService::Get().CreateBox(position, JPH::Vec3(0.5f, 0.5f, 0.5f), true);
        PhysicsService::Get().GetBodyInterface().SetLinearVelocity(box_id, JPH::Vec3(0.0f, -1.0f, 0.0f));
        bodies.push_back(box_id);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

    JPH::BodyInterface &body_interface = PhysicsService::Get().GetBodyInterface();

    PhysicsService::Get().Update();

    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(RenderService::Get().GetDevice());
    if (command_buffer == nullptr) {
        fprintf(stderr, "Unable to acquire GPU command buffer: %s", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    SDL_GPUTexture *swapchain_texture;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, Context::Get().GetWindow(), &swapchain_texture, nullptr, nullptr)) {
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

        RenderService::Get().UsePipeline(render_pass, "default_mesh");

        for (const JPH::BodyID &body_id : bodies) {
            JPH::Vec3 position = body_interface.GetCenterOfMassPosition(body_id);
            JPH::Quat rotation = body_interface.GetRotation(body_id);

            glm::quat glm_rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            model_matrix *= glm::mat4_cast(glm_rotation);

            // glm::mat4 vertex_uniform[3] = {
            //     //projection_matrix,
            //     //view_matrix,
            //     //model_matrix
            //     mvp
            // };

            //glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

            glm::mat4 vertex_uniform[3] = {
                projection_matrix,
                view_matrix,
                model_matrix,
            };

            glm::vec4 fragment_uniform[] = {
                glm::vec4(light_color, 1.0f),
                glm::vec4(light_position, 1.0f)
            };

            SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform, sizeof(glm::mat4) * 3);
            SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform, sizeof(glm::vec4) * 2);

            RenderService::Get().DrawCube(command_buffer, render_pass, mesh_handle);
        }

        // Draw light sources
        RenderService::Get().UsePipeline(render_pass, "light_source");

        glm::mat4 model_matrix = glm::mat4(1.0f);
        model_matrix = glm::translate(model_matrix, light_position);
        model_matrix = glm::scale(model_matrix, glm::vec3(0.1f, 0.1f, 0.1f));
        glm::mat4 mvp = projection_matrix * view_matrix * model_matrix;

        RenderService::Get().DrawLight(command_buffer, render_pass, mesh_handle, mvp);

        SDL_EndGPURenderPass(render_pass);
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
    // TODO: Properly handle frame timing
    SDL_Delay(1000 / 60);

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {
    if (event->type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

    for (const JPH::BodyID &body_id : bodies) {
        PhysicsService::Get().DestroyBody(body_id);
    }

    PhysicsService::Get().DestroyBody(floor_id);

    RenderService::Get().DestroyDepthStencil(depth_texture);
    RenderService::Get().DestroyMesh(mesh_handle);

    Context::Get().Shutdown();
}
