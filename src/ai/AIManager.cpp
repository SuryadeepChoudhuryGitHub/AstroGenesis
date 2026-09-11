#include "ai/AIManager.hpp"
#include "simulation/PhysicsEngine.hpp"
#include <iostream>

namespace AstroGenesis {
namespace ai {

AIManager::AIManager()
    : m_predictor(std::make_unique<OrbitalStabilityPredictor>()),
      m_inference(std::make_unique<ModelInference>()) {
}

AIManager::~AIManager() = default;

bool AIManager::initialize(const std::string& modelPath) {
    bool pOk = m_predictor->initialize(modelPath);
    bool iOk = m_inference->loadModel(modelPath);

    if (!pOk || !iOk) {
        std::cerr << "[AIManager] Warning: Could not initialize model from " << modelPath 
                  << ". Error: " << m_predictor->getLastError() 
                  << ". Simulation will run normally without AI features." << std::endl;
        m_currentPrediction.prediction = "UNAVAILABLE";
        m_currentPrediction.modelStatusStr = "Model file not found: " + modelPath;
        return false;
    }

    std::cout << "[AIManager] Subsystem initialized successfully with model: " << modelPath << std::endl;
    return true;
}

bool AIManager::isModelLoaded() const {
    return m_predictor && m_predictor->isModelLoaded();
}

const std::string& AIManager::getModelPath() const {
    return m_predictor->getModelPath();
}

const std::string& AIManager::getLastError() const {
    return m_predictor->getLastError();
}

void AIManager::update(const PhysicsEngine& physics, float deltaTime) {
    m_timeSinceLastUpdate += deltaTime;

    // Detect if bodies list changed (e.g. body added or removed)
    size_t curCount = physics.getBodies().size();
    bool countChanged = (curCount != m_lastBodyCount);
    m_lastBodyCount = curCount;

    if (m_timeSinceLastUpdate >= m_updateIntervalSec || countChanged) {
        m_timeSinceLastUpdate = 0.0f;
        forceRecompute(physics);
    }
}

void AIManager::forceRecompute(const PhysicsEngine& physics) {
    m_currentFeatures = FeatureExtractor::extract(physics);
    m_currentPrediction = m_predictor->predict(physics);
}

StabilityPrediction AIManager::evaluateWhatIf(const std::vector<CelestialBody>& perturbedBodies) const {
    if (!m_predictor) return StabilityPrediction{};
    return m_predictor->predictFromBodies(perturbedBodies, 0.0);
}

HabitabilityResult AIManager::evaluateHabitability(const CelestialBody& body, double starLumW) const {
    if (!m_inference) return HabitabilityResult{};
    return m_inference->estimateHabitability(body, starLumW);
}

} // namespace ai
} // namespace AstroGenesis
