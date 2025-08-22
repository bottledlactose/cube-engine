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
#include "InputService.hpp"

#define EASTL_DEFINE_OPERATOR_IMPL(...) void *__cdecl operator new[](size_t size, __VA_ARGS__) { return new uint8_t[size]; }

// One-time definitions of operator new[] for EASTL
EASTL_DEFINE_OPERATOR_IMPL(const char*, int, unsigned, const char*, int)
EASTL_DEFINE_OPERATOR_IMPL(size_t, size_t, const char*, int, unsigned int, const char*, int)

#include <EASTL/vector.h>

static Scene scene;

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv) {

    if (!Context::Get().Initialize({ "Cube Engine", 1270, 720 })) {
        return SDL_APP_FAILURE;
    }

    scene.Initialize();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate) {

    Context::Get().BeginFrame();

    if (Context::Get().IsWindowResized()) {
        // Resize the viewport
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

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event) {

    switch (event->type) {
        case SDL_EVENT_QUIT:
            return SDL_APP_SUCCESS;
        case SDL_EVENT_MOUSE_MOTION:
            InputService::Get().SetMousePosition(event->motion.x, event->motion.y);
            InputService::Get().SetMouseDelta(event->motion.xrel, event->motion.yrel);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
            if (event->button.button == SDL_BUTTON_LEFT) {
                InputService::Get().SetLeftMouseDown(true);
            }

            if (event->button.button == SDL_BUTTON_RIGHT) {
                InputService::Get().SetRightMouseDown(true);
            }
            break;
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT) {
                InputService::Get().SetLeftMouseDown(false);
            }

            if (event->button.button == SDL_BUTTON_RIGHT) {
                InputService::Get().SetRightMouseDown(false);
            }
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            InputService::Get().SetScrollY(event->wheel.y);
            break;
    }

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result) {
    scene.Shutdown();
    Context::Get().GetContent().Unload();
    Context::Get().Shutdown();
}
