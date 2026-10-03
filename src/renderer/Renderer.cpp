#include "renderer/Renderer.hpp"
#include "renderer/ShaderLoader.hpp"
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
    std::string celestialVertSrc = loadShaderSource("assets/shaders/celestial.vert");
    std::string celestialFragSrc = loadShaderSource("assets/shaders/celestial.frag");
    GLuint vShader = compileShader(GL_VERTEX_SHADER, celestialVertSrc.c_str());
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, celestialFragSrc.c_str());
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

    // Physical Material & Cloud Shadow Uniforms
    m_uWaterFractionLoc          = glGetUniformLocation(m_shaderProgram, "uWaterFraction");
    m_uIceFractionLoc            = glGetUniformLocation(m_shaderProgram, "uIceFraction");
    m_uRoughnessLoc              = glGetUniformLocation(m_shaderProgram, "uRoughness");
    m_uCloudShadowCoverageLoc    = glGetUniformLocation(m_shaderProgram, "uCloudShadowCoverage");
    m_uCloudShadowRotAngleLoc    = glGetUniformLocation(m_shaderProgram, "uCloudShadowRotAngle");
    m_uCinematicModeLoc          = glGetUniformLocation(m_shaderProgram, "uCinematicMode");
    m_uHasRingLoc                = glGetUniformLocation(m_shaderProgram, "uHasRing");
    m_uRingNormalLoc             = glGetUniformLocation(m_shaderProgram, "uRingNormal");
    m_uPlanetCenterLoc           = glGetUniformLocation(m_shaderProgram, "uPlanetCenter");
    m_uRingInnerRadiusLoc        = glGetUniformLocation(m_shaderProgram, "uRingInnerRadius");
    m_uRingOuterRadiusLoc        = glGetUniformLocation(m_shaderProgram, "uRingOuterRadius");

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
    std::string atmoVertSrc = loadShaderSource("assets/shaders/atmosphere.vert");
    std::string atmoFragSrc = loadShaderSource("assets/shaders/atmosphere.frag");
    GLuint atmoV = compileShader(GL_VERTEX_SHADER, atmoVertSrc.c_str());
    GLuint atmoF = compileShader(GL_FRAGMENT_SHADER, atmoFragSrc.c_str());
    m_atmosphereProgram = glCreateProgram();
    glAttachShader(m_atmosphereProgram, atmoV);
    glAttachShader(m_atmosphereProgram, atmoF);
    glLinkProgram(m_atmosphereProgram);
    glDeleteShader(atmoV);
    glDeleteShader(atmoF);

    m_uAtmoMVPLoc          = glGetUniformLocation(m_atmosphereProgram, "uMVP");
    m_uAtmoModelLoc        = glGetUniformLocation(m_atmosphereProgram, "uModel");
    m_uAtmoColorLoc        = glGetUniformLocation(m_atmosphereProgram, "uAtmoColor");
    m_uAtmoDensityLoc      = glGetUniformLocation(m_atmosphereProgram, "uAtmoDensity");
    m_uAtmoCameraPosLoc    = glGetUniformLocation(m_atmosphereProgram, "uCameraPos");
    m_uAtmoNumLightsLoc    = glGetUniformLocation(m_atmosphereProgram, "uNumLights");
    m_uAtmoScaleHeightLoc  = glGetUniformLocation(m_atmosphereProgram, "uAtmoScaleHeight");
    m_uAtmoMieFactorLoc    = glGetUniformLocation(m_atmosphereProgram, "uAtmoMieFactor");
    m_uAtmoPlanetRadiusLoc = glGetUniformLocation(m_atmosphereProgram, "uPlanetRadius");

    for (int i = 0; i < 4; ++i) {
        char pBuf[32], cBuf[32];
        snprintf(pBuf, sizeof(pBuf), "uLightPos[%d]", i);
        snprintf(cBuf, sizeof(cBuf), "uLightColor[%d]", i);
        m_uAtmoLightPosLoc[i] = glGetUniformLocation(m_atmosphereProgram, pBuf);
        m_uAtmoLightColorLoc[i] = glGetUniformLocation(m_atmosphereProgram, cBuf);
    }

    // 3. Compile Cloud Shader
    std::string cloudVertSrc = loadShaderSource("assets/shaders/cloud.vert");
    std::string cloudFragSrc = loadShaderSource("assets/shaders/cloud.frag");
    GLuint cloudV = compileShader(GL_VERTEX_SHADER, cloudVertSrc.c_str());
    GLuint cloudF = compileShader(GL_FRAGMENT_SHADER, cloudFragSrc.c_str());
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
    std::string coronaVertSrc = loadShaderSource("assets/shaders/corona.vert");
    std::string coronaFragSrc = loadShaderSource("assets/shaders/corona.frag");
    GLuint corV = compileShader(GL_VERTEX_SHADER, coronaVertSrc.c_str());
    GLuint corF = compileShader(GL_FRAGMENT_SHADER, coronaFragSrc.c_str());
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
    std::string bhVertSrc = loadShaderSource("assets/shaders/black_hole.vert");
    std::string bhFragSrc = loadShaderSource("assets/shaders/black_hole.frag");
    GLuint bhV = compileShader(GL_VERTEX_SHADER, bhVertSrc.c_str());
    GLuint bhF = compileShader(GL_FRAGMENT_SHADER, bhFragSrc.c_str());
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
    std::string impactVertSrc = loadShaderSource("assets/shaders/impact.vert");
    std::string impactFragSrc = loadShaderSource("assets/shaders/impact.frag");
    GLuint impV = compileShader(GL_VERTEX_SHADER, impactVertSrc.c_str());
    GLuint impF = compileShader(GL_FRAGMENT_SHADER, impactFragSrc.c_str());
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
    std::string skyboxVertSrc = loadShaderSource("assets/shaders/skybox.vert");
    std::string skyboxFragSrc = loadShaderSource("assets/shaders/skybox.frag");
    GLuint skyV = compileShader(GL_VERTEX_SHADER, skyboxVertSrc.c_str());
    GLuint skyF = compileShader(GL_FRAGMENT_SHADER, skyboxFragSrc.c_str());
    m_skyboxProgram = glCreateProgram();
    glAttachShader(m_skyboxProgram, skyV);
    glAttachShader(m_skyboxProgram, skyF);
    glLinkProgram(m_skyboxProgram);
    glDeleteShader(skyV);
    glDeleteShader(skyF);
    m_skyUVPLoc = glGetUniformLocation(m_skyboxProgram, "uVP");
    m_skyTexLoc = glGetUniformLocation(m_skyboxProgram, "uTexture");
    m_skyHasTexLoc = glGetUniformLocation(m_skyboxProgram, "uHasTexture");
    m_skyCinematicModeLoc = glGetUniformLocation(m_skyboxProgram, "uCinematicMode");

    std::string trailVertSrc = loadShaderSource("assets/shaders/trail.vert");
    std::string trailFragSrc = loadShaderSource("assets/shaders/trail.frag");
    GLuint trailV = compileShader(GL_VERTEX_SHADER, trailVertSrc.c_str());
    GLuint trailF = compileShader(GL_FRAGMENT_SHADER, trailFragSrc.c_str());
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

    std::string ringVertSrc = loadShaderSource("assets/shaders/ring.vert");
    std::string ringFragSrc = loadShaderSource("assets/shaders/ring.frag");
    GLuint ringV = compileShader(GL_VERTEX_SHADER, ringVertSrc.c_str());
    GLuint ringF = compileShader(GL_FRAGMENT_SHADER, ringFragSrc.c_str());
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

    // 8. Compile Bloom Blur Program
    std::string bloomBlurVertSrc = loadShaderSource("assets/shaders/bloom_blur.vert");
    std::string bloomBlurFragSrc = loadShaderSource("assets/shaders/bloom_blur.frag");
    GLuint bloomV = compileShader(GL_VERTEX_SHADER, bloomBlurVertSrc.c_str());
    GLuint bloomF = compileShader(GL_FRAGMENT_SHADER, bloomBlurFragSrc.c_str());
    m_bloomBlurProgram = glCreateProgram();
    glAttachShader(m_bloomBlurProgram, bloomV);
    glAttachShader(m_bloomBlurProgram, bloomF);
    glLinkProgram(m_bloomBlurProgram);
    glDeleteShader(bloomV);
    glDeleteShader(bloomF);
    m_uBloomBlurImageLoc = glGetUniformLocation(m_bloomBlurProgram, "uImage");
    m_uBloomBlurHorizLoc = glGetUniformLocation(m_bloomBlurProgram, "uHorizontal");

    // 9. Compile Cinematic Post-Processing Program
    std::string postProcessVertSrc = loadShaderSource("assets/shaders/post_process.vert");
    std::string postProcessFragSrc = loadShaderSource("assets/shaders/post_process.frag");
    GLuint postV = compileShader(GL_VERTEX_SHADER, postProcessVertSrc.c_str());
    GLuint postF = compileShader(GL_FRAGMENT_SHADER, postProcessFragSrc.c_str());
    m_postProcessProgram = glCreateProgram();
    glAttachShader(m_postProcessProgram, postV);
    glAttachShader(m_postProcessProgram, postF);
    glLinkProgram(m_postProcessProgram);
    glDeleteShader(postV);
    glDeleteShader(postF);

    m_uPostSceneTexLoc          = glGetUniformLocation(m_postProcessProgram, "uSceneTex");
    m_uPostBloomTexLoc          = glGetUniformLocation(m_postProcessProgram, "uBloomTex");
    m_uPostDepthTexLoc          = glGetUniformLocation(m_postProcessProgram, "uDepthTex");
    m_uPostExposureLoc          = glGetUniformLocation(m_postProcessProgram, "uExposure");
    m_uPostBloomIntensityLoc    = glGetUniformLocation(m_postProcessProgram, "uBloomIntensity");
    m_uPostToneMapModeLoc       = glGetUniformLocation(m_postProcessProgram, "uToneMapMode");
    m_uPostEnableDoFLoc         = glGetUniformLocation(m_postProcessProgram, "uEnableDoF");
    m_uPostFocusDistLoc         = glGetUniformLocation(m_postProcessProgram, "uFocusDist");
    m_uPostDoFApertureLoc       = glGetUniformLocation(m_postProcessProgram, "uDoFAperture");
    m_uPostNearPlaneLoc         = glGetUniformLocation(m_postProcessProgram, "uNearPlane");
    m_uPostFarPlaneLoc          = glGetUniformLocation(m_postProcessProgram, "uFarPlane");
    m_uPostEnableVignetteLoc    = glGetUniformLocation(m_postProcessProgram, "uEnableVignette");
    m_uPostEnableCALoc          = glGetUniformLocation(m_postProcessProgram, "uEnableCA");

    // Create Meshes
    m_sphereMesh = createSphereMesh(1.0f, 48, 48);
    m_ringMesh = createRingMesh(128);
    m_quadMesh = createQuadMesh();

    m_particleRenderer.initialize();
    m_deformableRenderer.initialize();

    return true;
}

