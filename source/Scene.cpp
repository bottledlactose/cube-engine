#include "Scene.hpp"

#include "macros/log.hpp"

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

#include "Transform.hpp"

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
        // Invert the throw direction
        throw_direction = -throw_direction;

        // Spawn a box at the camera position
        mPhysicsManager.DestroyBody(mBallID);
        mBallID = mPhysicsManager.CreateBall(JPH::Vec3(camera_position.x, camera_position.y, camera_position.z), 0.5f);

        // Throw the ball towards the blocks
        mPhysicsManager.GetBodyInterface().AddLinearVelocity(mBallID, JPH::Vec3(throw_direction.x * 20.0f, throw_direction.y * 20.0f, throw_direction.z * 20.0f));
    }

    mPhysicsManager.Update();
}

void Scene::Draw() {

    RenderState *state = RenderService::Get().BeginPass();
    if (state == nullptr) {
        LOG_ERROR("Unable to begin render pass");
        return;
    }

    RenderService::Get().UsePipeline(state->mRenderPass, "default_mesh");

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

    SDL_PushGPUFragmentUniformData(state->mCommandBuffer, 0, &fragment_uniform, sizeof(FragmentUniform));

    JPH::BodyInterface &body_interface = mPhysicsManager.GetBodyInterface();

    for (const JPH::BodyID &body_id : mCubeBodies) {
        Transform model_transform = Transform::FromBody(body_interface, body_id);

        glm::mat4 model_matrix = model_transform.GetModelMatrix();
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

        SDL_PushGPUVertexUniformData(state->mCommandBuffer, 0, &vertex_uniform, sizeof(glm::mat4) * 4);
        SDL_PushGPUFragmentUniformData(state->mCommandBuffer, 1, &material, sizeof(Material));

        RenderService::Get().DrawMesh(state->mRenderPass, mCubeMesh);
    }

    // Draw light sources
    RenderService::Get().UsePipeline(state->mRenderPass, "light_source");

    for (const glm::vec3 &light_position : sLightPositions) {

        Transform model_transform;
        model_transform.mPosition = light_position;
        model_transform.mScale = glm::vec3(0.2f);

        glm::mat4 model_matrix = model_transform.GetModelMatrix();
        glm::mat4 mvp = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix() * model_matrix;

        SDL_PushGPUVertexUniformData(state->mCommandBuffer, 0, &mvp, sizeof(glm::mat4));
        RenderService::Get().DrawMesh(state->mRenderPass, mCubeMesh);
    }

    Transform ball_transform = Transform::FromBody(body_interface, mBallID);
    glm::mat4 ball_model_matrix = ball_transform.GetModelMatrix();

    glm::mat4 mvp = mCamera.GetProjectionMatrix() * mCamera.GetViewMatrix() * ball_model_matrix;

    SDL_PushGPUVertexUniformData(state->mCommandBuffer, 0, &mvp, sizeof(glm::mat4));
    RenderService::Get().DrawMesh(state->mRenderPass, mBallMesh);

    RenderService::Get().EndPass(state);

    SDL_SubmitGPUCommandBuffer(state->mCommandBuffer);
}
