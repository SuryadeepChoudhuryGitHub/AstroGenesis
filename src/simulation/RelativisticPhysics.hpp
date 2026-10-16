#pragma once

#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <glm/glm.hpp>

namespace AstroGenesis {

/**
 * @brief Authoritative Relativistic Physics subsystem for AstroGenesis.
 * 
 * Provides physically rigorous formulations for:
 * 1. Schwarzschild metric geometry, event horizons, photon spheres, shadow radii, and static proper time.
 * 2. Kerr metric geometry, spin parameters, outer/inner horizons, ergospheres, frame dragging, and ISCO.
 * 3. Proper time accumulation along timelike worldlines and observer clock comparison.
 * 4. Gravitational redshift, relativistic Doppler shifts, and relativistic beaming.
 * 5. Novikov-Thorne relativistic accretion disk profiles.
 * 6. Numerical null geodesic integration and photon capture.
 * 7. Black hole tidal forces, Roche disruption radius, and causal horizon accretion.
 */
class RelativisticPhysics {
public:
    // Universal Physical Constants (SI Units)
    static constexpr double G_CONST        = 6.67430e-11;      // Gravitational constant (m^3 kg^-1 s^-2)
    static constexpr double SPEED_OF_LIGHT = 299792458.0;      // Speed of light in vacuum c (m/s)
    static constexpr double C_LIGHT_SQ     = SPEED_OF_LIGHT * SPEED_OF_LIGHT;
    static constexpr double SOLAR_MASS_KG  = 1.98847e30;       // Solar mass M_sun (kg)
    static constexpr double SOLAR_RADIUS_M = 696340000.0;      // Solar radius R_sun (m)
    static constexpr double AU_TO_METERS   = 149597870700.0;   // 1 AU in meters (IAU 2012)
    static constexpr double SIGMA_SB       = 5.670374419e-8;   // Stefan-Boltzmann constant (W m^-2 K^-4)
    static constexpr double PI             = 3.14159265358979323846;

    // ─────────────────────────────────────────────────────────────────────────
    // 1. Schwarzschild Spacetime Geometry (Non-rotating, uncharged black hole)
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Schwarzschild radius r_s = 2GM / c^2.
     * Causal boundary of the event horizon.
     */
    static inline double calculateSchwarzschildRadius(double massKg) {
        if (massKg <= 0.0) return 0.0;
        return (2.0 * G_CONST * massKg) / C_LIGHT_SQ;
    }

    /**
     * @brief Photon sphere radius r_ph = 3GM / c^2 = 1.5 * r_s.
     * Unstable circular orbit for photons.
     */
    static inline double calculatePhotonSphereRadius(double massKg) {
        return 1.5 * calculateSchwarzschildRadius(massKg);
    }

    /**
     * @brief Critical impact parameter b_crit = (3 * sqrt(3) / 2) * r_s.
     * Any photon with impact parameter b < b_crit is captured by the black hole.
     */
    static inline double calculateCriticalImpactParameter(double massKg) {
        // b_crit = 3*sqrt(3)*G*M / c^2 = (3*sqrt(3)/2) * r_s approx 2.598076211 * r_s
        static const double SQRT_27_DIV_2 = 2.598076211353316;
        return SQRT_27_DIV_2 * calculateSchwarzschildRadius(massKg);
    }

    /**
     * @brief Apparent shadow radius seen by an observer at distance D from the black hole.
     * r_shadow = b_crit / sqrt(1 - r_s / D). As D -> infinity, r_shadow -> b_crit.
     */
    static inline double calculateApparentShadowRadius(double massKg, double observerDistanceM) {
        double rs = calculateSchwarzschildRadius(massKg);
        if (observerDistanceM <= rs) {
            return rs; // Inside or on horizon
        }
        double bcrit = calculateCriticalImpactParameter(massKg);
        double denom = std::sqrt(1.0 - (rs / observerDistanceM));
        return (denom > 1e-6) ? (bcrit / denom) : bcrit;
    }

