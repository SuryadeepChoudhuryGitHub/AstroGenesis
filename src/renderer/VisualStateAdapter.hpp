#pragma once

#include <vector>
#include <string>
#include <deque>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "simulation/CelestialBody.hpp"
#include "simulation/DeformableBody.hpp"
#include "simulation/MaterialModel.hpp"

namespace AstroGenesis {

enum class VisualMode {
    Realistic = 0,   // Physically motivated photorealistic PBR, authentic albedos & scattering
    Scientific = 1,  // Clear high-contrast lighting, trajectory overlays, reference indicators
    Cinematic = 2,   // Enhanced bloom, atmospheric limb glow, dynamic solar corona
    Debug = 3        // False-color physical field overlays (Stress, Temp, Velocity, GR, Phase)
};

enum class DebugVisualOverlay {
    None = 0,
    VonMisesStress = 1,
    PlasticStrain = 2,
    DamageFracture = 3,
    Temperature = 4,
    VelocityVectors = 5,
    GravitationalField = 6,
    MaterialPhase = 7
};

enum class VisualQuality {
    Low = 0,
    Medium = 1,
    High = 2,
    Ultra = 3
};

struct StarLightSource {
    glm::vec3 positionAU{0.0f};
    glm::vec3 color{1.0f, 0.95f, 0.88f}; // Blackbody emitted light color
    float intensity = 1.0f;              // Normalized solar radiation luminosity (1.0 = Sol)
    float radiusAU = 0.00465f;           // Physical radius in AU
    std::string name;
};

struct VisualBodyState {
    int64_t dbId = 0;
    std::string id;
    std::string name;
    std::string type;
    
    // Position & Coordinates
    glm::vec3 positionAU{0.0f};
    glm::vec3 velocityAU{0.0f};
    
    // Geometry & Scale
    float renderRadius = 0.03f;          // Rendered radius in 3D AU viewport units
    float trueRadiusAU = 0.0f;           // 1:1 Physical radius in AU
    double physicalRadiusKm = 0.0;       // True radius in kilometers
    
    // Rotation & Axial Tilt
    float axialTiltDeg = 0.0f;
    float currentRotationAngle = 0.0f;
    float rotationSpeedRadPerSec = 0.0f;
    glm::mat4 rotationMatrix{1.0f};
    
    // Thermal & Surface Photometry
    double surfaceTempK = 287.0;
    glm::vec3 baseAlbedo{0.3f};
    glm::vec3 temperatureColor{1.0f};    // Planck blackbody color
    glm::vec3 emissionColor{0.0f};       // Magma incandescence or stellar emission
    float emissionIntensity = 0.0f;
    float thermalGlow = 0.0f;            // Magma crust fissure brightness [0, 1]
    
    // Atmospheric & Cloud Systems
    bool hasAtmosphere = false;
    float atmosphereRadius = 0.0f;       // Outer boundary of atmospheric scattering shell
    float atmosphereThickness = 0.0f;
    glm::vec3 atmosphereColor{0.25f, 0.55f, 0.95f}; // Rayleigh scattering wavelength tint
    float atmosphereDensity = 1.0f;      // Surface pressure / optical depth
    double scaleHeightKm = 8.5;          // Atmospheric scale height H = kT / (mu * g)
    bool hasClouds = false;
    float cloudCoverage = 0.0f;          // [0, 1]
    float cloudRotationAngle = 0.0f;
    
    // Stellar & Relativistic States
    bool isStar = false;
    bool isBlackHole = false;
    double luminosityWatts = 0.0;
    float coronaIntensity = 0.0f;
    float schwarzschildRadiusAU = 0.0f;
    float photonSphereRadiusAU = 0.0f;
    
