#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LocalPos;

uniform float uCloudCoverage;
uniform float uSimTime;
uniform int uNumLights;
uniform vec3 uLightPos[4];

void main() {
    vec3 norm = normalize(Normal);
    
    // Procedural multi-frequency cloud fractal pattern
    float c1 = sin(LocalPos.x * 12.0 + uSimTime * 0.02) * cos(LocalPos.y * 12.0);
    float c2 = sin(LocalPos.y * 24.0 + LocalPos.z * 18.0) * cos(LocalPos.x * 24.0);
    float c3 = sin(LocalPos.z * 48.0 - LocalPos.x * 32.0) * cos(LocalPos.y * 48.0);
    float cloudNoise = (c1 * 0.50 + c2 * 0.35 + c3 * 0.15) * 0.5 + 0.5;

    float cloudAlpha = smoothstep(1.0 - uCloudCoverage, 1.0, cloudNoise);
    if (cloudAlpha < 0.04) discard;

    // Multi-light cloud diffuse shading with silver-lining forward scattering
    float lightSum = 0.0;
    int numL = clamp(uNumLights, 0, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        float NdotL = max(dot(norm, lightDir), 0.0);
        float rim = pow(1.0 - max(dot(norm, vec3(0.0, 0.0, 1.0)), 0.0), 3.0) * 0.35;
        lightSum += NdotL + rim;
    }
    vec3 cloudColor = vec3(0.96, 0.98, 1.0) * clamp(lightSum, 0.22, 1.25);

    FragColor = vec4(cloudColor, cloudAlpha * 0.88);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
