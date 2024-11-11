#version 450

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
    float shininess;
};

struct Light {
    vec4 position;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec3 Normal;

layout (location = 0) out vec4 FragColor;

layout (binding = 0, set = 3) uniform UBO {
    //vec4 lightColor;
    //vec4 lightPos;
    vec4 viewPos;
    Material material;
    vec3 _padding; // padding needed to align the struct to a vec4
    Light light;
};

void main() {
    // ambient
    vec3 ambient = light.ambient.xyz * material.ambient.xyz;

    // diffuse
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(light.position.xyz - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = light.diffuse.xyz * (diff * material.diffuse.xyz);

    // specular
    vec3 viewDir = normalize(viewPos.xyz - FragPos);
    vec3 reflectDir = reflect(-lightDir, norm);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess);
    vec3 specular = light.specular.xyz * (spec * material.specular.xyz);

    vec3 result = (ambient + diffuse + specular) * Color.xyz;
    FragColor = vec4(result, 1.0);
}
