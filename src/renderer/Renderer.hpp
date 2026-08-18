#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include "renderer/Camera.hpp"
#include "renderer/ParticleRenderer.hpp"
#include "renderer/DeformableRenderer.hpp"
#include "simulation/CelestialBody.hpp"
#include "simulation/ParticleSystem.hpp"
#include "simulation/MatterSystem.hpp"

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
    void renderRings(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& sunPos, const glm::vec3& cameraTarget);
    void renderParticleField(const Camera& camera, float aspect, ParticleField& field, const glm::vec3& sunPos, const glm::vec3& cameraTarget, double simTime);
    void renderDeformableBodies(const Camera& camera, float aspect, const MatterSystem& matter, const glm::vec3& sunPos, const glm::vec3& cameraTarget);
    void renderSkybox(const Camera& camera, float aspect);
    void endViewport(int windowWidth, int windowHeight);

    GLuint loadTexture(const std::string& filepath);

private:
    MeshData createSphereMesh(float radius, int stacks, int sectors);
    MeshData createRingMesh(int radialSegments);

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

    // Planetary Ring shader & mesh
    GLuint m_ringProgram = 0;
    GLint m_uRingMVPLoc = -1;
    GLint m_uRingModelLoc = -1;
    GLint m_uRingNormalMatLoc = -1;
    GLint m_uRingSunPosLoc = -1;
    GLint m_uRingPlanetCenterLoc = -1;
    GLint m_uRingPlanetRadiusLoc = -1;
    GLint m_uRingColorLoc = -1;
    GLint m_uRingCameraPosLoc = -1;
    GLint m_uRingTexLoc = -1;
    GLint m_uRingHasTexLoc = -1;
    GLint m_uNumDisturbancesLoc = -1;
    GLint m_uDisturbancesLoc = -1;
    GLint m_uDistIntensityLoc = -1;
    GLuint m_ringTexture = 0;
    MeshData m_ringMesh;

    MeshData m_sphereMesh;
    std::unordered_map<std::string, GLuint> m_textures;
    ParticleRenderer m_particleRenderer;
    DeformableRenderer m_deformableRenderer;
};

} // namespace AstroGenesis
