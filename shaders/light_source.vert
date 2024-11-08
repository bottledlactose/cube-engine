#version 450

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec4 Color;

layout (location = 0) out vec4 outColor;

layout (binding = 0, set = 1) uniform UBO {
    mat4 mvp;
};

void main() {
    gl_Position = mvp * vec4(Position, 1);
}
