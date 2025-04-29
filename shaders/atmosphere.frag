#version 450

// Per-frame variables (set 0 is per-frame descriptor set)
layout(set = 0, binding = 0) uniform SceneInfo {
    mat4 view;
    mat4 proj;

    float time;
    vec3 cameraPosition;
    vec3 lightColor;
} si;

// Per-model variabales that change a lot 
layout(push_constant) uniform PushConstants {
    mat4 model;
    uint objectId;
} pc;

// Per-model variables that don't change a lot (set 1 is per-model descriptor set)
layout(set = 1, binding = 0) uniform atmosphereInfo {
    vec3 color;
} ai;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldViewPosition;
layout(location = 6) in vec3 worldViewNormal;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 center = pc.model[3].xyz;

    vec3 lightPosition = vec3(0.0, 0.0, 0.0);
    vec3 lightDir = normalize(lightPosition - center); // Calculate light direction in world space

    float normalDot = dot(worldNormal, lightDir); // Calculate the dot product between the normal and light direction
    float mixAmount = 1. / (1. + exp(-7. * (normalDot + 0.1))); // Calculate the mix amount based on the dot product

    vec3 viewDir = normalize(si.cameraPosition - worldPosition.xyz);
    float raw_intensity = clamp(1.0 - dot(viewDir, worldNormal), 0.0, 1.0);
    float intensity = pow(raw_intensity, 7.0);

    outColor = vec4(ai.color, intensity) * mixAmount;
}



