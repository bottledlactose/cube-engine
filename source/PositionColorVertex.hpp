#pragma once

#include <cstdint>

struct PositionColorVertex {
    float x, y, z;
    float nx, ny, nz;
    uint8_t r, g, b, a; // TODO: Ensure this is the correct format
};
