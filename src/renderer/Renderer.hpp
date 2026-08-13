#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
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
    void renderSphere(const Camera& camera, float aspect, const CelestialBody& body, const glm::vec3& sunPos, const glm::vec3& cameraTarget);
    void endViewport(int windowWidth, int windowHeight);

    GLuint loadTexture(const std::string& filepath);

private:
    MeshData createSphereMesh(float radius, int stacks, int sectors);

    GLuint m_shaderProgram = 0;
    GLint m_uMVPLoc = -1;
    GLint m_uModelLoc = -1;
    GLint m_uColorLoc = -1;
    GLint m_uUseTextureLoc = -1;
    GLint m_uTextureLoc = -1;
    GLint m_uLightPosLoc = -1;
    GLint m_uIsSunLoc = -1;

    MeshData m_sphereMesh;
    std::unordered_map<std::string, GLuint> m_textures;
};

} // namespace AstroGenesis
