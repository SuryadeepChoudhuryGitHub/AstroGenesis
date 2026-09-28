#pragma once

#include <string>
#include <vector>
#include <memory>
#include "ai/OrbitalStabilityPredictor.hpp"
#include "ai/FeatureExtractor.hpp"
#include "ai/ModelInference.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

class PhysicsEngine;

namespace ai {

class AIManager {
public:
    AIManager();
    ~AIManager();

    bool initialize(const std::string& modelPath = "assets/models/orbital_stability_model.json");
    void update(const PhysicsEngine& physics, float deltaTime);

    void forceRecompute(const PhysicsEngine& physics);

    // Live state getters for UI
    const StabilityPrediction& getCurrentPrediction() const { return m_currentPrediction; }
    const ExtractedFeatures& getCurrentFeatures() const { return m_currentFeatures; }
    bool isModelLoaded() const;
    const std::string& getModelPath() const;
    const std::string& getLastError() const;

    // What-If testing for the interactive studio
    StabilityPrediction evaluateWhatIf(const std::vector<CelestialBody>& perturbedBodies) const;

    // Habitability analysis (Step 10)
    HabitabilityResult evaluateHabitability(const CelestialBody& body, double starLumW = 3.828e26) const;

    // Configuration
    float getUpdateInterval() const { return m_updateIntervalSec; }
    void setUpdateInterval(float sec) { m_updateIntervalSec = sec; }

private:
    std::unique_ptr<OrbitalStabilityPredictor> m_predictor;
    std::unique_ptr<ModelInference> m_inference;

    StabilityPrediction m_currentPrediction;
    ExtractedFeatures m_currentFeatures;

    float m_updateIntervalSec = 0.40f; // Run inference every 400ms (zero FPS impact)
    float m_timeSinceLastUpdate = 0.0f;
    size_t m_lastBodyCount = 0;
};

} // namespace ai
} // namespace AstroGenesis
