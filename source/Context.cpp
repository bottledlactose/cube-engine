#include "Context.hpp"

#include "macros/log.hpp"

#include "graphics/RenderService.hpp"
#include "InputService.hpp"

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

    // Initialize input service
    InputService::Get();

    return true;
}

void Context::Shutdown() {
    RenderService::Get().Shutdown();

    if (mWindow != nullptr) {
        SDL_DestroyWindow(mWindow);
        mWindow = nullptr;
    }

    SDL_QuitSubSystem(SDL_INIT_VIDEO);
}

void Context::BeginFrame() {
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

    // Calculate delta time (in milliseconds)
    Uint64 mCurrentTime = SDL_GetTicks();
    mDeltaTime = (mCurrentTime - mPreviousTime) / 1000.0f;
    mPreviousTime = mCurrentTime;
}

void Context::EndFrame() {
    // Update input service
    InputService::Get().Update();

    // Calculate the elapsed time for the frame
    Uint64 frame_end_time = SDL_GetTicks();
    Uint64 frame_duration = frame_end_time - mPreviousTime;

    // Calculate target frame duration for 60 FPS in milliseconds
    const Uint64 target_frame_duration = 1000 / mTargetFPS;
    
    // If the frame finished too quickly, add a delay to cap the frame rate
    if (frame_duration < target_frame_duration) {
        SDL_Delay(target_frame_duration - frame_duration);
    }
}
