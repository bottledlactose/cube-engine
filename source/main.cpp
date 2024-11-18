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
#include "Camera.hpp"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#define _EASTL_DEFINE_OPERATOR_IMPL(...) void *__cdecl operator new[](__VA_ARGS__) { return new uint8_t[size]; }

// One-time definitions of operator new[] for EASTL
_EASTL_DEFINE_OPERATOR_IMPL(size_t size, const char*, int, unsigned, const char*, int)
_EASTL_DEFINE_OPERATOR_IMPL(size_t size, size_t, size_t, const char*, int, unsigned int, const char*, int)

#include <EASTL/vector.h>

static MeshHandle *mesh_handle = nullptr;

struct Material {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 shininess;
};

struct DirectionalLight {
    glm::vec4 direction;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

struct PointLight {
    glm::vec4 position;
    float constant;
    float linear;
    float quadratic;
    float _padding;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};

struct FragmentUniform {
    glm::vec4 camera_position;
    Material material;
    DirectionalLight directional_light;
    PointLight point_light[4];
};

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

static eastl::vector<glm::vec3> light_positions = {
    glm::vec3(2.0f, 0.2f, 2.0f),
    glm::vec3(-2.0f, 0.2f, 2.0f),
    glm::vec3(2.0f, 0.2f, -2.0f),
    glm::vec3(-2.0f, 0.2f, -2.0f)
};

static JPH::BodyID floor_id;
static JPH::BodyID ball_id;

static Camera camera(45.0f, 0.0f, -90.0f, 5.0f);

// test mesh data
struct MeshData {
    eastl::vector<PositionNormalColorVertex> vertices;
    eastl::vector<Uint16> indices;
};

static MeshData AssimpProcessMesh(const aiMesh *mesh, const aiScene *scene) {
    eastl::vector<PositionNormalColorVertex> vertices;
    eastl::vector<Uint16> indices;

    for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
        PositionNormalColorVertex vertex;

        vertex.x = mesh->mVertices[i].x;
        vertex.y = mesh->mVertices[i].y;
        vertex.z = mesh->mVertices[i].z;

        vertex.nx = mesh->mNormals[i].x;
        vertex.ny = mesh->mNormals[i].y;
        vertex.nz = mesh->mNormals[i].z;

        if (mesh->mColors[0]) {
            vertex.r = mesh->mColors[0][i].r;
            vertex.g = mesh->mColors[0][i].g;
            vertex.b = mesh->mColors[0][i].b;
        } else {
            vertex.r = 1.0f;
            vertex.g = 1.0f;
            vertex.b = 1.0f;
        }

        vertices.push_back(vertex);
    }

    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        aiFace face = mesh->mFaces[i];
        for (unsigned int j = 0; j < face.mNumIndices; j++) {
            indices.push_back(face.mIndices[j]);
        }
    }

    return {
        .vertices = vertices,
        .indices = indices
    };
}

