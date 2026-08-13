#include "renderer/Renderer.hpp"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <cmath>
#include <cstdio>

namespace AstroGenesis {

static const float PI = 3.14159265358979323846f;

static const char* vertShaderSrc = R"GLSL(
#version 330 core
layout(location = 0) in vec3 aPos;
uniform mat4 uMVP;
void main() {
    gl_Position = uMVP * vec4(aPos, 1.0);
}
)GLSL";

static const char* fragShaderSrc = R"GLSL(
#version 330 core
out vec4 FragColor;
uniform vec3 uColor;
void main() {
    FragColor = vec4(uColor, 1.0);
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
    glLineWidth(1.0f);

    GLuint vShader = compileShader(GL_VERTEX_SHADER, vertShaderSrc);
    GLuint fShader = compileShader(GL_FRAGMENT_SHADER, fragShaderSrc);
    m_shaderProgram = glCreateProgram();
    glAttachShader(m_shaderProgram, vShader);
    glAttachShader(m_shaderProgram, fShader);
    glLinkProgram(m_shaderProgram);
    glDeleteShader(vShader);
    glDeleteShader(fShader);

    m_uMVPLoc   = glGetUniformLocation(m_shaderProgram, "uMVP");
    m_uColorLoc = glGetUniformLocation(m_shaderProgram, "uColor");

    m_sphereMesh = createSphereMesh(1.0f, 32, 48);
    return true;
}

void Renderer::shutdown() {
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
}

MeshData Renderer::createSphereMesh(float radius, int stacks, int sectors) {
    std::vector<float> verts;
    std::vector<unsigned int> indices;

    for (int i = 0; i <= stacks; ++i) {
        float phi = PI / 2.0f - PI * (float)i / (float)stacks;
        float y   = radius * sinf(phi);
        float r   = radius * cosf(phi);
        for (int j = 0; j <= sectors; ++j) {
            float theta = 2.0f * PI * (float)j / (float)sectors;
            verts.push_back(r * cosf(theta));
            verts.push_back(y);
            verts.push_back(r * sinf(theta));
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

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    glBindVertexArray(0);
    return mesh;
}

void Renderer::beginViewport(int x, int y, int width, int height, const glm::vec4& clearColor) {
    glViewport(x, y, width, height);
    glClearColor(clearColor.r, clearColor.g, clearColor.b, clearColor.a);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void Renderer::renderWireframeSphere(const Camera& camera, float aspect, const CelestialBody& body) {
    glUseProgram(m_shaderProgram);

    glm::mat4 proj = camera.getProjectionMatrix(aspect);
    glm::mat4 view = camera.getViewMatrix();

    glm::mat4 model = glm::translate(glm::mat4(1.0f), body.position);
    model = glm::rotate(model, glm::radians(body.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
    model = glm::rotate(model, body.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
    model = glm::scale(model, glm::vec3(body.radius3D));

    glm::mat4 mvp = proj * view * model;
    glUniformMatrix4fv(m_uMVPLoc, 1, GL_FALSE, glm::value_ptr(mvp));
    glUniform3fv(m_uColorLoc, 1, glm::value_ptr(body.color));

    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    glBindVertexArray(m_sphereMesh.vao);
    glDrawElements(GL_TRIANGLES, m_sphereMesh.indexCount, GL_UNSIGNED_INT, 0);
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
}

void Renderer::endViewport(int windowWidth, int windowHeight) {
    glViewport(0, 0, windowWidth, windowHeight);
}

} // namespace AstroGenesis
