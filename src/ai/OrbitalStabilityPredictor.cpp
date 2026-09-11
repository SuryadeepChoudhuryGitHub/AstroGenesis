#include "ai/OrbitalStabilityPredictor.hpp"
#include "ai/FeatureExtractor.hpp"
#include "ai/ModelInference.hpp"
#include "simulation/PhysicsEngine.hpp"

namespace AstroGenesis {
namespace ai {

OrbitalStabilityPredictor::OrbitalStabilityPredictor()
    : m_inference(std::make_unique<ModelInference>()) {
}

OrbitalStabilityPredictor::~OrbitalStabilityPredictor() = default;

bool OrbitalStabilityPredictor::initialize(const std::string& modelPath) {
    return m_inference->loadModel(modelPath);
}

bool OrbitalStabilityPredictor::isModelLoaded() const {
    return m_inference && m_inference->isModelLoaded();
}

const std::string& OrbitalStabilityPredictor::getModelPath() const {
    return m_inference->getModelPath();
}

const std::string& OrbitalStabilityPredictor::getLastError() const {
    return m_inference->getLastError();
}

StabilityPrediction OrbitalStabilityPredictor::predict(const PhysicsEngine& physics) const {
    ExtractedFeatures feat = FeatureExtractor::extract(physics);
    return m_inference->predictStability(
        feat.values,
        feat.bodyCount,
        feat.minMutualHillSep,
        feat.meanMutualHillSep,
        feat.maxEccentricity,
        feat.meanEccentricity,
        feat.minPeriodRatio,
        feat.angularMomentumDeficit,
        feat.hasOrbitCrossing,
        feat.identifiedRisks
    );
}

StabilityPrediction OrbitalStabilityPredictor::predictFromBodies(
    const std::vector<CelestialBody>& bodies, double energyDriftPct) const 
{
    ExtractedFeatures feat = FeatureExtractor::extractFromBodies(bodies, energyDriftPct);
    return m_inference->predictStability(
        feat.values,
        feat.bodyCount,
        feat.minMutualHillSep,
        feat.meanMutualHillSep,
        feat.maxEccentricity,
        feat.meanEccentricity,
        feat.minPeriodRatio,
        feat.angularMomentumDeficit,
        feat.hasOrbitCrossing,
        feat.identifiedRisks
    );
}

} // namespace ai
} // namespace AstroGenesis
