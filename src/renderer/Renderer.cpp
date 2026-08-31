#include "renderer/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdio>
#include <algorithm>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace AstroGenesis {

static const float PI = 3.14159265358979323846f;

struct TrailVertex {
    glm::vec3 pos;
    glm::vec4 col;
};

// =========================================================================
// 1. Celestial Body PBR Uber-Shader (Multi-Star Lighting & Thermal Magma)
// =========================================================================
static const char* celestialVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 LocalPos;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMat;

void main() {
    LocalPos = aPos;
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(uNormalMat * aNormal);
    TexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* celestialFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

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

void main() {
    vec3 baseColor = uColor;
    if (uUseTexture) {
        baseColor = texture(uTexture, TexCoord).rgb;
    }

    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    // 1. Stellar Rendering (Blackbody Emission + Limb Darkening + Convection Granulation)
    if (uIsSun) {
        float NdotV = max(dot(norm, viewDir), 0.0);
        // Eddington approximation limb darkening: I(mu) = I0 * (0.4 + 0.6 * mu)
        float limb = 0.35 + 0.65 * pow(NdotV, 0.70);
        
        // Solar granulation surface modulation
        float granulation = 1.0 + 0.04 * sin(LocalPos.x * 45.0 + uSimTime * 1.5) * cos(LocalPos.y * 45.0 + uSimTime * 1.2);
        
        vec3 starColor = baseColor * uEmissionColor * limb * granulation * uEmissionIntensity;
        FragColor = vec4(starColor, 1.0);
        return;
    }

    // 2. Multi-Star Illumination (Diffuse + Blinn-Phong Specular)
    vec3 diffuseLight = vec3(0.0);
    vec3 specularLight = vec3(0.0);
    
    int numL = clamp(uNumLights, 1, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        float dist = length(uLightPos[i] - FragPos);
        float atten = clamp(1.0 / (1.0 + 0.05 * dist), 0.15, 1.0);

        // Diffuse (Lambertian)
        float diff = max(dot(norm, lightDir), 0.0);
        diffuseLight += uLightColor[i] * diff * uLightIntensity[i] * atten;

        // Specular (Blinn-Phong)
        vec3 halfDir = normalize(lightDir + viewDir);
        float spec = pow(max(dot(norm, halfDir), 0.0), 32.0);
        specularLight += uLightColor[i] * spec * 0.25 * uLightIntensity[i] * atten;
    }

    // Ambient baseline celestial lighting
    vec3 ambient = baseColor * 0.12;
    vec3 surfaceColor = ambient + baseColor * diffuseLight + specularLight;

    // 3. Thermal Incandescence (Magma fissures for T > 700 K)
    if (uThermalGlow > 0.01) {
        float fissurePattern = pow(sin(LocalPos.x * 25.0) * sin(LocalPos.y * 25.0) * sin(LocalPos.z * 25.0), 2.0);
        float magmaIntensity = uThermalGlow * (0.65 + 0.35 * fissurePattern);
        surfaceColor += uEmissionColor * magmaIntensity;
    }

    // 4. Debug False-Color Overlay
    if (uDebugOverlay > 0) {
        surfaceColor = mix(surfaceColor, uDebugColor * 1.3, 0.70);
    }

    FragColor = vec4(surfaceColor, 1.0);
}
)GLSL";

// =========================================================================
// 2. Atmospheric Scattering Shader (Rayleigh & Mie Scattering Shell)
// =========================================================================
static const char* atmoVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;

out vec3 FragPos;
out vec3 Normal;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(mat3(uModel) * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* atmoFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;

uniform vec3 uAtmoColor;
uniform float uAtmoDensity;
uniform vec3 uCameraPos;

uniform int uNumLights;
uniform vec3 uLightPos[4];
uniform vec3 uLightColor[4];

void main() {
    vec3 norm = normalize(Normal);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    // Fresnel limb scattering (strongest along the planetary rim)
    float NdotV = max(dot(norm, viewDir), 0.0);
    float rim = pow(1.0 - NdotV, 3.5);

    // Multi-light directional phase function (forward scattering towards camera)
    float lightPhase = 0.0;
    int numL = clamp(uNumLights, 1, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        float forwardScattering = max(dot(norm, lightDir), 0.0);
        lightPhase += forwardScattering;
    }
    lightPhase = clamp(lightPhase, 0.20, 1.20);

    float alpha = clamp(rim * uAtmoDensity * lightPhase * 1.25, 0.0, 0.90);
    vec3 color = uAtmoColor * (0.85 + 0.35 * rim);

    FragColor = vec4(color, alpha);
}
)GLSL";

// =========================================================================
// 3. Dynamic Rotating Cloud Layer Shader
// =========================================================================
static const char* cloudVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;
out vec3 LocalPos;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    LocalPos = aPos;
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(mat3(uModel) * aNormal);
    TexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* cloudFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

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
    float cloudNoise = (c1 * 0.6 + c2 * 0.4) * 0.5 + 0.5;

    float cloudAlpha = smoothstep(1.0 - uCloudCoverage, 1.0, cloudNoise);
    if (cloudAlpha < 0.05) discard;

    // Multi-light cloud diffuse shading
    float lightSum = 0.0;
    int numL = clamp(uNumLights, 1, 4);
    for (int i = 0; i < numL; ++i) {
        vec3 lightDir = normalize(uLightPos[i] - FragPos);
        lightSum += max(dot(norm, lightDir), 0.15);
    }
    vec3 cloudColor = vec3(0.95, 0.97, 1.0) * clamp(lightSum, 0.25, 1.15);

    FragColor = vec4(cloudColor, cloudAlpha * 0.85);
}
)GLSL";

// =========================================================================
// 4. Stellar Corona & Solar Flare Prominence Shader
// =========================================================================
static const char* coronaVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec2 TexCoord;
out vec3 Normal;
out vec3 LocalPos;

uniform mat4 uMVP;

void main() {
    LocalPos = aPos;
    TexCoord = aTexCoord;
    Normal = aNormal;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* coronaFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

in vec2 TexCoord;
in vec3 Normal;
in vec3 LocalPos;

uniform vec3 uCoronaColor;
uniform float uCoronaIntensity;
uniform float uSimTime;

void main() {
    float distFromCenter = length(LocalPos.xy);
    if (distFromCenter > 1.0) discard;

    // Radial exponential falloff
    float radial = pow(1.0 - distFromCenter, 2.2);

    // Dynamic solar flare prominences
    float angle = atan(LocalPos.y, LocalPos.x);
    float flare = sin(angle * 9.0 + uSimTime * 2.0) * cos(angle * 14.0 - uSimTime * 1.5);
    float flareIntensity = 1.0 + 0.25 * flare;

    float alpha = clamp(radial * flareIntensity * uCoronaIntensity * 0.65, 0.0, 1.0);
    vec3 col = uCoronaColor * (1.2 + 0.3 * flare);

    FragColor = vec4(col, alpha);
}
)GLSL";

