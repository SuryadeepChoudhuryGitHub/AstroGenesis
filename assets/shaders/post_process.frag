#version 330 core
out vec4 FragColor;
in vec2 TexCoord;

uniform sampler2D uSceneTex;
uniform sampler2D uBloomTex;
uniform sampler2D uDepthTex;

uniform float uExposure;
uniform float uBloomIntensity;
uniform int uToneMapMode;
uniform bool uEnableDoF;
uniform float uFocusDist;
uniform float uDoFAperture;
uniform float uNearPlane;
uniform float uFarPlane;
uniform bool uEnableVignette;
uniform bool uEnableCA;

// ACES Filmic Tone Mapping Curve
vec3 ACESFilm(vec3 x) {
    float a = 2.51;
    float b = 0.03;
    float c = 2.43;
    float d = 0.59;
    float e = 0.14;
    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);
}

vec3 Reinhard(vec3 x) {
    return x / (x + vec3(1.0));
}

vec3 Filmic(vec3 x) {
    vec3 X = max(vec3(0.0), x - 0.004);
    return (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);
}

float linearizeDepth(float depth) {
    float z = depth * 2.0 - 1.0;
    return (2.0 * uNearPlane * uFarPlane) / (uFarPlane + uNearPlane - z * (uFarPlane - uNearPlane));
}

void main() {
    vec2 uv = TexCoord;

    // 1. Subtle Chromatic Aberration
    vec3 sceneColor;
    if (uEnableCA) {
        vec2 distFromCenter = uv - 0.5;
        float distSq = dot(distFromCenter, distFromCenter);
        vec2 caOffset = distFromCenter * distSq * 0.012;
        float r = texture(uSceneTex, uv - caOffset).r;
        float g = texture(uSceneTex, uv).g;
        float b = texture(uSceneTex, uv + caOffset).b;
        sceneColor = vec3(r, g, b);
    } else {
        sceneColor = texture(uSceneTex, uv).rgb;
    }

    // 2. Depth of Field (Photo / Cinematic Mode close shots)
    if (uEnableDoF) {
        float depthVal = texture(uDepthTex, uv).r;
        if (depthVal < 0.9999) {
            float linDepth = linearizeDepth(depthVal);
            float coc = clamp(abs(linDepth - uFocusDist) * uDoFAperture, 0.0, 0.015);
            if (coc > 0.001) {
                vec3 blurCol = vec3(0.0);
                float totalW = 0.0;
                for (int x = -2; x <= 2; ++x) {
                    for (int y = -2; y <= 2; ++y) {
                        vec2 offset = vec2(float(x), float(y)) * coc * 0.5;
                        blurCol += texture(uSceneTex, uv + offset).rgb;
                        totalW += 1.0;
                    }
                }
                sceneColor = mix(sceneColor, blurCol / totalW, smoothstep(0.001, 0.006, coc));
            }
        }
    }

    // 3. Luminance Bloom Composite
    vec3 bloom = texture(uBloomTex, uv).rgb;
    sceneColor += bloom * uBloomIntensity;

    // 4. Exposure Scaling
    vec3 exposed = sceneColor * uExposure;

    // 5. Tone Mapping
    vec3 mapped;
    if (uToneMapMode == 0) {
        mapped = ACESFilm(exposed);
    } else if (uToneMapMode == 1) {
        mapped = Reinhard(exposed);
    } else {
        mapped = Filmic(exposed);
    }

    // 6. Subtle Vignette
    if (uEnableVignette) {
        vec2 vUV = uv * (1.0 - uv.yx);
        float vig = vUV.x * vUV.y * 15.0;
        vig = clamp(pow(vig, 0.18), 0.0, 1.0);
        mapped *= vig;
    }

    // 7. Gamma Correction into sRGB space
    mapped = pow(mapped, vec3(1.0 / 2.2));

    // 8. Subtle Film Grain (suppressed in deep space so pitch black remains pristine)
    float grain = fract(sin(dot(uv * 1000.0, vec2(12.9898, 78.233))) * 43758.5453);
    mapped += (grain - 0.5) * 0.006 * smoothstep(0.01, 0.25, length(mapped));

    FragColor = vec4(clamp(mapped, 0.0, 1.0), 1.0);
}
