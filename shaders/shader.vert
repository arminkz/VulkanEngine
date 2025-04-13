#version 450

layout(binding = 0) uniform MVP { // Model-View-Projection matrix
    mat4 model;
    mat4 view;
    mat4 proj;
} mvp;

layout(location = 0) in vec3 inPosition; // Vertex position
layout(location = 1) in vec3 inColor;    // Vertex color
layout(location = 2) in vec2 inTexCoord; // Vertex texture coordinate

// Passed in from the vertex shader to the fragment shader
layout(location = 0) out vec3 fragColor;
layout(location = 1) out vec2 fragTexCoord;

void main() {
    gl_Position = mvp.proj * mvp.view * mvp.model * vec4(inPosition, 1.0);
    fragColor = inColor;
    fragTexCoord = inTexCoord;
}