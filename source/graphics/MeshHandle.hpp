#pragma once

#include "macros/types.hpp"
#include <SDL3/SDL.h>

struct MeshHandle {
    SDL_GPUBuffer *mVertexBuffer;
    SDL_GPUBuffer *mIndexBuffer;
    u32 mVertexSize;
    u32 mIndexSize;
    u32 mVertexCount;
    u32 mIndexCount;
};
