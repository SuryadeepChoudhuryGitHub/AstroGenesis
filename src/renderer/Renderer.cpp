#include "renderer/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdio>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

namespace AstroGenesis {

static const float PI = 3.14159265358979323846f;

static const char* vertShaderSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 Normal;
out vec2 TexCoord;

uniform mat4 uMVP;
uniform mat4 uModel;

void main() {
    FragPos = vec3(uModel * vec4(aPos, 1.0));
    Normal = mat3(transpose(inverse(uModel))) * aNormal;
    TexCoord = aTexCoord;
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* fragShaderSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec2 TexCoord;

uniform vec3 uColor;
uniform sampler2D uTexture;
uniform bool uUseTexture;
uniform vec3 uLightPos;
uniform bool uIsSun;

void main() {
    vec3 baseColor = uColor;
    if (uUseTexture) {
        baseColor = texture(uTexture, TexCoord).rgb;
    }

    if (uIsSun) {
        FragColor = vec4(baseColor, 1.0);
        return;
    }

    // Solar Directional / Point Light Calculation
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(uLightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.25); // ambient factor 0.25

    vec3 result = baseColor * diff;
    FragColor = vec4(result, 1.0);
}
)GLSL";

// ---------- Skybox shaders (unlit, textured inside of a sphere) ----------
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

// ---------- 3D Celestial Motion Trail Shader ----------
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

// ---------- Planetary Continuous Granular Fluid Ring Shader ----------
static const char* ringVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoord;

out vec3 FragPos;
out vec3 WorldNormal;
out vec2 TexCoord;
out float NormalizedRadius;
out vec3 LocalPos;

uniform mat4 uMVP;
uniform mat4 uModel;
uniform mat3 uNormalMat;
uniform float uInnerRadius;
uniform float uOuterRadius;

void main() {
    float r = mix(uInnerRadius, uOuterRadius, aTexCoord.x);
    vec3 pos = vec3(aPos.x * r, aPos.y, aPos.z * r);
    LocalPos = pos;
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
in vec3 LocalPos;

uniform vec3 uSunPos;
uniform vec3 uPlanetCenter;
uniform float uPlanetRadius;
uniform vec3 uRingColor;
uniform vec3 uCameraPos;

uniform sampler2D uRingTexture;
uniform bool uHasRingTexture;

// Hydrodynamic disturbance voids & gravitational wakes (up to 16 active)
uniform int uNumDisturbances;
uniform vec4 uDisturbances[16]; // x: normRadius, y: azimuthRad, z: radialWidth, w: angularWidth
uniform float uDistIntensity[16];

// Ray-Sphere intersection for planetary shadow on ring
float calculatePlanetShadow(vec3 fragPos, vec3 sunPos, vec3 planetCenter, float planetRadius) {
    vec3 rayDir = normalize(sunPos - fragPos);
    vec3 L = planetCenter - fragPos;
    float tca = dot(L, rayDir);
    if (tca < 0.0) return 1.0;
    float d2 = dot(L, L) - tca * tca;
    float r2 = planetRadius * planetRadius;
    if (d2 > r2) return 1.0;

    // Soft shadow edge penumbra
    float penumbra = smoothstep(r2 * 0.90, r2 * 1.05, d2);
    return penumbra;
}

// Procedural fallback optical depth profile (if texture not loaded)
void getProceduralRingProfile(float u, out float opticalDepth, out vec3 tint) {
    opticalDepth = 0.0;
    tint = vec3(1.0);
    
    if (u < 0.08) {
        opticalDepth = smoothstep(0.0, 0.08, u) * 0.15;
        tint = vec3(0.75, 0.70, 0.65);
    } else if (u < 0.35) {
        float t = (u - 0.08) / 0.27;
        opticalDepth = 0.22 + 0.12 * sin(t * 18.0) * 0.5;
        tint = vec3(0.80, 0.76, 0.68);
    } else if (u < 0.65) {
        float t = (u - 0.35) / 0.30;
        opticalDepth = 0.90 + 0.08 * sin(t * 35.0);
        tint = vec3(0.96, 0.92, 0.82);
    } else if (u < 0.72) {
        float t = (u - 0.65) / 0.07;
        opticalDepth = 0.03 + 0.02 * sin(t * 6.0);
        tint = vec3(0.40, 0.38, 0.35);
    } else if (u < 0.96) {
        float t = (u - 0.72) / 0.24;
        opticalDepth = 0.65 + 0.06 * sin(t * 28.0);
        tint = vec3(0.88, 0.84, 0.75);
        if (u >= 0.87 && u <= 0.89) {
            opticalDepth = 0.04;
            tint = vec3(0.45, 0.42, 0.40);
        }
    } else {
        float t = (u - 0.96) / 0.04;
        opticalDepth = sin(t * 3.14159) * 0.30;
        tint = vec3(0.70, 0.68, 0.65);
    }
}

void main() {
    float u = clamp(NormalizedRadius, 0.0, 1.0);
    
    vec3 baseAlbedo;
    float baseOpacity;

    if (uHasRingTexture) {
        // Sample authentic Cassini/Voyager saturn_ring_alpha.png
        vec4 texCol = texture(uRingTexture, vec2(u, 0.5));
        baseAlbedo = texCol.rgb;
        baseOpacity = texCol.a;
    } else {
        vec3 tint;
        getProceduralRingProfile(u, baseOpacity, tint);
        baseAlbedo = uRingColor * tint;
    }

    // Evaluate Hydrodynamic Granular Fluid Disturbances & Wakes
    float densityMult = 1.0;
    vec3 wakeGlow = vec3(0.0);
    float localAzimuth = atan(LocalPos.z, LocalPos.x); // [-PI, PI]

    for (int i = 0; i < uNumDisturbances; ++i) {
        float dR = (u - uDisturbances[i].x) / max(uDisturbances[i].z, 0.01);
        float dTheta = localAzimuth - uDisturbances[i].y;
        if (dTheta > 3.14159265) dTheta -= 6.2831853;
        if (dTheta < -3.14159265) dTheta += 6.2831853;
        dTheta = dTheta / max(uDisturbances[i].w, 0.01);

        float distSq = dR * dR + dTheta * dTheta;
        if (distSq < 9.0) {
            float voidFactor = exp(-distSq * 1.5) * uDistIntensity[i];
            densityMult *= (1.0 - clamp(voidFactor, 0.0, 1.0));
            // Gravitational wake wave rim (compression ridge)
            float rim = smoothstep(0.8, 1.8, distSq) * (1.0 - smoothstep(1.8, 4.0, distSq)) * uDistIntensity[i];
            wakeGlow += vec3(0.4, 0.35, 0.25) * rim;
        }
    }

    float finalOpacity = baseOpacity * densityMult;
    if (finalOpacity <= 0.003) {
        discard;
    }

    vec3 lightDir = normalize(uSunPos - FragPos);
    vec3 viewDir = normalize(uCameraPos - FragPos);

    // Double-sided lighting
    float nDotL = abs(dot(WorldNormal, lightDir));
    float diff = max(nDotL, 0.12);

    // Ice particle Henyey-Greenstein / Mie forward scattering
    float cosTheta = dot(-lightDir, viewDir);
    float g = 0.65;
    float phase = (1.0 - g * g) / pow(1.0 + g * g - 2.0 * g * cosTheta, 1.5);
    phase = clamp(phase * 0.25, 0.0, 2.0);

    // Planetary shadow calculation (Saturn casting shadow on rings)
    float shadow = calculatePlanetShadow(FragPos, uSunPos, uPlanetCenter, uPlanetRadius);

    vec3 finalColor = (baseAlbedo + wakeGlow) * (diff + phase * 0.4) * shadow;
    FragColor = vec4(finalColor, clamp(finalOpacity * 0.98, 0.0, 0.98));
}
)GLSL";

struct TrailVertex {
    glm::vec3 pos;
    glm::vec4 col;
};

static GLuint compileShader(GLenum type, const char* src) {
    GLuint s = glCreateShader(type);
    glShaderSource(s, 1, &src, nullptr);
    glCompileShader(s);
    int ok;
    glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[512];
        glGetShaderInfoLog(s, 512, nullptr, log);
        fprintf(stderr, "Shader Compile Error: %s\n", log);
    }
    return s;
}

