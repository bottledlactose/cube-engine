#version 450

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;
layout (location = 2) in vec3 Color;

layout (location = 0) out vec3 outColor;
layout (location = 1) out vec3 outFragPos;
layout (location = 2) out vec3 outNormal;

layout (binding = 0, set = 1) uniform UBO {
    mat4 mvp;
};

void main() {
    outNormal = Normal;
    //outFragPos = vec3(model * vec4(Position, 1.0));
    outColor = Color;
    gl_Position = mvp * vec4(Position, 1);
}
