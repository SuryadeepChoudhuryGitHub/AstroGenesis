#pragma once

#include <string>
#include <vector>
#include "ai/OrbitalStabilityPredictor.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {
namespace ai {

struct DecisionTreeNode {
    int leftChild = -1;
    int rightChild = -1;
    int featureIndex = -1;
    float threshold = 0.0f;
    float probUnstable = 0.5f;
    float probStable = 0.5f;
};

struct DecisionTree {
    std::vector<DecisionTreeNode> nodes;
};

struct HabitabilityResult {
    float score = 0.0f;               // 0 to 100
    std::string classification;       // "Potentially Habitable", "Marginal", "Unfavorable"
    float temperatureESI = 0.0f;     // Sub-index
    float radiusESI = 0.0f;
    float gravityESI = 0.0f;
    float fluxESI = 0.0f;
    std::string diagnostic;
};

class ModelInference {
public:
    ModelInference();

    bool loadModel(const std::string& jsonModelPath);
    bool isModelLoaded() const { return m_isLoaded; }
    const std::string& getModelPath() const { return m_modelPath; }
    const std::string& getLastError() const { return m_lastError; }
    const std::string& getModelArchitecture() const { return m_modelArchitecture; }
    float getTrainingAccuracy() const { return m_trainingAccuracy; }
    float getTrainingF1() const { return m_trainingF1; }
    int getTreeCount() const { return (int)m_trees.size(); }

    // Run inference for orbital stability
    StabilityPrediction predictStability(const std::vector<float>& features,
                                        int bodyCount,
                                        float minMutualHillSep,
                                        float meanMutualHillSep,
                                        float maxEcc,
                                        float meanEcc,
                                        float minPeriodRatio,
                                        float amd,
                                        bool hasOrbitCrossing,
                                        const std::vector<std::string>& risks) const;

    // Optional Second Feature: Habitability Estimator
    HabitabilityResult estimateHabitability(const CelestialBody& body, double starLumW) const;

private:
    bool m_isLoaded = false;
    std::string m_modelPath;
    std::string m_lastError;
    std::string m_modelArchitecture = "RandomForest (50 trees, max_depth=8)";
    float m_trainingAccuracy = 0.0f;
    float m_trainingF1 = 0.0f;
    std::vector<std::string> m_featureNames;
    std::vector<DecisionTree> m_trees;
};

} // namespace ai
} // namespace AstroGenesis