Renderer::Renderer() {}

Renderer::~Renderer() {
    shutdown();
}

bool Renderer::initialize() {
    glEnable(GL_MULTISAMPLE);
    glEnable(GL_DEPTH_TEST);

    GLuint vShader = compileShader(GL_VERTEX_SHADER, vertShaderSrc);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vShader);
    glAttachShader(m_shaderProgram, fShader);
    glLinkProgram(m_shaderProgram);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    m_uMVPLoc        = glGetUniformLocation(m_shaderProgram, "uMVP");
    m_uModelLoc      = glGetUniformLocation(m_shaderProgram, "uModel");
    m_uColorLoc      = glGetUniformLocation(m_shaderProgram, "uColor");
    m_uUseTextureLoc = glGetUniformLocation(m_shaderProgram, "uUseTexture");
    m_uTextureLoc    = glGetUniformLocation(m_shaderProgram, "uTexture");
    m_uLightPosLoc   = glGetUniformLocation(m_shaderProgram, "uLightPos");
    m_uIsSunLoc      = glGetUniformLocation(m_shaderProgram, "uIsSun");

    m_sphereMesh = createSphereMesh(1.0f, 48, 64);

    // ---------- Skybox shader program ----------
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

    // Pre-load the skybox texture
    m_skyboxTexture = loadTexture("assets/textures/stars_milky_way.jpg");
    if (m_skyboxTexture == 0) {
        fprintf(stderr, "WARNING: Skybox texture failed to load initially (will retry on demand).\n");
    }

    // ---------- Trail shader & dynamic buffers ----------
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
    glBufferData(GL_ARRAY_BUFFER, 120000 * sizeof(TrailVertex), nullptr, GL_DYNAMIC_DRAW);

    GLsizei stride = sizeof(TrailVertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    // ---------- Planetary Ring Shader & Mesh ----------
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
    m_uRingCameraPosLoc    = glGetUniformLocation(m_ringProgram, "uCameraPos");
    m_uRingTexLoc          = glGetUniformLocation(m_ringProgram, "uRingTexture");
    m_uRingHasTexLoc       = glGetUniformLocation(m_ringProgram, "uHasRingTexture");
    m_uNumDisturbancesLoc  = glGetUniformLocation(m_ringProgram, "uNumDisturbances");
    m_uDisturbancesLoc     = glGetUniformLocation(m_ringProgram, "uDisturbances");
    m_uDistIntensityLoc    = glGetUniformLocation(m_ringProgram, "uDistIntensity");

    m_ringTexture = loadTexture("assets/textures/saturn_ring_alpha.png");

    m_ringMesh = createRingMesh(128);

    m_particleRenderer.initialize();
    m_deformableRenderer.initialize();

    return true;
}

