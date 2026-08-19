#pragma once

#include <vector>
#include <string>
#include "simulation/CelestialBody.hpp"
#include "simulation/ParticleSystem.hpp"
#include "simulation/MatterSystem.hpp"
#include "data/repositories/ObjectRepository.hpp"

namespace AstroGenesis {

enum class AsteroidPopulationMode {
    RealOnly,      // Only real asteroids from SQLite DB (e.g. Ceres, Vesta, Pallas, etc.)
    SyntheticOnly, // Procedural statistical particles
    Hybrid         // Real major asteroids + procedural background particles
};

class PhysicsEngine {
public:
    PhysicsEngine();

    // Data-Driven System Loading from Database
    bool loadFromDatabase(ObjectRepository& repo, const std::string& systemCategory = "Solar System");
    void reloadCurrentSystem(ObjectRepository& repo);
    void addBody(const CelestialBody& body);
    void clearBodies();

    void update(float deltaTime);

    // Time controls
    bool isPaused() const { return m_isPaused; }
    void setPaused(bool paused) { m_isPaused = paused; }
    void togglePause() { m_isPaused = !m_isPaused; }

    float getTimeScale() const { return m_timeScale; }
    void setTimeScale(float scale) { m_timeScale = scale; }

    bool isTrueScaleMode() const { return m_isTrueScaleMode; }
    void setTrueScaleMode(bool enabled) { m_isTrueScaleMode = enabled; updateBodyScales(); }

    float getSizeMultiplier() const { return m_sizeMultiplier; }
    void setSizeMultiplier(float val) { m_sizeMultiplier = val; updateBodyScales(); }

    bool isGeneralRelativityEnabled() const { return m_enableGeneralRelativity; }
    void setGeneralRelativityEnabled(bool enabled) { m_enableGeneralRelativity = enabled; }
    void toggleGeneralRelativity() { m_enableGeneralRelativity = !m_enableGeneralRelativity; }

    void stepFrameForward();
    void stepFrameBackward();

    std::string getSimulationTimeStr() const;
    std::string getSimVsRealTimeStr() const;
    std::string getTotalEnergyStr() const;
    std::string getTotalAngularMomentumStr() const;
    std::string getCurrentCategory() const { return m_currentCategory; }

    // Body getters/selection
    const std::vector<CelestialBody>& getBodies() const { return m_bodies; }
    std::vector<CelestialBody>& getBodies() { return m_bodies; }

    int getSelectedBodyIndex() const { return m_selectedBodyIndex; }
    void selectBody(int index);
    void selectBodyById(const std::string& id);

    const CelestialBody& getSelectedBody() const;
    void clearTrails();
    void triggerRingImpact(const std::string& planetId, float normRadius, float azimuthRad, float impactRadiusM = 4000000.0f);
    void triggerSaturnRingImpact();

    // Particle System & Asteroid Belt Population Control
    ParticleSystem& getParticleSystem() { return m_particleSystem; }
    const ParticleSystem& getParticleSystem() const { return m_particleSystem; }
    ParticleField& getAsteroidBelt() { return m_particleSystem.getAsteroidBelt(); }
    const ParticleField& getAsteroidBelt() const { return m_particleSystem.getAsteroidBelt(); }
    
    AsteroidPopulationMode getAsteroidPopulationMode() const { return m_asteroidPopulationMode; }
    void setAsteroidPopulationMode(AsteroidPopulationMode mode, ObjectRepository* repo = nullptr);
    void reseedAsteroidBelt(int physicalCount, int visualCount) { m_particleSystem.getAsteroidBelt().reseed(physicalCount, visualCount); }

    // Deformable Matter System getters
    MatterSystem& getMatterSystem() { return m_matterSystem; }
    const MatterSystem& getMatterSystem() const { return m_matterSystem; }

    // Global Physics & Conservation Stats
    int getObjectCount() const { return (int)m_bodies.size(); }
    float getPhysicsStepTimeMs() const { return m_physicsStepMs; }
    double getTotalEnergyJoules() const { return m_totalSystemEnergyJ; }
    double getTotalAngularMomentum() const { return m_totalSystemAngularMomentum; }
    double getEnergyConservationDriftPct() const { return m_energyConservationDriftPct; }
    double getSimulatedTimeSeconds() const { return m_simulatedTimeSeconds; }
    double getRealTimeElapsedSeconds() const { return m_realTimeElapsedSeconds; }

private:
    void updateBodyScales();
    void computeAccelerations(const std::vector<glm::dvec3>& positions,
                              const std::vector<glm::dvec3>& velocities,
                              std::vector<glm::dvec3>& outAccelerations);
    void integrateNBody(double deltaSeconds);
    void updatePhysicalQuantities();
    void updateRingHydrodynamics(double deltaSeconds);
    void computeSystemConservationStats();
    void computeBarycenterTransform();
    void generateOrbitalTrails();

    std::vector<CelestialBody> m_bodies;
    int m_selectedBodyIndex = 0;
    std::string m_currentCategory = "Solar System";

    bool m_isPaused = false;
    bool m_enableGeneralRelativity = true; // Einstein 1PN Post-Newtonian Gravity
    float m_timeScale = 86400.0f; // Default 1 day/sec for real astronomical motion
    bool m_isTrueScaleMode = true; // True 1:1 Astronomical Scale by default
    float m_sizeMultiplier = 1.0f;
    double m_simulatedTimeSeconds = 0.0;
    double m_realTimeElapsedSeconds = 0.0;
    float m_physicsStepMs = 2.45f;

    // Conservation quantities
    double m_initialSystemEnergyJ = 0.0;
    double m_totalSystemEnergyJ = 0.0;
    double m_totalSystemKineticJ = 0.0;
    double m_totalSystemPotentialJ = 0.0;
    double m_totalSystemAngularMomentum = 0.0;
    double m_energyConservationDriftPct = 0.0;

    AsteroidPopulationMode m_asteroidPopulationMode = AsteroidPopulationMode::Hybrid;
    ParticleSystem m_particleSystem;
    MatterSystem m_matterSystem;
};

} // namespace AstroGenesis
