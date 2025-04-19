#version 450

layout(binding = 1) uniform sampler2D baseColorTexture;
layout(binding = 2) uniform sampler2D unlitColorTexture;
layout(binding = 3) uniform sampler2D normalMapTexture;
layout(binding = 4) uniform sampler2D specularTexture;
layout(binding = 5) uniform sampler2D overlayColorTexture;

layout(location = 0) in vec3 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;

layout(location = 0) out vec4 outColor;

void main() {
    vec3 N = normalize(worldNormal);
    vec3 T = normalize(worldTangent);
    // fragment tangent: after interpolation it might no longer be exactly perpendicular to N.
    // Perform Gram-Schmidt
    T = normalize(T - dot(T, N) * N);
    // Bi-normal
    vec3 B = normalize(cross(N, T));
    mat3 TBN_to_world = mat3(T, B, N);
    mat3 world_to_TBN = transpose(TBN_to_world); // Because its an orthonormal matrix inverse is just transpose.

    vec3 lightPosition = vec3(0.0, 0.0, 0.0);
    vec3 lightDir = normalize(lightPosition - worldPosition.xyz);

    vec3 normalTBN = vec3(0.0, 0.0, 1.0);
    //vec3 normalTBN = texture(normalMapTexture, fragTexCoord).rgb * 2.0 - 1.0;
    vec3 lightDirTBN = world_to_TBN * lightDir;
    float diffuseDot = dot(normalTBN, lightDirTBN);
    
    vec3 baseColor = texture(baseColorTexture, fragTexCoord).rgb;
    vec3 unlitColor = texture(unlitColorTexture, fragTexCoord).rgb;
    vec3 overlayColor = texture(overlayColorTexture, fragTexCoord).rgb;

    vec3 color = mix(unlitColor, baseColor, smoothstep(-0.2, 0.2, diffuseDot));
    outColor = vec4(color, 1.0); //vec4(mix(color, overlayColor/2.0, overlayColor.r), 1.0);
}