    /**
     * @brief Exact Schwarzschild static proper-time dilation factor dtau / dt = sqrt(1 - r_s / r).
     * Valid for a clock held static at radius r outside the horizon.
     */
    static inline double calculateStaticProperTimeFactor(double massKg, double radiusM) {
        double rs = calculateSchwarzschildRadius(massKg);
        if (radiusM <= rs) return 0.0; // Frozen at horizon in Schwarzschild coordinates
        return std::sqrt(1.0 - (rs / radiusM));
    }

    /**
     * @brief Exact gravitational redshift between static emitter at rEmit and static observer at rObs:
     * nu_obs / nu_emit = sqrt( (1 - r_s / r_emit) / (1 - r_s / r_obs) ).
     */
    static inline double calculateStaticGravitationalRedshift(double massKg, double rEmitM, double rObsM) {
        double rs = calculateSchwarzschildRadius(massKg);
        if (rEmitM <= rs || rObsM <= rs) return 0.0;
        double num = 1.0 - (rs / rEmitM);
        double den = 1.0 - (rs / rObsM);
        if (num <= 0.0 || den <= 0.0) return 0.0;
        return std::sqrt(num / den);
    }

    /**
     * @brief Einstein weak-field gravitational deflection angle for light:
     * Delta_phi = 4GM / (c^2 * b) = 2 r_s / b.
     */
    static inline double calculateWeakFieldDeflection(double massKg, double impactParameterM) {
        if (impactParameterM <= 0.0) return 0.0;
        double rs = calculateSchwarzschildRadius(massKg);
        return (2.0 * rs) / impactParameterM;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 2. Kerr Spacetime Geometry (Rotating black hole)
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Dimensional spin parameter a = J / (M * c) in meters.
     * In terms of dimensionless spin a* in [-1.0, 1.0]: a = a* * GM / c^2 = a* * (r_s / 2).
     */
    static inline double calculateKerrDimensionalSpin(double massKg, double dimensionlessSpin) {
        double aStar = std::clamp(dimensionlessSpin, -1.0, 1.0);
        return aStar * (G_CONST * massKg) / C_LIGHT_SQ;
    }

    /**
     * @brief Kerr outer event horizon radius r_+ = GM/c^2 * (1 + sqrt(1 - a*^2)).
     */
    static inline double calculateKerrOuterHorizon(double massKg, double dimensionlessSpin) {
        double aStar = std::clamp(dimensionlessSpin, -1.0, 1.0);
        double rg = (G_CONST * massKg) / C_LIGHT_SQ;
        return rg * (1.0 + std::sqrt(std::max(0.0, 1.0 - aStar * aStar)));
    }

    /**
     * @brief Kerr inner Cauchy horizon radius r_- = GM/c^2 * (1 - sqrt(1 - a*^2)).
     */
    static inline double calculateKerrInnerHorizon(double massKg, double dimensionlessSpin) {
        double aStar = std::clamp(dimensionlessSpin, -1.0, 1.0);
        double rg = (G_CONST * massKg) / C_LIGHT_SQ;
        return rg * (1.0 - std::sqrt(std::max(0.0, 1.0 - aStar * aStar)));
    }

    /**
     * @brief Kerr ergosphere / static limit surface radius as a function of polar angle theta:
     * r_ergo(theta) = GM/c^2 * (1 + sqrt(1 - a*^2 * cos^2(theta))).
     * Note: At the poles (theta = 0, pi), r_ergo = r_+. At the equator (theta = pi/2), r_ergo = 2 GM/c^2 = r_s.
     */
    static inline double calculateKerrErgosphereRadius(double massKg, double dimensionlessSpin, double thetaRad) {
        double aStar = std::clamp(dimensionlessSpin, -1.0, 1.0);
        double rg = (G_CONST * massKg) / C_LIGHT_SQ;
        double cosT = std::cos(thetaRad);
        return rg * (1.0 + std::sqrt(std::max(0.0, 1.0 - aStar * aStar * cosT * cosT)));
    }

    /**
     * @brief Innermost Stable Circular Orbit (ISCO) radius in Kerr metric:
     * For a* = 0: r_isco = 6 GM/c^2 = 3 r_s.
     * For a* = +1 (prograde): r_isco = 1 GM/c^2 = 0.5 r_s.
     * For a* = -1 (retrograde): r_isco = 9 GM/c^2 = 4.5 r_s.
     */
    static inline double calculateISCORadius(double massKg, double dimensionlessSpin, bool prograde = true) {
        double aStar = std::clamp(dimensionlessSpin, -1.0, 1.0);
        double rg = (G_CONST * massKg) / C_LIGHT_SQ;

        double s = prograde ? 1.0 : -1.0;
        if (aStar < 0.0) {
            // Retrograde spin orientation
            s = -s;
            aStar = std::abs(aStar);
        }

        double term1 = std::cbrt(1.0 + aStar);
        double term2 = std::cbrt(1.0 - aStar);
        double z1 = 1.0 + std::cbrt(1.0 - aStar * aStar) * (term1 + term2);
        double z2 = std::sqrt(3.0 * aStar * aStar + z1 * z1);

        double factor = 3.0 + z2 - s * std::sqrt(std::max(0.0, (3.0 - z1) * (3.0 + z1 + 2.0 * z2)));
        return rg * factor;
    }

    /**
     * @brief Frame dragging angular velocity Omega(r, theta) in Kerr spacetime (rad/s).
     */
    static inline double calculateFrameDraggingRate(double massKg, double dimensionlessSpin, double radiusM, double thetaRad) {
        double aStar = std::clamp(dimensionlessSpin, -0.99999, 0.99999);
        double rg = (G_CONST * massKg) / C_LIGHT_SQ;
        double a = aStar * rg;
        double r = std::max(radiusM, rg * 0.1);
        double r2 = r * r;
        double a2 = a * a;
        double sinT = std::sin(thetaRad);
        double delta = r2 - 2.0 * rg * r + a2;

        double numerator = 2.0 * a * rg * r * SPEED_OF_LIGHT;
        double denom = (r2 + a2) * (r2 + a2) - a2 * delta * sinT * sinT;
        if (std::abs(denom) < 1e-6) return 0.0;
        return numerator / denom;
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 3. Proper Time Accumulation Along Timelike Worldlines
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Evaluates proper-time rate dtau / dt for an object at position posM with velocity velMps
     * in the gravitational field of multiple massive bodies.
     * 
     * Uses the exact 1PN metric interval:
     * (dtau / dt)^2 = 1 - sum_j (2GM_j / (c^2 * r_j)) - (v^2 / c^2)
     * 
     * For a static observer outside a single Schwarzschild black hole, this matches sqrt(1 - r_s / r).
     * For a fast-moving object, kinematic dilation is included consistently without double counting.
     */
    static double calculateProperTimeRate(
        const glm::dvec3& posM,
        const glm::dvec3& velMps,
        const std::vector<glm::dvec3>& attractorPositionsM,
        const std::vector<double>& attractorMassesKg
    );

    /**
     * @brief Compares proper times accumulated along two worldlines over a simulation run.
     */
    struct ClockComparisonResult {
        double coordinateTimeDeltaSec = 0.0;
        double properTimeA = 0.0;
        double properTimeB = 0.0;
        double clockRateA = 1.0;
        double clockRateB = 1.0;
        double clockRatioAoverB = 1.0;
        double cumulativeTimeDifferenceSec = 0.0;
        double timeDifferenceMicrosec = 0.0;
        std::string description;
    };

    static ClockComparisonResult compareClocks(
        double tauA,
        double tauB,
        double rateA,
        double rateB,
        double coordinateDeltaT,
        const std::string& nameA,
        const std::string& nameB
    );

    // ─────────────────────────────────────────────────────────────────────────
    // 4. Relativistic Doppler Shifts & Beaming
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Relativistic Doppler factor delta = 1 / ( gamma * (1 - beta * cos(theta)) ).
     * where beta = v / c, gamma = 1 / sqrt(1 - beta^2),
     * and cos(theta) = dot(v, n) / v with n being the unit vector pointing toward the observer.
     */
    static inline double calculateRelativisticDopplerFactor(const glm::dvec3& velocityMps, const glm::dvec3& toObserverDir) {
        double v = glm::length(velocityMps);
        if (v < 1e-6) return 1.0;
        double beta = std::min(v / SPEED_OF_LIGHT, 0.99999);
        double gamma = 1.0 / std::sqrt(1.0 - beta * beta);
        glm::dvec3 vNorm = velocityMps / v;
        double cosTheta = glm::dot(vNorm, glm::normalize(toObserverDir));
        double denom = gamma * (1.0 - beta * cosTheta);
        if (denom <= 1e-6) return 100.0; // Extremely beamed
        return 1.0 / denom;
    }

    /**
     * @brief Total relativistic frequency shift g = nu_obs / nu_emit
     * combining gravitational redshift and relativistic Doppler shift:
     * g = delta * sqrt(1 - r_s / r_emit) / sqrt(1 - r_s / r_obs).
     */
    static inline double calculateCombinedFrequencyShift(
        double massKg,
        double rEmitM,
        double rObsM,
        const glm::dvec3& emitterVelMps,
        const glm::dvec3& toObserverDir
    ) {
        double doppler = calculateRelativisticDopplerFactor(emitterVelMps, toObserverDir);
        double grav = calculateStaticGravitationalRedshift(massKg, rEmitM, rObsM);
        return doppler * grav;
    }

    /**
     * @brief Relativistic beaming factor for bolometric flux: F_obs = g^4 * F_emit.
     * For monochromatic intensity: I_obs = g^3 * I_emit.
     */
    static inline double calculateBeamingIntensityMultiplier(double totalShiftFactor, bool bolometric = true) {
        double g = std::max(0.0, totalShiftFactor);
        return bolometric ? std::pow(g, 4.0) : std::pow(g, 3.0);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 5. Novikov-Thorne Accretion Disk Physics
    // ─────────────────────────────────────────────────────────────────────────

    struct AccretionDiskState {
        bool hasDisk = false;
        double innerRadiusM = 0.0;        // r_in = r_isco
        double outerRadiusM = 0.0;        // r_out
        double accretionRateKgPerSec = 0.0;// Mdot
        double peakTemperatureK = 0.0;     // T_peak
        double totalLuminosityW = 0.0;    // Radiated power L_disk = eta * Mdot * c^2
        double radiativeEfficiency = 0.057;// eta ~ 0.057 for a*=0, ~0.42 for a*=1
    };

    /**
     * @brief Computes Novikov-Thorne relativistic thin-disk parameters for a black hole.
     */
    static AccretionDiskState calculateAccretionDiskProperties(
        double massKg,
        double dimensionlessSpin,
        double accretionRateSolarMassPerYear,
        double outerRadiusMultipleRs = 40.0
    );

    /**
     * @brief Computes the effective blackbody temperature T(r) across the disk profile:
     * T(r) = [ (3 * G * M * Mdot) / (8 * pi * sigma_SB * r^3) * (1 - sqrt(r_in / r)) ]^(1/4).
     */
    static double calculateDiskTemperatureAtRadius(
        double massKg,
        double accretionRateKgPerSec,
        double rM,
        double innerRadiusM
    );

    /**
     * @brief Keplerian orbital velocity in the accretion disk plane (m/s).
     * Accounts for spin-dependent Kerr circular orbit velocity.
     */
    static double calculateDiskOrbitalVelocity(
        double massKg,
        double dimensionlessSpin,
        double rM,
        bool prograde = true
    );

    /**
     * @brief Accretion radiative efficiency eta(a*) for Kerr thin accretion disk:
     * eta = 1 - sqrt(1 - 2GM / (3 * c^2 * r_isco))
     * ~ 0.0572 for a*=0, ~ 0.19 for a*=0.95, ~ 0.42 for a*=1.0.
     */
    static inline double calculateAccretionEfficiency(double dimensionlessSpin) {
        double rIscoOverRg = calculateISCORadius(1.0, dimensionlessSpin, true) / (G_CONST / C_LIGHT_SQ);
        double term = 2.0 / (3.0 * rIscoOverRg);
        double eta = 1.0 - std::sqrt(std::max(0.001, 1.0 - term));
        return std::clamp(eta, 0.05, 0.425);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 6. Tidal Forces & Roche Disruption
    // ─────────────────────────────────────────────────────────────────────────

    /**
     * @brief Roche Tidal Disruption Radius r_T = R_body * (2 * M_BH / M_body)^(1/3).
     * When an extended body crosses r_T, tidal shear overcomes self-gravity and tensile strength.
     */
    static inline double calculateTidalDisruptionRadius(
        double blackHoleMassKg,
        double bodyMassKg,
        double bodyRadiusM,
        bool fluidBody = true
    ) {
        if (bodyMassKg <= 0.0 || bodyRadiusM <= 0.0) return 0.0;
        double massRatio = blackHoleMassKg / bodyMassKg;
        double coeff = fluidBody ? 2.0 : 1.0;
        return bodyRadiusM * std::cbrt(coeff * massRatio);
    }

    /**
     * @brief Computes the differential tidal acceleration vector Delta_a across an offset vector dr:
     * Delta_a = - (GM / r^3) * [ dr - 3 (r_hat . dr) r_hat ].
     */
    static inline glm::dvec3 calculateTidalAcceleration(
        double blackHoleMassKg,
        const glm::dvec3& relPosM,
        const glm::dvec3& offsetFromCenterM
    ) {
        double r2 = glm::dot(relPosM, relPosM);
        double r = std::sqrt(r2);
        if (r < 1e-3) return glm::dvec3(0.0);
        glm::dvec3 rHat = relPosM / r;
        double coeff = (G_CONST * blackHoleMassKg) / (r2 * r);
        return -coeff * (offsetFromCenterM - 3.0 * glm::dot(rHat, offsetFromCenterM) * rHat);
    }

    // ─────────────────────────────────────────────────────────────────────────
    // 7. Numerical Geodesic Integration (C++ Ray Tracer & Analytical Baseline)
    // ─────────────────────────────────────────────────────────────────────────

    struct GeodesicRayResult {
        bool capturedByHorizon = false;
        double turningRadiusM = 0.0;
        double totalDeflectionAngleRad = 0.0;
        int stepCount = 0;
        std::vector<glm::dvec2> trajectoryPoints; // (r, phi) coordinates in orbit plane
    };

    /**
     * @brief Numerically integrates a null geodesic in Schwarzschild spacetime
     * using 4th-order Runge-Kutta on the equation: d^2 u / dphi^2 + u = (3/2) * r_s * u^2,
     * where u = 1 / r.
     * 
     * @param massKg Black hole mass
     * @param initialRadiusM Starting distance (e.g. 50 * r_s)
     * @param impactParameterM Photon impact parameter b
     * @param maxSteps Maximum integration steps
     * @return GeodesicRayResult Containing capture flag, deflection angle, and trajectory points.
     */
    static GeodesicRayResult traceNullGeodesicRK4(
        double massKg,
        double initialRadiusM,
        double impactParameterM,
        int maxSteps = 4000
    );
};

} // namespace AstroGenesis
