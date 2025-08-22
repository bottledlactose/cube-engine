#pragma once

#include <glm/glm.hpp>

struct PositionNormalTextureVertex {
    glm::vec3 mPosition;
    glm::vec3 mNormal;
    glm::vec2 mTexCoords;
};