void Renderer::shutdown() {
    destroyHDRFramebuffers();

    if (m_shaderProgram) { glDeleteProgram(m_shaderProgram); m_shaderProgram = 0; }
    if (m_atmosphereProgram) { glDeleteProgram(m_atmosphereProgram); m_atmosphereProgram = 0; }
    if (m_cloudProgram) { glDeleteProgram(m_cloudProgram); m_cloudProgram = 0; }
    if (m_coronaProgram) { glDeleteProgram(m_coronaProgram); m_coronaProgram = 0; }
    if (m_blackHoleProgram) { glDeleteProgram(m_blackHoleProgram); m_blackHoleProgram = 0; }
    if (m_impactProgram) { glDeleteProgram(m_impactProgram); m_impactProgram = 0; }
    if (m_skyboxProgram) { glDeleteProgram(m_skyboxProgram); m_skyboxProgram = 0; }
    if (m_trailProgram) { glDeleteProgram(m_trailProgram); m_trailProgram = 0; }
    if (m_ringProgram) { glDeleteProgram(m_ringProgram); m_ringProgram = 0; }
    if (m_bloomBlurProgram) { glDeleteProgram(m_bloomBlurProgram); m_bloomBlurProgram = 0; }
    if (m_postProcessProgram) { glDeleteProgram(m_postProcessProgram); m_postProcessProgram = 0; }

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
        if (pair.second != 0) {
            glDeleteTextures(1, &pair.second);
        }
    }
    m_textures.clear();
    m_skyboxTexture = 0;
    m_ringTexture = 0;

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

            vertices.push_back(1.0f - (float)j / sectors);
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

