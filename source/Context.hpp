#pragma once

#include "singleton.hpp"
#include <string>

// Forward declarations
struct SDL_GPUDevice;
struct SDL_Window;

class Context {
    MAKE_SINGLETON(Context)

private:
    SDL_GPUDevice *device;
    SDL_Window *window;

    std::string base_path;

public:
    bool Initialize();

    inline SDL_GPUDevice *GetDevice() const {
        return device;
    }

    inline SDL_Window *GetWindow() const {
        return window;
    }

    inline std::string GetBasePath() const {
        return base_path;
    }
};