void Renderer::shutdown() {
    for (auto& pair : m_textures) {
        if (pair.second) {
            glDeleteTextures(1, &pair.second);
        }
    }
    m_textures.clear();

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
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_skyboxProgram) {
        glDeleteProgram(m_skyboxProgram);
        m_skyboxProgram = 0;
    }
    if (m_ringProgram) {
        glDeleteProgram(m_ringProgram);
        m_ringProgram = 0;
    }
    if (m_trailVAO) {
        glDeleteVertexArrays(1, &m_trailVAO);
        glDeleteBuffers(1, &m_trailVBO);
        m_trailVAO = 0;
        m_trailVBO = 0;
    }
    if (m_trailProgram) {
        glDeleteProgram(m_trailProgram);
        m_trailProgram = 0;
    }
    m_particleRenderer.shutdown();
    m_deformableRenderer.shutdown();
}

GLuint Renderer::loadTexture(const std::string& filepath) {
    if (filepath.empty()) return 0;
    auto it = m_textures.find(filepath);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Extract filename from filepath
    std::string filename = filepath;
    size_t lastSlash = filepath.find_last_of("/\\");
    if (lastSlash != std::string::npos) {
        filename = filepath.substr(lastSlash + 1);
    }

    // Try multiple candidate paths to find the file from any working directory
    std::vector<std::string> candidates = {
        filepath,
        "../" + filepath,
        "../../" + filepath,
        "assets/textures/" + filename,
        "../assets/textures/" + filename,
        "../../assets/textures/" + filename,
        "build/" + filepath,
        "build/Debug/" + filepath,
        "build/Release/" + filepath,
        "../Debug/" + filepath,
        "../Release/" + filepath,
        "Debug/" + filepath,
        "Release/" + filepath
    };

    int width = 0, height = 0, nrChannels = 0;
    unsigned char* data = nullptr;
    std::string matchedPath;

    stbi_set_flip_vertically_on_load(false);
    for (const auto& path : candidates) {
        data = stbi_load(path.c_str(), &width, &height, &nrChannels, 0);
        if (data) {
            matchedPath = path;
            break;
        }
    }

    if (!data) {
        fprintf(stderr, "Failed to load texture: %s (searched candidate directories)\n", filepath.c_str());
        m_textures[filepath] = 0;
        return 0;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    GLuint textureID = 0;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    printf("Successfully loaded texture: %s from %s (%dx%d, %d channels)\n", filepath.c_str(), matchedPath.c_str(), width, height, nrChannels);

    m_textures[filepath] = textureID;
    return textureID;
}

MeshData Renderer::createSphereMesh(float radius, int stacks, int sectors) {
    std::vector<float> verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI / 2.0f - PI * (float)i / (float)stacks;
        float y   = radius * sinf(phi);
        float r   = radius * cosf(phi);

        float v = (float)i / (float)stacks;

        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * PI * (float)j / (float)sectors;
            float x = r * cosf(theta);
            float z = r * sinf(theta);

            float u = 1.0f - (float)j / (float)sectors;

            // Normal vector
            float nx = x / radius;
            float ny = y / radius;
            float nz = z / radius;

            // Position (3)
            verts.push_back(x);
            verts.push_back(y);
            verts.push_back(z);

            // Normal (3)
            verts.push_back(nx);
            verts.push_back(ny);
            verts.push_back(nz);

            // TexCoords (2)
            verts.push_back(u);
            verts.push_back(v);
        }
    }

    for (int i = 0; i < stacks; ++i) {
        for (int j = 0; j < sectors; ++j) {
            int a = i * (sectors + 1) + j;
            int b = a + sectors + 1;
            indices.push_back(a); indices.push_back(b); indices.push_back(a + 1);
            indices.push_back(a + 1); indices.push_back(b); indices.push_back(b + 1);
        }
    }

    MeshData mesh;
    mesh.indexCount = (int)indices.size();
    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);
    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    GLsizei stride = 8 * sizeof(float);
    // Attribute 0: Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glEnableVertexAttribArray(0);

    // Attribute 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // Attribute 2: TexCoords
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);
    return mesh;
}

