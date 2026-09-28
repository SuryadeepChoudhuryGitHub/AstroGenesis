#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec3 LocalPos;

uniform vec3 uCameraPos;
uniform float uSimTime;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    float NdotV = max(dot(norm, viewDir), 0.0);
    
    // Pure black Schwarzschild Event Horizon shadow at center
    if (NdotV > 0.16) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Relativistic Photon Sphere Ring & Accretion Disk with Doppler beaming
        float photonRing = pow(1.0 - NdotV, 12.0);
        float doppler = 1.0 + 0.60 * clamp(LocalPos.x, -1.0, 1.0);
        vec3 ringColor = mix(vec3(1.0, 0.55, 0.15), vec3(0.35, 0.75, 1.3), clamp((doppler - 0.5) * 0.8, 0.0, 1.0));
        ringColor *= (photonRing * 2.8 * doppler);
        FragColor = vec4(ringColor, photonRing);
        BrightColor = vec4(ringColor * 1.8, 1.0);
    }
}