// Box-filter 2x2 downsample for 4-channel RGBA8 data
static void downsampleRGBA_2x2(const unsigned char* src, int w, int h, unsigned char* dst) {
    int nw = std::max(1, w / 2);
    int nh = std::max(1, h / 2);
    for (int y = 0; y < nh; ++y) {
        int srcY0 = y * 2;
        int srcY1 = std::min(srcY0 + 1, h - 1);
        const unsigned char* row0 = src + (size_t)srcY0 * w * 4;
        const unsigned char* row1 = src + (size_t)srcY1 * w * 4;
        unsigned char* outRow = dst + (size_t)y * nw * 4;

        for (int x = 0; x < nw; ++x) {
            int srcX0 = x * 2;
            int srcX1 = std::min(srcX0 + 1, w - 1);

            const unsigned char* p00 = row0 + srcX0 * 4;
            const unsigned char* p10 = row0 + srcX1 * 4;
            const unsigned char* p01 = row1 + srcX0 * 4;
            const unsigned char* p11 = row1 + srcX1 * 4;
            unsigned char* out = outRow + x * 4;

            for (int c = 0; c < 4; ++c) {
                out[c] = (unsigned char)(((int)p00[c] + (int)p10[c] + (int)p01[c] + (int)p11[c] + 2) >> 2);
            }
        }
    }
}

