#include "renderer/DeformableRenderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace AstroGenesis {

static const double AU_METERS = 149597870700.0;

static const char* deformVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec4 aColor;
layout(location = 3) in float aScalar;

out vec3 FragPos;
out vec3 Normal;
out vec4 VertexColor;
out float ScalarVal;

uniform mat4 uVP;

void main() {
    FragPos = aPos;
    Normal = aNormal;
    VertexColor = aColor;
    ScalarVal = aScalar;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* deformFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;

in vec3 FragPos;
in vec3 Normal;
in vec4 VertexColor;
in float ScalarVal;

uniform vec3 uLightPos;
uniform int uVisMode;
uniform vec3 uBaseColor;
uniform float uMetallic;
uniform float uRoughness;

vec3 colormapTurbo(float x) {
    x = clamp(x, 0.0, 1.0);
    return clamp(vec3(
        1.6 * x - 0.3,
        sin(x * 3.14159),
        1.1 - 1.6 * x
    ), 0.0, 1.0);
}

vec3 colormapTemperature(float t) {
    t = clamp(t, 0.0, 1.0);
    if (t < 0.33) {
        return mix(vec3(0.05, 0.05, 0.1), vec3(0.85, 0.15, 0.0), t * 3.0);
    } else if (t < 0.66) {
        return mix(vec3(0.85, 0.15, 0.0), vec3(1.0, 0.75, 0.1), (t - 0.33) * 3.0);
    } else {
        return mix(vec3(1.0, 0.75, 0.1), vec3(1.0, 1.0, 1.0), (t - 0.66) * 3.0);
    }
}

vec3 colormapDamage(float d) {
    d = clamp(d, 0.0, 1.0);
    return mix(vec3(0.18, 0.72, 0.28), vec3(0.95, 0.12, 0.08), d);
}

void main() {
    vec3 N = normalize(Normal);
    vec3 L = normalize(uLightPos - FragPos);
    float diff = max(dot(N, L), 0.22);

    vec3 finalColor = uBaseColor * diff;

    if (uVisMode == 1) { // Von Mises Stress
        finalColor = colormapTurbo(ScalarVal) * diff;
    } else if (uVisMode == 2) { // Mechanical Strain
        finalColor = mix(vec3(0.1, 0.4, 0.8), vec3(1.0, 0.2, 0.8), ScalarVal) * diff;
    } else if (uVisMode == 3) { // Temperature Heatmap
        vec3 heatCol = colormapTemperature(ScalarVal);
        finalColor = heatCol * diff + heatCol * max(0.0, ScalarVal - 0.35) * 1.8; // Blackbody incandescence glow
    } else if (uVisMode == 4) { // Damage & Fracture
        finalColor = colormapDamage(ScalarVal) * diff;
    } else if (uVisMode == 5) { // Plastic Strain
        finalColor = mix(vec3(0.35, 0.35, 0.40), vec3(0.95, 0.45, 0.85), ScalarVal) * diff;
    } else if (uVisMode == 6) { // Tidal Gravity Field
        finalColor = mix(vec3(0.15, 0.30, 0.85), vec3(1.0, 0.90, 0.15), ScalarVal) * diff;
    }

    FragColor = vec4(finalColor, 1.0);
}
)GLSL";

static const char* lineVertSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec4 aColor;

out vec4 LineColor;

uniform mat4 uVP;

void main() {
    LineColor = aColor;
    gl_Position = uVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* lineFragSrc = R"GLSL(
#version 330 core
out vec4 FragColor;
in vec4 LineColor;

void main() {
    FragColor = LineColor;
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
        fprintf(stderr, "Deformable Shader Compile Error: %s\n", log);
    }
    return s;
}

DeformableRenderer::DeformableRenderer() {}

DeformableRenderer::~DeformableRenderer() {
    shutdown();
}

