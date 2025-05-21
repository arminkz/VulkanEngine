#version 450

// Per-frame variables (set 0 is per-frame descriptor set)
layout(set = 0, binding = 0) uniform SceneInfo {
    mat4 view;
    mat4 proj;

    float time;
    vec3 cameraPosition;
    vec3 lightColor;
} si;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;

void main() {
    // draw a circle with smooth edges
    vec2 uv = fragTexCoord * 2.0 - 1.0; // Convert to NDC coordinates
    float dist = length(uv);
    float radius = 1.0;
    float pixelTickness = 0.5;
    float edge = pixelTickness * fwidth(dist);

    uv.x *= -1.0;

    if ((dist < radius - edge) || (dist > radius + edge)) {
        discard; // Discard fragments outside the circle
    }

    // Radial gradient
    float angle = atan(uv.y, uv.x);
    float gradient = mix(0.01, 0.1, (angle / 3.1415) - 0.2); // Smooth transition from center to edge

    outColor = vec4(1.0, 1.0, 1.0, gradient); // Set the color to white
}