// =========================================================================
// 5. Relativistic Black Hole Shader (Event Horizon & Accretion Ring)
// =========================================================================
static const char* bhVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec3 LocalPos;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    LocalPos = aPos;
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = normalize(mat3(uModel) * aNormal);
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* bhFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

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
    if (NdotV > 0.15) {
        FragColor = vec4(0.0, 0.0, 0.0, 1.0);
    } else {
        // Relativistic Photon Sphere Ring Lensing Rim
        float photonRing = pow(1.0 - NdotV, 12.0);
        vec3 ringColor = vec3(0.3, 0.6, 1.0) * (photonRing * 2.5);
        FragColor = vec4(ringColor, photonRing);
    }
}
)GLSL";

// =========================================================================
// 6. Collision & Impact Shockwave FX Shader
// =========================================================================
static const char* impactVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;

uniform mat4 uVP;
uniform vec3 uImpactCenter;
uniform vec3 uImpactNormal;
uniform float uFlashRadius;

void main() {
    vec3 pos = uImpactCenter + aPos * uFlashRadius;
    gl_Position = uVP * vec4(pos, 1.0);
}
)GLSL";

static const char* impactFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

uniform vec3 uImpactColor;
uniform float uIntensity;
uniform float uAge;

void main() {
    float alpha = uIntensity * (1.0 - uAge * 0.33);
    FragColor = vec4(uImpactColor * 2.0, clamp(alpha, 0.0, 1.0));
}
)GLSL";

// =========================================================================
// Skybox, Trail, Ring Shaders
// =========================================================================
static const char* skyboxVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 2) in vec2 aTexCoord;
out vec2 TexCoord;
uniform mat4 uVP;
void main() {
    TexCoord = aTexCoord;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* skyboxFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;
in vec2 TexCoord;
uniform sampler2D uTexture;
uniform bool uHasTexture;
void main() {
    if (uHasTexture) {
        FragColor = texture(uTexture, TexCoord);
    } else {
        FragColor = vec4(0.015, 0.025, 0.05, 1.0);
    }
}
)GLSL";

static const char* trailVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;
out vec4 vColor;
uniform mat4 uVP;
void main() {
    vColor = aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* trailFragSrc = R"GLSL(
#version 330 core
in vec4 vColor;
out vec4 FragColor;
void main() {
    FragColor = vColor;
}
)GLSL";

static const char* ringVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out float NormalizedRadius;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMat;
uniform float uInnerRadius;
uniform float uOuterRadius;

void main() {
    float r = mix(uInnerRadius, uOuterRadius, aTexCoord.x);
    vec3 pos = vec3(aPos.x * r, aPos.y, aPos.z * r);
    FragPos = vec3(uModel * vec4(pos, 1.0));
    WorldNormal = normalize(uNormalMat * vec3(0.0, 1.0, 0.0));
    TexCoord = aTexCoord;
    NormalizedRadius = aTexCoord.x;
    gl_Position = uMVP * vec4(pos, 1.0);
}
)GLSL";

static const char* ringFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

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

float calculatePlanetShadow(vec3 fragPos, vec3 sunPos, vec3 planetCenter, float planetRadius) {
    vec3 rayDir = normalize(sunPos - fragPos);
    vec3 L = planetCenter - fragPos;
    float tca = dot(L, rayDir);
    if (tca < 0.0) return 1.0;
    float d2 = dot(L, L) - tca * tca;
    float r2 = planetRadius * planetRadius;
    if (d2 > r2) return 1.0;
    return smoothstep(r2 * 0.90, r2 * 1.05, d2);
}

void main() {
    float u = clamp(NormalizedRadius, 0.0, 1.0);
    vec3 baseAlbedo;
    float baseOpacity;

    if (uHasRingTexture) {
        vec4 texCol = texture(uRingTexture, vec2(u, 0.5));
        baseAlbedo = texCol.rgb;
        baseOpacity = texCol.a;
    } else {
        baseOpacity = 0.70;
        baseAlbedo = uRingColor;
    }

    float shadow = calculatePlanetShadow(FragPos, uSunPos, uPlanetCenter, uPlanetRadius);
    vec3 lightDir = normalize(uSunPos - FragPos);
    float diff = max(abs(dot(WorldNormal, lightDir)), 0.20);

    vec3 finalCol = baseAlbedo * diff * shadow;
    FragColor = vec4(finalCol, baseOpacity * (0.35 + 0.65 * shadow));
}
)GLSL";

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024];
        glGetShaderInfoLog(s, 1024, nullptr, log);
        fprintf(stderr, "[Renderer] Shader Compile Error: %s\n", log);
    }
    return s;
}

