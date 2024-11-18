#pragma once

#include <SDL3/SDL.h>

struct MeshHandle {
    SDL_GPUBuffer *mVertexBuffer;
    SDL_GPUBuffer *mIndexBuffer;
    Uint32 mVertexSize;
    Uint32 mIndexSize;
    Uint32 mVertexCount;
    Uint32 mIndexCount;
};
