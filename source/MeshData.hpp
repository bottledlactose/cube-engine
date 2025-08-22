#pragma once

#include <SDL3/SDL.h>

#include "graphics/vertices/PositionNormalTextureVertex.hpp"

#include <EASTL/vector.h>

struct MeshData {
    eastl::vector<PositionNormalTextureVertex> vertices;
    eastl::vector<Uint16> indices;
};
