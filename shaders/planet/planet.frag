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
} pc;

layout(set = 1, binding = 0) uniform sampler2D baseColorTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldViewPosition;
layout(location = 6) in vec3 normalView;

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
    mat3 world_to_TBN = transpose(TBN_to_world); // Because its an orthonormal matrix, its inverse is just transpose.

    vec3 center = pc.model[3].xyz;

    vec3 lightPosition = vec3(0.0, 0.0, 0.0);
    vec3 lightDir = normalize(lightPosition - center); // Calculate light direction in world space
    vec3 lightDirTBN = world_to_TBN * lightDir;

    vec3 normalTBN = vec3(0.0, 0.0, 1.0);
    float normalDot = dot(normalTBN, lightDirTBN);
    float diffuseDot = dot(normalTBN, lightDirTBN);
    
    float mixAmount = 1.0;
    vec3 color = fragColor.rgb;
    vec4 baseColor = texture(baseColorTexture, fragTexCoord);
    color = baseColor.rgb;

    vec4 unlitColor = vec4(0.0, 0.0, 0.0, 1.0);

    mixAmount = 1. / (1. + exp(-20. * normalDot));
    mixAmount *= 1.0 + 5.0 * (diffuseDot - normalDot);
    mixAmount = clamp(mixAmount, 0.0, 1.0);
    color = mix(unlitColor, baseColor, mixAmount).rgb;
   
    float reflectRatio = 0.12;
    reflectRatio += 0.03;

    vec3 viewDir = normalize(si.cameraPosition - worldPosition.xyz);
    vec3 viewDirTBN = world_to_TBN * viewDir;
    vec3 reflectDirTBN = reflect(-lightDirTBN, normalTBN);

    float specPower = clamp(max(dot(viewDirTBN, reflectDirTBN),0.), 0.0, 1.0);
    color += reflectRatio * pow(specPower, 2.0) * si.lightColor;

    outColor = vec4(color, 1.0);
}

