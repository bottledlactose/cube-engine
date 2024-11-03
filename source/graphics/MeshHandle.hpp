#pragma once

#include "macros/types.hpp"
#include <SDL3/SDL.h>

struct MeshHandle {
    SDL_GPUBuffer *vertex_buffer;
    u32 vertex_size;
};