bool DeformableRenderer::initialize() {
    // 1. Surface Shader
    GLuint vShader = compileShader(GL_VERTEX_SHADER, deformVertSrc);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, deformFragSrc);
    m_program = glCreateProgram();
    glAttachShader(m_program, vShader);
    glAttachShader(m_program, fShader);
    glLinkProgram(m_program);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    m_uVPLoc = glGetUniformLocation(m_program, "uVP");
    m_uLightPosLoc = glGetUniformLocation(m_program, "uLightPos");
    m_uVisModeLoc = glGetUniformLocation(m_program, "uVisMode");
    m_uBaseColorLoc = glGetUniformLocation(m_program, "uBaseColor");
    m_uMetallicLoc = glGetUniformLocation(m_program, "uMetallic");
    m_uRoughnessLoc = glGetUniformLocation(m_program, "uRoughness");

    // 2. Wireframe / Constraint Line Shader
    GLuint lvShader = compileShader(GL_VERTEX_SHADER, lineVertSrc);
    GLuint lfShader = compileShader(GL_FRAGMENT_SHADER, lineFragSrc);
    m_lineProgram = glCreateProgram();
    glAttachShader(m_lineProgram, lvShader);
    glAttachShader(m_lineProgram, lfShader);
    glLinkProgram(m_lineProgram);
    glDeleteShader(lvShader);
    glDeleteShader(lfShader);

    m_uLineVPLoc = glGetUniformLocation(m_lineProgram, "uVP");

    // Buffers setup
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);

    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);

    GLsizei stride = sizeof(DeformVertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(DeformVertex, pos));
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(DeformVertex, normal));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(DeformVertex, color));
    glEnableVertexAttribArray(2);

    glVertexAttribPointer(3, 1, GL_FLOAT, GL_FALSE, stride, (void*)offsetof(DeformVertex, scalarValue));
    glEnableVertexAttribArray(3);

    glBindVertexArray(0);

    // Line VAO/VBO
    glGenVertexArrays(1, &m_lineVAO);
    glGenBuffers(1, &m_lineVBO);

    glBindVertexArray(m_lineVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);

    struct LineVertex {
        glm::vec3 pos;
        glm::vec4 color;
    };
    GLsizei lineStride = sizeof(LineVertex);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, lineStride, (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, lineStride, (void*)sizeof(glm::vec3));
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);

    return true;
}

void DeformableRenderer::shutdown() {
    if (m_vao) {
        glDeleteVertexArrays(1, &m_vao);
        glDeleteBuffers(1, &m_vbo);
        m_vao = 0;
        m_vbo = 0;
    }
    if (m_lineVAO) {
        glDeleteVertexArrays(1, &m_lineVAO);
        glDeleteBuffers(1, &m_lineVBO);
        m_lineVAO = 0;
        m_lineVBO = 0;
    }
    if (m_program) {
        glDeleteProgram(m_program);
        m_program = 0;
    }
    if (m_lineProgram) {
        glDeleteProgram(m_lineProgram);
        m_lineProgram = 0;
    }
}

