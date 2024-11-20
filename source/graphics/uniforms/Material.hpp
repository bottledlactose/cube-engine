#pragma once

#include <glm/glm.hpp>

struct Material {
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
    glm::vec4 shininess;
};
