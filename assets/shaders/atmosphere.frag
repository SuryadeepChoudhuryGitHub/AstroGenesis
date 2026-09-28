#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uAtmoColor;
uniform float uAtmoDensity;
uniform vec3 uCameraPos;

uniform int uNumLights;
uniform vec3 uLightPos[4];
uniform vec3 uLightColor[4];
uniform float uAtmoScaleHeight;
uniform float uAtmoMieFactor;
uniform float uPlanetRadius;

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    // Fresnel limb optical path length (Chapman function approximation: thin, subtle limb)
    float NdotV = max(dot(norm, viewDir), 0.0);
    float rim = pow(1.0 - NdotV, 3.8);

    // Multi-light directional phase function (Rayleigh + Mie forward scattering)
    vec3 totalScattered = vec3(0.0);
    int numL = clamp(uNumLights, 0, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        float cosTheta = dot(norm, lightDir);
        
        // Twilight illumination: atmosphere scatters slightly past surface terminator
        // (geometric solar depression angle at upper atmospheric altitude)
        float twilight = smoothstep(-0.12, 0.20, cosTheta);
        if (twilight <= 0.0) continue;

        // Viewing phase angle for forward/backward scattering
        float cosViewLight = dot(viewDir, -lightDir);

        // Rayleigh phase function: 3/(16*PI) * (1 + cos^2(theta))
        float rayleighPhase = 0.75 * (1.0 + cosViewLight * cosViewLight);

        // Mie forward scattering phase function (Henyey-Greenstein for aerosols/dust)
        float g = 0.80;
        float miePhase = (1.0 - g * g) / pow(max(1.0 + g * g - 2.0 * g * cosViewLight, 0.01), 1.5) * 0.18;

        float combinedPhase = mix(rayleighPhase, miePhase, clamp(uAtmoMieFactor, 0.0, 1.0));
        vec3 lightCol = uLightColor[i];
        totalScattered += lightCol * combinedPhase * twilight;
    }

    // Unilluminated night side must be completely pitch black (zero scatter)
    float scatterMagnitude = length(totalScattered);
    if (scatterMagnitude < 0.0005) {
        discard;
    }

    // Controlled, restrained alpha: visible only along illuminated limb
    float alpha = clamp(rim * uAtmoDensity * 0.85 * smoothstep(0.0, 0.30, scatterMagnitude), 0.0, 0.90);
    if (alpha < 0.002) discard;

    // Linear color output (composition-dependent tint * light) - NO artificial ambient +0.08 glow
    vec3 color = uAtmoColor * totalScattered * (0.80 + 0.50 * rim);

    FragColor = vec4(color, alpha);
    
    // Atmospheric limb does NOT bloom
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
