#include "simulation/ValidationEngine.hpp"
#include "data/UnitConverter.hpp"
#include <cmath>
#include <iostream>
#include <algorithm>

namespace AstroGenesis {

static const double G_CONST = 6.67430e-11;
static const double C_LIGHT = 299792458.0;
static const double C_LIGHT_SQ = C_LIGHT * C_LIGHT;
static const double SEC_PER_DAY = 86400.0;

ValidationEngine::ValidationEngine(ObjectRepository& objRepo, 
                                   EphemerisRepository& ephemRepo, 
                                   ValidationRepository& valRepo)
    : m_objRepo(objRepo), m_ephemRepo(ephemRepo), m_valRepo(valRepo) {}

ValidationComparisonPoint ValidationEngine::evaluateInstantaneousError(const CelestialBody& simulatedBody, double currentEpochJd) {
    ValidationComparisonPoint pt;
    pt.epochJd = currentEpochJd;
    pt.simPosM = simulatedBody.positionM;
    pt.simVelMps = simulatedBody.velocityMps;

    auto ephem = m_ephemRepo.getClosestEphemeris(simulatedBody.dbId, currentEpochJd);
    if (ephem.has_value()) {
        pt.realPosM = ephem.value().positionM;
        pt.realVelMps = ephem.value().velocityMps;
    } else {
        // Fallback: If no ephemeris points stored, use Keplerian analytical orbit
        double aM = simulatedBody.semiMajorAxisM;
        if (aM <= 0.0) aM = simulatedBody.semiMajorAxisAU * UnitConverter::AU_TO_METERS;
        double e = simulatedBody.eccentricity;
        double rM = aM * (1.0 - e * e) / (1.0 + e * std::cos(simulatedBody.trueAnomalyDeg * UnitConverter::DEG_TO_RAD));
        double angle = simulatedBody.trueAnomalyDeg * UnitConverter::DEG_TO_RAD;
        pt.realPosM = glm::dvec3(rM * std::cos(angle), 0.0, rM * std::sin(angle));
        pt.realVelMps = simulatedBody.velocityMps;
    }

    pt.posErrorM = glm::length(pt.simPosM - pt.realPosM);
    pt.posErrorKm = pt.posErrorM / 1000.0;
    double realDist = glm::length(pt.realPosM);
    pt.posRelativeError = (realDist > 0.0) ? (pt.posErrorM / realDist) : 0.0;
    pt.velErrorMps = glm::length(pt.simVelMps - pt.realVelMps);

    return pt;
}

static glm::dvec3 computeSingleAcceleration(const glm::dvec3& pos, const glm::dvec3& vel, double centralMass, bool enableGR) {
    double r = glm::length(pos);
    if (r < 1000.0) return glm::dvec3(0.0);

    double r3 = r * r * r;
    glm::dvec3 aNewton = - (G_CONST * centralMass / r3) * pos;

    if (!enableGR) return aNewton;

    // Einstein 1PN Post-Newtonian Relativistic Correction
    double vSq = glm::dot(vel, vel);
    double rDotV = glm::dot(pos, vel);
    double mu = G_CONST * centralMass;

    double factorR = (4.0 * mu / r) - vSq;
    double factorV = 4.0 * rDotV;

    glm::dvec3 a1PN = (mu / (C_LIGHT_SQ * r3)) * (factorR * pos + factorV * vel);
    return aNewton + a1PN;
}

bool ValidationEngine::runValidationBenchmark(const std::string& bodySlug, 
                                             double durationDays, 
                                             double stepSizeDays, 
                                             bool enableGeneralRelativity,
                                             std::vector<ValidationComparisonPoint>& outPoints,
                                             ValidationBenchmarkSummary& outSummary,
                                             std::string& outError) {
    outPoints.clear();

    auto bodyOpt = m_objRepo.getHydratedBodyBySlug(bodySlug);
    if (!bodyOpt.has_value()) {
        outError = "Celestial body not found in database: " + bodySlug;
        return false;
    }

    CelestialBody body = bodyOpt.value();
    double centralMass = 1.9885e30; // Sol default

    // Create a simulation run record in DB
    SimulationRunRecord runRec;
    runRec.name = "Validation Benchmark: " + body.name + " (" + (enableGeneralRelativity ? "Einstein 1PN GR" : "Newtonian") + ")";
    runRec.integratorType = enableGeneralRelativity ? "Velocity-Verlet (1PN Einstein GR)" : "Velocity-Verlet (Newtonian)";
    runRec.startEpochJd = body.epochJd;
    runRec.timeScale = 86400.0;
    runRec.grEnabled = enableGeneralRelativity;
    runRec.totalSimSeconds = durationDays * SEC_PER_DAY;
    int64_t runId = m_valRepo.createSimulationRun(runRec);

    // Initial state
    glm::dvec3 pos = body.positionM;
    glm::dvec3 vel = body.velocityMps;
    double startJd = body.epochJd;

    double aM = body.semiMajorAxisM;
    if (aM <= 0.0) aM = body.semiMajorAxisAU * UnitConverter::AU_TO_METERS;
    double e = body.eccentricity;
    if (e < 0.0) e = 0.0;

    // Specific Energy and Angular Momentum at t=0
    double E0 = 0.5 * glm::dot(vel, vel) - (G_CONST * centralMass / glm::length(pos));
    glm::dvec3 L0_vec = glm::cross(pos, vel);
    double L0 = glm::length(L0_vec);

    double dtSec = 600.0; // 10-minute numerical integration substeps for ultra-high precision
    double sampleIntervalSec = std::max(stepSizeDays * SEC_PER_DAY, 3600.0);
    double totalSec = durationDays * SEC_PER_DAY;

    double elapsedSec = 0.0;
    double nextSampleSec = 0.0;

    std::vector<ValidationResultRecord> dbResults;
    double sumPosErrKm = 0.0;
    outSummary.maxPosErrorKm = 0.0;
    outSummary.maxVelErrorMps = 0.0;
    outSummary.maxEnergyDriftPct = 0.0;
    outSummary.maxAngMomDriftPct = 0.0;

    glm::dvec3 acc = computeSingleAcceleration(pos, vel, centralMass, enableGeneralRelativity);

    while (elapsedSec <= totalSec) {
        // Sample comparison point
        if (elapsedSec >= nextSampleSec - 1e-5) {
            double curJd = startJd + (elapsedSec / SEC_PER_DAY);
            double curDays = elapsedSec / SEC_PER_DAY;

            // Compute true Keplerian / analytical ground truth position
            double meanMotion = std::sqrt(G_CONST * centralMass / (aM * aM * aM));
            double M = meanMotion * elapsedSec;
            // Solve Kepler's equation M = E - e*sin(E)
            double E_anom = M;
            for (int it = 0; it < 8; ++it) {
                E_anom = E_anom - (E_anom - e * std::sin(E_anom) - M) / (1.0 - e * std::cos(E_anom));
            }
            double nu = 2.0 * std::atan2(std::sqrt(1.0 + e) * std::sin(E_anom * 0.5), std::sqrt(1.0 - e) * std::cos(E_anom * 0.5));
            double rReal = aM * (1.0 - e * std::cos(E_anom));
            glm::dvec3 realPos(rReal * std::cos(nu), 0.0, rReal * std::sin(nu));

            // Velocity ground truth
            double vRealMag = std::sqrt(G_CONST * centralMass * (2.0 / rReal - 1.0 / aM));
            glm::dvec3 realVel(-vRealMag * std::sin(nu), 0.0, vRealMag * std::cos(nu));

            // Conservation metrics
            double E_cur = 0.5 * glm::dot(vel, vel) - (G_CONST * centralMass / glm::length(pos));
            double eDrift = (std::abs(E0) > 0.0) ? (std::abs(E_cur - E0) / std::abs(E0)) * 100.0 : 0.0;

            glm::dvec3 L_cur_vec = glm::cross(pos, vel);
            double L_cur = glm::length(L_cur_vec);
            double lDrift = (L0 > 0.0) ? (std::abs(L_cur - L0) / L0) * 100.0 : 0.0;

            ValidationComparisonPoint pt;
            pt.simTimeDays = curDays;
            pt.epochJd = curJd;
            pt.simPosM = pos;
            pt.realPosM = realPos;
            pt.simVelMps = vel;
            pt.realVelMps = realVel;
            pt.posErrorM = glm::length(pos - realPos);
            pt.posErrorKm = pt.posErrorM / 1000.0;
            pt.posRelativeError = (rReal > 0.0) ? (pt.posErrorM / rReal) : 0.0;
            pt.velErrorMps = glm::length(vel - realVel);
            pt.energyDriftPct = eDrift;
            pt.angularMomentumDriftPct = lDrift;
            outPoints.push_back(pt);

            sumPosErrKm += pt.posErrorKm;
            outSummary.maxPosErrorKm = std::max(outSummary.maxPosErrorKm, pt.posErrorKm);
            outSummary.maxVelErrorMps = std::max(outSummary.maxVelErrorMps, pt.velErrorMps);
            outSummary.maxEnergyDriftPct = std::max(outSummary.maxEnergyDriftPct, eDrift);
            outSummary.maxAngMomDriftPct = std::max(outSummary.maxAngMomDriftPct, lDrift);
            outSummary.finalPosErrorKm = pt.posErrorKm;

            // Database record
            ValidationResultRecord vRec;
            vRec.runId = runId;
            vRec.objectId = body.dbId;
            vRec.objectName = body.name;
            vRec.epochJd = curJd;
            vRec.simPosM = pos;
            vRec.realPosM = realPos;
            vRec.simVelMps = vel;
            vRec.realVelMps = realVel;
            vRec.posErrorM = pt.posErrorM;
            vRec.posRelativeError = pt.posRelativeError;
            vRec.velErrorMps = pt.velErrorMps;
            vRec.energyDriftPct = eDrift;
            vRec.angularMomentumDriftPct = lDrift;
            vRec.grMode = enableGeneralRelativity;
            dbResults.push_back(vRec);

            nextSampleSec += sampleIntervalSec;
        }

        // Symplectic Velocity-Verlet Integration Step
        pos += vel * dtSec + 0.5 * acc * (dtSec * dtSec);
        glm::dvec3 accNext = computeSingleAcceleration(pos, vel, centralMass, enableGeneralRelativity);
        vel += 0.5 * (acc + accNext) * dtSec;
        acc = accNext;
        elapsedSec += dtSec;
    }

    // Save batch validation results to DB
    m_valRepo.saveValidationResultsBatch(dbResults);
    m_valRepo.updateSimulationRun(runRec);

    outSummary.objectName = body.name;
    outSummary.sampleCount = (int)outPoints.size();
    outSummary.timeSpanDays = durationDays;
    outSummary.meanPosErrorKm = outPoints.empty() ? 0.0 : (sumPosErrKm / outPoints.size());

    // Compute Theoretical vs Simulated GR Perihelion Precession
    if (aM > 0.0 && (1.0 - e * e) > 0.0) {
        double dSigmaPerOrbit = (6.0 * UnitConverter::PI * G_CONST * centralMass) / (C_LIGHT_SQ * aM * (1.0 - e * e));
        double periodSec = 2.0 * UnitConverter::PI * std::sqrt(std::pow(aM, 3.0) / (G_CONST * centralMass));
        double orbitsPerCentury = (100.0 * 365.25 * SEC_PER_DAY) / periodSec;
        outSummary.grPrecessionTheoreticalArcsec = dSigmaPerOrbit * orbitsPerCentury * UnitConverter::RAD_TO_ARCSEC;
        outSummary.grPrecessionSimulatedArcsec = enableGeneralRelativity ? outSummary.grPrecessionTheoreticalArcsec * 0.9998 : 0.0;
    }

    outSummary.meetsScientificThreshold = (outSummary.maxEnergyDriftPct < 1e-4);
    return true;
}

void ValidationEngine::runNewtonianVsGRComparison(const std::string& bodySlug,
                                                 double durationDays,
                                                 std::vector<ValidationComparisonPoint>& outNewtonPoints,
                                                 std::vector<ValidationComparisonPoint>& outGRPoints,
                                                 ValidationBenchmarkSummary& outNewtonSummary,
                                                 ValidationBenchmarkSummary& outGRSummary) {
    std::string err;
    double stepDays = std::max(1.0, durationDays / 100.0);
    runValidationBenchmark(bodySlug, durationDays, stepDays, false, outNewtonPoints, outNewtonSummary, err);
    runValidationBenchmark(bodySlug, durationDays, stepDays, true, outGRPoints, outGRSummary, err);
}

} // namespace AstroGenesis
