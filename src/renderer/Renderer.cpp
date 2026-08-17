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

void main() {
    FragColor = texture(uTexture, TexCoord);
}
)GLSL";

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

    // Pre-load the skybox texture
    m_skyboxTexture = loadTexture("assets/textures/stars_milky_way.jpg");
    if (m_skyboxTexture == 0) {
        fprintf(stderr, "WARNING: Skybox texture failed to load!\n");
    }

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
    if (m_shaderProgram) {
        glDeleteProgram(m_shaderProgram);
        m_shaderProgram = 0;
    }
    if (m_skyboxProgram) {
        glDeleteProgram(m_skyboxProgram);
        m_skyboxProgram = 0;
    }
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
        fprintf(stderr, "Failed to load texture: %s\n", filepath.c_str());
        m_textures[filepath] = 0;
        return 0;
    }

    GLenum format = GL_RGB;
    if (nrChannels == 1) format = GL_RED;
    else if (nrChannels == 3) format = GL_RGB;
    else if (nrChannels == 4) format = GL_RGBA;

    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    stbi_image_free(data);
    printf("Successfully loaded planet texture: %s (%dx%d, %d channels)\n", filepath.c_str(), width, height, nrChannels);

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
    // Skybox renders first — disable depth entirely, no depth tricks needed
    glDisable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);

    // Cull front faces so we see the inside of the sphere
    glEnable(GL_CULL_FACE);
    glCullFace(GL_FRONT);

    glUseProgram(m_skyboxProgram);

    // Use a fixed projection that guarantees the skybox sphere is visible.
    // Near=0.1, Far=1000 ensures the sphere at radius 100 is never clipped.
    glm::mat4 proj = glm::perspective(glm::radians(45.0f), std::max(aspect, 0.1f), 0.1f, 1000.0f);

    // Strip translation from view matrix so the skybox stays centered on camera
    glm::mat4 view = glm::mat4(glm::mat3(camera.getViewMatrix()));

    // Scale the unit sphere to radius 100 — well within [0.1, 1000] frustum
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(100.0f));
    glm::mat4 vp = proj * view * model;

    glUniformMatrix4fv(m_skyUVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

    if (m_skyboxTexture > 0) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_skyboxTexture);
        glUniform1i(m_skyTexLoc, 0);
    }

    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);

    // Restore state
    glCullFace(GL_BACK);
    glDisable(GL_CULL_FACE);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

} // namespace AstroGenesis