MeshData Renderer::createRingMesh(int radialSegments) {
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= radialSegments; ++i) {
        float theta = 2.0f * PI * (float)i / (float)radialSegments;
        float cosT = std::cos(theta);
        float sinT = std::sin(theta);

        // Inner vertex: normalized radial coordinate u = 0.0
        vertices.push_back(cosT);
        vertices.push_back(0.0f);
        vertices.push_back(sinT);
        vertices.push_back(0.0f); // u = 0.0 (inner boundary)
        vertices.push_back((float)i / (float)radialSegments); // v

        // Outer vertex: normalized radial coordinate u = 1.0
        vertices.push_back(cosT);
        vertices.push_back(0.0f);
        vertices.push_back(sinT);
        vertices.push_back(1.0f); // u = 1.0 (outer boundary)
        vertices.push_back((float)i / (float)radialSegments); // v
    }

    for (int i = 0; i < radialSegments; ++i) {
        unsigned int i0 = (unsigned int)(i * 2);
        unsigned int i1 = (unsigned int)(i0 + 1);
        unsigned int i2 = (unsigned int)((i + 1) * 2);
        unsigned int i3 = (unsigned int)(i2 + 1);

        // Top face
        indices.push_back(i0);
        indices.push_back(i1);
        indices.push_back(i2);

        indices.push_back(i1);
        indices.push_back(i3);
        indices.push_back(i2);

        // Bottom face (double-sided)
        indices.push_back(i2);
        indices.push_back(i1);
        indices.push_back(i0);

        indices.push_back(i2);
        indices.push_back(i3);
        indices.push_back(i1);
    }

    MeshData mesh;
    mesh.indexCount = (int)indices.size();

    glGenVertexArrays(1, &mesh.vao);
    glGenBuffers(1, &mesh.vbo);
    glGenBuffers(1, &mesh.ebo);

    glBindVertexArray(mesh.vao);

    glBindBuffer(GL_ARRAY_BUFFER, mesh.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh.ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Position: layout 0 (3 floats)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // TexCoord: layout 1 (2 floats)
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    return mesh;
}

