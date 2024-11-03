#pragma once

#include <cstdint>

struct PositionNormalColorVertex {
    float x, y, z;
    float nx, ny, nz;
    uint8_t r, g, b, a;
};
