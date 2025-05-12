#version 450


layout(set = 0, binding = 0) uniform BlurSettings
{
	float blurScale;
	float blurStrength;
} bs;

layout(set = 0, binding = 1) uniform sampler2D samplerColor; // Input texture (from the previous pass)

layout(constant_id = 0) const int blurDirection = 0;         // Specialization: 0 for vertical, 1 for horizontal

layout(location = 0) in vec2 fragTexCoord;

layout(location = 0) out vec4 outColor;


void main() {
    float weight[5] = float[] (0.227027, 0.1945946, 0.1216216, 0.054054, 0.016216);

    vec2 texOffset = 1.0 / textureSize(samplerColor, 0) * bs.blurScale;
    vec3 result = texture(samplerColor, fragTexCoord).rgb * weight[0]; // Current fragment's color
    for (int i = 1; i < 5; i++) {
        if (blurDirection == 1)
		{
			// H
			result += texture(samplerColor, fragTexCoord + vec2(texOffset.x * i, 0.0)).rgb * weight[i] * bs.blurStrength;
			result += texture(samplerColor, fragTexCoord - vec2(texOffset.x * i, 0.0)).rgb * weight[i] * bs.blurStrength;
		}
		else
		{
			// V
			result += texture(samplerColor, fragTexCoord + vec2(0.0, texOffset.y * i)).rgb * weight[i] * bs.blurStrength;
			result += texture(samplerColor, fragTexCoord - vec2(0.0, texOffset.y * i)).rgb * weight[i] * bs.blurStrength;
		}
    }

    outColor = vec4(result, 1.0); // Output the final color
}