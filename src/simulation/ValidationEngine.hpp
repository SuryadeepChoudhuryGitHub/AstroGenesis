#pragma once

#include <vector>
#include <string>
#include <memory>
#include "data/AstronomicalModels.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/repositories/EphemerisRepository.hpp"
#include "data/repositories/ValidationRepository.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

struct ValidationComparisonPoint {
    double simTimeDays = 0.0;
    double epochJd = 0.0;
    glm::dvec3 simPosM{0.0};
    glm::dvec3 realPosM{0.0};
    glm::dvec3 simVelMps{0.0};
    glm::dvec3 realVelMps{0.0};
    double posErrorM = 0.0;
    double posErrorKm = 0.0;
    double posRelativeError = 0.0;
    double velErrorMps = 0.0;
    double energyDriftPct = 0.0;
    double angularMomentumDriftPct = 0.0;
};

struct ValidationBenchmarkSummary {
    std::string objectName;
    int sampleCount = 0;
    double timeSpanDays = 0.0;
    double maxPosErrorKm = 0.0;
    double meanPosErrorKm = 0.0;
    double finalPosErrorKm = 0.0;
    double maxVelErrorMps = 0.0;
    double maxEnergyDriftPct = 0.0;
    double maxAngMomDriftPct = 0.0;
    double grPrecessionTheoreticalArcsec = 0.0;
    double grPrecessionSimulatedArcsec = 0.0;
    bool meetsScientificThreshold = true;
};

class ValidationEngine {
public:
    ValidationEngine(ObjectRepository& objRepo, 
                     EphemerisRepository& ephemRepo, 
                     ValidationRepository& valRepo);

    // Evaluate live body position against stored ephemeris ground truth
    ValidationComparisonPoint evaluateInstantaneousError(const CelestialBody& simulatedBody, double currentEpochJd);

    // Run full time-series benchmark simulation comparison for a body
    bool runValidationBenchmark(const std::string& bodySlug, 
                                double durationDays, 
                                double stepSizeDays, 
                                bool enableGeneralRelativity,
                                std::vector<ValidationComparisonPoint>& outPoints,
                                ValidationBenchmarkSummary& outSummary,
                                std::string& outError);

    // Compare Newtonian Gravity vs Einstein 1PN General Relativity for Mercury/Planets
    void runNewtonianVsGRComparison(const std::string& bodySlug,
                                  double durationDays,
                                  std::vector<ValidationComparisonPoint>& outNewtonPoints,
                                  std::vector<ValidationComparisonPoint>& outGRPoints,
                                  ValidationBenchmarkSummary& outNewtonSummary,
                                  ValidationBenchmarkSummary& outGRSummary);

private:
    ObjectRepository& m_objRepo;
    EphemerisRepository& m_ephemRepo;
    ValidationRepository& m_valRepo;
};

} // namespace AstroGenesis
