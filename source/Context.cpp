#include "Context.hpp"

#include "macros/log.hpp"

#include "graphics/RenderService.hpp"
#include "physics/PhysicsService.hpp"

bool Context::Initialize(const ContextCreateInfo &inCreateInfo) {
    // Initialize the SDL video subsystem, needed for window creation and rendering
    if (!SDL_InitSubSystem(SDL_INIT_VIDEO)) {
        LOG_ERROR("Unable to initialize SDL video subsystem: %s", SDL_GetError());
        return false;
    }

    // Create the main window for the game
    mWindow = SDL_CreateWindow(inCreateInfo.mTitle, inCreateInfo.mWidth, inCreateInfo.mHeight, SDL_WINDOW_RESIZABLE);
    if (mWindow == nullptr) {
        LOG_ERROR("Unable to create window: %s", SDL_GetError());
        return false;
    }

    // Begin initializating services here
    if (!RenderService::Get().Initialize(mWindow)) {
        return false;
    }

    if (!PhysicsService::Get().Initialize()) {
        return false;
    }

    return true;
}

void Context::Shutdown() {
    PhysicsService::Get().Shutdown();
    RenderService::Get().Shutdown();

    if (mWindow != nullptr) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Context::Update() {
    // Update and check window width and height
    int width, height;
    SDL_GetWindowSize(mWindow, &width, &height);

    if (width != mWindowWidth || height != mWindowHeight) {
        mWindowWidth = width;
        mWindowHeight = height;
        mIsWindowResized = true;
    } else {
        mIsWindowResized = false;
    }
}
