#version 450

layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec3 Normal;

layout (location = 0) out vec4 FragColor;

layout (binding = 0, set = 3) uniform UBO {
    vec4 lightColor;
    vec4 lightPos;
    vec4 viewPos;
};

void main() {
    float ambientStrength = 0.1;
    vec3 ambient = ambientStrength * lightColor.xyz;

    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos.xyz - FragPos);

    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * lightColor.xyz;

    float specularStrength = 0.5;
    vec3 viewDir = normalize(viewPos.xyz - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);

    float spec = pow(max(dot(viewDir, reflectDir), 0.0), 32);
    vec3 specular = specularStrength * spec * lightColor.xyz;

    vec3 result = (ambient + diffuse + specular) * Color.xyz;
    FragColor = vec4(result, 1.0);
}