Renderer::Renderer() {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    // 1. Compile Celestial PBR Shader
    GLuint vShader = compileShader(GL_VERTEX_SHADER, celestialVertSrc);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, celestialFragSrc);
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vShader);
    glAttachShader(m_shaderProgram, fShader);
    glLinkProgram(m_shaderProgram);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    m_uMVPLoc                = glGetUniformLocation(m_shaderProgram, "uMVP");
    m_uModelLoc              = glGetUniformLocation(m_shaderProgram, "uModel");
    m_uNormalMatLoc          = glGetUniformLocation(m_shaderProgram, "uNormalMat");
    m_uColorLoc              = glGetUniformLocation(m_shaderProgram, "uColor");
    m_uUseTextureLoc         = glGetUniformLocation(m_shaderProgram, "uUseTexture");
    m_uTextureLoc            = glGetUniformLocation(m_shaderProgram, "uTexture");
    m_uEmissionColorLoc      = glGetUniformLocation(m_shaderProgram, "uEmissionColor");
    m_uEmissionIntensityLoc  = glGetUniformLocation(m_shaderProgram, "uEmissionIntensity");
    m_uThermalGlowLoc        = glGetUniformLocation(m_shaderProgram, "uThermalGlow");
    m_uIsSunLoc              = glGetUniformLocation(m_shaderProgram, "uIsSun");
    m_uSimTimeLoc            = glGetUniformLocation(m_shaderProgram, "uSimTime");
    m_uCameraPosLoc          = glGetUniformLocation(m_shaderProgram, "uCameraPos");
    m_uNumLightsLoc          = glGetUniformLocation(m_shaderProgram, "uNumLights");
    m_uDebugOverlayLoc       = glGetUniformLocation(m_shaderProgram, "uDebugOverlay");
    m_uDebugColorLoc         = glGetUniformLocation(m_shaderProgram, "uDebugColor");
    m_uDebugScalarLoc        = glGetUniformLocation(m_shaderProgram, "uDebugScalar");

    for (int i = 0; i < 4; ++i) {
        char pBuf[32], cBuf[32], iBuf[32];
        snprintf(pBuf, sizeof(pBuf), "uLightPos[%d]", i);
        snprintf(cBuf, sizeof(cBuf), "uLightColor[%d]", i);
        snprintf(iBuf, sizeof(iBuf), "uLightIntensity[%d]", i);
        m_uLightPosLoc[i] = glGetUniformLocation(m_shaderProgram, pBuf);
        m_uLightColorLoc[i] = glGetUniformLocation(m_shaderProgram, cBuf);
        m_uLightIntensityLoc[i] = glGetUniformLocation(m_shaderProgram, iBuf);
    }

    // 2. Compile Atmosphere Shader
    GLuint atmoV = compileShader(GL_VERTEX_SHADER, atmoVertSrc);
    GLuint atmoF = compileShader(GL_FRAGMENT_SHADER, atmoFragSrc);
    m_atmosphereProgram = glCreateProgram();
    glAttachShader(m_atmosphereProgram, atmoV);
    glAttachShader(m_atmosphereProgram, atmoF);
    glLinkProgram(m_atmosphereProgram);
    glDeleteShader(atmoV);
    glDeleteShader(atmoF);

    m_uAtmoMVPLoc       = glGetUniformLocation(m_atmosphereProgram, "uMVP");
    m_uAtmoModelLoc     = glGetUniformLocation(m_atmosphereProgram, "uModel");
    m_uAtmoColorLoc     = glGetUniformLocation(m_atmosphereProgram, "uAtmoColor");
    m_uAtmoDensityLoc   = glGetUniformLocation(m_atmosphereProgram, "uAtmoDensity");
    m_uAtmoCameraPosLoc = glGetUniformLocation(m_atmosphereProgram, "uCameraPos");
    m_uAtmoNumLightsLoc = glGetUniformLocation(m_atmosphereProgram, "uNumLights");
    for (int i = 0; i < 4; ++i) {
        char pBuf[32], cBuf[32];
        snprintf(pBuf, sizeof(pBuf), "uLightPos[%d]", i);
        snprintf(cBuf, sizeof(cBuf), "uLightColor[%d]", i);
        m_uAtmoLightPosLoc[i] = glGetUniformLocation(m_atmosphereProgram, pBuf);
        m_uAtmoLightColorLoc[i] = glGetUniformLocation(m_atmosphereProgram, cBuf);
    }

    // 3. Compile Cloud Shader
    GLuint cloudV = compileShader(GL_VERTEX_SHADER, cloudVertSrc);
    GLuint cloudF = compileShader(GL_FRAGMENT_SHADER, cloudFragSrc);
    m_cloudProgram = glCreateProgram();
    glAttachShader(m_cloudProgram, cloudV);
    glAttachShader(m_cloudProgram, cloudF);
    glLinkProgram(m_cloudProgram);
    glDeleteShader(cloudV);
    glDeleteShader(cloudF);

    m_uCloudMVPLoc       = glGetUniformLocation(m_cloudProgram, "uMVP");
    m_uCloudModelLoc     = glGetUniformLocation(m_cloudProgram, "uModel");
    m_uCloudCoverageLoc  = glGetUniformLocation(m_cloudProgram, "uCloudCoverage");
    m_uCloudSimTimeLoc   = glGetUniformLocation(m_cloudProgram, "uSimTime");
    m_uCloudNumLightsLoc = glGetUniformLocation(m_cloudProgram, "uNumLights");
    for (int i = 0; i < 4; ++i) {
        char pBuf[32];
        snprintf(pBuf, sizeof(pBuf), "uLightPos[%d]", i);
        m_uCloudLightPosLoc[i] = glGetUniformLocation(m_cloudProgram, pBuf);
    }

    // 4. Compile Corona Shader
    GLuint corV = compileShader(GL_VERTEX_SHADER, coronaVertSrc);
    GLuint corF = compileShader(GL_FRAGMENT_SHADER, coronaFragSrc);
    m_coronaProgram = glCreateProgram();
    glAttachShader(m_coronaProgram, corV);
    glAttachShader(m_coronaProgram, corF);
    glLinkProgram(m_coronaProgram);
    glDeleteShader(corV);
    glDeleteShader(corF);

    m_uCoronaMVPLoc       = glGetUniformLocation(m_coronaProgram, "uMVP");
    m_uCoronaColorLoc     = glGetUniformLocation(m_coronaProgram, "uCoronaColor");
    m_uCoronaIntensityLoc = glGetUniformLocation(m_coronaProgram, "uCoronaIntensity");
    m_uCoronaSimTimeLoc   = glGetUniformLocation(m_coronaProgram, "uSimTime");

    // 5. Compile Black Hole Shader
    GLuint bhV = compileShader(GL_VERTEX_SHADER, bhVertSrc);
    GLuint bhF = compileShader(GL_FRAGMENT_SHADER, bhFragSrc);
    m_blackHoleProgram = glCreateProgram();
    glAttachShader(m_blackHoleProgram, bhV);
    glAttachShader(m_blackHoleProgram, bhF);
    glLinkProgram(m_blackHoleProgram);
    glDeleteShader(bhV);
    glDeleteShader(bhF);

    m_uBhMVPLoc         = glGetUniformLocation(m_blackHoleProgram, "uMVP");
    m_uBhModelLoc       = glGetUniformLocation(m_blackHoleProgram, "uModel");
    m_uBhCameraPosLoc   = glGetUniformLocation(m_blackHoleProgram, "uCameraPos");
    m_uBhSchwRadiusLoc  = glGetUniformLocation(m_blackHoleProgram, "uSchwRadius");
    m_uBhPhotonRadiusLoc= glGetUniformLocation(m_blackHoleProgram, "uPhotonRadius");
    m_uBhSimTimeLoc     = glGetUniformLocation(m_blackHoleProgram, "uSimTime");

    // 6. Compile Impact FX Shader
    GLuint impV = compileShader(GL_VERTEX_SHADER, impactVertSrc);
    GLuint impF = compileShader(GL_FRAGMENT_SHADER, impactFragSrc);
    m_impactProgram = glCreateProgram();
    glAttachShader(m_impactProgram, impV);
    glAttachShader(m_impactProgram, impF);
    glLinkProgram(m_impactProgram);
    glDeleteShader(impV);
    glDeleteShader(impF);

    m_uImpVPLoc        = glGetUniformLocation(m_impactProgram, "uVP");
    m_uImpCenterLoc    = glGetUniformLocation(m_impactProgram, "uImpactCenter");
    m_uImpNormalLoc    = glGetUniformLocation(m_impactProgram, "uImpactNormal");
    m_uImpRadiusLoc    = glGetUniformLocation(m_impactProgram, "uFlashRadius");
    m_uImpIntensityLoc = glGetUniformLocation(m_impactProgram, "uIntensity");
    m_uImpColorLoc     = glGetUniformLocation(m_impactProgram, "uImpactColor");
    m_uImpAgeLoc       = glGetUniformLocation(m_impactProgram, "uAge");

    // 7. Compile Skybox, Trail, Ring Shaders
    GLuint skyV = compileShader(GL_VERTEX_SHADER, skyboxVertSrc);
    GLuint skyF = compileShader(GL_FRAGMENT_SHADER, skyboxFragSrc);
    m_skyboxProgram = glCreateProgram();
    glAttachShader(m_skyboxProgram, skyV);
    glAttachShader(m_skyboxProgram, skyF);
    glLinkProgram(m_skyboxProgram);
    glDeleteShader(skyV);
    glDeleteShader(skyF);
    m_skyUVPLoc = glGetUniformLocation(m_skyboxProgram, "uVP");
    m_skyTexLoc = glGetUniformLocation(m_skyboxProgram, "uTexture");
    m_skyHasTexLoc = glGetUniformLocation(m_skyboxProgram, "uHasTexture");

    GLuint trailV = compileShader(GL_VERTEX_SHADER, trailVertSrc);
    GLuint trailF = compileShader(GL_FRAGMENT_SHADER, trailFragSrc);
    m_trailProgram = glCreateProgram();
    glAttachShader(m_trailProgram, trailV);
    glAttachShader(m_trailProgram, trailF);
    glLinkProgram(m_trailProgram);
    glDeleteShader(trailV);
    glDeleteShader(trailF);
    m_uTrailVPLoc = glGetUniformLocation(m_trailProgram, "uVP");

    glGenVertexArrays(1, &m_trailVAO);
    glGenBuffers(1, &m_trailVBO);
    glBindVertexArray(m_trailVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)offsetof(TrailVertex, pos));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(TrailVertex), (void*)offsetof(TrailVertex, col));
    glBindVertexArray(0);

    GLuint ringV = compileShader(GL_VERTEX_SHADER, ringVertSrc);
    GLuint ringF = compileShader(GL_FRAGMENT_SHADER, ringFragSrc);
    m_ringProgram = glCreateProgram();
    glAttachShader(m_ringProgram, ringV);
    glAttachShader(m_ringProgram, ringF);
    glLinkProgram(m_ringProgram);
    glDeleteShader(ringV);
    glDeleteShader(ringF);

    m_uRingMVPLoc          = glGetUniformLocation(m_ringProgram, "uMVP");
    m_uRingModelLoc        = glGetUniformLocation(m_ringProgram, "uModel");
    m_uRingNormalMatLoc    = glGetUniformLocation(m_ringProgram, "uNormalMat");
    m_uRingSunPosLoc       = glGetUniformLocation(m_ringProgram, "uSunPos");
    m_uRingPlanetCenterLoc = glGetUniformLocation(m_ringProgram, "uPlanetCenter");
    m_uRingPlanetRadiusLoc = glGetUniformLocation(m_ringProgram, "uPlanetRadius");
    m_uRingColorLoc        = glGetUniformLocation(m_ringProgram, "uRingColor");
    m_uRingTexLoc          = glGetUniformLocation(m_ringProgram, "uRingTexture");
    m_uRingHasTexLoc       = glGetUniformLocation(m_ringProgram, "uHasRingTexture");

    // Create Meshes
    m_sphereMesh = createSphereMesh(1.0f, 48, 48);
    m_ringMesh = createRingMesh(128);
    m_quadMesh = createQuadMesh();

    m_particleRenderer.initialize();
    m_deformableRenderer.initialize();

    return true;
}

