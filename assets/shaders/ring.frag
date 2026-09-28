#version 330 core
layout(location = 0) out vec4 FragColor;
layout(location = 1) out vec4 BrightColor;

in vec3 FragPos;
in vec3 WorldNormal;
in vec2 TexCoord;
in float NormalizedRadius;

uniform vec3 uSunPos;
uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform vec3 uRingColor;
uniform sampler2D uRingTexture;
uniform bool uHasRingTexture;
uniform bool uCinematicMode;

float calculatePlanetShadow(vec3 fragPos, vec3 sunPos, vec3 planetCenter, float planetRadius) {
    vec3 rayDir = normalize(sunPos - fragPos);
    vec3 L = planetCenter - fragPos;
    float tca = dot(L, rayDir);
    if (tca < 0.0) return 1.0;
    float d2 = dot(L, L) - tca * tca;
    float r2 = planetRadius * planetRadius;
    if (d2 > r2) return 1.0;
    return smoothstep(r2 * 0.94, r2 * 1.02, d2);
}

void main() {
    float u = clamp(NormalizedRadius, 0.0, 1.0);
    vec3 baseAlbedo;
    float baseOpacity;

    if (uHasRingTexture) {
        vec4 texCol = texture(uRingTexture, vec2(u, 0.5));
        baseAlbedo = uCinematicMode ? pow(texCol.rgb, vec3(2.2)) : texCol.rgb;
        baseOpacity = texCol.a;
    } else {
        baseOpacity = 0.70;
        baseAlbedo = uCinematicMode ? pow(uRingColor, vec3(2.2)) : uRingColor;
    }

    float shadow = calculatePlanetShadow(FragPos, uSunPos, uPlanetCenter, uPlanetRadius);
    vec3 lightDir = normalize(uSunPos - FragPos);
    
    // Physical diffuse scattering on ring particles with zero arbitrary ambient clamp
    float cosIncidence = abs(dot(WorldNormal, lightDir));
    float diff = uCinematicMode ? max(cosIncidence, 0.02) : max(cosIncidence, 0.20);

    // In shadow, ring particles receive zero direct starlight
    vec3 finalCol = baseAlbedo * diff * shadow;
    
    // In cinematic mode, unlit ring particles inside planet shadow do not glow
    float opacity = uCinematicMode ? (baseOpacity * (0.05 + 0.95 * shadow)) : (baseOpacity * (0.35 + 0.65 * shadow));
    FragColor = vec4(finalCol, opacity);
    BrightColor = vec4(0.0, 0.0, 0.0, 1.0);
}
