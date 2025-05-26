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
layout(set = 1, binding = 1) uniform sampler2D unlitColorTexture;
layout(set = 1, binding = 2) uniform sampler2D normalMapTexture;
layout(set = 1, binding = 3) uniform sampler2D specularTexture;
layout(set = 1, binding = 4) uniform sampler2D overlayColorTexture;

layout(location = 0) in vec4 fragColor;
layout(location = 1) in vec2 fragTexCoord;
layout(location = 2) in vec4 worldPosition;
layout(location = 3) in vec3 worldNormal;
layout(location = 4) in vec3 worldTangent;
layout(location = 5) in vec3 worldViewPosition;
layout(location = 6) in vec3 normalView;

layout(location = 0) out vec4 outColor;


// 2D Random
float random (in vec3 st) {
    return fract(sin(dot(st,vec3(12.9898,78.233,23.112)))*12943.145);
}

// 3D noise
float noise (in vec3 _pos) {
    vec3 i_pos = floor(_pos);
    vec3 f_pos = fract(_pos);

    float i_time = floor(si.time*0.2);
    float f_time = fract(si.time*0.2);

    // 8 corners of a cube
    float aa = random(i_pos + i_time);
    float ab = random(i_pos + i_time + vec3(1., 0., 0.));
    float ac = random(i_pos + i_time + vec3(0., 1., 0.));
    float ad = random(i_pos + i_time + vec3(1., 1., 0.));
    float ae = random(i_pos + i_time + vec3(0., 0., 1.));
    float af = random(i_pos + i_time + vec3(1., 0., 1.));
    float ag = random(i_pos + i_time + vec3(0., 1., 1.));
    float ah = random(i_pos + i_time + vec3(1., 1., 1.));

    float ba = random(i_pos + (i_time + 1.));
    float bb = random(i_pos + (i_time + 1.) + vec3(1., 0., 0.));
    float bc = random(i_pos + (i_time + 1.) + vec3(0., 1., 0.));
    float bd = random(i_pos + (i_time + 1.) + vec3(1., 1., 0.));
    float be = random(i_pos + (i_time + 1.) + vec3(0., 0., 1.));
    float bf = random(i_pos + (i_time + 1.) + vec3(1., 0., 1.));
    float bg = random(i_pos + (i_time + 1.) + vec3(0., 1., 1.));
    float bh = random(i_pos + (i_time + 1.) + vec3(1., 1., 1.));

    // Smooth step
    vec3 t = smoothstep(0., 1., f_pos);
    float t_time = smoothstep(0., 1., f_time);

    // Mix 8 corners
    return 
    mix(
        mix(
            mix(mix(aa,ab,t.x), mix(ac,ad,t.x), t.y),
            mix(mix(ae,af,t.x), mix(ag,ah,t.x), t.y), 
        t.z),
        mix(
            mix(mix(ba,bb,t.x), mix(bc,bd,t.x), t.y),
            mix(mix(be,bf,t.x), mix(bg,bh,t.x), t.y), 
        t.z), 
    t_time);
}

#define NUM_OCTAVES 6
float fBm ( in vec3 _pos, in float sz) {
    float v = 0.0;
    float a = 0.2;
    _pos *= sz;

    vec3 angle = vec3(-0.001*si.time,0.0001*si.time,0.0004*si.time);
    mat3 rotx = mat3(1, 0, 0,
                    0, cos(angle.x), -sin(angle.x),
                    0, sin(angle.x), cos(angle.x));
    mat3 roty = mat3(cos(angle.y), 0, sin(angle.y),
                    0, 1, 0,
                    -sin(angle.y), 0, cos(angle.y));
    mat3 rotz = mat3(cos(angle.z), -sin(angle.z), 0,
                    sin(angle.z), cos(angle.z), 0,
                    0, 0, 1);

    for (int i = 0; i < NUM_OCTAVES; ++i) {
        v += a * noise(_pos);
        _pos = rotx * roty * rotz * _pos * 2.0;
        a *= 0.8;
    }
    return v;
}

void main() {
    vec3 st = worldPosition.xyz;

    vec3 q = vec3(0.);
    q.x = fBm( st, 5.);
    q.y = fBm( st + vec3(1.2,3.2,1.52), 5.);
    q.z = fBm( st + vec3(0.02,0.12,0.152), 5.);

    float n = fBm(st+q+vec3(1.82,1.32,1.09), 5.);

    vec3 color = vec3(0.);
    color = mix(vec3(.9, 0.15, 0.), vec3(.9, .7, .0), n*n);
    color = mix(color, vec3(.9, 0., 0.), q);
    outColor = vec4(color, 1.);
}