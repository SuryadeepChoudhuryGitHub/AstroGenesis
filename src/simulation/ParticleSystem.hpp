#pragma once

#include <vector>
#include <string>
#include <random>
#include <memory>
#include <glm/glm.hpp>
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

struct KeplerianElements {
    double semiMajorAxisAU = 2.7;           // a in AU
    double eccentricity = 0.08;             // e (0 to 1)
    double inclinationDeg = 7.0;            // i in degrees
    double longitudeAscendingNodeDeg = 0.0; // Omega in degrees
    double argumentOfPeriapsisDeg = 0.0;    // omega in degrees
    double meanAnomalyDeg = 0.0;            // M in degrees
    double trueAnomalyDeg = 0.0;            // nu in degrees
    double orbitalPeriodDays = 1600.0;      // T in days
};

struct PhysicalParticle {
    glm::dvec3 positionM{0.0};              // Absolute position in meters (Barycentric)
    glm::dvec3 velocityMps{0.0};            // Absolute velocity in m/s
    glm::dvec3 accelerationMps2{0.0};       // Instantaneous gravitational acceleration

    double massKg = 1.0e15;                 // Mass in kg
    double radiusM = 1000.0;                // Physical radius in meters
    KeplerianElements osculating;           // Instantaneous osculating orbital elements

    bool isEscaped = false;                 // Escaped / unbound orbit
    bool collidedWithCentral = false;       // Perihelion < Central body radius
    float colorSeed = 0.0f;                 // Particle color tint variance
};

struct VisualParticle {
    float semiMajorAxisAU = 2.7f;
    float eccentricity = 0.08f;
    float inclinationRad = 0.12f;
    float longitudeAscendingNodeRad = 0.0f;
    float argumentOfPeriapsisRad = 0.0f;
    float meanAnomaly0Rad = 0.0f;
    float meanMotionRadPerSec = 0.0f;
    float sizeScale = 1.0f;
    float colorSeed = 0.0f;
};

// Compact GPU instance buffer data layout (32 bytes aligned)
struct ParticleInstanceData {
    glm::vec3 pos;       // Camera-relative position in AU (layout 2)
    float scale;         // Visual scale multiplier (layout 3)
    glm::vec4 color;     // Albedo and specular intensity (layout 4)
};

enum class ParticleFieldType {
    AsteroidBelt,
    KuiperBelt,
    AccretionDisk,
    DebrisDisk,
    PlanetaryDebris,
    Custom
};

struct ParticleFieldConfig {
    std::string name = "Main Asteroid Belt";
    ParticleFieldType type = ParticleFieldType::AsteroidBelt;

    double innerRadiusAU = 2.08;
    double outerRadiusAU = 3.28;
    double eccentricitySigma = 0.08;
    double eccentricityMax = 0.32;
    double inclinationSigmaDeg = 6.8;
    double inclinationMaxDeg = 28.0;

    double minRadiusM = 400.0;
    double maxRadiusM = 470000.0;
    double densityKgM3 = 2200.0;

    glm::vec3 colorA{0.50f, 0.46f, 0.42f};
    glm::vec3 colorB{0.72f, 0.65f, 0.55f};

    int physicalCount = 1500;
    int visualCount = 120000;

    // Presets
    static ParticleFieldConfig createMainAsteroidBelt() {
        ParticleFieldConfig cfg;
        cfg.name = "Main Asteroid Belt";
        cfg.type = ParticleFieldType::AsteroidBelt;
        cfg.innerRadiusAU = 2.08;
        cfg.outerRadiusAU = 3.28;
        cfg.eccentricitySigma = 0.08;
        cfg.eccentricityMax = 0.32;
        cfg.inclinationSigmaDeg = 6.8;
        cfg.inclinationMaxDeg = 28.0;
        cfg.minRadiusM = 400.0;
        cfg.maxRadiusM = 470000.0;
        cfg.densityKgM3 = 2200.0;
        cfg.colorA = glm::vec3(0.50f, 0.46f, 0.42f);
        cfg.colorB = glm::vec3(0.72f, 0.65f, 0.55f);
        cfg.physicalCount = 1500;
        cfg.visualCount = 120000;
        return cfg;
    }

    static ParticleFieldConfig createKuiperBelt() {
        ParticleFieldConfig cfg;
        cfg.name = "Kuiper Belt";
        cfg.type = ParticleFieldType::KuiperBelt;
        cfg.innerRadiusAU = 30.0;
        cfg.outerRadiusAU = 55.0;
        cfg.eccentricitySigma = 0.06;
        cfg.eccentricityMax = 0.25;
        cfg.inclinationSigmaDeg = 12.0;
        cfg.inclinationMaxDeg = 35.0;
        cfg.minRadiusM = 1000.0;
        cfg.maxRadiusM = 1200000.0; // Pluto/Eris size
        cfg.densityKgM3 = 1800.0;   // Icy composition
        cfg.colorA = glm::vec3(0.40f, 0.45f, 0.55f);
        cfg.colorB = glm::vec3(0.65f, 0.70f, 0.85f);
        cfg.physicalCount = 1000;
        cfg.visualCount = 80000;
        return cfg;
    }
};

