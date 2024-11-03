#pragma once

#include "macros/singleton.hpp"

#include "MeshHandle.hpp"
#include <SDL3/SDL.h>

class Renderer {
MAKE_SINGLETON(Renderer)

private:
    SDL_GPUDevice *device;

public:
    bool Initialize(SDL_Window *window);
    void Shutdown();

    MeshHandle *CreateMesh(
        void *vertex_data, u32 vertex_size,
        void *index_data, u32 index_size
    ) const;
    void DestroyMesh(MeshHandle *mesh) const;

    inline SDL_GPUDevice *GetDevice() const {
        return device;
    }
};