void Renderer::shutdown() {
    if (m_shaderProgram) { glDeleteProgram(m_shaderProgram); m_shaderProgram = 0; }
    if (m_atmosphereProgram) { glDeleteProgram(m_atmosphereProgram); m_atmosphereProgram = 0; }
    if (m_cloudProgram) { glDeleteProgram(m_cloudProgram); m_cloudProgram = 0; }
    if (m_coronaProgram) { glDeleteProgram(m_coronaProgram); m_coronaProgram = 0; }
    if (m_blackHoleProgram) { glDeleteProgram(m_blackHoleProgram); m_blackHoleProgram = 0; }
    if (m_impactProgram) { glDeleteProgram(m_impactProgram); m_impactProgram = 0; }
    if (m_skyboxProgram) { glDeleteProgram(m_skyboxProgram); m_skyboxProgram = 0; }
    if (m_trailProgram) { glDeleteProgram(m_trailProgram); m_trailProgram = 0; }
    if (m_ringProgram) { glDeleteProgram(m_ringProgram); m_ringProgram = 0; }

    if (m_trailVAO) { glDeleteVertexArrays(1, &m_trailVAO); m_trailVAO = 0; }
    if (m_trailVBO) { glDeleteBuffers(1, &m_trailVBO); m_trailVBO = 0; }

    if (m_sphereMesh.vao) {
        glDeleteVertexArrays(1, &m_sphereMesh.vao);
        glDeleteBuffers(1, &m_sphereMesh.vbo);
        glDeleteBuffers(1, &m_sphereMesh.ebo);
        m_sphereMesh = {};
    }
    if (m_ringMesh.vao) {
        glDeleteVertexArrays(1, &m_ringMesh.vao);
        glDeleteBuffers(1, &m_ringMesh.vbo);
        glDeleteBuffers(1, &m_ringMesh.ebo);
        m_ringMesh = {};
    }
    if (m_quadMesh.vao) {
        glDeleteVertexArrays(1, &m_quadMesh.vao);
        glDeleteBuffers(1, &m_quadMesh.vbo);
        m_quadMesh = {};
    }

    for (auto& pair : m_textures) {
        glDeleteTextures(1, &pair.second);
    }
    m_textures.clear();

    m_particleRenderer.shutdown();
    m_deformableRenderer.shutdown();
}

