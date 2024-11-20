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

#include "graphics/vertices/PositionNormalTextureVertex.hpp"
#include "Context.hpp"
#include "graphics/RenderService.hpp"
#include "physics/PhysicsManager.hpp"
#include "Camera.hpp"
#include "Scene.hpp"

#define EASTL_DEFINE_OPERATOR_IMPL(...) void *__cdecl operator new[](size_t size, __VA_ARGS__) { return new uint8_t[size]; }

// One-time definitions of operator new[] for EASTL
EASTL_DEFINE_OPERATOR_IMPL(const char*, int, unsigned, const char*, int)
EASTL_DEFINE_OPERATOR_IMPL(size_t, size_t, const char*, int, unsigned int, const char*, int)

#include <EASTL/vector.h>

static Scene scene;
//static Camera camera(45.0f, 0.0f, -90.0f, 5.0f);

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize({ "boomblox", 1270, 720 })) {
        return SDL_APP_FAILURE;
    }

    scene.Initialize();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

    Context::Get().BeginFrame();

    if (Context::Get().IsWindowResized()) {
        RenderService::Get().SetViewport(
            Context::Get().GetWindowWidth(),
            Context::Get().GetWindowHeight()
        );
    }

    scene.Update();
    scene.Draw();

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

            // if (isRightMouseButtonDown) {
            //     camera.SetYaw(camera.GetYaw() + event->motion.xrel);
            //     camera.SetPitch(camera.GetPitch() + event->motion.yrel);
            // }

            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:


            // // print throw direction
            // if (event->button.button == SDL_BUTTON_LEFT) {
            //     glm::vec3 camera_position = camera.GetPosition();
            //     glm::vec3 throw_direction = camera.GetThrowDirection(event->button.x, event->button.y, Context::Get().GetWindowWidth(), Context::Get().GetWindowHeight());
            //     printf("World position: %f, %f, %f\n", throw_direction.x, throw_direction.y, throw_direction.z);

            //     // invert throw direction
            //     throw_direction = -throw_direction;

            //     // Spawn a box at the throw direction
            //     // remove old ball first
            //     physics_manager.DestroyBody(ball_id);
            //     ball_id = physics_manager.CreateBall(JPH::Vec3(camera_position.x, camera_position.y, camera_position.z), 0.5f);

            //     // Throw the ball towards the blocks
            //     physics_manager.GetBodyInterface().AddLinearVelocity(ball_id, JPH::Vec3(throw_direction.x * 20.0f, throw_direction.y * 20.0f, throw_direction.z * 20.0f));


            // }


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
            //camera.SetDistance(camera.GetDistance() - event->wheel.y);
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    scene.Shutdown();
    Context::Get().GetContent().Unload();
    Context::Get().Shutdown();
}
