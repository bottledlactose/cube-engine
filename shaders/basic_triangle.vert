#version 450

layout (location = 0) in vec3 Position;
layout (location = 1) in vec3 Normal;

layout (location = 0) out vec3 outFragPos;
layout (location = 1) out vec3 outNormal;

layout (binding = 0, set = 1) uniform UBO {
    mat4x4 projection;
    mat4x4 view;
    mat4x4 model;
    mat4x4 model_inverse_transpose;
};

void main() {
    outNormal = mat3(model_inverse_transpose) * Normal;  
    outFragPos = vec3(model * vec4(Position, 1.0));
    gl_Position = projection * view * model * vec4(Position, 1);
}
