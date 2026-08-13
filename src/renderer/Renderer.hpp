#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include "renderer/Camera.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

struct MeshData {
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;
    int indexCount = 0;
};

class Renderer {
public:
    Renderer();
    ~Renderer();

    bool initialize();
    void shutdown();

    void beginViewport(int x, int y, int width, int height, const glm::vec4& clearColor);
    void renderWireframeSphere(const Camera& camera, float aspect, const CelestialBody& body);
    void endViewport(int windowWidth, int windowHeight);

private:
    MeshData createSphereMesh(float radius, int stacks, int sectors);

    GLuint m_shaderProgram = 0;
    GLint m_uMVPLoc = -1;
    GLint m_uColorLoc = -1;

    MeshData m_sphereMesh;
};

} // namespace AstroGenesis