GLuint Renderer::loadTexture(const std::string& filepath) {
    if (filepath.empty()) return 0;

    auto it = m_textures.find(filepath);
    if (it != m_textures.end()) {
        return it->second;
    }

    // Candidate path resolution
    std::vector<std::string> candidates;
    candidates.push_back(filepath);
    candidates.push_back("../" + filepath);
    candidates.push_back("../../" + filepath);

    std::string exeDir = getExecutableDir();
    if (!exeDir.empty()) {
        candidates.push_back(exeDir + "/" + filepath);
        candidates.push_back(exeDir + "/../" + filepath);
        candidates.push_back(exeDir + "/../../" + filepath);
        candidates.push_back(exeDir + "/../../../" + filepath);
    }

    int width = 0, height = 0, nrChannels = 0;
    stbi_set_flip_vertically_on_load(false);
    unsigned char* initialData = nullptr;

    for (const auto& cand : candidates) {
        // Request 4 channels (RGBA) to eliminate unpack alignment mismatches & support all formats
        initialData = stbi_load(cand.c_str(), &width, &height, &nrChannels, 4);
        if (initialData) {
            break;
        }
    }

    if (!initialData) {
        std::cerr << "[Renderer] Warning: Could not find or load texture: " << filepath << std::endl;
        m_textures[filepath] = 0; // Negative cache: prevent re-querying disk every frame
        return 0;
    }

    // Query hardware texture limit and cap dimensions to prevent GPU driver crashes or TDR
    GLint maxTexSize = 4096;
    glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTexSize);
    if (maxTexSize <= 0) maxTexSize = 4096;
    GLint targetMax = std::min(maxTexSize, 4096);

    int curW = width;
    int curH = height;
    unsigned char* curData = initialData;
    bool isAllocated = false;

    // High quality 2x2 box downsampling if image exceeds targetMax
    while (curW > targetMax || curH > targetMax) {
        int nextW = std::max(1, curW / 2);
        int nextH = std::max(1, curH / 2);
        unsigned char* nextData = (unsigned char*)malloc((size_t)nextW * nextH * 4);
        if (!nextData) break; // Out of memory fallback

        downsampleRGBA_2x2(curData, curW, curH, nextData);
        if (isAllocated) {
            free(curData);
        }
        curData = nextData;
        curW = nextW;
        curH = nextH;
        isAllocated = true;
    }

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Clear any previous OpenGL errors
    while (glGetError() != GL_NO_ERROR) {}

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, curW, curH, 0, GL_RGBA, GL_UNSIGNED_BYTE, curData);
    GLenum err = glGetError();

    if (err != GL_NO_ERROR) {
        std::cerr << "[Renderer] Error: glTexImage2D failed (0x" << std::hex << err << std::dec
                  << ") for texture: " << filepath << std::endl;
        glDeleteTextures(1, &texture);
        if (isAllocated) free(curData);
        stbi_image_free(initialData);
        m_textures[filepath] = 0;
        return 0;
    }

    // Generate mipmaps safely; fall back to linear if mipmap generation fails
    glGenerateMipmap(GL_TEXTURE_2D);
    GLenum mipErr = glGetError();
    if (mipErr != GL_NO_ERROR) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    if (isAllocated) {
        free(curData);
    }
    stbi_image_free(initialData);

    m_textures[filepath] = texture;
    return texture;
}