void DeformableRenderer::render(
    const Camera& camera,
    float aspect,
    const std::vector<std::shared_ptr<DeformableBody>>& bodies,
    MatterVisualizationMode visMode,
    const glm::vec3& sunPos,
    const glm::vec3& cameraTarget,
    bool drawWireframe
) {
    if (bodies.empty() || m_program == 0) return;

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 vp = proj * view;
    glm::vec3 relSun = sunPos - cameraTarget;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);

    // 1. Render Surface Meshes
    glUseProgram(m_program);
    glUniformMatrix4fv(m_uVPLoc, 1, GL_FALSE, glm::value_ptr(vp));
    glUniform3fv(m_uLightPosLoc, 1, glm::value_ptr(relSun));
    glUniform1i(m_uVisModeLoc, (int)visMode);

    glBindVertexArray(m_vao);

    for (const auto& body : bodies) {
        if (!body) continue;

        const auto& nodes = body->getNodes();
        const auto& triangles = body->getSurfaceTriangles();
        if (nodes.empty() || triangles.empty()) continue;

        const auto& mat = body->getMaterial();
        glUniform3fv(m_uBaseColorLoc, 1, glm::value_ptr(mat.baseColor));
        glUniform1f(m_uMetallicLoc, mat.metallic);
        glUniform1f(m_uRoughnessLoc, mat.roughness);

        std::vector<DeformVertex> vertices;
        vertices.reserve(triangles.size() * 3);

        for (const auto& tri : triangles) {
            if (tri.i0 >= (int)nodes.size() || tri.i1 >= (int)nodes.size() || tri.i2 >= (int)nodes.size()) continue;

            const auto& n0 = nodes[tri.i0];
            const auto& n1 = nodes[tri.i1];
            const auto& n2 = nodes[tri.i2];

            glm::vec3 p0 = glm::vec3(n0.positionM.x / AU_METERS, n0.positionM.y / AU_METERS, n0.positionM.z / AU_METERS) - cameraTarget;
            glm::vec3 p1 = glm::vec3(n1.positionM.x / AU_METERS, n1.positionM.y / AU_METERS, n1.positionM.z / AU_METERS) - cameraTarget;
            glm::vec3 p2 = glm::vec3(n2.positionM.x / AU_METERS, n2.positionM.y / AU_METERS, n2.positionM.z / AU_METERS) - cameraTarget;

            glm::vec3 norm = glm::normalize(glm::cross(p1 - p0, p2 - p0));
            glm::vec4 baseCol(mat.baseColor, 1.0f);

            vertices.push_back({ p0, norm, baseCol, n0.scalarValue });
            vertices.push_back({ p1, norm, baseCol, n1.scalarValue });
            vertices.push_back({ p2, norm, baseCol, n2.scalarValue });
        }

        if (!vertices.empty()) {
            glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
            glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(DeformVertex), vertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_TRIANGLES, 0, (GLsizei)vertices.size());
        }
    }

    glBindVertexArray(0);

    // 2. Render Constraints Wireframe / Fracture Lines
    if (drawWireframe) {
        struct LineVertex {
            glm::vec3 pos;
            glm::vec4 color;
        };
        std::vector<LineVertex> lineVertices;

        for (const auto& body : bodies) {
            if (!body) continue;
            const auto& nodes = body->getNodes();
            const auto& constraints = body->getConstraints();

            for (const auto& c : constraints) {
                if (c.nodeA >= (int)nodes.size() || c.nodeB >= (int)nodes.size()) continue;

                const auto& nA = nodes[c.nodeA];
                const auto& nB = nodes[c.nodeB];

                glm::vec3 pA = glm::vec3(nA.positionM.x / AU_METERS, nA.positionM.y / AU_METERS, nA.positionM.z / AU_METERS) - cameraTarget;
                glm::vec3 pB = glm::vec3(nB.positionM.x / AU_METERS, nB.positionM.y / AU_METERS, nB.positionM.z / AU_METERS) - cameraTarget;

                glm::vec4 lineCol;
                if (c.isBroken) {
                    lineCol = glm::vec4(1.0f, 0.15f, 0.15f, 0.65f); // Red fracture line
                } else if (c.damage > 0.3) {
                    lineCol = glm::vec4(1.0f, 0.65f, 0.15f, 0.5f);  // Orange stressed line
                } else {
                    lineCol = glm::vec4(0.25f, 0.55f, 0.85f, 0.35f); // Blue intact constraint
                }

                lineVertices.push_back({ pA, lineCol });
                lineVertices.push_back({ pB, lineCol });
            }
        }

        if (!lineVertices.empty()) {
            glUseProgram(m_lineProgram);
            glUniformMatrix4fv(m_uLineVPLoc, 1, GL_FALSE, glm::value_ptr(vp));

            glBindVertexArray(m_lineVAO);
            glBindBuffer(GL_ARRAY_BUFFER, m_lineVBO);
            glBufferData(GL_ARRAY_BUFFER, lineVertices.size() * sizeof(LineVertex), lineVertices.data(), GL_DYNAMIC_DRAW);
            glDrawArrays(GL_LINES, 0, (GLsizei)lineVertices.size());
            glBindVertexArray(0);
        }
    }
}

} // namespace AstroGenesis