void Renderer::beginViewport(int x, int y, int width, int height, const glm::vec4& clearColor) {
    glViewport(x, y, width, height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderSphere(const Camera& camera, float aspect, const CelestialBody& body, const glm::vec3& sunPos, const glm::vec3& cameraTarget) {
    glUseProgram(m_shaderProgram);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    // Camera-relative rendering: subtract camera target so focused body is at origin.
    // This maximizes float32 precision and eliminates z-fighting at large AU coordinates.
    glm::vec3 relativePos = body.position - cameraTarget;
    glm::vec3 relativeSunPos = sunPos - cameraTarget;

    glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
    model = glm::rotate(model, glm::radians(body.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(body.radius3D));

    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(m_uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniformMatrix4fv(m_uModelLoc, 1, GL_FALSE, glm::value_ptr(model));
    glUniform3fv(m_uColorLoc, 1, glm::value_ptr(body.color));
    glUniform3fv(m_uLightPosLoc, 1, glm::value_ptr(relativeSunPos));
    glUniform1i(m_uIsSunLoc, (body.id == "sol") ? 1 : 0);

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

void Renderer::endViewport(int windowWidth, int windowHeight) {
    glViewport(0, 0, windowWidth, windowHeight);
}

void Renderer::renderSkybox(const Camera& camera, float aspect) {
    if (m_skyboxTexture == 0) {
        m_skyboxTexture = loadTexture("assets/textures/stars_milky_way.jpg");
    }

    // Skybox renders first — disable depth test and depth writing
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glDisable(GL_CULL_FACE);

    glUseProgram(m_skyboxProgram);

    // Fixed projection: 45 degree FOV matching scene, guaranteed within near/far planes
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), std::max(aspect, 0.1f), 0.1f, 1000.0f);

    // Strip translation from view matrix so skybox is always centered at camera origin
    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));

    // Scale unit sphere to radius 100
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
    glm::mat4 vp = proj * view * model;

    glUniformMatrix4fv(m_skyUVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    if (m_skyboxTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_skyboxTexture);
        glUniform1i(m_skyTexLoc, 0);
        if (m_skyHasTexLoc != -1) {
            glUniform1i(m_skyHasTexLoc, 1);
        }
    } else {
        if (m_skyHasTexLoc != -1) {
            glUniform1i(m_skyHasTexLoc, 0);
        }
    }

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    // Restore state for subsequent celestial body rendering
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderTrails(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& cameraTarget, int selectedIndex) {
    if (bodies.empty() || m_trailProgram == 0 || m_trailVAO == 0) return;

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 vp = proj * view;

    glUseProgram(m_trailProgram);
    glUniformMatrix4fv(m_uTrailVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE); // Additive luminous celestial blend
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Trails don't write to depth buffer so they don't occlude planets

    // 1. Render subtle full orbital guide rings
    std::vector<TrailVertex> guideVerts;
    const int circleSegments = 128;
    for (int i = 0; i < (int)bodies.size(); ++i) {
        const auto& body = bodies[i];
        if (body.id == "sol" || body.realOrbitRadiusAU <= 0.0) continue;

        bool isSelected = (i == selectedIndex);
        float guideAlpha = isSelected ? 0.28f : 0.12f;
        glm::vec3 ringColor = body.color * (isSelected ? 1.0f : 0.7f);

        for (int s = 0; s < circleSegments; ++s) {
            float theta1 = 2.0f * PI * (float)s / (float)circleSegments;
            float theta2 = 2.0f * PI * (float)(s + 1) / (float)circleSegments;

            glm::vec3 p1 = glm::vec3((float)(body.realOrbitRadiusAU * std::cos(theta1)), 0.0f, (float)(body.realOrbitRadiusAU * std::sin(theta1))) - cameraTarget;
            glm::vec3 p2 = glm::vec3((float)(body.realOrbitRadiusAU * std::cos(theta2)), 0.0f, (float)(body.realOrbitRadiusAU * std::sin(theta2))) - cameraTarget;

            guideVerts.push_back({ p1, glm::vec4(ringColor, guideAlpha) });
            guideVerts.push_back({ p2, glm::vec4(ringColor, guideAlpha) });
        }
    }

    if (!guideVerts.empty()) {
        glBindVertexArray(m_trailVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
        glBufferData(GL_ARRAY_BUFFER, guideVerts.size() * sizeof(TrailVertex), guideVerts.data(), GL_DYNAMIC_DRAW);
        glDrawArrays(GL_LINES, 0, (GLsizei)guideVerts.size());
    }

    // 2. Render dynamic fading motion trails tracking actual real-time positions
    for (int i = 0; i < (int)bodies.size(); ++i) {
        const auto& body = bodies[i];
        if (body.id == "sol" || body.trailHistory.size() < 2) continue;

        bool isSelected = (i == selectedIndex);
        std::vector<TrailVertex> trailVerts;
        trailVerts.reserve(body.trailHistory.size() + 1);

        size_t n = body.trailHistory.size();
        for (size_t pt = 0; pt < n; ++pt) {
            float t = (float)pt / (float)(n - 1); // 0.0 at oldest tail point, 1.0 at head point
            float alpha = std::pow(t, 1.5f) * (isSelected ? 0.98f : 0.82f);
            glm::vec3 col = body.color * (0.5f + 0.7f * t);
            if (isSelected) col *= 1.25f;

            glm::vec3 relPos = body.trailHistory[pt] - cameraTarget;
            trailVerts.push_back({ relPos, glm::vec4(col, alpha) });
        }

        // Connect directly to current planet position
        glm::vec3 currRelPos = body.position - cameraTarget;
        glm::vec3 headCol = body.color * (isSelected ? 1.5f : 1.2f);
        trailVerts.push_back({ currRelPos, glm::vec4(headCol, isSelected ? 1.0f : 0.95f) });

        if (!trailVerts.empty()) {
            glBindVertexArray(m_trailVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_trailVBO);
            glBufferData(GL_ARRAY_BUFFER, trailVerts.size() * sizeof(TrailVertex), trailVerts.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)trailVerts.size());

            // Secondary pass for selected body to give rich glowing halo
            if (isSelected) {
                glDrawArrays(GL_LINE_STRIP, 0, (GLsizei)trailVerts.size());
            }
        }
    }

    glBindVertexArray(0);

    // Restore state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderRings(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& sunPos, const glm::vec3& cameraTarget) {
    if (bodies.empty() || m_ringProgram == 0 || m_ringMesh.vao == 0) return;

    if (m_ringTexture == 0) {
        m_ringTexture = loadTexture("assets/textures/saturn_ring_alpha.png");
    }

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::vec3 camPos = camera.getEyePosition();

    glUseProgram(m_ringProgram);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE); // Semi-transparent rings
    glDisable(GL_CULL_FACE); // Double-sided rendering

    if (m_ringTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_ringTexture);
        glUniform1i(m_uRingTexLoc, 0);
        glUniform1i(m_uRingHasTexLoc, 1);
    } else {
        glUniform1i(m_uRingHasTexLoc, 0);
    }

    for (const auto& body : bodies) {
        if (!body.ring.hasRing || body.ring.outerRadius3D <= 0.0f) continue;

        glm::vec3 relativePos = body.position - cameraTarget;
        glm::vec3 relativeSun = sunPos - cameraTarget;

        glm::mat4 model = glm::translate(glm::mat4(1.0f), relativePos);
        // Apply axial tilt (must match planet's exact tilt axis (0, 0, 1) for equatorial coplanarity)
        model = glm::rotate(model, glm::radians(body.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
        // Apply planetary rotation
        model = glm::rotate(model, body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));

        glm::mat4 mvp = proj * view * model;
        glm::mat3 normMat = glm::transpose(glm::inverse(glm::mat3(model)));

        glUniformMatrix4fv(m_uRingMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
        glUniformMatrix4fv(m_uRingModelLoc, 1, GL_FALSE, glm::value_ptr(model));
        glUniformMatrix3fv(m_uRingNormalMatLoc, 1, GL_FALSE, glm::value_ptr(normMat));

        glUniform1f(glGetUniformLocation(m_ringProgram, "uInnerRadius"), body.ring.innerRadius3D);
        glUniform1f(glGetUniformLocation(m_ringProgram, "uOuterRadius"), body.ring.outerRadius3D);

        glUniform3fv(m_uRingSunPosLoc, 1, glm::value_ptr(relativeSun));
        glUniform3fv(m_uRingPlanetCenterLoc, 1, glm::value_ptr(relativePos));
        glUniform1f(m_uRingPlanetRadiusLoc, body.radius3D);
        glUniform3fv(m_uRingColorLoc, 1, glm::value_ptr(body.ring.baseColor));
        glUniform3fv(m_uRingCameraPosLoc, 1, glm::value_ptr(camPos));

        // Pack dynamic hydrodynamic disturbances
        int numDist = std::min((int)body.ring.disturbances.size(), (int)PlanetaryRing::MAX_DISTURBANCES);
        glm::vec4 distVecs[PlanetaryRing::MAX_DISTURBANCES];
        float distIntensities[PlanetaryRing::MAX_DISTURBANCES];

        for (int d = 0; d < numDist; ++d) {
            const auto& dist = body.ring.disturbances[d];
            distVecs[d] = glm::vec4(dist.normRadius, dist.azimuthRad, dist.radialWidth, dist.angularWidth);
            distIntensities[d] = dist.intensity;
        }

        glUniform1i(m_uNumDisturbancesLoc, numDist);
        if (numDist > 0) {
            glUniform4fv(m_uDisturbancesLoc, numDist, glm::value_ptr(distVecs[0]));
            glUniform1fv(m_uDistIntensityLoc, numDist, distIntensities);
        }

        glBindVertexArray(m_ringMesh.vao);
        glDrawElements(GL_TRIANGLES, m_ringMesh.indexCount, GL_UNSIGNED_INT, 0);
    }

    glBindVertexArray(0);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
}

void Renderer::renderParticleField(const Camera& camera, float aspect, ParticleField& field,
                                  const glm::vec3& sunPos, const glm::vec3& cameraTarget, double simTime) {
    field.updateVisualInstanceBuffer(simTime, cameraTarget, field.getVisualSizeMultiplier());
    m_particleRenderer.render(camera, aspect, field.getInstanceData(), sunPos, cameraTarget);
}

void Renderer::renderDeformableBodies(const Camera& camera, float aspect, const MatterSystem& matter,
                                     const glm::vec3& sunPos, const glm::vec3& cameraTarget) {
    m_deformableRenderer.render(camera, aspect, matter.getBodies(), matter.getVisualizationMode(), sunPos, cameraTarget, true);
}

} // namespace AstroGenesis