void Renderer::setCinematicParameters(
    bool enabled,
    float exposure,
    float bloomIntensity,
    int toneMappingMode,
    bool enableDoF,
    float focusDistance,
    float dofAperture,
    bool enableVignette,
    bool enableChromaticAberration,
    float nearPlane,
    float farPlane
) {
    m_cinematicEnabled = enabled;
    m_cinematicExposure = exposure;
    m_cinematicBloomIntensity = bloomIntensity;
    m_cinematicToneMappingMode = toneMappingMode;
    m_cinematicEnableDoF = enableDoF;
    m_cinematicFocusDistance = focusDistance;
    m_cinematicDoFAperture = dofAperture;
    m_cinematicEnableVignette = enableVignette;
    m_cinematicEnableCA = enableChromaticAberration;
    m_cinematicNearPlane = nearPlane;
    m_cinematicFarPlane = farPlane;
}

bool Renderer::initHDRFramebuffers(int width, int height) {
    width = std::max(width, 1);
    height = std::max(height, 1);
    if (m_hdrFBO != 0 && m_hdrWidth == width && m_hdrHeight == height) {
        return true;
    }
    destroyHDRFramebuffers();

    m_hdrWidth = width;
    m_hdrHeight = height;

    // 1. Create HDR Scene FBO (2 Floating-point color targets + depth)
    glGenFramebuffers(1, &m_hdrFBO);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);

    // Color Attachment 0: Linear Scene Radiance
    glGenTextures(1, &m_hdrColorTex);
    glBindTexture(GL_TEXTURE_2D, m_hdrColorTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_hdrColorTex, 0);

    // Color Attachment 1: Bright-Pass Radiance (for Bloom)
    glGenTextures(1, &m_hdrBrightTex);
    glBindTexture(GL_TEXTURE_2D, m_hdrBrightTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, m_hdrBrightTex, 0);

    // Depth Attachment: Depth Texture (for DoF and depth sampling)
    glGenTextures(1, &m_hdrDepthTex);
    glBindTexture(GL_TEXTURE_2D, m_hdrDepthTex);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, width, height, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_hdrDepthTex, 0);

    GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
    glDrawBuffers(2, attachments);

    if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
        fprintf(stderr, "[Renderer] HDR FBO setup incomplete!\n");
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return false;
    }

    // 2. Create Ping-Pong Bloom FBOs at half resolution
    m_bloomWidth = std::max(width / 2, 1);
    m_bloomHeight = std::max(height / 2, 1);
    glGenFramebuffers(2, m_bloomFBO);
    glGenTextures(2, m_bloomTex);

    for (int i = 0; i < 2; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFBO[i]);
        glBindTexture(GL_TEXTURE_2D, m_bloomTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, m_bloomWidth, m_bloomHeight, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_bloomTex[i], 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            fprintf(stderr, "[Renderer] Bloom FBO %d setup incomplete!\n", i);
        }
    }

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    return true;
}

void Renderer::destroyHDRFramebuffers() {
    if (m_hdrFBO) { glDeleteFramebuffers(1, &m_hdrFBO); m_hdrFBO = 0; }
    if (m_hdrColorTex) { glDeleteTextures(1, &m_hdrColorTex); m_hdrColorTex = 0; }
    if (m_hdrBrightTex) { glDeleteTextures(1, &m_hdrBrightTex); m_hdrBrightTex = 0; }
    if (m_hdrDepthTex) { glDeleteTextures(1, &m_hdrDepthTex); m_hdrDepthTex = 0; }
    if (m_bloomFBO[0]) { glDeleteFramebuffers(2, m_bloomFBO); m_bloomFBO[0] = m_bloomFBO[1] = 0; }
    if (m_bloomTex[0]) { glDeleteTextures(2, m_bloomTex); m_bloomTex[0] = m_bloomTex[1] = 0; }
    m_hdrWidth = m_hdrHeight = 0;
    m_bloomWidth = m_bloomHeight = 0;
}