MeshData Renderer::createSphereMesh(float radius, int stacks, int sectors) {
    MeshData mesh;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float stackAngle = PI / 2 - (float)i * PI / stacks;
        float xy = radius * cosf(stackAngle);
        float z = radius * sinf(stackAngle);

        for (int j = 0; j <= sectors; ++j) {
            float sectorAngle = (float)j * 2 * PI / sectors;
            float x = xy * cosf(sectorAngle);
            float y = xy * sinf(sectorAngle);

            vertices.push_back(x);
            vertices.push_back(z);
            vertices.push_back(y);

            vertices.push_back(x / radius);
            vertices.push_back(z / radius);
            vertices.push_back(y / radius);

            vertices.push_back((float)j / sectors);
            vertices.push_back((float)i / stacks);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        int k1 = i * (sectors + 1);
        int k2 = k1 + sectors + 1;

        for (int j = 0; j < sectors; ++j, ++k1, ++k2) {
            if (i != 0) {
                indices.push_back(k1);
                indices.push_back(k2);
                indices.push_back(k1 + 1);
            }
            if (i != (stacks - 1)) {
                indices.push_back(k1 + 1);
                indices.push_back(k2);
                indices.push_back(k2 + 1);
            }
        }
    }

    mesh.indexCount = (int)indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Stride = 8 floats: pos (3), normal (3), texCoord (2)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

MeshData Renderer::createRingMesh(int radialSegments) {
    MeshData mesh;
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= radialSegments; ++i) {
        float theta = 2.0f * PI * (float)i / (float)radialSegments;
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        // Inner edge vertex (u = 0.0)
        vertices.push_back(cosT); vertices.push_back(0.0f); vertices.push_back(sinT);
        vertices.push_back(0.0f); vertices.push_back((float)i / radialSegments);

        // Outer edge vertex (u = 1.0)
        vertices.push_back(cosT); vertices.push_back(0.0f); vertices.push_back(sinT);
        vertices.push_back(1.0f); vertices.push_back((float)i / radialSegments);
    }

    for (int i = 0; i < radialSegments; ++i) {
        int i0 = i * 2;
        int i1 = i0 + 1;
        int i2 = i0 + 2;
        int i3 = i0 + 3;

        indices.push_back(i0); indices.push_back(i1); indices.push_back(i2);
        indices.push_back(i2); indices.push_back(i1); indices.push_back(i3);
    }

    mesh.indexCount = (int)indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return mesh;
}

