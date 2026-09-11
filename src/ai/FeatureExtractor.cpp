#include "ai/FeatureExtractor.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "data/UnitConverter.hpp"

#include <cmath>
#include <algorithm>
#include <iostream>

namespace AstroGenesis {
namespace ai {

static const double SOLAR_MASS_KG = 1.9885e30;
static const double G_CONST       = 6.67430e-11;

ExtractedFeatures FeatureExtractor::extract(const PhysicsEngine& physics) {
    return extractFromBodies(physics.getBodies(), physics.getEnergyConservationDriftPct());
}

ExtractedFeatures FeatureExtractor::extractFromBodies(const std::vector<CelestialBody>& bodies, double energyDriftPct) {
    ExtractedFeatures feat;
    feat.featureNames = {
        "body_count",
        "star_mass_solar",
        "total_mass_ratio",
        "max_mass_ratio",
        "min_mutual_hill_sep",
        "mean_mutual_hill_sep",
        "max_eccentricity",
        "mean_eccentricity",
        "orbit_crossing_flag",
        "min_period_ratio",
        "angular_momentum_deficit",
        "energy_drift_pct"
    };

    if (bodies.empty()) {
        feat.values.assign(12, 0.0f);
        return feat;
    }

    // 1. Identify primary star / central attractor (Sol or highest mass star)
    size_t starIdx = 0;
    double maxMass = 0.0;
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (bodies[i].id == "sol" || bodies[i].type.find("Star") != std::string::npos) {
            starIdx = i;
            break;
        }
        if (bodies[i].massKg > maxMass) {
            maxMass = bodies[i].massKg;
            starIdx = i;
        }
    }

    const auto& star = bodies[starIdx];
    feat.starMassKg = (star.massKg > 0.0) ? star.massKg : SOLAR_MASS_KG;
    float starMassSolar = (float)(feat.starMassKg / SOLAR_MASS_KG);

    // 2. Collect primary orbiting bodies (planets and major bodies orbiting the central star)
    std::vector<const CelestialBody*> planets;
    for (size_t i = 0; i < bodies.size(); ++i) {
        if (i == starIdx) continue;
        const auto& b = bodies[i];
        
        // Skip synthetic particles or satellites whose parent is not the central star
        if (b.isSynthetic) continue;
        if (b.parentObjectId.has_value() && star.dbId > 0 && b.parentObjectId.value() != star.dbId) {
            // This is a moon orbiting another planet
            continue;
        }
        bool isMoon = (b.type.find("Moon") != std::string::npos || 
                       b.type.find("Satellite") != std::string::npos ||
                       b.id == "moon" || b.id == "ganymede" || b.id == "europa" || 
                       b.id == "io" || b.id == "callisto" || b.id == "titan" ||
                       b.id == "phobos" || b.id == "deimos" || b.id == "enceladus" || 
                       b.id == "triton" || b.id == "charon");
        if (isMoon) continue;

        planets.push_back(&b);
    }

    // If the system contains multiple primary planets (e.g. Mercury - Neptune),
    // exclude dwarf planets/asteroids (e.g. Pluto, Ceres) which belong to minor body belts
    int primaryPlanetCount = 0;
    for (const auto* p : planets) {
        if (p->type.find("Planet") != std::string::npos && p->type.find("Dwarf") == std::string::npos) {
            primaryPlanetCount++;
        }
    }

    if (primaryPlanetCount >= 2) {
        std::vector<const CelestialBody*> filtered;
        for (const auto* p : planets) {
            bool isMinor = (p->type.find("Dwarf") != std::string::npos || 
                            p->type.find("Asteroid") != std::string::npos ||
                            p->id == "pluto" || p->id == "ceres");
            if (!isMinor) {
                filtered.push_back(p);
            }
        }
        if (filtered.size() >= 2) {
            planets = std::move(filtered);
        }
    }

    feat.bodyCount = (int)planets.size();

