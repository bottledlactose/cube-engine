#version 450

layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec3 Normal;

layout (location = 0) out vec4 FragColor;

layout (binding = 0, set = 3) uniform UBO {
    vec4 lightColor;
    vec4 lightPos;
};

void main() {

    // testing

    float ambientStrength = 0.2;
    vec3 ambient = ambientStrength * lightColor.xyz;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightColor.xyz - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor.xyz;

    vec3 result = (diffuse + ambient) * Color.xyz;
    //FragColor = vec4(result, 1.0);
    //FragColor = vec4(Color, 1.0);

    // TODO: FragPos is not being set yet

    FragColor = vec4(FragPos, 1.0);
}
