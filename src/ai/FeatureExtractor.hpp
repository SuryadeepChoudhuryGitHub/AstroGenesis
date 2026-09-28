#pragma once

#include <vector>
#include <string>
#include <utility>
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

class PhysicsEngine;

namespace ai {

struct PlanetPairMetric {
    std::string innerName;
    std::string outerName;
    float semiMajorInnerAU = 0.0f;
    float semiMajorOuterAU = 0.0f;
    float deltaHill = 0.0f;          // Separation in mutual Hill radii
    float periodRatio = 0.0f;        // P_outer / P_inner
    bool isHillUnstable = false;     // deltaHill < 3.46 (Gladman criterion)
    bool isOrbitCrossing = false;    // r_apo_inner >= r_peri_outer
};

struct ExtractedFeatures {
    std::vector<float> values; // 12 features matching model input order
    std::vector<std::string> featureNames;

    int bodyCount = 0;
    double starMassKg = 0.0;
    double totalPlanetMassRatio = 0.0;
    double maxMassRatio = 0.0;
    float minMutualHillSep = 0.0f;
    float meanMutualHillSep = 0.0f;
    float maxEccentricity = 0.0f;
    float meanEccentricity = 0.0f;
    bool hasOrbitCrossing = false;
    float minPeriodRatio = 0.0f;
    float angularMomentumDeficit = 0.0f;
    float energyDriftPct = 0.0f;

    std::vector<PlanetPairMetric> pairMetrics;
    std::vector<std::pair<std::string, float>> bodyEccentricities;
    std::vector<std::string> identifiedRisks;
};

class FeatureExtractor {
public:
    static ExtractedFeatures extract(const PhysicsEngine& physics);
    static ExtractedFeatures extractFromBodies(const std::vector<CelestialBody>& bodies, double energyDriftPct = 0.0);

private:
    static double computeAMD(const std::vector<const CelestialBody*>& planets, double starMassKg);
};

} // namespace ai
} // namespace AstroGenesis