    // Sort planets radially by semi-major axis (or instantaneous distance if SMA is 0)
    std::sort(planets.begin(), planets.end(), [](const CelestialBody* a, const CelestialBody* b) {
        double rA = (a->semiMajorAxisAU > 0.0) ? a->semiMajorAxisAU : a->distanceAU;
        double rB = (b->semiMajorAxisAU > 0.0) ? b->semiMajorAxisAU : b->distanceAU;
        return rA < rB;
    });

    for (const auto* p : planets) {
        feat.bodyEccentricities.push_back({ p->name, (float)p->eccentricity });
    }

    if (planets.empty()) {
        feat.values = { 0.0f, starMassSolar, 0.0f, 0.0f, 50.0f, 50.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, (float)energyDriftPct };
        return feat;
    }

    if (planets.size() == 1) {
        const auto* p = planets[0];
        float mRatio = (float)(p->massKg / feat.starMassKg);
        float ecc = (float)std::clamp(p->eccentricity, 0.0, 0.99);
        feat.minMutualHillSep = 50.0f;
        feat.meanMutualHillSep = 50.0f;
        feat.maxEccentricity = ecc;
        feat.meanEccentricity = ecc;
        feat.minPeriodRatio = 10.0f;
        feat.angularMomentumDeficit = (float)(1.0 - std::sqrt(std::max(0.0, 1.0 - ecc * ecc)));
        feat.energyDriftPct = (float)energyDriftPct;

        feat.values = {
            1.0f, starMassSolar, mRatio, mRatio,
            50.0f, 50.0f, ecc, ecc, 0.0f, 10.0f,
            feat.angularMomentumDeficit, feat.energyDriftPct
        };
        return feat;
    }

    // Multi-planet metrics
    double totalPlanetMass = 0.0;
    double maxPlanetMass = 0.0;
    double sumEcc = 0.0;
    double maxEcc = 0.0;

    for (const auto* p : planets) {
        totalPlanetMass += p->massKg;
        if (p->massKg > maxPlanetMass) maxPlanetMass = p->massKg;
        sumEcc += p->eccentricity;
        if (p->eccentricity > maxEcc) maxEcc = p->eccentricity;

        if (p->eccentricity > 0.40) {
            char buf[128];
            snprintf(buf, sizeof(buf), "High eccentricity on %s (e = %.3f)", p->name.c_str(), p->eccentricity);
            feat.identifiedRisks.push_back(buf);
        }
    }

    feat.totalPlanetMassRatio = totalPlanetMass / feat.starMassKg;
    feat.maxMassRatio = maxPlanetMass / feat.starMassKg;
    feat.maxEccentricity = (float)maxEcc;
    feat.meanEccentricity = (float)(sumEcc / planets.size());

    // Compute adjacent pair metrics: Hill separation, period ratios, orbit crossings
    std::vector<float> deltas;
    std::vector<float> pRatios;
    bool hasCrossing = false;

