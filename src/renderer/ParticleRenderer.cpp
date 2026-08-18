#include "renderer/ParticleRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace AstroGenesis {

static const char* particleVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;           // Mesh vertex position
layout(location = 1) in vec3 aNormal;        // Mesh vertex normal
layout(location = 2) in vec3 aInstancePos;   // Instance camera-relative position (AU)
layout(location = 3) in float aInstanceScale;// Instance scale
layout(location = 4) in vec4 aInstanceColor; // Instance color + opacity

out vec3 FragPos;
out vec3 Normal;
out vec4 InstanceColor;

uniform mat4 uVP;

void main() {
    vec3 worldPos = aInstancePos + aPos * aInstanceScale;
    FragPos = worldPos;
    Normal = aNormal;
    InstanceColor = aInstanceColor;
    gl_Position = uVP * vec4(worldPos, 1.0);
}
)GLSL";

static const char* particleFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 InstanceColor;

uniform vec3 uLightPos; // Camera-relative Sol position

void main() {
    vec3 lightDir = normalize(uLightPos - FragPos);
    vec3 norm = normalize(Normal);

    // Directional point diffuse from Sol + ambient
    float diff = max(dot(norm, lightDir), 0.20);
    vec3 color = InstanceColor.rgb * diff;

    FragColor = vec4(color, InstanceColor.a);
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
        fprintf(stderr, "Particle Shader Compile Error: %s\n", log);
    }
    return s;
}

ParticleRenderer::ParticleRenderer() {}

ParticleRenderer::~ParticleRenderer() {
    shutdown();
}

bool ParticleRenderer::initialize() {
    GLuint vShader = compileShader(GL_VERTEX_SHADER, particleVertSrc);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, particleFragSrc);
    m_program = glCreateProgram();
    glAttachShader(m_program, vShader);
    glAttachShader(m_program, fShader);
    glLinkProgram(m_program);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    m_uVPLoc = glGetUniformLocation(m_program, "uVP");
    m_uLightPosLoc = glGetUniformLocation(m_program, "uLightPos");

    createParticleMesh();
    ensureInstanceBufferCapacity(150000);

    return true;
}

void ParticleRenderer::shutdown() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        glDeleteBuffers(1, &m_ebo);
        glDeleteBuffers(1, &m_instanceVBO);
        m_vao = 0;
        m_vbo = 0;
        m_ebo = 0;
        m_instanceVBO = 0;
    }
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    m_instanceBufferCapacity = 0;
}

void ParticleRenderer::createParticleMesh() {
    // Generate an irregular, low-poly rocky/icy polyhedron for particles
    const float t = (1.0f + std::sqrt(5.0f)) / 2.0f;
    std::vector<glm::vec3> baseVertices = {
        {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
        { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
        { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };

    // Perturb vertices with deterministic irregularities for rocky particle shapes
    for (size_t i = 0; i < baseVertices.size(); ++i) {
        baseVertices[i] = glm::normalize(baseVertices[i]);
        float noise = 0.85f + 0.30f * std::sin((float)i * 3.7f + 1.2f);
        baseVertices[i] *= noise;
    }

    std::vector<unsigned int> indices = {
        0, 11,  5,    0,  5,  1,    0,  1,  7,    0,  7, 10,    0, 10, 11,
        1,  5,  9,    5, 11,  4,   11, 10,  2,   10,  7,  6,    7,  1,  8,
        3,  9,  4,    3,  4,  2,    3,  2,  6,    3,  6,  8,    3,  8,  9,
        4,  9,  5,    2,  4, 11,    6,  2, 10,    8,  6,  7,    9,  8,  1
    };

    struct Vertex {
        glm::vec3 pos;
        glm::vec3 norm;
    };
    std::vector<Vertex> vertices;
    vertices.reserve(baseVertices.size());

    for (const auto& v : baseVertices) {
        vertices.push_back({ v, glm::normalize(v) });
    }

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);
    glGenBuffers(1, &m_instanceVBO);

    glBindVertexArray(m_vao);

    // Mesh vertex buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

    // Layout 0: Position
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
    glEnableVertexAttribArray(0);

    // Layout 1: Normal
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)(sizeof(glm::vec3)));
    glEnableVertexAttribArray(1);

    // Instance VBO Setup
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);

    GLsizei instanceStride = sizeof(ParticleInstanceData);

    // Layout 2: Instance Position (vec3)
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, instanceStride, (void*)offsetof(ParticleInstanceData, pos));
    glEnableVertexAttribArray(2);
    glVertexAttribDivisor(2, 1);

    // Layout 3: Instance Scale (float)
    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, instanceStride, (void*)offsetof(ParticleInstanceData, scale));
    glEnableVertexAttribArray(3);
    glVertexAttribDivisor(3, 1);

    // Layout 4: Instance Color (vec4)
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, instanceStride, (void*)offsetof(ParticleInstanceData, color));
    glEnableVertexAttribArray(4);
    glVertexAttribDivisor(4, 1);

    glBindVertexArray(0);
    m_indexCount = (int)indices.size();
}

void ParticleRenderer::ensureInstanceBufferCapacity(size_t count) {
    if (count <= m_instanceBufferCapacity) return;

    m_instanceBufferCapacity = std::max(count, m_instanceBufferCapacity * 2);
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, m_instanceBufferCapacity * sizeof(ParticleInstanceData), nullptr, GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void ParticleRenderer::render(const Camera& camera, float aspect,
                              const std::vector<ParticleInstanceData>& instances,
                              const glm::vec3& sunPos, const glm::vec3& cameraTarget) {
    if (instances.empty() || m_program == 0 || m_vao == 0) return;

    ensureInstanceBufferCapacity(instances.size());

    // Upload instance buffer
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferSubData(GL_ARRAY_BUFFER, 0, instances.size() * sizeof(ParticleInstanceData), instances.data());
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 vp = proj * view;

    glm::vec3 relSun = sunPos - cameraTarget;

    glUseProgram(m_program);
    glUniformMatrix4fv(m_uVPLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform3fv(m_uLightPosLoc, 1, glm::value_ptr(relSun));

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    glBindVertexArray(m_vao);
    glDrawElementsInstanced(GL_TRIANGLES, m_indexCount, GL_UNSIGNED_INT, 0, (GLsizei)instances.size());
    glBindVertexArray(0);
}

} // namespace AstroGenesis
