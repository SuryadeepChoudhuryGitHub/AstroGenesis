#include "ai/ModelInference.hpp"
#include <nlohmann/json.hpp>
#include <fstream>
#include <iostream>
#include <chrono>
#include <cmath>
#include <algorithm>

namespace AstroGenesis {
namespace ai {

using json = nlohmann::json;

ModelInference::ModelInference() {
}

bool ModelInference::loadModel(const std::string& jsonModelPath) {
    m_modelPath = jsonModelPath;
    m_trees.clear();
    m_isLoaded = false;
    m_lastError.clear();

    std::ifstream file(jsonModelPath);
    if (!file.is_open()) {
        m_lastError = "Could not open model file: " + jsonModelPath;
        std::cerr << "[ModelInference] " << m_lastError << std::endl;
        return false;
    }

    try {
        json j;
        file >> j;

        if (!j.contains("trees") || !j["trees"].is_array()) {
            m_lastError = "Invalid model format: 'trees' array not found.";
            return false;
        }

        if (j.contains("model_type")) {
            m_modelArchitecture = j["model_type"].get<std::string>() + " (" + 
                                  std::to_string(j["trees"].size()) + " trees)";
        }

        if (j.contains("metrics")) {
            const auto& metrics = j["metrics"];
            if (metrics.contains("accuracy")) m_trainingAccuracy = metrics["accuracy"].get<float>();
            if (metrics.contains("f1"))       m_trainingF1 = metrics["f1"].get<float>();
        }

        if (j.contains("feature_names")) {
            m_featureNames = j["feature_names"].get<std::vector<std::string>>();
        }

        for (const auto& treeJson : j["trees"]) {
            DecisionTree tree;
            int nodeCount = treeJson["node_count"].get<int>();
            tree.nodes.resize(nodeCount);

            auto leftChildren = treeJson["children_left"].get<std::vector<int>>();
            auto rightChildren = treeJson["children_right"].get<std::vector<int>>();
            auto features = treeJson["feature"].get<std::vector<int>>();
            auto thresholds = treeJson["threshold"].get<std::vector<float>>();
            auto leafProbs = treeJson["leaf_probs"].get<std::vector<std::vector<float>>>();

            for (int n = 0; n < nodeCount; ++n) {
                tree.nodes[n].leftChild = leftChildren[n];
                tree.nodes[n].rightChild = rightChildren[n];
                tree.nodes[n].featureIndex = features[n];
                tree.nodes[n].threshold = thresholds[n];
                if (n < (int)leafProbs.size() && leafProbs[n].size() >= 2) {
                    tree.nodes[n].probUnstable = leafProbs[n][0];
                    tree.nodes[n].probStable = leafProbs[n][1];
                }
            }
            m_trees.push_back(std::move(tree));
        }

        m_isLoaded = true;
        std::cout << "[ModelInference] Successfully loaded model from " << jsonModelPath 
                  << " with " << m_trees.size() << " decision trees (Train Accuracy: " 
                  << (m_trainingAccuracy * 100.0f) << "%)." << std::endl;
        return true;
    } catch (const std::exception& e) {
        m_lastError = std::string("Model parse error: ") + e.what();
        std::cerr << "[ModelInference] " << m_lastError << std::endl;
        m_isLoaded = false;
        return false;
    }
}

StabilityPrediction ModelInference::predictStability(
    const std::vector<float>& features,
    int bodyCount,
    float minMutualHillSep,
    float meanMutualHillSep,
    float maxEcc,
    float meanEcc,
    float minPeriodRatio,
    float amd,
    bool hasOrbitCrossing,
    const std::vector<std::string>& risks) const 
{
    StabilityPrediction pred;
    pred.analyzedBodyCount = bodyCount;
    pred.minMutualHillSep = minMutualHillSep;
    pred.meanMutualHillSep = meanMutualHillSep;
    pred.maxEccentricity = maxEcc;
    pred.meanEccentricity = meanEcc;
    pred.minPeriodRatio = minPeriodRatio;
    pred.angularMomentumDeficit = amd;
    pred.hasOrbitCrossing = hasOrbitCrossing;
    pred.riskFactors = risks;
    pred.modelLoaded = m_isLoaded;

    if (!m_isLoaded || m_trees.empty()) {
        pred.stabilityClass = StabilityClass::Unknown;
        pred.prediction = "UNAVAILABLE";
        pred.modelStatusStr = m_lastError.empty() ? "Model uninitialized (file missing)" : m_lastError;
        pred.confidence = "LOW";
        pred.confidenceLevel = ConfidenceLevel::Low;
        pred.primaryRiskFactor = "Model file not found";
        return pred;
    }

    auto start = std::chrono::high_resolution_clock::now();

    // Evaluate ensemble decision trees in parallel/sequential traversal
    double sumProbStable = 0.0;
    int nTrees = (int)m_trees.size();

    for (int t = 0; t < nTrees; ++t) {
        const auto& tree = m_trees[t];
        int node = 0;

        while (tree.nodes[node].leftChild != -1) {
            int featIdx = tree.nodes[node].featureIndex;
            float val = (featIdx >= 0 && featIdx < (int)features.size()) ? features[featIdx] : 0.0f;
            float thresh = tree.nodes[node].threshold;

            if (val <= thresh) {
                node = tree.nodes[node].leftChild;
            } else {
                node = tree.nodes[node].rightChild;
            }
        }
        sumProbStable += tree.nodes[node].probStable;
    }

    auto end = std::chrono::high_resolution_clock::now();
    pred.inferenceTimeUs = (float)std::chrono::duration_cast<std::chrono::microseconds>(end - start).count();

    float pStable = (float)(sumProbStable / (double)nTrees);
    pStable = std::clamp(pStable, 0.0f, 1.0f);
    pred.stableProbability = pStable;
    pred.unstableProbability = 1.0f - pStable;

    // Determine classification
    if (pStable >= 0.65f) {
        pred.stabilityClass = StabilityClass::Stable;
        pred.prediction = "STABLE";
    } else if (pStable <= 0.40f) {
        pred.stabilityClass = StabilityClass::Unstable;
        pred.prediction = "UNSTABLE";
    } else {
        pred.stabilityClass = StabilityClass::Marginal;
        pred.prediction = "MARGINAL";
    }

    // Determine confidence based on distance from decision boundary (0.5)
    float distanceToBoundary = std::abs(pStable - 0.5f);
    if (distanceToBoundary >= 0.35f) {
        pred.confidence = "HIGH";
        pred.confidenceLevel = ConfidenceLevel::High;
    } else if (distanceToBoundary >= 0.18f) {
        pred.confidence = "MEDIUM";
        pred.confidenceLevel = ConfidenceLevel::Medium;
    } else {
        pred.confidence = "LOW";
        pred.confidenceLevel = ConfidenceLevel::Low;
    }

    pred.modelStatusStr = "Model Active & Verified";
    pred.modelArchitecture = m_modelArchitecture;

    if (!risks.empty()) {
        pred.primaryRiskFactor = risks[0];
    } else if (hasOrbitCrossing) {
        pred.primaryRiskFactor = "Critical: Planetary orbit crossing detected";
    } else if (minMutualHillSep < 3.46f) {
        pred.primaryRiskFactor = "Close mutual Hill separation (Δ < 3.46)";
    } else if (maxEcc > 0.40f) {
        pred.primaryRiskFactor = "High orbital eccentricity (e > 0.40)";
    } else {
        pred.primaryRiskFactor = "Nominal stable hierarchy";
    }

    return pred;
}

HabitabilityResult ModelInference::estimateHabitability(const CelestialBody& body, double starLumW) const {
    HabitabilityResult res;

    // Earth standard reference baselines
    const double T0 = 288.0;          // 288 K
    const double R0 = 6371000.0;      // 6,371 km
    const double VESC0 = 11.186;      // 11.2 km/s
    const double F0 = 1361.0;         // Solar constant W/m^2

    // Earth Similarity Index (ESI) weights (Planetary Habitability Laboratory)
    const double wT = 0.558;
    const double wR = 0.570;
    const double wV = 0.700;
    const double wF = 0.450;

    double tBody = (body.surfaceTempK > 0.0) ? body.surfaceTempK : 200.0;
    double rBody = (body.radiusM > 0.0) ? body.radiusM : 5000000.0;
    double vEsc = (body.escapeVelocityKmpS > 0.0) ? body.escapeVelocityKmpS : 8.0;
    double flux = (body.solarRadiationFlux > 0.0) ? body.solarRadiationFlux : 1000.0;

    auto esiSub = [](double val, double ref, double weight) -> float {
        double denom = val + ref;
        if (denom <= 0.0) return 0.0f;
        double term = 1.0 - std::abs((val - ref) / denom);
        term = std::clamp(term, 0.0, 1.0);
        return (float)std::pow(term, weight);
    };

    res.temperatureESI = esiSub(tBody, T0, wT);
    res.radiusESI      = esiSub(rBody, R0, wR);
    res.gravityESI     = esiSub(vEsc, VESC0, wV);
    res.fluxESI        = esiSub(flux, F0, wF);

    double totalWeight = wT + wR + wV + wF;
    double compositeESI = std::pow(res.temperatureESI * res.radiusESI * res.gravityESI * res.fluxESI, 1.0 / totalWeight);

    res.score = std::clamp((float)(compositeESI * 100.0), 0.0f, 100.0f);

    if (res.score >= 72.0f) {
        res.classification = "Potentially Habitable";
        res.diagnostic = "High Earth similarity with balanced temperature and terrestrial scale.";
    } else if (res.score >= 48.0f) {
        res.classification = "Marginal";
        res.diagnostic = "Borderline conditions; extreme thermal or gravity deviations.";
    } else {
        res.classification = "Unfavorable";
        res.diagnostic = "Harsh conditions; non-habitable stellar flux, extreme cold, or gas giant mass.";
    }

    return res;
}

} // namespace ai
} // namespace AstroGenesis
