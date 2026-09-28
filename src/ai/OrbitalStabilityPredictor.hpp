#pragma once

#include <string>
#include <vector>
#include <memory>
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

class PhysicsEngine;

namespace ai {

class ModelInference;

enum class StabilityClass {
    Stable,
    Marginal,
    Unstable,
    Unknown
};

enum class ConfidenceLevel {
    High,
    Medium,
    Low
};

struct StabilityPrediction {
    StabilityClass stabilityClass = StabilityClass::Unknown;
    std::string prediction = "UNKNOWN";   // "STABLE", "MARGINAL", "UNSTABLE", "UNAVAILABLE"
    float stableProbability = 0.0f;      // [0.0, 1.0]
    float unstableProbability = 0.0f;    // [0.0, 1.0]
    std::string confidence = "LOW";      // "HIGH", "MEDIUM", "LOW"
    ConfidenceLevel confidenceLevel = ConfidenceLevel::Low;

    // Physical metrics extracted from current system
    int analyzedBodyCount = 0;
    float minMutualHillSep = 0.0f;       // Gladman Hill separation Delta
    float meanMutualHillSep = 0.0f;
    float maxEccentricity = 0.0f;
    float meanEccentricity = 0.0f;
    float minPeriodRatio = 0.0f;
    float angularMomentumDeficit = 0.0f; // AMD
    bool hasOrbitCrossing = false;

    // Diagnostics & Transparency
    std::vector<std::string> riskFactors;
    std::string primaryRiskFactor = "None detected";
    float inferenceTimeUs = 0.0f;
    bool modelLoaded = false;
    std::string modelStatusStr = "Model uninitialized";
    std::string modelArchitecture = "RandomForest (50 trees, max_depth=8)";
};

class OrbitalStabilityPredictor {
public:
    OrbitalStabilityPredictor();
    ~OrbitalStabilityPredictor();

    bool initialize(const std::string& modelPath);
    bool isModelLoaded() const;

    StabilityPrediction predict(const PhysicsEngine& physics) const;
    StabilityPrediction predictFromBodies(const std::vector<CelestialBody>& bodies, double energyDriftPct = 0.0) const;

    const std::string& getModelPath() const;
    const std::string& getLastError() const;

private:
    std::unique_ptr<ModelInference> m_inference;
};

} // namespace ai
} // namespace AstroGenesis
