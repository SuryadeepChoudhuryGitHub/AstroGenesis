#include <iostream>
#include <cassert>
#include <cmath>
#include <chrono>

#include "data/DatabaseManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/SeedData.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "ai/FeatureExtractor.hpp"
#include "ai/ModelInference.hpp"
#include "ai/OrbitalStabilityPredictor.hpp"
#include "ai/AIManager.hpp"

using namespace AstroGenesis;
using namespace AstroGenesis::ai;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " AstroGenesis AI Subsystem & ML Stability Predictor Tests" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Initialize Database & Physics Engine
    DatabaseManager& db = DatabaseManager::getInstance();
    assert(db.initialize("data/astrogenesis.db") && "DB init failed");
    ObjectRepository repo(db);
    SeedData::seedDefaultDatabase(repo);

    PhysicsEngine physics;
    bool loaded = physics.loadFromDatabase(repo, "Solar System");
    assert(loaded && "Solar System load failed");
    std::cout << "[Test 1] Database & Solar System physics initialized -> PASS ("
              << physics.getBodies().size() << " bodies loaded)" << std::endl;

    // 2. Feature Extractor Verification
    ExtractedFeatures feat = FeatureExtractor::extract(physics);
    std::cout << "[Test 2] FeatureExtractor::extract -> PASS" << std::endl;
    std::cout << "  - Extracted feature count: " << feat.values.size() << " (expected 12)" << std::endl;
    assert(feat.values.size() == 12);
    assert(feat.bodyCount >= 8 && "Expected at least 8 planets/dwarfs in Solar System");
    assert(feat.starMassKg > 1.9e30 && feat.starMassKg < 2.1e30);
    assert(feat.minMutualHillSep > 5.0f && "Solar System should have wide Hill separations");
    std::cout << "  - Solar System hasOrbitCrossing: " << (feat.hasOrbitCrossing ? "TRUE" : "FALSE") << std::endl;
    for (const auto& pair : feat.pairMetrics) {
        std::cout << "    Pair: " << pair.innerName << " - " << pair.outerName
                  << " | a_in=" << pair.semiMajorInnerAU << " a_out=" << pair.semiMajorOuterAU
                  << " | delta=" << pair.deltaHill << " | crossing=" << (pair.isOrbitCrossing ? "YES" : "NO") << std::endl;
    }
    assert(feat.angularMomentumDeficit < 0.05f && "Solar System AMD should be low");

    for (size_t i = 0; i < feat.values.size(); ++i) {
        assert(!std::isnan(feat.values[i]) && !std::isinf(feat.values[i]) && "Feature value is NaN or Inf");
    }

    // 3. Model Loading Verification
    ModelInference inference;
    bool modelLoaded = inference.loadModel("assets/models/orbital_stability_model.json");
    std::cout << "[Test 3] ModelInference::loadModel -> " << (modelLoaded ? "PASS" : "FAIL") << std::endl;
    assert(modelLoaded && "Model failed to load from assets/models/orbital_stability_model.json");
    std::cout << "  - Architecture: " << inference.getModelArchitecture() << std::endl;
    std::cout << "  - Trees loaded: " << inference.getTreeCount() << std::endl;
    std::cout << "  - Training Accuracy: " << (inference.getTrainingAccuracy() * 100.0f) << "%" << std::endl;
    std::cout << "  - Training F1: " << inference.getTrainingF1() << std::endl;
    assert(inference.getTreeCount() >= 40);

    // 4. Stable System Prediction (Solar System)
    StabilityPrediction solarPred = inference.predictStability(
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
    std::cout << "[Test 4] Stable System Inference (Solar System) -> PASS" << std::endl;
    std::cout << "  - Prediction: " << solarPred.prediction << std::endl;
    std::cout << "  - Stable Probability: " << (solarPred.stableProbability * 100.0f) << "%" << std::endl;
    std::cout << "  - Unstable Probability: " << (solarPred.unstableProbability * 100.0f) << "%" << std::endl;
    std::cout << "  - Confidence: " << solarPred.confidence << std::endl;
    std::cout << "  - Inference Time: " << solarPred.inferenceTimeUs << " µs" << std::endl;
    assert(solarPred.prediction == "STABLE" && "Solar System must be predicted STABLE");
    assert(solarPred.stableProbability >= 0.60f);
    assert(solarPred.unstableProbability <= 0.40f);
    assert(std::abs((solarPred.stableProbability + solarPred.unstableProbability) - 1.0f) < 1e-4f);

    // 5. Unstable System Prediction (Artificially Crossing Orbits & Extreme Eccentricity)
    std::vector<CelestialBody> chaoticBodies = physics.getBodies();
    // Perturb Earth to have high eccentricity crossing Venus and Mars
    for (auto& b : chaoticBodies) {
        if (b.id == "earth" || b.name == "Earth") {
            b.eccentricity = 0.65;
            b.semiMajorAxisAU = 1.0;
        }
    }
    ExtractedFeatures chaoticFeat = FeatureExtractor::extractFromBodies(chaoticBodies);
    StabilityPrediction chaoticPred = inference.predictStability(
        chaoticFeat.values,
        chaoticFeat.bodyCount,
        chaoticFeat.minMutualHillSep,
        chaoticFeat.meanMutualHillSep,
        chaoticFeat.maxEccentricity,
        chaoticFeat.meanEccentricity,
        chaoticFeat.minPeriodRatio,
        chaoticFeat.angularMomentumDeficit,
        chaoticFeat.hasOrbitCrossing,
        chaoticFeat.identifiedRisks
    );
    std::cout << "[Test 5] Unstable System Inference (Perturbed Orbit-Crossing System) -> PASS" << std::endl;
    std::cout << "  - Prediction: " << chaoticPred.prediction << std::endl;
    std::cout << "  - Stable Probability: " << (chaoticPred.stableProbability * 100.0f) << "%" << std::endl;
    std::cout << "  - Unstable Probability: " << (chaoticPred.unstableProbability * 100.0f) << "%" << std::endl;
    std::cout << "  - Primary Risk: " << chaoticPred.primaryRiskFactor << std::endl;
    assert(chaoticPred.prediction == "UNSTABLE" && "Chaotic crossing orbits must be predicted UNSTABLE");
    assert(chaoticPred.unstableProbability >= 0.60f);

    // 6. Graceful Handling of Missing Model File
    ModelInference missingModel;
    bool missingLoaded = missingModel.loadModel("assets/models/non_existent_file.json");
    assert(!missingLoaded && "Missing file should return false");
    StabilityPrediction missingPred = missingModel.predictStability(feat.values, 8, 10.0f, 15.0f, 0.05f, 0.03f, 1.5f, 0.01f, false, {});
    std::cout << "[Test 6] Graceful Missing Model Fallback -> PASS" << std::endl;
    std::cout << "  - Handled status: " << missingPred.prediction << " (" << missingPred.modelStatusStr << ")" << std::endl;
    assert(missingPred.prediction == "UNAVAILABLE");
    assert(!missingPred.modelLoaded);

    // 7. AIManager Integration Test
    AIManager aiManager;
    bool aiInit = aiManager.initialize("assets/models/orbital_stability_model.json");
    assert(aiInit && "AIManager failed to initialize");
    aiManager.forceRecompute(physics);
    const auto& mgrPred = aiManager.getCurrentPrediction();
    assert(mgrPred.prediction == "STABLE");
    std::cout << "[Test 7] AIManager High-Level Subsystem Integration -> PASS" << std::endl;

    // 8. Habitability Estimator Verification (Step 10)
    for (const auto& b : physics.getBodies()) {
        if (b.id == "earth" || b.name == "Earth") {
            auto earthHab = inference.estimateHabitability(b, 3.828e26);
            std::cout << "[Test 8] Earth Habitability Estimation -> PASS" << std::endl;
            std::cout << "  - Earth Score: " << earthHab.score << "/100 (" << earthHab.classification << ")" << std::endl;
            std::cout << "  - Temp ESI: " << earthHab.temperatureESI << " | Radius ESI: " << earthHab.radiusESI << std::endl;
            assert(earthHab.score >= 85.0f && "Earth must have high habitability score");
            assert(earthHab.classification == "Potentially Habitable");
        }
    }

    // 9. Inference Latency Benchmark
    const int BENCHMARK_ITERS = 1000;
    auto tStart = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < BENCHMARK_ITERS; ++i) {
        inference.predictStability(feat.values, 8, 10.0f, 15.0f, 0.05f, 0.03f, 1.5f, 0.01f, false, {});
    }
    auto tEnd = std::chrono::high_resolution_clock::now();
    double totalUs = std::chrono::duration<double, std::micro>(tEnd - tStart).count();
    double avgUs = totalUs / BENCHMARK_ITERS;
    std::cout << "[Test 9] Inference Speed Benchmark -> PASS" << std::endl;
    std::cout << "  - Average Inference Latency: " << avgUs << " µs per evaluation across "
              << BENCHMARK_ITERS << " iterations" << std::endl;
    assert(avgUs < 500.0 && "Inference should easily run in under 500 microseconds");

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " ALL 9 AI SUBSYSTEM TEST SUITES PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
