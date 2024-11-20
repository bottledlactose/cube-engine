#include "Scene.hpp"

#include <EASTL/vector.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>

#include "Context.hpp"
#include "graphics/RenderService.hpp"
#include "InputService.hpp"

#include "graphics/uniforms/Material.hpp"
#include "graphics/uniforms/DirectionalLight.hpp"
#include "graphics/uniforms/PointLight.hpp"

// Temp struct to just make things work
struct FragmentUniform {
    glm::vec4 camera_position;
    //Material material;
    DirectionalLight directional_light;
    PointLight point_light[4];
};

static eastl::vector<JPH::Vec3> sBoxPositions = {
    JPH::Vec3(0.0f, 0.0f, 0.0f),
    JPH::Vec3(1.0f, 0.0f, 0.0f),
    JPH::Vec3(0.0f, 1.0f, 0.0f),
    JPH::Vec3(0.0f, -1.0f, 0.0f),
    JPH::Vec3(0.0f, 0.0f, 1.0f),
    JPH::Vec3(0.0f, 0.0f, -1.0f),
    JPH::Vec3(0.55f, 5.0f, 0.0f),
};

static eastl::vector<glm::vec3> sLightPositions = {
    glm::vec3(2.0f, 0.2f, 2.0f),
    glm::vec3(-2.0f, 0.2f, 2.0f),
    glm::vec3(2.0f, 0.2f, -2.0f),
    glm::vec3(-2.0f, 0.2f, -2.0f)
};

Scene::Scene() : mCamera(45.0f, 0.0f, -90.0f, 5.0f) {}

void Scene::Initialize() {
    mPhysicsManager.Initialize();

    mCubeMesh = mContentManager.LoadMesh("content/1x1.glb");
    mBallMesh = mContentManager.LoadMesh("content/ball.glb");

    mFloorID = mPhysicsManager.CreateBox(JPH::Vec3(0.0f, -2.0f, 0.0f), JPH::Vec3(100.0f, 0.1f, 100.0f));
    mBallID = mPhysicsManager.CreateBall(JPH::Vec3(-5.0f, 0.0f, 0.0f), 0.5f);

    for (const JPH::Vec3 &position : sBoxPositions) {
        JPH::BodyID box_id = mPhysicsManager.CreateBox(position, JPH::Vec3(0.5f, 0.5f, 0.5f), true);
        mPhysicsManager.GetBodyInterface().SetLinearVelocity(box_id, JPH::Vec3(0.0f, -1.0f, 0.0f));
        mCubeBodies.push_back(box_id);
    }

    // return true;
}

void Scene::Shutdown() {

    mPhysicsManager.DestroyBody(mFloorID);
    mPhysicsManager.DestroyBody(mBallID);

    for (const JPH::BodyID &body_id : mCubeBodies) {
        mPhysicsManager.DestroyBody(body_id);
    }

    mContentManager.UnloadMesh("content/1x1.glb");
    mContentManager.UnloadMesh("content/ball.glb");

    mPhysicsManager.Shutdown();
}

void Scene::Update() {
    if (Context::Get().IsWindowResized()) {
        mCamera.SetAspectRatio(
            static_cast<float>(Context::Get().GetWindowWidth()),
            static_cast<float>(Context::Get().GetWindowHeight())
        );
    }

    // TODO: apply delta time
    mCamera.SetDistance(mCamera.GetDistance() - InputService::Get().GetScrollY());

    if (InputService::Get().IsRightMouseDown()) {
        glm::vec2 mouse_delta = InputService::Get().GetMouseDelta();
        // TODO: apply delta time
        mCamera.SetYaw(mCamera.GetYaw() + mouse_delta.x);
        mCamera.SetPitch(mCamera.GetPitch() + mouse_delta.y);
    }

    if (InputService::Get().IsLeftMouseDown()) {

        glm::vec3 camera_position = mCamera.GetPosition();
        glm::vec2 mouse_position = InputService::Get().GetMousePosition();

        glm::vec3 throw_direction = mCamera.GetThrowDirection(
            mouse_position.x, mouse_position.y,
            static_cast<float>(Context::Get().GetWindowWidth()),
            static_cast<float>(Context::Get().GetWindowHeight())
        );
        throw_direction = -throw_direction;

        // Spawn a box at the throw direction
        mPhysicsManager.DestroyBody(mBallID);
        mBallID = mPhysicsManager.CreateBall(JPH::Vec3(camera_position.x, camera_position.y, camera_position.z), 0.5f);

        // Throw the ball towards the blocks
        mPhysicsManager.GetBodyInterface().AddLinearVelocity(mBallID, JPH::Vec3(throw_direction.x * 20.0f, throw_direction.y * 20.0f, throw_direction.z * 20.0f));
    }

    mPhysicsManager.Update();
}

