#pragma once

#include <glm/glm.hpp>

struct PointLight {
    glm::vec4 position;
    float constant;
    float linear;
    float quadratic;
    float _padding;
    glm::vec4 ambient;
    glm::vec4 diffuse;
    glm::vec4 specular;
};