void Renderer::renderPostProcessingPass() {
    if (!m_postProcessProgram || !m_bloomBlurProgram || !m_quadMesh.vao) return;

    // 1. Ping-pong separable Gaussian blur on bright-pass buffer
    bool horizontal = true, first_iteration = true;
    int blurPasses = 8; // 4 horizontal + 4 vertical passes
    glUseProgram(m_bloomBlurProgram);
    glUniform1i(m_uBloomBlurImageLoc, 0);

    for (int i = 0; i < blurPasses; ++i) {
        glBindFramebuffer(GL_FRAMEBUFFER, m_bloomFBO[horizontal ? 1 : 0]);
        glViewport(0, 0, m_bloomWidth, m_bloomHeight);
        glUniform1i(m_uBloomBlurHorizLoc, horizontal ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, first_iteration ? m_hdrBrightTex : m_bloomTex[!horizontal]);

        glBindVertexArray(m_quadMesh.vao);
        glDrawArrays(GL_TRIANGLES, 0, 6);

        horizontal = !horizontal;
        if (first_iteration) first_iteration = false;
    }

    // 2. Render Fullscreen Quad with Tone Mapping + Bloom Composite + DoF
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(m_lastVpX, m_lastVpY, m_lastVpW, m_lastVpH);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_BLEND);

    glUseProgram(m_postProcessProgram);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrColorTex);
    glUniform1i(m_uPostSceneTexLoc, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_bloomTex[!horizontal]);
    glUniform1i(m_uPostBloomTexLoc, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_hdrDepthTex);
    glUniform1i(m_uPostDepthTexLoc, 2);

    glUniform1f(m_uPostExposureLoc, m_cinematicExposure);
    glUniform1f(m_uPostBloomIntensityLoc, m_cinematicBloomIntensity);
    glUniform1i(m_uPostToneMapModeLoc, m_cinematicToneMappingMode);
    glUniform1i(m_uPostEnableDoFLoc, m_cinematicEnableDoF ? 1 : 0);
    glUniform1f(m_uPostFocusDistLoc, m_cinematicFocusDistance);
    glUniform1f(m_uPostDoFApertureLoc, m_cinematicDoFAperture);
    glUniform1f(m_uPostNearPlaneLoc, m_cinematicNearPlane);
    glUniform1f(m_uPostFarPlaneLoc, m_cinematicFarPlane);
    glUniform1i(m_uPostEnableVignetteLoc, m_cinematicEnableVignette ? 1 : 0);
    glUniform1i(m_uPostEnableCALoc, m_cinematicEnableCA ? 1 : 0);

    glBindVertexArray(m_quadMesh.vao);
    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::beginViewport(int x, int y, int width, int height, const glm::vec4& clearColor) {
    m_lastVpX = x;
    m_lastVpY = y;
    m_lastVpW = width;
    m_lastVpH = height;

    if (m_cinematicEnabled) {
        initHDRFramebuffers(width, height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO);
        GLenum attachments[2] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);
        glViewport(0, 0, width, height);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    } else {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(x, y, width, height);
        glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }
}

