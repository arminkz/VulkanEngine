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

void main() {
    gl_Position = mvp.proj * mvp.view * pc.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}