MeshData Renderer::createQuadMesh() {
    MeshData mesh;
    float vertices[] = {
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         1.0f,  1.0f, 0.0f,  1.0f, 1.0f,
        -1.0f,  1.0f, 0.0f,  0.0f, 1.0f
    };
    mesh.indexCount = 6;

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

GLuint Renderer::loadTexture(const std::string& filepath) {
    if (filepath.empty()) return 0;

    auto it = m_textures.find(filepath);
    if (it != m_textures.end()) {
        return it->second;
    }

    int width, height, nrChannels;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* data = stbi_load(filepath.c_str(), &width, &height, &nrChannels, 0);

    if (!data) {
        return 0;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 4) format = GL_RGBA;

    GLuint texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    stbi_image_free(data);
    m_textures[filepath] = texture;
    return texture;
}

void Renderer::beginViewport(int x, int y, int width, int height, const glm::vec4& clearColor) {
    glViewport(x, y, width, height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::endViewport(int windowWidth, int windowHeight) {
    glViewport(0, 0, windowWidth, windowHeight);
}

void Renderer::renderCelestialBody(
    const Camera& camera,
    float aspect,
    const VisualBodyState& vBody,
    const std::vector<StarLightSource>& stars,
    const glm::vec3& cameraTarget,
    const std::string& texturePath,
    VisualMode visMode,
    DebugVisualOverlay debugOverlay,
    float simTime
) {
    if (vBody.isBlackHole) {
        renderBlackHole(camera, aspect, vBody, cameraTarget, simTime);
        return;
    }

    glUseProgram(m_shaderProgram);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 camPos = camera.getEyePosition();

    glm::vec3 relativePos = vBody.positionAU - cameraTarget;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = model * vBody.rotationMatrix;
    model = glm::scale(model, glm::vec3(vBody.renderRadius));

    glm::mat4 mvp = proj * view * model;
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

    glUniformMatrix4fv(m_uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(m_uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform3fv(m_uColorLoc, 1, glm::value_ptr(vBody.baseAlbedo));
    glUniform3fv(m_uEmissionColorLoc, 1, glm::value_ptr(vBody.emissionColor));
    glUniform1f(m_uEmissionIntensityLoc, vBody.emissionIntensity);
    glUniform1f(m_uThermalGlowLoc, vBody.thermalGlow);
    glUniform1i(m_uIsSunLoc, vBody.isStar ? 1 : 0);
    glUniform1f(m_uSimTimeLoc, simTime);
    glUniform3fv(m_uCameraPosLoc, 1, glm::value_ptr(camPos));

    // Multi-Star Lighting Upload
    int numLights = (int)std::min(stars.size(), (size_t)4);
    glUniform1i(m_uNumLightsLoc, numLights);
    for (int i = 0; i < numLights; ++i) {
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uLightPosLoc[i], 1, glm::value_ptr(relLightPos));
        glUniform3fv(m_uLightColorLoc[i], 1, glm::value_ptr(stars[i].color));
        glUniform1f(m_uLightIntensityLoc[i], stars[i].intensity);
    }

    // Debug Overlays
    glUniform1i(m_uDebugOverlayLoc, (int)debugOverlay);
    glUniform3fv(m_uDebugColorLoc, 1, glm::value_ptr(vBody.debugColor));
    glUniform1f(m_uDebugScalarLoc, vBody.debugScalar);

    GLuint texId = 0;
    if (!texturePath.empty()) {
        texId = loadTexture(texturePath);
    }

    if (texId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        glUniform1i(m_uTextureLoc, 0);
        glUniform1i(m_uUseTextureLoc, 1);
    } else {
        glUniform1i(m_uUseTextureLoc, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    // Render Atmospheric Shell
    if (vBody.hasAtmosphere && !vBody.isStar) {
        renderAtmosphereShell(camera, aspect, vBody, stars, cameraTarget);
        if (vBody.hasClouds) {
            renderCloudLayer(camera, aspect, vBody, stars, cameraTarget);
        }
    }

    // Render Stellar Corona
    if (vBody.isStar) {
        renderStellarCorona(camera, aspect, vBody, cameraTarget, simTime);
    }
}

void Renderer::renderAtmosphereShell(
    const Camera& camera,
    float aspect,
    const VisualBodyState& vBody,
    const std::vector<StarLightSource>& stars,
    const glm::vec3& cameraTarget
) {
    if (m_atmosphereProgram == 0) return;

    glUseProgram(m_atmosphereProgram);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous atmospheric glow
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 camPos = camera.getEyePosition();

    glm::vec3 relativePos = vBody.positionAU - cameraTarget;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = glm::scale(model, glm::vec3(vBody.atmosphereRadius));

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(m_uAtmoMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uAtmoModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(m_uAtmoColorLoc, 1, glm::value_ptr(vBody.atmosphereColor));
    glUniform1f(m_uAtmoDensityLoc, vBody.atmosphereDensity);
    glUniform3fv(m_uAtmoCameraPosLoc, 1, glm::value_ptr(camPos));

    int numLights = (int)std::min(stars.size(), (size_t)4);
    glUniform1i(m_uAtmoNumLightsLoc, numLights);
    for (int i = 0; i < numLights; ++i) {
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uAtmoLightPosLoc[i], 1, glm::value_ptr(relLightPos));
        glUniform3fv(m_uAtmoLightColorLoc[i], 1, glm::value_ptr(stars[i].color));
    }

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderCloudLayer(
    const Camera& camera,
    float aspect,
    const VisualBodyState& vBody,
    const std::vector<StarLightSource>& stars,
    const glm::vec3& cameraTarget
) {
    if (m_cloudProgram == 0) return;

    glUseProgram(m_cloudProgram);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    glm::vec3 relativePos = vBody.positionAU - cameraTarget;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = glm::rotate(model, glm::radians(vBody.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, vBody.cloudRotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(vBody.renderRadius * 1.015f));

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(m_uCloudMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uCloudModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform1f(m_uCloudCoverageLoc, vBody.cloudCoverage);
    glUniform1f(m_uCloudSimTimeLoc, vBody.cloudRotationAngle);

    int numLights = (int)std::min(stars.size(), (size_t)4);
    glUniform1i(m_uCloudNumLightsLoc, numLights);
    for (int i = 0; i < numLights; ++i) {
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uCloudLightPosLoc[i], 1, glm::value_ptr(relLightPos));
    }

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderStellarCorona(
    const Camera& camera,
    float aspect,
    const VisualBodyState& vBody,
    const glm::vec3& cameraTarget,
    float simTime
) {
    if (m_coronaProgram == 0 || m_quadMesh.vao == 0) return;

    glUseProgram(m_coronaProgram);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Pure luminous additive corona
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    // Billboard quad facing camera
    glm::vec3 relativePos = vBody.positionAU - cameraTarget;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);

    // Extract camera rotation to face billboard
    model[0][0] = view[0][0]; model[0][1] = view[1][0]; model[0][2] = view[2][0];
    model[1][0] = view[0][1]; model[1][1] = view[1][1]; model[1][2] = view[2][1];
    model[2][0] = view[0][2]; model[2][1] = view[1][2]; model[2][2] = view[2][2];

    float coronaScale = vBody.renderRadius * 2.4f;
    model = glm::scale(model, glm::vec3(coronaScale));

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(m_uCoronaMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(m_uCoronaColorLoc, 1, glm::value_ptr(vBody.temperatureColor));
    glUniform1f(m_uCoronaIntensityLoc, vBody.coronaIntensity);
    glUniform1f(m_uCoronaSimTimeLoc, simTime);

    glBindVertexArray(m_quadMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderBlackHole(
    const Camera& camera,
    float aspect,
    const VisualBodyState& vBody,
    const glm::vec3& cameraTarget,
    float simTime
) {
    if (m_blackHoleProgram == 0) return;

    glUseProgram(m_blackHoleProgram);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 camPos = camera.getEyePosition();

    glm::vec3 relativePos = vBody.positionAU - cameraTarget;
    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = glm::scale(model, glm::vec3(vBody.renderRadius));

    glm::mat4 mvp = proj * view * model;

    glUniformMatrix4fv(m_uBhMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uBhModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(m_uBhCameraPosLoc, 1, glm::value_ptr(camPos));
    glUniform1f(m_uBhSimTimeLoc, simTime);

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    glDisable(GL_BLEND);
}

void Renderer::renderImpactFX(
    const Camera& camera,
    float aspect,
    const std::vector<VisualImpactEvent>& impacts,
    const glm::vec3& cameraTarget
) {
    if (impacts.empty() || m_impactProgram == 0 || m_quadMesh.vao == 0) return;

    glUseProgram(m_impactProgram);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive flash
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 vp = proj * view;
    glUniformMatrix4fv(m_uImpVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    for (const auto& imp : impacts) {
        glm::vec3 relPos = imp.positionAU - cameraTarget;
        glUniform3fv(m_uImpCenterLoc, 1, glm::value_ptr(relPos));
        glUniform3fv(m_uImpNormalLoc, 1, glm::value_ptr(imp.normal));
        glUniform1f(m_uImpRadiusLoc, imp.flashRadiusAU);
        glUniform1f(m_uImpIntensityLoc, imp.intensity);
        glUniform3fv(m_uImpColorLoc, 1, glm::value_ptr(imp.thermalColor));
        glUniform1f(m_uImpAgeLoc, imp.ageSeconds);

        glBindVertexArray(m_sphereMesh.vao);
        glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderSphere(const Camera& camera, float aspect, const CelestialBody& body, const glm::vec3& sunPos, const glm::vec3& cameraTarget) {
    glUseProgram(m_shaderProgram);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 camPos = camera.getEyePosition();

    glm::vec3 relativePos = body.position - cameraTarget;
    glm::vec3 relativeSunPos = sunPos - cameraTarget;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = glm::rotate(model, glm::radians(body.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(body.radius3D));

    glm::mat4 mvp = proj * view * model;
    glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

    glUniformMatrix4fv(m_uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix3fv(m_uNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));
    glUniform3fv(m_uColorLoc, 1, glm::value_ptr(body.color));
    glUniform3fv(m_uCameraPosLoc, 1, glm::value_ptr(camPos));

    bool isStar = (body.id == "sol" || body.type.find("Star") != std::string::npos);
    glUniform1i(m_uIsSunLoc, isStar ? 1 : 0);

    // Multi-light fallback to single star
    glUniform1i(m_uNumLightsLoc, 1);
    glUniform3fv(m_uLightPosLoc[0], 1, glm::value_ptr(relativeSunPos));
    glm::vec3 whiteSun(1.0f, 0.96f, 0.88f);
    glUniform3fv(m_uLightColorLoc[0], 1, glm::value_ptr(whiteSun));
    glUniform1f(m_uLightIntensityLoc[0], 1.0f);

    glUniform1i(m_uDebugOverlayLoc, 0);

    GLuint texId = 0;
    if (!body.texturePath.empty()) {
        texId = loadTexture(body.texturePath);
    }

    if (texId > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texId);
        glUniform1i(m_uTextureLoc, 0);
        glUniform1i(m_uUseTextureLoc, 1);
    } else {
        glUniform1i(m_uUseTextureLoc, 0);
    }

    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
}

void Renderer::renderSkybox(const Camera& camera, float aspect) {
    if (m_skyboxTexture == 0) {
        m_skyboxTexture = loadTexture("assets/textures/stars_milky_way.jpg");
    }

    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_skyboxProgram);

    glm::mat4 proj = glm::perspective(glm::radians(45.0f), std::max(aspect, 0.1f), 0.1f, 1000.0f);
    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
    glm::mat4 vp = proj * view * model;

    glUniformMatrix4fv(m_skyUVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    if (m_skyboxTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_skyboxTexture);
        glUniform1i(m_skyTexLoc, 0);
        if (m_skyHasTexLoc != -1) glUniform1i(m_skyHasTexLoc, 1);
    } else {
        if (m_skyHasTexLoc != -1) glUniform1i(m_skyHasTexLoc, 0);
    }

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderTrails(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& cameraTarget, int selectedIndex) {
    if (bodies.empty() || m_trailProgram == 0 || m_trailVAO == 0) return;
    if (selectedIndex < 0 || selectedIndex >= (int)bodies.size()) selectedIndex = 0;

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 vp = proj * view;

    glUseProgram(m_trailProgram);
    glUniformMatrix4fv(m_uTrailVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous celestial blend
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Find primary star position
    glm::vec3 starPos{0.0f};
    for (const auto& b : bodies) {
        if (b.id == "sol" || b.type.find("Star") != std::string::npos) {
            starPos = b.position;
            break;
        }
    }

    // 1. Render dynamic 3D Keplerian osculating orbit curve
    for (int i = 0; i < (int)bodies.size(); ++i) {
        const auto& body = bodies[i];
        if (body.id == "sol" || body.type.find("Star") != std::string::npos) continue;

        bool isSelected = (i == selectedIndex);
        float guideAlpha = isSelected ? 0.48f : 0.24f;
        glm::vec3 ringColor = body.color * (isSelected ? 1.30f : 0.88f);

        glm::vec3 centerPos = starPos;
        if (body.parentObjectId.has_value()) {
            for (const auto& p : bodies) {
                if (p.dbId == body.parentObjectId.value()) {
                    centerPos = p.position;
                    break;
                }
            }
        } else {
            bool isMoon = (body.type.find("Moon") != std::string::npos || body.type.find("Satellite") != std::string::npos ||
                           body.id == "moon" || body.id == "ganymede" || body.id == "europa" || body.id == "io" || body.id == "callisto" || body.id == "titan" ||
                           body.id == "phobos" || body.id == "deimos" || body.id == "enceladus" || body.id == "triton" || body.id == "charon");

            if (isMoon) {
                for (const auto& p : bodies) {
                    if ((body.id == "moon" && p.id == "earth") ||
                        ((body.id == "ganymede" || body.id == "europa" || body.id == "io" || body.id == "callisto") && p.id == "jupiter") ||
                        ((body.id == "titan" || body.id == "enceladus" || body.id == "mimas") && p.id == "saturn") ||
                        ((body.id == "phobos" || body.id == "deimos") && p.id == "mars") ||
                        ((body.id == "triton" || body.id == "proteus") && p.id == "neptune") ||
                        (body.id == "charon" && p.id == "pluto")) {
                        centerPos = p.position;
                        break;
                    }
                }
            }
        }
        glm::vec3 relCenterPos = centerPos - cameraTarget;

        std::vector<TrailVertex> orbitVerts;
        if (body.dynamicOrbitCurve.size() >= 2) {
            orbitVerts.reserve(body.dynamicOrbitCurve.size());
            for (size_t s = 0; s < body.dynamicOrbitCurve.size(); ++s) {
                const auto& pt = body.dynamicOrbitCurve[s];
                if (!std::isnan(pt.x) && !std::isnan(pt.y) && !std::isnan(pt.z) &&
                    !std::isinf(pt.x) && !std::isinf(pt.y) && !std::isinf(pt.z) &&
                    glm::length(pt) < 500.0f) {
                    glm::vec3 p = relCenterPos + pt;
                    orbitVerts.push_back({ p, glm::vec4(ringColor, guideAlpha) });
                }
            }
        } else {
            double orbitRadiusAU = (body.realOrbitRadiusAU > 0.0) ? body.realOrbitRadiusAU : (body.semiMajorAxisAU > 0.0 ? body.semiMajorAxisAU : (double)glm::length(body.position - centerPos));
            if (orbitRadiusAU > 0.00005 && orbitRadiusAU < 500.0) {
                const int circleSegments = 256;
                orbitVerts.reserve(circleSegments + 1);
                for (int s = 0; s <= circleSegments; ++s) {
                    float theta = 2.0f * PI * (float)s / (float)circleSegments;
                    glm::vec3 p = centerPos + glm::vec3((float)(orbitRadiusAU * std::cos(theta)), 0.0f, (float)(orbitRadiusAU * std::sin(theta))) - cameraTarget;
                    orbitVerts.push_back({ p, glm::vec4(ringColor, guideAlpha) });
                }
            }
        }

        if (!orbitVerts.empty()) {
            glBindVertexArray(m_trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
            glBufferData(GL_ARRAY_BUFFER, orbitVerts.size() * sizeof(TrailVertex), orbitVerts.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)orbitVerts.size());
        }
    }

    // 2. Render dynamic fading motion trails
    auto catmullRom = [](const glm::vec3& p0, const glm::vec3& p1, const glm::vec3& p2, const glm::vec3& p3, float t) -> glm::vec3 {
        float t2 = t * t;
        float t3 = t2 * t;
        return 0.5f * ((2.0f * p1) +
                       (-p0 + p2) * t +
                       (2.0f * p0 - 5.0f * p1 + 4.0f * p2 - p3) * t2 +
                       (-p0 + 3.0f * p1 - 3.0f * p2 + p3) * t3);
    };

    glm::vec3 camEye = cameraTarget + camera.getEyePosition();

    for (int i = 0; i < (int)bodies.size(); ++i) {
        const auto& body = bodies[i];
        if (body.id == "sol" || body.type.find("Star") != std::string::npos || body.trailHistory.size() < 2) continue;

        bool isSelected = (i == selectedIndex);

        std::vector<glm::vec3> pts;
        pts.reserve(body.trailHistory.size() + 1);

        double maxStepAU = 5.0;
        if (body.semiMajorAxisAU > 0.0) maxStepAU = std::max(2.0, body.semiMajorAxisAU * 0.5);

        for (const auto& p : body.trailHistory) {
            if (std::isnan(p.x) || std::isnan(p.y) || std::isnan(p.z) ||
                std::isinf(p.x) || std::isinf(p.y) || std::isinf(p.z) ||
                glm::length(p) > 1000.0f) continue;

            if (!pts.empty()) {
                float segDist = glm::distance(pts.back(), p);
                if (segDist > (float)maxStepAU) {
                    pts.clear();
                }
            }
            pts.push_back(p);
        }

        if (!pts.empty()) {
            float distToHead = glm::distance(pts.back(), body.position);
            if (distToHead > (float)maxStepAU) {
                pts.clear();
            }
        }
        pts.push_back(body.position);

        size_t nPts = pts.size();
        if (nPts < 2) continue;

        float distToCam = glm::length(body.position - camEye);
        int subdivisions = (isSelected || distToCam < 6.0f) ? 4 : (distToCam < 20.0f ? 3 : 1);

        std::vector<TrailVertex> trailVerts;
        trailVerts.reserve((nPts - 1) * subdivisions + 2);

        for (size_t seg = 0; seg < nPts - 1; ++seg) {
            const glm::vec3& p1 = pts[seg];
            const glm::vec3& p2 = pts[seg + 1];
            glm::vec3 p0 = (seg > 0) ? pts[seg - 1] : p1 + (p1 - p2);
            glm::vec3 p3 = (seg + 2 < nPts) ? pts[seg + 2] : p2 + (p2 - p1);

            float tStart = (float)seg / (float)(nPts - 1);
            float tEnd = (float)(seg + 1) / (float)(nPts - 1);

            float segDist = glm::distance(p1, p2);
            int numSteps = (segDist > 1.0f) ? 1 : subdivisions;

            for (int s = 0; s < numSteps; ++s) {
                float u = (float)s / (float)numSteps;
                glm::vec3 interpolatedWorld = (numSteps > 1) ? catmullRom(p0, p1, p2, p3, u) : p1;
                glm::vec3 relPos = interpolatedWorld - cameraTarget;

                if (std::isnan(relPos.x) || std::isnan(relPos.y) || std::isnan(relPos.z) ||
                    std::isinf(relPos.x) || std::isinf(relPos.y) || std::isinf(relPos.z)) continue;

                float tGlobal = tStart + u * (tEnd - tStart);
                float alpha = std::pow(tGlobal, 1.4f) * (isSelected ? 0.98f : 0.82f);
                glm::vec3 col = body.color * (0.55f + 0.65f * tGlobal);
                if (isSelected) col *= 1.25f;

                trailVerts.push_back({ relPos, glm::vec4(col, alpha) });
            }
        }

        glm::vec3 currRelPos = body.position - cameraTarget;
        glm::vec3 headCol = body.color * (isSelected ? 1.5f : 1.2f);
        trailVerts.push_back({ currRelPos, glm::vec4(headCol, isSelected ? 1.0f : 0.95f) });

        if (!trailVerts.empty()) {
            glBindVertexArray(m_trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
            glBufferData(GL_ARRAY_BUFFER, trailVerts.size() * sizeof(TrailVertex), trailVerts.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)trailVerts.size());

            if (isSelected) {
                glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)trailVerts.size());
            }
        }
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderRings(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const std::vector<StarLightSource>& stars, const glm::vec3& cameraTarget) {
    if (bodies.empty() || m_ringProgram == 0 || m_ringMesh.vao == 0) return;

    if (m_ringTexture == 0) {
        m_ringTexture = loadTexture("assets/textures/saturn_ring_alpha.png");
    }

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    glUseProgram(m_ringProgram);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    if (m_ringTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ringTexture);
        glUniform1i(m_uRingTexLoc, 0);
        glUniform1i(m_uRingHasTexLoc, 1);
    } else {
        glUniform1i(m_uRingHasTexLoc, 0);
    }

    glm::vec3 sunPos = stars.empty() ? glm::vec3(0.0f) : stars[0].positionAU;

    for (const auto& body : bodies) {
        if (!body.ring.hasRing) continue;

        glm::vec3 relativePlanetCenter = body.position - cameraTarget;
        glm::vec3 relativeSunPos = sunPos - cameraTarget;

        float innerR = (body.ring.innerRadius3D > 0.0f) ? body.ring.innerRadius3D : (float)(body.radius3D * 1.35f);
        float outerR = (body.ring.outerRadius3D > 0.0f) ? body.ring.outerRadius3D : (float)(body.radius3D * 2.45f);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePlanetCenter);
        model = glm::rotate(model, glm::radians(body.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
        model = glm::rotate(model, body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 mvp = proj * view * model;
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(model)));

        glUniformMatrix4fv(m_uRingMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(m_uRingModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(m_uRingNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normalMat));

        glUniform1f(glGetUniformLocation(m_ringProgram, "uInnerRadius"), innerR);
        glUniform1f(glGetUniformLocation(m_ringProgram, "uOuterRadius"), outerR);
        glUniform3fv(m_uRingSunPosLoc, 1, glm::value_ptr(relativeSunPos));
        glUniform3fv(m_uRingPlanetCenterLoc, 1, glm::value_ptr(relativePlanetCenter));
        glUniform1f(m_uRingPlanetRadiusLoc, body.radius3D);
        glUniform3fv(m_uRingColorLoc, 1, glm::value_ptr(body.ring.baseColor));

        glBindVertexArray(m_ringMesh.vao);
        glDrawElements(GL_TRIANGLES, m_ringMesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glEnable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderParticleField(const Camera& camera, float aspect, ParticleField& field, const glm::vec3& sunPos, const glm::vec3& cameraTarget, double simTime) {
    field.updateVisualInstanceBuffer(simTime, cameraTarget, 1.0f);
    m_particleRenderer.render(camera, aspect, field.getInstanceData(), sunPos, cameraTarget);
}



void Renderer::renderDeformableBodies(const Camera& camera, float aspect, const MatterSystem& matter, const std::vector<StarLightSource>& stars, const glm::vec3& cameraTarget, MatterVisualizationMode visMode) {
    if (matter.getBodies().empty()) return;
    glm::vec3 sunPos = stars.empty() ? glm::vec3(0.0f) : stars[0].positionAU;
    m_deformableRenderer.render(camera, aspect, matter.getBodies(), visMode, sunPos, cameraTarget, true);
}

} // namespace AstroGenesis
