#pragma once

#include <vector>
#include <string>
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

class PhysicsEngine {
public:
    PhysicsEngine();

    void initializeDefaultSolarSystem();
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

    void stepFrameForward();
    void stepFrameBackward();

    std::string getSimulationTimeStr() const;

    // Body getters/selection
    const std::vector<CelestialBody>& getBodies() const { return m_bodies; }
    std::vector<CelestialBody>& getBodies() { return m_bodies; }

    int getSelectedBodyIndex() const { return m_selectedBodyIndex; }
    void selectBody(int index);

    const CelestialBody& getSelectedBody() const;

    // Stats
    int getObjectCount() const { return (int)m_bodies.size(); }
    float getPhysicsStepTimeMs() const { return m_physicsStepMs; }

private:
    void updateBodyScales();

    std::vector<CelestialBody> m_bodies;
    int m_selectedBodyIndex = 3; // Earth default

    bool m_isPaused = false;
    float m_timeScale = 86400.0f; // Default 1 day/sec for real astronomical motion
    bool m_isTrueScaleMode = true; // True 1:1 Astronomical Scale by default
    float m_sizeMultiplier = 1.0f;
    double m_simulatedTimeSeconds = 0.0;
    float m_physicsStepMs = 2.45f;
};

} // namespace AstroGenesis
