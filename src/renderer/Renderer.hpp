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
    void renderTrails(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& cameraTarget, int selectedIndex);
    void renderSkybox(const Camera& camera, float aspect);
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

    // Skybox shader
    GLuint m_skyboxProgram = 0;
    GLint m_skyUVPLoc = -1;
    GLint m_skyTexLoc = -1;
    GLint m_skyHasTexLoc = -1;
    GLuint m_skyboxTexture = 0;

    // Trail shader & dynamic buffers
    GLuint m_trailProgram = 0;
    GLint m_uTrailVPLoc = -1;
    GLuint m_trailVAO = 0;
    GLuint m_trailVBO = 0;

    MeshData m_sphereMesh;
    std::unordered_map<std::string, GLuint> m_textures;
};

} // namespace AstroGenesis
