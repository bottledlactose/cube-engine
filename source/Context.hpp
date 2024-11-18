#pragma once

#include "macros/singleton.hpp"

#include "ContentManager.hpp"

#include <EASTL/string.h>
#include <SDL3/SDL.h>

struct ContextCreateInfo {
    const char *mTitle;
    int mWidth;
    int mHeight;
};

class Context {
MAKE_SINGLETON(Context)
private:
    SDL_Window *mWindow;
    ContentManager mContentManager;

    int mWindowWidth;
    int mWindowHeight;
    bool mIsWindowResized;

    int mTargetFPS = 60;

    Uint64 mPreviousTime;
    float mDeltaTime;

public:
    bool Initialize(const ContextCreateInfo &inCreateInfo);
    void Shutdown();

    void BeginFrame();
    void EndFrame();

    inline SDL_Window *GetWindow() const {
        return mWindow;
    }

    inline ContentManager &GetContent() {
        return mContentManager;
    }

    inline int GetWindowWidth() const {
        return mWindowWidth;
    }

    inline int GetWindowHeight() const {
        return mWindowHeight;
    }

    inline bool IsWindowResized() const {
        return mIsWindowResized;
    }

    inline float GetDeltaTime() const {
        return mDeltaTime;
    }

    inline eastl::string GetBasePath() const {
        return SDL_GetBasePath();
    }
};