void Scene::Draw() {
    // Dirty rendering pass begin and end
    
    SDL_GPUCommandBuffer *command_buffer = SDL_AcquireGPUCommandBuffer(RenderService::Get().GetDevice());
    if (command_buffer == nullptr) {
        fprintf(stderr, "Unable to acquire GPU command buffer: %s", SDL_GetError());
        // return SDL_APP_FAILURE;
        // return false;
    }

    SDL_GPUTexture *swapchain_texture;
    if (!SDL_AcquireGPUSwapchainTexture(command_buffer, Context::Get().GetWindow(), &swapchain_texture, nullptr, nullptr)) {
        fprintf(stderr, "Unable to acquire GPU swapchain texture: %s", SDL_GetError());
        //return SDL_APP_FAILURE;
        // return false;
    }

    if (swapchain_texture != nullptr) {
        SDL_GPURenderPass *render_pass;
        SDL_GPUColorTargetInfo color_target_info;

        SDL_zero(color_target_info);
        color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};

        // TODO: Implement better way to check if MSAA is active
        if (RenderService::Get().GetMSAATexture() != nullptr && RenderService::Get().GetResolveTexture() != nullptr) {
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_RESOLVE;
            color_target_info.texture = RenderService::Get().GetMSAATexture();
            color_target_info.resolve_texture = RenderService::Get().GetResolveTexture();
            color_target_info.cycle = true;
            color_target_info.cycle_resolve_texture = true;
        } else {
            color_target_info.texture = swapchain_texture;
            //color_target_info.clear_color = {0.1f, 0.1f, 0.1f, 1.0f};
            color_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
            color_target_info.store_op = SDL_GPU_STOREOP_STORE;
        }

        SDL_GPUDepthStencilTargetInfo depth_stencil_target_info;
        SDL_zero(depth_stencil_target_info);
        depth_stencil_target_info.clear_depth = 1.0f;
        depth_stencil_target_info.load_op = SDL_GPU_LOADOP_CLEAR;
        depth_stencil_target_info.store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
        depth_stencil_target_info.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
        depth_stencil_target_info.texture = RenderService::Get().GetDepthTexture();
        depth_stencil_target_info.cycle = true;

        render_pass = SDL_BeginGPURenderPass(command_buffer, &color_target_info, 1, &depth_stencil_target_info);
        
        RenderService::Get().UsePipeline(render_pass, "default_mesh");

        FragmentUniform fragment_uniform = {
            glm::vec4(mCamera.GetPosition(), 1.0f),
            {
                glm::vec4(-0.2f, -1.0f, -0.3f, 1.0f),
                glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)
            },
            {
                {
                    glm::vec4(sLightPositions[0], 1.0f),
                    1.0f,
                    0.09f,
                    0.032f,
                    0.0f,
                    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                    glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                    glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                },
                {
                    glm::vec4(sLightPositions[1], 1.0f),
                    1.0f,
                    0.09f,
                    0.032f,
                    0.0f,
                    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                    glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                    glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                },
                {
                    glm::vec4(sLightPositions[2], 1.0f),
                    1.0f,
                    0.09f,
                    0.032f,
                    0.0f,
                    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                    glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                    glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                },
                {
                    glm::vec4(sLightPositions[3], 1.0f),
                    1.0f,
                    0.09f,
                    0.032f,
                    0.0f,
                    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                    glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                    glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                }
            }
        };

        SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform, sizeof(FragmentUniform));

        JPH::BodyInterface &body_interface = mPhysicsManager.GetBodyInterface();

        for (const JPH::BodyID &body_id : mCubeBodies) {
            JPH::Vec3 position = body_interface.GetCenterOfMassPosition(body_id);
            JPH::Quat rotation = body_interface.GetRotation(body_id);

            glm::quat glm_rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            model_matrix *= glm::mat4_cast(glm_rotation);

            glm::mat4 model_inverse_transpose = glm::transpose(glm::inverse(model_matrix));

            glm::mat4 vertex_uniform[4] = {
                mCamera.GetProjectionMatrix(),
                mCamera.GetViewMatrix(),
                model_matrix,
                model_inverse_transpose,
            };

            Material material = {
                glm::vec4(1.0f, 0.5f, 0.31f, 0.0f),
                glm::vec4(1.0f, 0.5f, 0.31f, 0.0f),
                glm::vec4(0.5f, 0.5f, 0.5f, 0.0f),
                glm::vec4(8.0f)
            };

            SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform, sizeof(glm::mat4) * 4);
            SDL_PushGPUFragmentUniformData(command_buffer, 1, &material, sizeof(Material));

            RenderService::Get().DrawMesh(render_pass, mCubeMesh);
        }

        // Draw light sources
        RenderService::Get().UsePipeline(render_pass, "light_source");

        for (const glm::vec3 &light_position : sLightPositions) {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, light_position);
            model_matrix = glm::scale(model_matrix, glm::vec3(0.2f));
            glm::mat4 mvp = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix() * model_matrix;

            SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
            RenderService::Get().DrawMesh(render_pass, mCubeMesh);
        }

        // BALL POSITION TESTING
        // Draw ball as a cube
        JPH::Vec3 ball_position = body_interface.GetCenterOfMassPosition(mBallID);
        JPH::Quat ball_rotation = body_interface.GetRotation(mBallID);

        glm::quat glm_ball_rotation = glm::quat(ball_rotation.GetW(), ball_rotation.GetX(), ball_rotation.GetY(), ball_rotation.GetZ());

        glm::mat4 ball_model_matrix = glm::mat4(1.0f);
        ball_model_matrix = glm::translate(ball_model_matrix, glm::vec3(ball_position.GetX(), ball_position.GetY(), ball_position.GetZ()));
        ball_model_matrix *= glm::mat4_cast(glm_ball_rotation);
        ball_model_matrix = glm::scale(ball_model_matrix, glm::vec3(1.0f));

        glm::mat4 mvp = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix() * ball_model_matrix;

        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
        RenderService::Get().DrawMesh(render_pass, mBallMesh);

        SDL_EndGPURenderPass(render_pass);

        if (RenderService::Get().GetMSAATexture() != nullptr) {
            SDL_GPUBlitInfo blit_info;
            SDL_zero(blit_info);

            blit_info.source.texture = RenderService::Get().GetResolveTexture();
            blit_info.source.w = Context::Get().GetWindowWidth();
            blit_info.source.h = Context::Get().GetWindowHeight();

            blit_info.destination.texture = swapchain_texture;
            blit_info.destination.w = Context::Get().GetWindowWidth();
            blit_info.destination.h = Context::Get().GetWindowHeight();

            blit_info.load_op = SDL_GPU_LOADOP_DONT_CARE;
            blit_info.filter = SDL_GPU_FILTER_LINEAR;

            SDL_BlitGPUTexture(command_buffer, &blit_info);
        }
    }

    SDL_SubmitGPUCommandBuffer(command_buffer);
}
