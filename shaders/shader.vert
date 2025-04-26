#version 450

layout(push_constant) uniform PushConstants {
    mat4 model;
} pc;

layout(set = 0, binding = 0) uniform MVP { // Model-View-Projection matrix (Set 0 is per-frame descriptor set)
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec4 inColor;    // Vertex color
layout(location = 2) in vec2 inTexCoord; // Vertex texture coordinate
layout(location = 3) in vec3 inNormal;   // Vertex normal
layout(location = 4) in vec3 inTangent;  // Vertex tangent

// Passed from the vertex shader to the fragment shader
layout(location = 0) out vec4 fragColor;
layout(location = 1) out vec2 fragTexCoord;
layout(location = 2) out vec4 worldPosition;
layout(location = 3) out vec3 worldNormal;
layout(location = 4) out vec3 worldTangent;

void main() {
    worldPosition = pc.model * vec4(inPosition, 1.0);
    worldNormal = (pc.model * vec4(inNormal, 0.0)).xyz;
    worldTangent = (pc.model * vec4(inTangent, 0.0)).xyz;

    gl_Position = mvp.proj * mvp.view * worldPosition;
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}