void Renderer::endViewport(int windowWidth, int windowHeight) {
    if (m_cinematicEnabled && m_hdrFBO != 0) {
        renderPostProcessingPass();
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
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

    // Physical Material & Cloud Shadow Uniforms
    glUniform1f(m_uWaterFractionLoc, vBody.waterFraction);
    glUniform1f(m_uIceFractionLoc, vBody.iceFraction);
    glUniform1f(m_uRoughnessLoc, vBody.surfaceRoughness);
    glUniform1f(m_uCloudShadowCoverageLoc, vBody.cloudCoverage);
    glUniform1f(m_uCloudShadowRotAngleLoc, vBody.cloudRotationAngle);

    if (m_uCinematicModeLoc != -1) glUniform1i(m_uCinematicModeLoc, m_cinematicEnabled ? 1 : 0);

    // Planetary Ring Shadow projection parameters
    glUniform1i(m_uHasRingLoc, vBody.hasRing ? 1 : 0);
    if (vBody.hasRing) {
        float innerR = (vBody.ringInnerRadiusAU > 0.0f) ? vBody.ringInnerRadiusAU : (vBody.renderRadius * 1.35f);
        float outerR = (vBody.ringOuterRadiusAU > 0.0f) ? vBody.ringOuterRadiusAU : (vBody.renderRadius * 2.45f);
        glm::vec3 ringNorm = glm::normalize(glm::mat3(vBody.rotationMatrix) * glm::vec3(0.0f, 1.0f, 0.0f));
        glUniform3fv(m_uRingNormalLoc, 1, glm::value_ptr(ringNorm));
        glUniform3fv(m_uPlanetCenterLoc, 1, glm::value_ptr(relativePos));
        glUniform1f(m_uRingInnerRadiusLoc, innerR);
        glUniform1f(m_uRingOuterRadiusLoc, outerR);
    }

    // Multi-Star Lighting Upload (A body cannot be illuminated by itself)
    int numLights = 0;
    for (size_t i = 0; i < stars.size() && numLights < 4; ++i) {
        if (glm::distance(stars[i].positionAU, vBody.positionAU) < 0.0001f) {
            continue;
        }
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uLightPosLoc[numLights], 1, glm::value_ptr(relLightPos));
        glUniform3fv(m_uLightColorLoc[numLights], 1, glm::value_ptr(stars[i].color));
        glUniform1f(m_uLightIntensityLoc[numLights], stars[i].intensity);
        numLights++;
    }
    glUniform1i(m_uNumLightsLoc, numLights);

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

    // Render Atmospheric Shell (procedural generic clouds bypassed to preserve real textures)
    if (vBody.hasAtmosphere && !vBody.isStar) {
        renderAtmosphereShell(camera, aspect, vBody, stars, cameraTarget);
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
    glUniform1f(m_uAtmoScaleHeightLoc, (float)vBody.scaleHeightKm);
    glUniform1f(m_uAtmoMieFactorLoc, vBody.mieHazeFactor);
    glUniform1f(m_uAtmoPlanetRadiusLoc, vBody.renderRadius);

    int numLights = 0;
    for (size_t i = 0; i < stars.size() && numLights < 4; ++i) {
        if (glm::distance(stars[i].positionAU, vBody.positionAU) < 0.0001f) {
            continue;
        }
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uAtmoLightPosLoc[numLights], 1, glm::value_ptr(relLightPos));
        glUniform3fv(m_uAtmoLightColorLoc[numLights], 1, glm::value_ptr(stars[i].color));
        numLights++;
    }
    glUniform1i(m_uAtmoNumLightsLoc, numLights);

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

    int numLights = 0;
    for (size_t i = 0; i < stars.size() && numLights < 4; ++i) {
        if (glm::distance(stars[i].positionAU, vBody.positionAU) < 0.0001f) {
            continue;
        }
        glm::vec3 relLightPos = stars[i].positionAU - cameraTarget;
        glUniform3fv(m_uCloudLightPosLoc[numLights], 1, glm::value_ptr(relLightPos));
        numLights++;
    }
    glUniform1i(m_uCloudNumLightsLoc, numLights);

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

    glm::mat4 proj = glm::perspective(glm::radians(camera.getFOV()), std::max(aspect, 0.1f), 0.1f, 1000.0f);
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
    if (m_skyCinematicModeLoc != -1) glUniform1i(m_skyCinematicModeLoc, m_cinematicEnabled ? 1 : 0);

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

void Renderer::renderTrails(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& cameraTarget, int selectedIndex, bool showOrbitLines, bool showMotionTrails) {
    if (bodies.empty() || m_trailProgram == 0 || m_trailVAO == 0) return;
    if (!showOrbitLines && !showMotionTrails) return;
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
    if (showOrbitLines) {
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
    }

    // 2. Render dynamic fading motion trails
    if (showMotionTrails) {
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
        glUniform1i(glGetUniformLocation(m_ringProgram, "uCinematicMode"), m_cinematicEnabled ? 1 : 0);

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
