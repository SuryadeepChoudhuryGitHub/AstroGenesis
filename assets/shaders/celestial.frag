#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;
in vec3 LocalPos;

uniform vec3 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform vec3 uEmissionColor;
uniform float uEmissionIntensity;
uniform float uThermalGlow;
uniform bool uIsSun;
uniform float uSimTime;
uniform vec3 uCameraPos;

// Multi-Star Lighting (up to 4 stellar sources)
uniform int uNumLights;
uniform vec3 uLightPos[4];
uniform vec3 uLightColor[4];
uniform float uLightIntensity[4];

// Debug Physical Overlays
uniform int uDebugOverlay; // 0 = None, 1 = Stress, 2 = Strain, 3 = Damage, 4 = Temp, 5 = Vel, 6 = GR, 7 = Phase
uniform vec3 uDebugColor;
uniform float uDebugScalar;

// Physical Material & Cloud Shadows
uniform float uWaterFraction;
uniform float uIceFraction;
uniform float uRoughness;
uniform float uCloudShadowCoverage;
uniform float uCloudShadowRotAngle;

// Cinematic Mode & Geometric Shadows
uniform bool uCinematicMode;
uniform bool uHasRing;
uniform vec3 uRingNormal;
uniform vec3 uPlanetCenter;
uniform float uRingInnerRadius;
uniform float uRingOuterRadius;

void main() {
    vec3 baseColor = uColor;
    if (uUseTexture) {
        vec3 rawColor = texture(uTexture, TexCoord).rgb;
        baseColor = uCinematicMode ? pow(rawColor, vec3(2.2)) : rawColor;
    }

    // Dynamic temperature-dependent ice albedo brightening
    if (uIceFraction > 0.01) {
        baseColor = mix(baseColor, vec3(0.92, 0.95, 1.0), uIceFraction * 0.82);
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    // 1. Stellar Rendering (Blackbody Emission + Limb Darkening + Convection Granulation)
    if (uIsSun) {
        float NdotV = max(dot(norm, viewDir), 0.0);
        // Eddington approximation limb darkening: I(mu) = I0 * (0.35 + 0.65 * mu^0.7)
        float limb = 0.35 + 0.65 * pow(NdotV, 0.70);
        
        // Solar granulation surface modulation
        float granulation = 1.0 + 0.04 * sin(LocalPos.x * 45.0 + uSimTime * 1.5) * cos(LocalPos.y * 45.0 + uSimTime * 1.2);
        
        vec3 starColor = baseColor * uEmissionColor * limb * granulation * uEmissionIntensity;
        FragColor = vec4(starColor, 1.0);
        BrightColor = vec4(starColor * 1.5, 1.0);
        return;
    }

    // 2. Multi-Star Illumination (Physical inverse-square attenuation + Diffuse + Specular)
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    
    int numL = clamp(uNumLights, 0, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        float dist = length(uLightPos[i] - FragPos);
        // Physical inverse-square attenuation with near softening
        float atten = 1.0 / (1.0 + 0.06 * dist + 0.012 * dist * dist);

        float shadowFactor = 1.0;

        // Planetary Ring Shadow: fragments test intersection of light ray with the tilted ring plane
        if (uHasRing) {
            float LdotP = dot(lightDir, uRingNormal);
            if (abs(LdotP) > 1e-4) {
                float t = dot(uPlanetCenter - FragPos, uRingNormal) / LdotP;
                if (t > 0.0) {
                    vec3 hitPoint = FragPos + t * lightDir;
                    float rHit = length(hitPoint - uPlanetCenter);
                    if (rHit >= uRingInnerRadius && rHit <= uRingOuterRadius) {
                        shadowFactor *= 0.08; // Sharp dark ring shadow on planet surface
                    }
                }
            }
        }

        // Diffuse (Lambertian with smooth physical terminator)
        float diff = max(dot(norm, lightDir), 0.0);
        diffuseLight += uLightColor[i] * diff * uLightIntensity[i] * atten * shadowFactor;

        // Specular (Roughness-dependent Blinn-Phong + Water Fresnel sunglint)
        vec3 halfDir = normalize(lightDir + viewDir);
        float specExponent = mix(128.0, 16.0, uRoughness);
        float specStrength = mix(0.50, 0.10, uRoughness);
        float spec = pow(max(dot(norm, halfDir), 0.0), specExponent);
        specularLight += uLightColor[i] * spec * specStrength * uLightIntensity[i] * atten * shadowFactor;

        // Liquid Water Specular Reflection (Ocean sunglint + Fresnel)
        if (uWaterFraction > 0.02) {
            float NdotV = max(dot(norm, viewDir), 0.0);
            float fresnel = 0.02 + 0.98 * pow(1.0 - NdotV, 5.0);
            vec3 waveNorm = normalize(norm + 0.015 * vec3(sin(LocalPos.y * 50.0 + uSimTime * 2.5), cos(LocalPos.x * 50.0 + uSimTime * 2.0), 0.0));
            vec3 waveHalf = normalize(lightDir + viewDir);
            float oceanSpec = pow(max(dot(waveNorm, waveHalf), 0.0), 160.0) * fresnel * 2.2;
            specularLight += uLightColor[i] * oceanSpec * uLightIntensity[i] * atten * uWaterFraction * shadowFactor;
        }
    }

    // Deep space: NO artificial ambient floor in cinematic mode. Night sides are genuinely dark.
    // In normal mode, subtle baseline ambient is kept for scientific clarity.
    vec3 ambient = uCinematicMode ? vec3(0.0) : (baseColor * 0.08);
    vec3 surfaceColor = ambient + baseColor * diffuseLight + specularLight;

    // 3. Thermal Incandescence (Magma fissures for T > 600 K)
    if (uThermalGlow > 0.01) {
        float fissurePattern = pow(sin(LocalPos.x * 25.0) * sin(LocalPos.y * 25.0) * sin(LocalPos.z * 25.0), 2.0);
        float magmaIntensity = uThermalGlow * (0.65 + 0.35 * fissurePattern);
        vec3 magmaGlow = uEmissionColor * magmaIntensity;
        surfaceColor += magmaGlow;
    }

    // 4. Debug False-Color Overlay
    if (uDebugOverlay > 0) {
        surfaceColor = mix(surfaceColor, uDebugColor * 1.3, 0.70);
    }

    FragColor = vec4(surfaceColor, 1.0);

    // 5. Luminance Bright-Pass Extraction (for Bloom)
    // Only true stars, incandescent magma, and extreme sunglint bloom. Ordinary surfaces NEVER bloom.
    float luminance = dot(surfaceColor, vec3(0.2126, 0.7152, 0.0722));
    if (uThermalGlow > 0.35) {
        BrightColor = vec4(uEmissionColor * uThermalGlow * 0.4, 1.0);
    } else if (luminance > 3.0) {
        BrightColor = vec4((surfaceColor - vec3(3.0)) * 0.4, 1.0);
    } else {
        BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
    }
}