    // Material Phase & Debug Overlays
    MaterialPhase phase = MaterialPhase::Solid;
    float phaseFraction = 0.0f;          // [0, 1] (e.g. melting progress)
    glm::vec3 debugColor{0.0f};
    float debugScalar = 0.0f;
};

struct VisualImpactEvent {
    glm::vec3 positionAU{0.0f};
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
    double impactEnergyJoules = 0.0;
    float flashRadiusAU = 0.01f;
    float intensity = 1.0f;              // Decays from 1.0 to 0.0
    float ageSeconds = 0.0f;
    float maxAgeSeconds = 3.0f;
    glm::vec3 thermalColor{1.0f, 0.6f, 0.2f};
};

class VisualStateAdapter {
public:
    VisualStateAdapter();
    ~VisualStateAdapter() = default;

    // Core Translation Pipeline: Physical Simulation State -> Visual State
    void update(
        const std::vector<CelestialBody>& bodies,
        double deltaSimSeconds,
        bool isTrueScaleMode,
        float visualScaleMultiplier,
        VisualMode visualMode,
        DebugVisualOverlay debugOverlay
    );

    // Star & Lighting Discovery
    const std::vector<StarLightSource>& getStarLightSources() const { return m_starLights; }
    const std::vector<VisualBodyState>& getVisualBodies() const { return m_visualBodies; }
    const VisualBodyState* getVisualBody(const std::string& id) const;
    const VisualBodyState* getVisualBody(int64_t dbId) const;

    // Collision & Impact Event System
    void registerImpact(const glm::vec3& posAU, const glm::vec3& normal, double impactEnergyJ);
    void updateImpactEvents(float deltaRealSeconds);
    const std::vector<VisualImpactEvent>& getActiveImpacts() const { return m_impactEvents; }

    // Physical Mapping Utilities
    static glm::vec3 temperatureToPlanckRGB(double kelvin);
    static glm::vec3 getSpectralClassColor(double kelvin, std::string& outSpectralType);
    static float calculateAtmosphericScaleHeightKm(double surfaceTempK, double surfaceGravityMps2, double meanMolarMassKgMol = 0.02897);
    static float calculateRenderRadius(double physicalRadiusM, double realRadiusAU, bool isTrueScale, float scaleMultiplier, float systemReferenceScale);

    // Configuration
    VisualMode getVisualMode() const { return m_visualMode; }
    void setVisualMode(VisualMode mode) { m_visualMode = mode; }

    DebugVisualOverlay getDebugOverlay() const { return m_debugOverlay; }
    void setDebugOverlay(DebugVisualOverlay overlay) { m_debugOverlay = overlay; }

    VisualQuality getVisualQuality() const { return m_quality; }
    void setVisualQuality(VisualQuality q) { m_quality = q; }

    bool areAtmospheresEnabled() const { return m_enableAtmospheres; }
    void setAtmospheresEnabled(bool val) { m_enableAtmospheres = val; }

    bool areCloudsEnabled() const { return m_enableClouds; }
    void setCloudsEnabled(bool val) { m_enableClouds = val; }

    bool isMultiStarLightingEnabled() const { return m_enableMultiStarLighting; }
    void setMultiStarLightingEnabled(bool val) { m_enableMultiStarLighting = val; }

    bool areShadowsEnabled() const { return m_enableShadows; }
    void setShadowsEnabled(bool val) { m_enableShadows = val; }

    bool areImpactFXEnabled() const { return m_enableImpactFX; }
    void setImpactFXEnabled(bool val) { m_enableImpactFX = val; }

private:
    VisualMode m_visualMode = VisualMode::Realistic;
    DebugVisualOverlay m_debugOverlay = DebugVisualOverlay::None;
    VisualQuality m_quality = VisualQuality::High;

    bool m_enableAtmospheres = true;
    bool m_enableClouds = true;
    bool m_enableMultiStarLighting = true;
    bool m_enableShadows = true;
    bool m_enableImpactFX = true;

    std::vector<VisualBodyState> m_visualBodies;
    std::vector<StarLightSource> m_starLights;
    std::vector<VisualImpactEvent> m_impactEvents;

    float m_globalCloudRotationTimer = 0.0f;
};

} // namespace AstroGenesis