static MeshData AssimpProcessNode(const aiNode *node, const aiScene *scene) {
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        MeshData data = AssimpProcessMesh(mesh, scene);

        return data; // only process the first mesh for now
    }

    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        return AssimpProcessNode(node->mChildren[i], scene);
    }
}

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize({ "boomblox", 1270, 720 })) {
        return SDL_APP_FAILURE;
    }

    // TEST ASSIMP
    Assimp::Importer importer;
    const aiScene *scene = importer.ReadFile("../../content/cube.obj", aiProcess_Triangulate | aiProcess_FlipUVs);
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
        fprintf(stderr, "ERROR::ASSIMP::%s\n", importer.GetErrorString());
        return SDL_APP_FAILURE;
    }

    MeshData mesh_data = AssimpProcessNode(scene->mRootNode, scene);

    // Create a mesh from the data
    // TODO: Clean up input parameters (separate unit sizes from counts to ease things up)
    mesh_handle = RenderService::Get().CreateMesh(
        mesh_data.vertices.data(),
        sizeof(PositionNormalColorVertex) * mesh_data.vertices.size(),
        mesh_data.vertices.size(),
        mesh_data.indices.data(),
        sizeof(Uint16) * mesh_data.indices.size(),
        mesh_data.indices.size()
    );

    floor_id = PhysicsService::Get().CreateBox(JPH::Vec3(0.0f, -2.0f, 0.0f), JPH::Vec3(100.0f, 0.1f, 100.0f));

    ball_id = PhysicsService::Get().CreateBall(JPH::Vec3(-5.0f, 0.0f, 0.0f), 0.5f);
    // Throw the ball towards the blocks
    PhysicsService::Get().GetBodyInterface().AddLinearVelocity(ball_id, JPH::Vec3(20.0f, 0.0f, 0.0f));

    for (const JPH::Vec3 &position : box_positions) {
        JPH::BodyID box_id = PhysicsService::Get().CreateBox(position, JPH::Vec3(0.5f, 0.5f, 0.5f), true);
        PhysicsService::Get().GetBodyInterface().SetLinearVelocity(box_id, JPH::Vec3(0.0f, -1.0f, 0.0f));
        bodies.push_back(box_id);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

    Context::Get().BeginFrame();

    if (Context::Get().IsWindowResized()) {
        // Resize the camera aspect ratio
        camera.SetAspectRatio(Context::Get().GetWindowWidth() / (float)Context::Get().GetWindowHeight());
        // Resize renderer
        RenderService::Get().SetViewport(Context::Get().GetWindowWidth(), Context::Get().GetWindowHeight());
    }

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

        for (const JPH::BodyID &body_id : bodies) {
            JPH::Vec3 position = body_interface.GetCenterOfMassPosition(body_id);
            JPH::Quat rotation = body_interface.GetRotation(body_id);

            glm::quat glm_rotation = glm::quat(rotation.GetW(), rotation.GetX(), rotation.GetY(), rotation.GetZ());

            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, glm::vec3(position.GetX(), position.GetY(), position.GetZ()));
            model_matrix *= glm::mat4_cast(glm_rotation);

            glm::mat4 model_inverse_transpose = glm::transpose(glm::inverse(model_matrix));

            glm::mat4 vertex_uniform[4] = {
                camera.GetProjectionMatrix(),
                camera.GetViewMatrix(),
                model_matrix,
                model_inverse_transpose,
            };

            FragmentUniform fragment_uniform = {
                glm::vec4(camera.GetPosition(), 1.0f),
                {
                    glm::vec4(1.0f, 0.5f, 0.31f, 0.0f),
                    glm::vec4(1.0f, 0.5f, 0.31f, 0.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 0.0f),
                    glm::vec4(8.0f)
                },
                {
                    glm::vec4(-0.2f, -1.0f, -0.3f, 1.0f),
                    glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                    glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
                    glm::vec4(0.8f, 0.8f, 0.8f, 1.0f)
                },
                {
                    {
                        glm::vec4(light_positions[0], 1.0f),
                        1.0f,
                        0.09f,
                        0.032f,
                        0.0f,
                        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                    },
                    {
                        glm::vec4(light_positions[1], 1.0f),
                        1.0f,
                        0.09f,
                        0.032f,
                        0.0f,
                        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                    },
                    {
                        glm::vec4(light_positions[2], 1.0f),
                        1.0f,
                        0.09f,
                        0.032f,
                        0.0f,
                        glm::vec4(0.2f, 0.2f, 0.2f, 1.0f),
                        glm::vec4(0.6f, 0.6f, 0.6f, 1.0f),
                        glm::vec4(0.7f, 0.7f, 0.7f, 1.0f)
                    },
                    {
                        glm::vec4(light_positions[3], 1.0f),
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

            SDL_PushGPUVertexUniformData(command_buffer, 0, &vertex_uniform, sizeof(glm::mat4) * 4);
            SDL_PushGPUFragmentUniformData(command_buffer, 0, &fragment_uniform, sizeof(FragmentUniform));

            RenderService::Get().DrawMesh(render_pass, mesh_handle);
        }

        // Draw light sources
        RenderService::Get().UsePipeline(render_pass, "light_source");

        for (const glm::vec3 &light_position : light_positions) {
            glm::mat4 model_matrix = glm::mat4(1.0f);
            model_matrix = glm::translate(model_matrix, light_position);
            model_matrix = glm::scale(model_matrix, glm::vec3(0.2f));
            glm::mat4 mvp = camera.GetProjectionMatrix() * camera.GetViewMatrix() * model_matrix;

            SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
            RenderService::Get().DrawMesh(render_pass, mesh_handle);
        }

        // BALL POSITION TESTING
        // Draw ball as a cube
        JPH::Vec3 ball_position = body_interface.GetCenterOfMassPosition(ball_id);
        JPH::Quat ball_rotation = body_interface.GetRotation(ball_id);

        glm::quat glm_ball_rotation = glm::quat(ball_rotation.GetW(), ball_rotation.GetX(), ball_rotation.GetY(), ball_rotation.GetZ());

        glm::mat4 ball_model_matrix = glm::mat4(1.0f);
        ball_model_matrix = glm::translate(ball_model_matrix, glm::vec3(ball_position.GetX(), ball_position.GetY(), ball_position.GetZ()));
        ball_model_matrix *= glm::mat4_cast(glm_ball_rotation);
        ball_model_matrix = glm::scale(ball_model_matrix, glm::vec3(0.2f));

        glm::mat4 mvp = camera.GetProjectionMatrix() * camera.GetViewMatrix() * ball_model_matrix;

        SDL_PushGPUVertexUniformData(command_buffer, 0, &mvp, sizeof(glm::mat4));
        RenderService::Get().DrawMesh(render_pass, mesh_handle);

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

    Context::Get().EndFrame();

    return SDL_APP_CONTINUE;
}

// tesing
static bool isRightMouseButtonDown = false;

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_MOUSE_MOTION:

            if (isRightMouseButtonDown) {
                camera.SetYaw(camera.GetYaw() + event->motion.xrel);
                camera.SetPitch(camera.GetPitch() + event->motion.yrel);
            }

            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_RIGHT) {
                isRightMouseButtonDown = true;
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_RIGHT) {
                isRightMouseButtonDown = false;
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            camera.SetDistance(camera.GetDistance() - event->wheel.y);
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {

    for (const JPH::BodyID &body_id : bodies) {
        PhysicsService::Get().DestroyBody(body_id);
    }

    PhysicsService::Get().DestroyBody(floor_id);

    RenderService::Get().DestroyMesh(mesh_handle);

    Context::Get().Shutdown();
}
