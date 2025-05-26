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
    vec3 center = pc.model[3].xyz;
    vec3 lightPosition = vec3(0.0, 0.0, 0.0);
    vec3 lightDir = normalize(lightPosition - center); // Vector from center to light position
    vec3 viewDir = normalize(si.cameraPosition - worldPosition.xyz); // Vector from fragment position to camera position
    vec3 reflectDir = reflect(-lightDir, worldNormal);

    float NdotL = dot(worldNormal, lightDir); // Normal dot Light in TBN 

    vec3 unlitColor = vec3(0.0); // Unlit color, can be adjusted
    vec3 litColor = texture(baseColorTexture, fragTexCoord).rgb;

    float mixAmount = 1. / (1. + exp(-20. * (NdotL - 0.15)));
    vec3 color = mix(unlitColor, litColor, mixAmount).rgb;


    //Specular
    float specPower = dot(reflectDir, viewDir);
    color += mixAmount * pow(specPower, 4.0) * 0.2;

    // // Specular (Phong)
    // //vec3 halfVector = normalize(lightDirTBN + viewDirTBN);
    // float NdotR = max(dot(viewDirTBN, reflectDirTBN), 0.0);
    // float specularStrength = 0.3; // Adjust for specular strength
    // float shininess = 4.0;         // Adjust for sharpness
    // float spec = pow(NdotR, shininess);
    // vec3 specular =  spec * si.lightColor * specularStrength;
    // if (NdotL < 0.0) {
    //     specular = vec3(0.0);
    // }

    // // Final color
    // vec3 litColor = ambient + diffuse + specular;
    // vec3 unlitColor = ambient; // Unlit color, can be adjusted



    outColor = vec4(color, 1.0);

    // float mixAmount = 1.0;
    // mixAmount = 1. / (1. + exp(-20. * normalDot));
    // mixAmount = clamp(mixAmount, 0.0, 1.0);
    // vec3 color = mix(unlitColor, baseColor, mixAmount).rgb;
   
    // float reflectRatio = 0.15;

    // vec3 viewDir = normalize(si.cameraPosition - worldPosition.xyz);
    // vec3 viewDirTBN = world_to_TBN * viewDir;
    // vec3 reflectDirTBN = reflect(lightDirTBN, normalTBN);

    // float specPower = clamp(max(dot(viewDirTBN, -reflectDirTBN),0.), 0.0, 1.0);
    // color += reflectRatio * pow(specPower, 2.0) * si.lightColor * max(normalDot, 0.0);

    // outColor = vec4(color, 1.0);
}

