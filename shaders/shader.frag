#version 450

layout(binding = 1) uniform materialUBO {
    int hasBaseColorTexture;
    int hasUnlitColorTexture;
    int hasNormalMapTexture;
    int hasSpecularTexture;
    int hasOverlayColorTexture;

    float ambientStrength;
    float specularStrength;
    float overlayOffset;

    int sunShadeMode;
} material;

layout(binding = 2) uniform sceneInfoUBO {
    float time;
    vec3 cameraPosition;
    vec3 lightColor;
} sceneInfo;

layout(binding = 3) uniform sampler2D baseColorTexture;
layout(binding = 4) uniform sampler2D unlitColorTexture;
layout(binding = 5) uniform sampler2D normalMapTexture;
layout(binding = 6) uniform sampler2D specularTexture;
layout(binding = 7) uniform sampler2D overlayColorTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;

layout(location = 0) out vec4 outColor;


const mat2 rot = mat2( 1.6,  1.2, -1.2,  1.6 );

vec2 distortUV(vec2 uv, float time) {
    // Sample noise texture to get offset values
    float noise = texture(overlayColorTexture, uv * 3.0 + time * 0.1).r;
    float noise2 = texture(overlayColorTexture, rot * uv * 3.0 + time * 0.1).r;
    vec2 offset = vec2(noise * 0.01, noise2 * 0.01); // distortion amount
    return uv + offset;
}

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

    vec3 lightPosition = vec3(0.0, 0.0, 0.0);
    vec3 lightDir = normalize(lightPosition - worldPosition.xyz); // Point light
    vec3 lightDirTBN = world_to_TBN * lightDir;

    vec3 normalTBN = vec3(0.0, 0.0, 1.0);
    if (material.hasNormalMapTexture == 1) {
        normalTBN = texture(normalMapTexture, fragTexCoord).rgb * 2.0 - 1.0;
    }
    float diffuseDot = dot(normalTBN, lightDirTBN);
    
    vec4 color = fragColor;
    if (material.hasBaseColorTexture == 1) {
        vec4 baseColor = texture(baseColorTexture, fragTexCoord);
        color = baseColor;

        if(material.hasUnlitColorTexture == 1) {
            vec4 unlitColor = texture(unlitColorTexture, fragTexCoord);
            unlitColor.rgb *= 5.f;
            color = mix(unlitColor, baseColor, smoothstep(-0.1, 0.01, diffuseDot));
        }
    }

    vec3 viewDir = normalize(sceneInfo.cameraPosition - worldPosition.xyz);
    vec3 viewDirTBN = world_to_TBN * viewDir;
    vec3 reflectDirTBN = reflect(-lightDirTBN, normalTBN);

    vec3 ambient = material.ambientStrength * sceneInfo.lightColor;
    vec3 diffuse = max(diffuseDot, 0.0) * sceneInfo.lightColor;
    vec3 specular = vec3(0.,0.,0.);

    vec3 specularStrength = material.specularStrength * vec3(1.);
    if(material.hasSpecularTexture == 1) {
        specularStrength *= texture(specularTexture, fragTexCoord).rgb;
    }
    specular = specularStrength * pow(max(dot(viewDirTBN, reflectDirTBN), 0.0), 8) * sceneInfo.lightColor;
    color = vec4(ambient + diffuse + specular, 1.0) * color;

    if (material.sunShadeMode == 1) {
        // Distort UV coordinates for turbulence
        vec2 uv = fragTexCoord;
        uv = distortUV(uv, sceneInfo.time * 0.3);

        // Scroll another noise layer for extra movement
        float scrollNoise = texture(overlayColorTexture, uv * 5.0 + vec2(sceneInfo.time * 0.05, 0.0)).r;
        float turbulence = scrollNoise * 0.5;

        // Final color from sun texture with turbulence applied
        vec3 baseColor = texture(baseColorTexture, uv + turbulence * 0.01).rgb;

        // Boost color to simulate brightness
        color = vec4(baseColor,1.0); //* vec3(1.5, 1.2, 0.9); // warm tone
    }

    outColor = color;
}

