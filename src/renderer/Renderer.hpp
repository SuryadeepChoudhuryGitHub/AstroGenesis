#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <unordered_map>
#include "renderer/Camera.hpp"
#include "renderer/ParticleRenderer.hpp"
#include "renderer/DeformableRenderer.hpp"
#include "renderer/VisualStateAdapter.hpp"
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
    void endViewport(int windowWidth, int windowHeight);

    // Advanced Physical-to-Visual Celestial Body Rendering
    void renderCelestialBody(
        const Camera& camera,
        float aspect,
        const VisualBodyState& vBody,
        const std::vector<StarLightSource>& stars,
        const glm::vec3& cameraTarget,
        const std::string& texturePath,
        VisualMode visMode,
        DebugVisualOverlay debugOverlay,
        float simTime
    );

    void renderAtmosphereShell(
        const Camera& camera,
        float aspect,
        const VisualBodyState& vBody,
        const std::vector<StarLightSource>& stars,
        const glm::vec3& cameraTarget
    );

    void renderCloudLayer(
        const Camera& camera,
        float aspect,
        const VisualBodyState& vBody,
        const std::vector<StarLightSource>& stars,
        const glm::vec3& cameraTarget
    );

    void renderStellarCorona(
        const Camera& camera,
        float aspect,
        const VisualBodyState& vBody,
        const glm::vec3& cameraTarget,
        float simTime
    );

    void renderBlackHole(
        const Camera& camera,
        float aspect,
        const VisualBodyState& vBody,
        const glm::vec3& cameraTarget,
        float simTime
    );

    void renderImpactFX(
        const Camera& camera,
        float aspect,
        const std::vector<VisualImpactEvent>& impacts,
        const glm::vec3& cameraTarget
    );

    // Baseline & Scene Render Passes
    void renderSphere(const Camera& camera, float aspect, const CelestialBody& body, const glm::vec3& sunPos, const glm::vec3& cameraTarget);
    void renderTrails(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const glm::vec3& cameraTarget, int selectedIndex);
    void renderRings(const Camera& camera, float aspect, const std::vector<CelestialBody>& bodies, const std::vector<StarLightSource>& stars, const glm::vec3& cameraTarget);
    void renderParticleField(const Camera& camera, float aspect, ParticleField& field, const glm::vec3& sunPos, const glm::vec3& cameraTarget, double simTime);
    void renderDeformableBodies(const Camera& camera, float aspect, const MatterSystem& matter, const std::vector<StarLightSource>& stars, const glm::vec3& cameraTarget, MatterVisualizationMode visMode);
    void renderSkybox(const Camera& camera, float aspect);

    GLuint loadTexture(const std::string& filepath);

private:
    MeshData createSphereMesh(float radius, int stacks, int sectors);
    MeshData createRingMesh(int radialSegments);
    MeshData createQuadMesh();

    // 1. Celestial PBR Uber-Shader (Multi-Star, Thermal Glow, Limb Darkening, Debug Overlays)
    GLuint m_shaderProgram = 0;
    GLint m_uMVPLoc = -1;
    GLint m_uModelLoc = -1;
    GLint m_uNormalMatLoc = -1;
    GLint m_uColorLoc = -1;
    GLint m_uUseTextureLoc = -1;
    GLint m_uTextureLoc = -1;
    GLint m_uEmissionColorLoc = -1;
    GLint m_uEmissionIntensityLoc = -1;
    GLint m_uThermalGlowLoc = -1;
    GLint m_uIsSunLoc = -1;
    GLint m_uSimTimeLoc = -1;
    GLint m_uCameraPosLoc = -1;
    GLint m_uNumLightsLoc = -1;
    GLint m_uLightPosLoc[4];
    GLint m_uLightColorLoc[4];
    GLint m_uLightIntensityLoc[4];
    GLint m_uDebugOverlayLoc = -1;
    GLint m_uDebugColorLoc = -1;
    GLint m_uDebugScalarLoc = -1;

    // 2. Atmospheric Scattering Shader
    GLuint m_atmosphereProgram = 0;
    GLint m_uAtmoMVPLoc = -1;
    GLint m_uAtmoModelLoc = -1;
    GLint m_uAtmoColorLoc = -1;
    GLint m_uAtmoDensityLoc = -1;
    GLint m_uAtmoCameraPosLoc = -1;
    GLint m_uAtmoNumLightsLoc = -1;
    GLint m_uAtmoLightPosLoc[4];
    GLint m_uAtmoLightColorLoc[4];

    // 3. Dynamic Rotating Cloud Layer Shader
    GLuint m_cloudProgram = 0;
    GLint m_uCloudMVPLoc = -1;
    GLint m_uCloudModelLoc = -1;
    GLint m_uCloudCoverageLoc = -1;
    GLint m_uCloudSimTimeLoc = -1;
    GLint m_uCloudNumLightsLoc = -1;
    GLint m_uCloudLightPosLoc[4];

    // 4. Stellar Corona & Solar Flare Shader
    GLuint m_coronaProgram = 0;
    GLint m_uCoronaMVPLoc = -1;
    GLint m_uCoronaColorLoc = -1;
    GLint m_uCoronaIntensityLoc = -1;
    GLint m_uCoronaSimTimeLoc = -1;

    // 5. Relativistic Black Hole Shader
    GLuint m_blackHoleProgram = 0;
    GLint m_uBhMVPLoc = -1;
    GLint m_uBhModelLoc = -1;
    GLint m_uBhCameraPosLoc = -1;
    GLint m_uBhSchwRadiusLoc = -1;
    GLint m_uBhPhotonRadiusLoc = -1;
    GLint m_uBhSimTimeLoc = -1;

    // 6. Collision & Impact Shockwave FX Shader
    GLuint m_impactProgram = 0;
    GLint m_uImpVPLoc = -1;
    GLint m_uImpCenterLoc = -1;
    GLint m_uImpNormalLoc = -1;
    GLint m_uImpRadiusLoc = -1;
    GLint m_uImpIntensityLoc = -1;
    GLint m_uImpColorLoc = -1;
    GLint m_uImpAgeLoc = -1;

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
    MeshData m_quadMesh;
    std::unordered_map<std::string, GLuint> m_textures;
    ParticleRenderer m_particleRenderer;
    DeformableRenderer m_deformableRenderer;
};

} // namespace AstroGenesis
