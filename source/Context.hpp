#pragma once

#include "macros/singleton.hpp"
#include "macros/types.hpp"

#include "ContentManager.hpp"

#include <EASTL/string.h>
#include <SDL3/SDL.h>

struct ContextCreateInfo {
    const char *mTitle;
    i32 mWidth;
    i32 mHeight;
};

class Context {
MAKE_SINGLETON(Context)
private:
    SDL_Window *mWindow;
    ContentManager mContentManager;

    i32 mWindowWidth;
    i32 mWindowHeight;
    bool mIsWindowResized;

public:
    bool Initialize(const ContextCreateInfo &inCreateInfo);
    void Shutdown();

    void Update();

    inline SDL_Window *GetWindow() const {
        return mWindow;
    }

    inline ContentManager &GetContent() {
        return mContentManager;
    }

    inline i32 GetWindowWidth() const {
        return mWindowWidth;
    }

    inline i32 GetWindowHeight() const {
        return mWindowHeight;
    }

    inline bool IsWindowResized() const {
        return mIsWindowResized;
    }

    inline eastl::string GetBasePath() const {
        return SDL_GetBasePath();
    }
};
