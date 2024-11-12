#version 450

struct Material {
    vec4 ambient;
    vec4 diffuse;
    vec4 specular; 
    vec4 shininess;
};

struct DirectionalLight {
    vec4 direction;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

struct PointLight {    
    vec4 position;
    float constant;
    float linear;
    float quadratic;
    float _padding;
    vec4 ambient;
    vec4 diffuse;
    vec4 specular;
};

layout (location = 0) in vec3 Color;
layout (location = 1) in vec3 FragPos;
layout (location = 2) in vec3 Normal;

layout (location = 0) out vec4 FragColor;

layout (binding = 0, set = 3) uniform UBO {
    vec4 viewPos;
    Material material;
    DirectionalLight directional_light;
    PointLight point_lights[4];
};

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir);
vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir);

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(viewPos.xyz - FragPos);

    vec3 result = CalcDirectionalLight(directional_light, norm, viewDir);

    for (int i = 0; i < 4; i++) {
        result += CalcPointLight(point_lights[i], norm, FragPos, viewDir);
    }

    FragColor = vec4(result, 1.0);
}

vec3 CalcDirectionalLight(DirectionalLight light, vec3 normal, vec3 viewDir) {
    vec3 lightDir = normalize(-light.direction.xyz);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess.x);
    // combine results
    vec3 ambient = light.ambient.xyz * vec3(material.diffuse);
    vec3 diffuse = light.diffuse.xyz * diff * vec3(material.diffuse);
    vec3 specular = light.specular.xyz * spec * vec3(material.specular);

    return (ambient + diffuse + specular);
} 

vec3 CalcPointLight(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir) {
    vec3 lightDir = normalize(light.position.xyz - fragPos);
    // diffuse shading
    float diff = max(dot(normal, lightDir), 0.0);
    // specular shading
    vec3 reflectDir = reflect(-lightDir, normal);
    float spec = pow(max(dot(viewDir, reflectDir), 0.0), material.shininess.x);
    // attenuation
    float distance = length(light.position.xyz - fragPos);
    float attenuation = 1.0 / (light.constant + light.linear * distance + light.quadratic * (distance * distance));
    // combine results
    vec3 ambient = light.ambient.xyz * vec3(material.diffuse);
    vec3 diffuse  = light.diffuse.xyz * diff * vec3(material.diffuse);
    vec3 specular = light.specular.xyz * spec * vec3(material.specular);
    ambient  *= attenuation;
    diffuse  *= attenuation;
    specular *= attenuation;
    return (ambient + diffuse + specular);
}