    for (size_t i = 0; i < planets.size() - 1; ++i) {
        const auto* pInner = planets[i];
        const auto* pOuter = planets[i + 1];

        double aInnerAU = (pInner->semiMajorAxisAU > 0.0) ? pInner->semiMajorAxisAU : pInner->distanceAU;
        double aOuterAU = (pOuter->semiMajorAxisAU > 0.0) ? pOuter->semiMajorAxisAU : pOuter->distanceAU;
        if (aInnerAU <= 0.0) aInnerAU = 0.01;
        if (aOuterAU <= aInnerAU) aOuterAU = aInnerAU * 1.05;

        double mSum = pInner->massKg + pOuter->massKg;
        double aMeanAU = 0.5 * (aInnerAU + aOuterAU);
        double rHillAU = std::cbrt(mSum / (3.0 * feat.starMassKg)) * aMeanAU;

        float deltaHill = (rHillAU > 1e-9) ? (float)((aOuterAU - aInnerAU) / rHillAU) : 50.0f;
        deltas.push_back(deltaHill);

        float pRatio = (float)std::pow(aOuterAU / aInnerAU, 1.5);
        pRatios.push_back(pRatio);

        double rApoInner = aInnerAU * (1.0 + pInner->eccentricity);
        double rPeriOuter = aOuterAU * (1.0 - pOuter->eccentricity);
        bool crossing = (rApoInner >= rPeriOuter);
        if (crossing) hasCrossing = true;

        bool hillUnstable = (deltaHill < 3.46f); // Gladman Hill stability criterion (2 * sqrt(3))

        PlanetPairMetric pair;
        pair.innerName = pInner->name;
        pair.outerName = pOuter->name;
        pair.semiMajorInnerAU = (float)aInnerAU;
        pair.semiMajorOuterAU = (float)aOuterAU;
        pair.deltaHill = deltaHill;
        pair.periodRatio = pRatio;
        pair.isHillUnstable = hillUnstable;
        pair.isOrbitCrossing = crossing;
        feat.pairMetrics.push_back(pair);

        if (crossing) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Orbit crossing between %s (apo: %.2f AU) and %s (peri: %.2f AU)",
                     pInner->name.c_str(), rApoInner, pOuter->name.c_str(), rPeriOuter);
            feat.identifiedRisks.push_back(buf);
        } else if (hillUnstable) {
            char buf[128];
            snprintf(buf, sizeof(buf), "Close Hill spacing between %s and %s (Δ = %.2f < 3.46)",
                     pInner->name.c_str(), pOuter->name.c_str(), deltaHill);
            feat.identifiedRisks.push_back(buf);
        }
    }

    feat.minMutualHillSep = deltas.empty() ? 50.0f : *std::min_element(deltas.begin(), deltas.end());
    float sumDeltas = 0.0f;
    for (float d : deltas) sumDeltas += d;
    feat.meanMutualHillSep = deltas.empty() ? 50.0f : (sumDeltas / deltas.size());

    feat.minPeriodRatio = pRatios.empty() ? 1.0f : *std::min_element(pRatios.begin(), pRatios.end());
    feat.hasOrbitCrossing = hasCrossing;
    feat.angularMomentumDeficit = (float)computeAMD(planets, feat.starMassKg);
    feat.energyDriftPct = (float)energyDriftPct;

    if (feat.angularMomentumDeficit > 0.035f) {
        char buf[128];
        snprintf(buf, sizeof(buf), "High Angular Momentum Deficit (AMD = %.4f)", feat.angularMomentumDeficit);
        feat.identifiedRisks.push_back(buf);
    }

    // Populate the 12-feature vector for the ML model
    feat.values = {
        (float)feat.bodyCount,
        starMassSolar,
        (float)feat.totalPlanetMassRatio,
        (float)feat.maxMassRatio,
        feat.minMutualHillSep,
        feat.meanMutualHillSep,
        feat.maxEccentricity,
        feat.meanEccentricity,
        hasCrossing ? 1.0f : 0.0f,
        feat.minPeriodRatio,
        feat.angularMomentumDeficit,
        feat.energyDriftPct
    };

    return feat;
}

double FeatureExtractor::computeAMD(const std::vector<const CelestialBody*>& planets, double starMassKg) {
    double num = 0.0;
    double den = 0.0;
    for (const auto* p : planets) {
        double aM = (p->semiMajorAxisM > 0.0) ? p->semiMajorAxisM : (p->semiMajorAxisAU * UnitConverter::AU_TO_METERS);
        if (aM <= 0.0) aM = 1.0 * UnitConverter::AU_TO_METERS;
        double e = std::clamp(p->eccentricity, 0.0, 0.999);

        double lambdaCirc = p->massKg * std::sqrt(G_CONST * starMassKg * aM);
        num += lambdaCirc * (1.0 - std::sqrt(std::max(0.0, 1.0 - e * e)));
        den += lambdaCirc;
    }
    return (den > 0.0) ? (num / den) : 0.0;
}

} // namespace ai
} // namespace AstroGenesis
