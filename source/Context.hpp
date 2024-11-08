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

public:
    bool Initialize(const ContextCreateInfo &inCreateInfo);
    void Shutdown();

    inline SDL_Window *GetWindow() const {
        return mWindow;
    }

    inline ContentManager &GetContent() {
        return mContentManager;
    }

    inline eastl::string GetBasePath() const {
        return SDL_GetBasePath();
    }
};
