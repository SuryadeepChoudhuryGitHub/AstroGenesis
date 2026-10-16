#include "simulation/RelativisticPhysics.hpp"
#include <cstdio>
#include <cmath>
#include <iostream>

namespace AstroGenesis {

double RelativisticPhysics::calculateProperTimeRate(
    const glm::dvec3& posM,
    const glm::dvec3& velMps,
    const std::vector<glm::dvec3>& attractorPositionsM,
    const std::vector<double>& attractorMassesKg
) {
    double potentialSum = 0.0;
    size_t count = std::min(attractorPositionsM.size(), attractorMassesKg.size());

    for (size_t i = 0; i < count; ++i) {
        if (attractorMassesKg[i] <= 0.0) continue;
        double distM = glm::distance(posM, attractorPositionsM[i]);
        if (distM < 1.0) distM = 1.0;

        double rs = calculateSchwarzschildRadius(attractorMassesKg[i]);
        if (distM <= rs) {
            // Inside or on event horizon: proper time ceases for static distant observer coordinate time
            return 0.0;
        }

        potentialSum += (2.0 * G_CONST * attractorMassesKg[i]) / (C_LIGHT_SQ * distM);
    }

    double vSq = glm::dot(velMps, velMps);
    double betaSq = vSq / C_LIGHT_SQ;
    if (betaSq >= 1.0) betaSq = 0.999999;

    double metricFactor = 1.0 - potentialSum - betaSq;
    if (metricFactor <= 0.0) return 0.0;

    return std::sqrt(metricFactor);
}

RelativisticPhysics::ClockComparisonResult RelativisticPhysics::compareClocks(
    double tauA,
    double tauB,
    double rateA,
    double rateB,
    double coordinateDeltaT,
    const std::string& nameA,
    const std::string& nameB
) {
    ClockComparisonResult res;
    res.coordinateTimeDeltaSec = coordinateDeltaT;
    res.properTimeA = tauA;
    res.properTimeB = tauB;
    res.clockRateA = rateA;
    res.clockRateB = rateB;
    res.clockRatioAoverB = (rateB > 1e-12) ? (rateA / rateB) : 0.0;
    res.cumulativeTimeDifferenceSec = tauA - tauB;
    res.timeDifferenceMicrosec = res.cumulativeTimeDifferenceSec * 1.0e6;

    char buf[256];
    double dailyDriftMicrosec = (rateA - rateB) * 86400.0 * 1.0e6;
    snprintf(buf, sizeof(buf), "[%s vs %s] Ratio: %.9f | Rel. Drift: %+.3f us/day (Total: %+.4f ms)",
             nameA.c_str(), nameB.c_str(), res.clockRatioAoverB, dailyDriftMicrosec, res.cumulativeTimeDifferenceSec * 1000.0);
    res.description = buf;
    return res;
}

RelativisticPhysics::AccretionDiskState RelativisticPhysics::calculateAccretionDiskProperties(
    double massKg,
    double dimensionlessSpin,
    double accretionRateSolarMassPerYear,
    double outerRadiusMultipleRs
) {
    AccretionDiskState state;
    if (massKg <= 0.0 || accretionRateSolarMassPerYear <= 0.0) {
        state.hasDisk = false;
        return state;
    }

    state.hasDisk = true;
    double rs = calculateSchwarzschildRadius(massKg);
    state.innerRadiusM = calculateISCORadius(massKg, dimensionlessSpin, true);
    state.outerRadiusM = std::max(state.innerRadiusM * 2.0, rs * outerRadiusMultipleRs);

    // Accretion rate in kg/s: 1 M_sun/yr = 1.98847e30 / (365.25 * 86400) kg/s
    static const double SEC_PER_YR = 365.25 * 86400.0;
    state.accretionRateKgPerSec = (accretionRateSolarMassPerYear * SOLAR_MASS_KG) / SEC_PER_YR;

    // Radiative efficiency eta = 1 - sqrt(1 - 2*G*M / (3*c^2 * r_in))
    double efficiencyTerm = (2.0 * G_CONST * massKg) / (3.0 * C_LIGHT_SQ * state.innerRadiusM);
    state.radiativeEfficiency = 1.0 - std::sqrt(std::max(0.01, 1.0 - efficiencyTerm));
    state.radiativeEfficiency = std::clamp(state.radiativeEfficiency, 0.05, 0.42);

    // Total bolometric accretion disk luminosity L = eta * Mdot * c^2
    state.totalLuminosityW = state.radiativeEfficiency * state.accretionRateKgPerSec * C_LIGHT_SQ;

    // Peak temperature occurs at r ~ 1.36 * r_in
    double rPeak = 1.36 * state.innerRadiusM;
    state.peakTemperatureK = calculateDiskTemperatureAtRadius(massKg, state.accretionRateKgPerSec, rPeak, state.innerRadiusM);

    return state;
}

double RelativisticPhysics::calculateDiskTemperatureAtRadius(
    double massKg,
    double accretionRateKgPerSec,
    double rM,
    double innerRadiusM
) {
    if (rM <= innerRadiusM || massKg <= 0.0 || accretionRateKgPerSec <= 0.0) {
        return 0.0;
    }

    // Novikov-Thorne standard thin-disk equation:
    // T(r) = [ (3 * G * M * Mdot) / (8 * pi * sigma_SB * r^3) * (1 - sqrt(r_in / r)) ]^(1/4)
    double r3 = rM * rM * rM;
    double innerBoundaryFactor = 1.0 - std::sqrt(innerRadiusM / rM);
    if (innerBoundaryFactor <= 0.0) return 0.0;

    double numerator = 3.0 * G_CONST * massKg * accretionRateKgPerSec;
    double denom = 8.0 * PI * SIGMA_SB * r3;
    if (denom <= 0.0) return 0.0;

    double t4 = (numerator / denom) * innerBoundaryFactor;
    return std::pow(t4, 0.25);
}

double RelativisticPhysics::calculateDiskOrbitalVelocity(
    double massKg,
    double dimensionlessSpin,
    double rM,
    bool prograde
) {
    if (rM <= 0.0 || massKg <= 0.0) return 0.0;
    double rg = (G_CONST * massKg) / C_LIGHT_SQ;
    double aStar = std::clamp(dimensionlessSpin, -0.99999, 0.99999);
    double s = prograde ? 1.0 : -1.0;

    // Relativistic circular orbit velocity in Kerr equatorial plane:
    // v_phi = sqrt(GM / r) / (1 + s * a* * (rg / r)^(3/2))
    double vKepler = std::sqrt((G_CONST * massKg) / rM);
    double denominator = 1.0 + s * aStar * std::pow(rg / rM, 1.5);
    if (std::abs(denominator) < 1e-6) denominator = 1e-6;

    double v = vKepler / denominator;
    return std::min(v, 0.9999 * SPEED_OF_LIGHT);
}

RelativisticPhysics::GeodesicRayResult RelativisticPhysics::traceNullGeodesicRK4(
    double massKg,
    double initialRadiusM,
    double impactParameterM,
    int maxSteps
) {
    GeodesicRayResult result;
    if (massKg <= 0.0 || initialRadiusM <= 0.0 || impactParameterM <= 0.0) {
        return result;
    }

    double rs = calculateSchwarzschildRadius(massKg);
    double bcrit = calculateCriticalImpactParameter(massKg);
    double b = impactParameterM;

    double u = 1.0 / initialRadiusM;
    // Initial derivative w = du / dphi
    // In Schwarzschild metric: (du/dphi)^2 = 1/b^2 - u^2 * (1 - rs * u)
    double wSq = (1.0 / (b * b)) - (u * u) * (1.0 - rs * u);
    if (wSq < 0.0) {
        // Ray cannot reach this impact parameter from infinity
        result.capturedByHorizon = false;
        result.turningRadiusM = initialRadiusM;
        return result;
    }

    double w = std::sqrt(wSq); // Positive: moving inward (r decreasing, u increasing)
    double phi = 0.0;
    double dPhi = 0.003; // Integration step in radians
    double uHorizon = 1.0 / rs;

    result.trajectoryPoints.reserve(maxSteps / 2);
    result.turningRadiusM = initialRadiusM;

    auto d2u_dphi2 = [rs](double uVal) -> double {
        return 1.5 * rs * uVal * uVal - uVal;
    };

    bool hasTurned = false;
    double maxU = u;

    for (int step = 0; step < maxSteps; ++step) {
        if (step % 4 == 0) {
            double rCurrent = (u > 1e-12) ? (1.0 / u) : initialRadiusM * 2.0;
            result.trajectoryPoints.push_back(glm::dvec2(rCurrent, phi));
        }

        // Check if inside event horizon
        if (u >= uHorizon * 0.999) {
            result.capturedByHorizon = true;
            result.turningRadiusM = rs;
            result.stepCount = step;
            return result;
        }

        maxU = std::max(maxU, u);

        // RK4 step for system:
        // du/dphi = w
        // dw/dphi = 1.5 * rs * u^2 - u
        double k1_u = w;
        double k1_w = d2u_dphi2(u);

        double k2_u = w + 0.5 * dPhi * k1_w;
        double k2_w = d2u_dphi2(u + 0.5 * dPhi * k1_u);

        double k3_u = w + 0.5 * dPhi * k2_w;
        double k3_w = d2u_dphi2(u + 0.5 * dPhi * k2_u);

        double k4_u = w + dPhi * k3_w;
        double k4_w = d2u_dphi2(u + dPhi * k3_u);

        u += (dPhi / 6.0) * (k1_u + 2.0 * k2_u + 2.0 * k3_u + k4_u);
        w += (dPhi / 6.0) * (k1_w + 2.0 * k2_w + 2.0 * k3_w + k4_w);
        phi += dPhi;

        // Detect turning point: w passes through 0 from positive to negative
        if (!hasTurned && w <= 0.0) {
            hasTurned = true;
            w = -std::abs(w);
            result.turningRadiusM = 1.0 / maxU;
        }

        // Ray escaping back to infinity
        if (hasTurned && (u <= 1.0 / (initialRadiusM * 0.8) || u <= 1e-9)) {
            result.capturedByHorizon = false;
            result.turningRadiusM = 1.0 / maxU;
            // Total deflection Delta_phi = phi_total - pi
            result.totalDeflectionAngleRad = phi - PI;
            result.stepCount = step;
            return result;
        }
    }

    result.capturedByHorizon = (b <= bcrit * 1.01);
    result.turningRadiusM = (maxU > 1e-12) ? (1.0 / maxU) : initialRadiusM;
    result.stepCount = maxSteps;
    return result;
}

} // namespace AstroGenesis