struct ParticleHistogram {
    static constexpr int NUM_BINS = 60;
    float minAU = 2.0f;
    float maxAU = 3.6f;
    float binWidth = (maxAU - minAU) / (float)NUM_BINS;
    std::vector<int> counts;
    int maxBinCount = 1;

    // Major Jupiter Mean-Motion Resonance locations (AU)
    static constexpr float RES_4_1 = 2.064f; // 4:1 resonance (Inner belt edge)
    static constexpr float RES_3_1 = 2.501f; // 3:1 resonance (Kirkwood Gap 1)
    static constexpr float RES_5_2 = 2.824f; // 5:2 resonance (Kirkwood Gap 2)
    static constexpr float RES_7_3 = 2.957f; // 7:3 resonance (Kirkwood Gap 3)
    static constexpr float RES_2_1 = 3.277f; // 2:1 resonance (Hecuba Gap / Outer edge)
};

struct ParticleDiagnostics {
    int totalPhysical = 0;
    int activePhysical = 0;
    int escapedPhysical = 0;
    int sunCollidedPhysical = 0;
    int totalVisual = 0;

    double meanSemiMajorAxisAU = 0.0;
    double meanEccentricity = 0.0;
    double maxEccentricity = 0.0;
    double meanInclinationDeg = 0.0;
    double totalMassKg = 0.0;
    int highlyExcitedCount = 0; // e > 0.25 (pumped by resonances)

    double initialEnergyJ = 0.0;
    double currentEnergyJ = 0.0;
    double energyDriftPct = 0.0;
};

// Generic physical + GPU particle field representing any astronomical particle disk
class ParticleField {
public:
    ParticleField();
    explicit ParticleField(const ParticleFieldConfig& config);
    ~ParticleField();

    void initialize(const ParticleFieldConfig& config);
    void reseed(int physicalCount, int visualCount);

    void update(double deltaSeconds, const std::vector<CelestialBody>& attractors, bool enableGR, double simulatedTimeSeconds);
    void updateVisualInstanceBuffer(double simulatedTimeSeconds, const glm::vec3& cameraTarget, float visualSizeMultiplier);

    // Diagnostics & Inspection
    const ParticleFieldConfig& getConfig() const { return m_config; }
    const ParticleHistogram& getHistogram() const { return m_histogram; }
    const ParticleDiagnostics& getDiagnostics() const { return m_diagnostics; }
    const std::vector<ParticleInstanceData>& getInstanceData() const { return m_instanceData; }

    // Physical particles accessors
    const std::vector<PhysicalParticle>& getPhysicalParticles() const { return m_physicalParticles; }
    int getPhysicalCount() const { return (int)m_physicalParticles.size(); }
    int getVisualCount() const { return (int)m_visualParticles.size(); }
    float getVisualSizeMultiplier() const { return m_visualSizeMultiplier; }
    void setVisualSizeMultiplier(float mult) { m_visualSizeMultiplier = mult; }

    void triggerResonanceImpulseTest();

    // Astrodynamics coordinate transformations
    static void keplerianToCartesian(const KeplerianElements& elem, double centralMassKg,
                                     glm::dvec3& outPosM, glm::dvec3& outVelMps);
    static void cartesianToKeplerian(const glm::dvec3& posM, const glm::dvec3& velMps, double centralMassKg,
                                     KeplerianElements& outElem);

private:
    void generatePhysicalPopulation(int count);
    void generateVisualPopulation(int count);
    void computePhysicalAccelerations(const std::vector<CelestialBody>& attractors, bool enableGR);
    void integratePhysicalParticles(double dt, const std::vector<CelestialBody>& attractors, bool enableGR);
    void updateDiagnostics(const std::vector<CelestialBody>& attractors);

    ParticleFieldConfig m_config;
    std::vector<PhysicalParticle> m_physicalParticles;
    std::vector<VisualParticle> m_visualParticles;
    std::vector<ParticleInstanceData> m_instanceData;

    ParticleHistogram m_histogram;
    ParticleDiagnostics m_diagnostics;

    float m_visualSizeMultiplier = 1.0f;
    std::mt19937 m_rng{1337};
};

// Global Particle System managing all particle fields (Asteroid Belt, Kuiper Belt, Debris Disks, etc.)
class ParticleSystem {
public:
    ParticleSystem();
    ~ParticleSystem();

    void initializeDefaultSystem();
    void update(double deltaSeconds, const std::vector<CelestialBody>& attractors, bool enableGR, double simulatedTimeSeconds);

    ParticleField& getAsteroidBelt() { return m_asteroidBelt; }
    const ParticleField& getAsteroidBelt() const { return m_asteroidBelt; }

    const std::vector<std::shared_ptr<ParticleField>>& getFields() const { return m_fields; }

private:
    ParticleField m_asteroidBelt;
    std::vector<std::shared_ptr<ParticleField>> m_fields;
};

} // namespace AstroGenesis
