#version 450


layout(set = 0, binding = 0) uniform sampler2D blurTexture; // Input blur texture
layout(set = 0, binding = 1) uniform sampler2D sceneTexture; // Input scene texture

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {

    vec4 blurColor = texture(blurTexture, fragTexCoord);   // Sample the blur texture
    vec4 sceneColor = texture(sceneTexture, fragTexCoord); // Sample the scene texture

    vec3 hdrColor = sceneColor.rgb + blurColor.rgb; // Combine the two textures
    vec3 result = vec3(1.0) - exp(-hdrColor * 1.0f); // Apply HDR tone mapping
    
    outColor = vec4(result, 1.); // Output the final color
}