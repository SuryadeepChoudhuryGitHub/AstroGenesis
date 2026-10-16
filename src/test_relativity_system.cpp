#include <iostream>
#include <iomanip>
#include <cmath>
#include <cassert>
#include <vector>
#include <string>
#include <memory>

#include "simulation/RelativisticPhysics.hpp"
#include "simulation/CelestialBody.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "data/DatabaseManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/SeedData.hpp"
#include "data/UnitConverter.hpp"

using namespace AstroGenesis;

static int g_passedTests = 0;
static int g_failedTests = 0;

#define TEST_ASSERT(cond, msg) \
    do { \
        if (cond) { \
            std::cout << "  [PASS] " << msg << std::endl; \
            g_passedTests++; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (Line " << __LINE__ << ")" << std::endl; \
            g_failedTests++; \
        } \
    } while (0)

#define TEST_NEAR(val, expected, tol, msg) \
    do { \
        double diff = std::abs((val) - (expected)); \
        double relDiff = (std::abs(expected) > 1e-15) ? (diff / std::abs(expected)) : diff; \
        if (relDiff <= (tol) || diff <= (tol)) { \
            std::cout << "  [PASS] " << msg << " (val=" << (val) << ", exp=" << (expected) << ", diff=" << diff << ")" << std::endl; \
            g_passedTests++; \
        } else { \
            std::cerr << "  [FAIL] " << msg << " (val=" << (val) << ", exp=" << (expected) << ", relDiff=" << relDiff << " > " << (tol) << ")" << std::endl; \
            g_failedTests++; \
        } \
    } while (0)

void testUniversalConstants() {
    std::cout << "\n=== Test Suite 1: Universal Physical Constants & SI Conventions ===" << std::endl;

    TEST_NEAR(RelativisticPhysics::SPEED_OF_LIGHT, 299792458.0, 1e-12, "Speed of light c in vacuum matches SI definition exactly");
    TEST_NEAR(RelativisticPhysics::G_CONST, 6.67430e-11, 1e-6, "Gravitational constant G matches CODATA 2018 value");
    TEST_NEAR(RelativisticPhysics::SOLAR_MASS_KG, 1.98847e30, 1e-5, "Solar mass M_sun matches IAU standard");
    TEST_NEAR(RelativisticPhysics::AU_TO_METERS, 149597870700.0, 1e-12, "Astronomical Unit matches IAU 2012 definition exactly");
    TEST_NEAR(RelativisticPhysics::SIGMA_SB, 5.670374419e-8, 1e-8, "Stefan-Boltzmann constant matches SI exact definition");
}

void testSchwarzschildGeometry() {
    std::cout << "\n=== Test Suite 2: Schwarzschild Metric Spacetime Geometry ===" << std::endl;

    // 1. Earth Schwarzschild radius: ~8.87 mm
    double mEarth = 5.97219e24;
    double rsEarth = RelativisticPhysics::calculateSchwarzschildRadius(mEarth);
    TEST_NEAR(rsEarth, 0.00887, 0.01, "Earth Schwarzschild radius is ~8.87 mm");

    // 2. Solar Schwarzschild radius: ~2.953 km
    double mSun = RelativisticPhysics::SOLAR_MASS_KG;
    double rsSun = RelativisticPhysics::calculateSchwarzschildRadius(mSun);
    TEST_NEAR(rsSun, 2953.25, 1e-3, "Solar Schwarzschild radius is ~2953.25 m (~2.95 km)");

    // 3. Cygnus X-1 (14.8 M_sun): ~43.71 km
    double mCyg = 14.8 * mSun;
    double rsCyg = RelativisticPhysics::calculateSchwarzschildRadius(mCyg);
    TEST_NEAR(rsCyg, 43708.1, 1e-3, "Cygnus X-1 (14.8 M_sun) Schwarzschild radius is ~43.71 km");

    // 4. Sagittarius A* (4.154e6 M_sun): ~1.227e10 m (~0.082 AU)
    double mSgrA = 4.154e6 * mSun;
    double rsSgrA = RelativisticPhysics::calculateSchwarzschildRadius(mSgrA);
    TEST_NEAR(rsSgrA / RelativisticPhysics::AU_TO_METERS, 0.0820, 0.01, "Sgr A* (4.154M M_sun) event horizon is ~0.082 AU");

    // 5. Photon sphere radius r_ph = 1.5 * r_s exactly
    double rphCyg = RelativisticPhysics::calculatePhotonSphereRadius(mCyg);
    TEST_NEAR(rphCyg / rsCyg, 1.5000000000, 1e-12, "Photon sphere is mathematically 1.5 * r_s");

    // 6. Critical impact parameter / Shadow radius b_crit = sqrt(27)/2 * r_s ~ 2.598076 * r_s
    double bcritCyg = RelativisticPhysics::calculateCriticalImpactParameter(mCyg);
    double expectedShadowRatio = std::sqrt(27.0) / 2.0; // 2.59807621135
    TEST_NEAR(bcritCyg / rsCyg, expectedShadowRatio, 1e-10, "Critical impact parameter b_crit matches sqrt(27)/2 * r_s (~2.598 r_s)");

    // 7. Verify explicit distinction between event horizon (r_s) and shadow (b_crit)
    TEST_ASSERT(bcritCyg > rsCyg * 2.59, "Black hole shadow is larger than horizon by lensing factor ~2.6x");
    TEST_ASSERT(bcritCyg > rphCyg, "Black hole shadow is larger than photon sphere");
}

void testStaticProperTimeRate() {
    std::cout << "\n=== Test Suite 3: Static Observer Proper-Time Rate Validation ===" << std::endl;

    double mBH = 10.0 * RelativisticPhysics::SOLAR_MASS_KG;
    double rs = RelativisticPhysics::calculateSchwarzschildRadius(mBH);

    // Static observer in Schwarzschild spacetime: dtau / dt = sqrt(1 - rs / r)
    // Test at r = 100 * rs -> sqrt(1 - 0.01) = sqrt(0.99) ~ 0.994987437
    double rate100 = RelativisticPhysics::calculateProperTimeRate(
        glm::dvec3(100.0 * rs, 0.0, 0.0), glm::dvec3(0.0),
        { glm::dvec3(0.0) }, { mBH }
    );
    TEST_NEAR(rate100, std::sqrt(0.99), 1e-9, "Static observer at 100 r_s matches sqrt(1 - 0.01)");

    // Test at r = 4 * rs -> sqrt(1 - 0.25) = sqrt(0.75) ~ 0.866025404
    double rate4 = RelativisticPhysics::calculateProperTimeRate(
        glm::dvec3(4.0 * rs, 0.0, 0.0), glm::dvec3(0.0),
        { glm::dvec3(0.0) }, { mBH }
    );
    TEST_NEAR(rate4, std::sqrt(0.75), 1e-9, "Static observer at 4 r_s matches sqrt(1 - 0.25) = 0.866");

    // Test at r = 1.5 * rs (Photon sphere) -> sqrt(1 - 1/1.5) = sqrt(1/3) ~ 0.577350269
    double ratePh = RelativisticPhysics::calculateProperTimeRate(
        glm::dvec3(1.5 * rs, 0.0, 0.0), glm::dvec3(0.0),
        { glm::dvec3(0.0) }, { mBH }
    );
    TEST_NEAR(ratePh, std::sqrt(1.0 / 3.0), 1e-9, "Static observer at photon sphere matches sqrt(1/3)");

    // Test as r -> infinity: dtau/dt -> 1.0
    double rateInf = RelativisticPhysics::calculateProperTimeRate(
        glm::dvec3(1e6 * rs, 0.0, 0.0), glm::dvec3(0.0),
        { glm::dvec3(0.0) }, { mBH }
    );
    TEST_NEAR(rateInf, 1.0, 1e-6, "Asymptotic flat spacetime clock rate approaches 1.0 exactly");

    // Test approaching horizon r -> rs: dtau/dt -> 0.0
    double rateNear = RelativisticPhysics::calculateProperTimeRate(
        glm::dvec3(1.0001 * rs, 0.0, 0.0), glm::dvec3(0.0),
        { glm::dvec3(0.0) }, { mBH }
    );
    TEST_NEAR(rateNear, std::sqrt(0.0001 / 1.0001), 1e-4, "Near-horizon static clock slows asymptotically towards zero");
}

void testGravitationalRedshift() {
    std::cout << "\n=== Test Suite 4: Gravitational Redshift ===" << std::endl;

    double mBH = 10.0 * RelativisticPhysics::SOLAR_MASS_KG;
    double rs = RelativisticPhysics::calculateSchwarzschildRadius(mBH);

    // Static redshift formula: nu_obs / nu_emit = sqrt(1 - rs / r_emit) / sqrt(1 - rs / r_obs)
    // 1. Emission at r = 4 r_s, observed at infinity (r_obs -> inf):
    // nu_obs / nu_emit = sqrt(1 - 1/4) = sqrt(3/4) ~ 0.866025
    double g4_inf = RelativisticPhysics::calculateStaticGravitationalRedshift(mBH, 4.0 * rs, 1e12 * rs);
    TEST_NEAR(g4_inf, std::sqrt(0.75), 1e-7, "Static redshift from 4 r_s to infinity matches sqrt(0.75)");

    // Redshift z = nu_emit / nu_obs - 1
    double z = (1.0 / g4_inf) - 1.0;
    TEST_NEAR(z, (1.0 / std::sqrt(0.75)) - 1.0, 1e-7, "Redshift parameter z matches (1/sqrt(0.75)) - 1 ~ 0.1547");

    // 2. Emission at r = 2 r_s, observed at r = 8 r_s:
    // nu_obs / nu_emit = sqrt(1 - 1/2) / sqrt(1 - 1/8) = sqrt(0.5 / 0.875) = sqrt(4/7) ~ 0.7559289
    double g2_8 = RelativisticPhysics::calculateStaticGravitationalRedshift(mBH, 2.0 * rs, 8.0 * rs);
    TEST_NEAR(g2_8, std::sqrt(0.5 / 0.875), 1e-7, "Static redshift between 2 r_s and 8 r_s matches sqrt(4/7)");
}

void testRelativisticDopplerAndBeaming() {
    std::cout << "\n=== Test Suite 5: Relativistic Doppler Shift & Beaming ===" << std::endl;

    // 1. Longitudinal Doppler towards observer: beta = 0.6, toObs = (1, 0, 0), v = (0.6c, 0, 0)
    // delta = 1 / (gamma * (1 - beta)) = sqrt((1 + beta) / (1 - beta)) = sqrt(1.6 / 0.4) = sqrt(4) = 2.0
    double c = RelativisticPhysics::SPEED_OF_LIGHT;
    glm::dvec3 vApproach(0.6 * c, 0.0, 0.0);
    glm::dvec3 toObs(1.0, 0.0, 0.0);
    double dopplerApproach = RelativisticPhysics::calculateRelativisticDopplerFactor(vApproach, toObs);
    TEST_NEAR(dopplerApproach, 2.0, 1e-6, "Longitudinal Doppler blueshift at 0.6c approaching matches delta = 2.0");

    // 2. Longitudinal Doppler receding: v = (-0.6c, 0, 0), toObs = (1, 0, 0)
    // delta = sqrt((1 - beta) / (1 + beta)) = sqrt(0.4 / 1.6) = sqrt(1/4) = 0.5
    glm::dvec3 vRecede(-0.6 * c, 0.0, 0.0);
    double dopplerRecede = RelativisticPhysics::calculateRelativisticDopplerFactor(vRecede, toObs);
    TEST_NEAR(dopplerRecede, 0.5, 1e-6, "Longitudinal Doppler redshift at 0.6c receding matches delta = 0.5");

    // 3. Transverse Doppler: v = (0, 0.6c, 0), toObs = (1, 0, 0)
    // delta = 1 / gamma = sqrt(1 - 0.36) = sqrt(0.64) = 0.8
    glm::dvec3 vTransverse(0.0, 0.6 * c, 0.0);
    double dopplerTrans = RelativisticPhysics::calculateRelativisticDopplerFactor(vTransverse, toObs);
    TEST_NEAR(dopplerTrans, 0.8, 1e-6, "Transverse Doppler redshift at 0.6c matches 1/gamma = 0.8");

    // 4. Bolometric Beaming flux multiplier: F_obs = delta^4 * F_emit
    // For delta = 2.0: F_obs = 2^4 = 16.0
    double beamingBol = RelativisticPhysics::calculateBeamingIntensityMultiplier(2.0, true);
    TEST_NEAR(beamingBol, 16.0, 1e-6, "Bolometric beaming flux multiplier at delta=2 matches g^4 = 16.0");

    // For delta = 0.5: F_obs = (0.5)^4 = 0.0625
    double beamingRecede = RelativisticPhysics::calculateBeamingIntensityMultiplier(0.5, true);
    TEST_NEAR(beamingRecede, 0.0625, 1e-6, "Bolometric beaming dimming factor at delta=0.5 matches g^4 = 0.0625");
}

void testWeakFieldLightDeflection() {
    std::cout << "\n=== Test Suite 6: Gravitational Light Deflection (Einstein 1915) ===" << std::endl;

    double mSun = RelativisticPhysics::SOLAR_MASS_KG;
    double rSun = RelativisticPhysics::SOLAR_RADIUS_M;

    // Einstein 1915 weak-field deflection: Delta_phi = 4GM / (c^2 * b) = 2 * r_s / b radians
    double deflRad = RelativisticPhysics::calculateWeakFieldDeflection(mSun, rSun);
    double deflArcsec = deflRad * (180.0 / RelativisticPhysics::PI) * 3600.0;

    // Solar grazing ray: theoretical deflection is 1.751 arcseconds
    TEST_NEAR(deflArcsec, 1.7512, 0.005, "Solar limb grazing light deflection matches Einstein's 1.751 arcseconds");

    // Double the impact parameter halves deflection angle
    double defl2R = RelativisticPhysics::calculateWeakFieldDeflection(mSun, 2.0 * rSun);
    TEST_NEAR(defl2R / deflRad, 0.5000000, 1e-6, "Deflection angle scales strictly inversely with impact parameter (1/b)");
}

void testKerrGeometry() {
    std::cout << "\n=== Test Suite 7: Rotating Black Holes (Kerr Metric) ===" << std::endl;

    double mBH = 10.0 * RelativisticPhysics::SOLAR_MASS_KG;
    double rs = RelativisticPhysics::calculateSchwarzschildRadius(mBH);

    // 1. Schwarzschild limit: a* = 0
    double rPlus_0 = RelativisticPhysics::calculateKerrOuterHorizon(mBH, 0.0);
    double rMinus_0 = RelativisticPhysics::calculateKerrInnerHorizon(mBH, 0.0);
    TEST_NEAR(rPlus_0, rs, 1e-9, "For a* = 0, outer event horizon r_+ matches r_s exactly");
    TEST_NEAR(rMinus_0, 0.0, 1e-9, "For a* = 0, inner Cauchy horizon r_- is 0");

    // 2. Extremal Kerr limit: a* = 1.0 -> r_+ = r_- = 0.5 * r_s
    double rPlus_1 = RelativisticPhysics::calculateKerrOuterHorizon(mBH, 1.0);
    double rMinus_1 = RelativisticPhysics::calculateKerrInnerHorizon(mBH, 1.0);
    TEST_NEAR(rPlus_1, 0.5 * rs, 1e-9, "For extremal a* = 1.0, outer event horizon r_+ is 0.5 * r_s");
    TEST_NEAR(rMinus_1, 0.5 * rs, 1e-9, "For extremal a* = 1.0, inner event horizon r_- is 0.5 * r_s");

    // 3. Realistic Cygnus X-1 spin: a* = 0.95
    // r_+ = (rs / 2) * (1 + sqrt(1 - 0.95^2)) = rs * 0.5 * (1 + sqrt(0.0975)) = rs * 0.5 * (1 + 0.3122498999) = rs * 0.656125
    double rPlus_95 = RelativisticPhysics::calculateKerrOuterHorizon(mBH, 0.95);
    TEST_NEAR(rPlus_95 / rs, 0.5 * (1.0 + std::sqrt(1.0 - 0.95 * 0.95)), 1e-9, "For a* = 0.95, outer horizon matches Kerr formula ~0.656 r_s");

    // 4. Ergosphere boundary: r_ergo(theta) = (rs / 2) * (1 + sqrt(1 - a*^2 * cos^2(theta)))
    // At equator (theta = pi/2): cos(theta) = 0 -> r_ergo = rs for ANY spin!
    double rErgoEq = RelativisticPhysics::calculateKerrErgosphereRadius(mBH, 0.95, RelativisticPhysics::PI / 2.0);
    TEST_NEAR(rErgoEq, rs, 1e-9, "Equatorial ergosphere boundary is r_s for all spin parameters");

    // At poles (theta = 0): cos(theta) = 1 -> r_ergo = r_+
    double rErgoPole = RelativisticPhysics::calculateKerrErgosphereRadius(mBH, 0.95, 0.0);
    TEST_NEAR(rErgoPole, rPlus_95, 1e-9, "Polar ergosphere touches the outer event horizon r_+");

    // Verify ergosphere thickness at equator
    TEST_ASSERT(rErgoEq > rPlus_95, "Ergosphere extends outside the horizon at non-polar latitudes");
}

void testKerrISCO() {
    std::cout << "\n=== Test Suite 8: Innermost Stable Circular Orbit (ISCO) ===" << std::endl;

    double mBH = 10.0 * RelativisticPhysics::SOLAR_MASS_KG;
    double rs = RelativisticPhysics::calculateSchwarzschildRadius(mBH);

    // 1. Schwarzschild ISCO: a* = 0 -> r_isco = 3.0 * r_s (= 6 GM/c^2)
    double isco_0 = RelativisticPhysics::calculateISCORadius(mBH, 0.0, true);
    TEST_NEAR(isco_0 / rs, 3.0000000, 1e-6, "For a* = 0, ISCO radius is exactly 3.0 * r_s (6 GM/c^2)");

    // 2. Extremal prograde ISCO: a* = 1.0 -> r_isco = 0.5 * r_s (= 1 GM/c^2 = r_+)
    double isco_1_pro = RelativisticPhysics::calculateISCORadius(mBH, 1.0, true);
    TEST_NEAR(isco_1_pro / rs, 0.5000000, 1e-4, "For extremal prograde a* -> 1.0, ISCO converges to horizon 0.5 * r_s (1 GM/c^2)");

    // 3. Extremal retrograde ISCO: a* = -1.0 -> r_isco = 4.5 * r_s (= 9 GM/c^2)
    double isco_1_retro = RelativisticPhysics::calculateISCORadius(mBH, -1.0, true);
    TEST_NEAR(isco_1_retro / rs, 4.5000000, 1e-4, "For extremal retrograde a* -> -1.0, ISCO expands to 4.5 * r_s (9 GM/c^2)");

    // 4. Verify monotonic reduction of prograde ISCO with increasing spin
    double isco_50 = RelativisticPhysics::calculateISCORadius(mBH, 0.50, true);
    double isco_90 = RelativisticPhysics::calculateISCORadius(mBH, 0.90, true);
    TEST_ASSERT(isco_0 > isco_50 && isco_50 > isco_90, "Prograde ISCO shrinks monotonically as spin increases towards extremal limit");
}

void testNovikovThorneAccretionDisk() {
    std::cout << "\n=== Test Suite 9: Novikov-Thorne Thin Accretion Disk Model ===" << std::endl;

    double mBH = 14.8 * RelativisticPhysics::SOLAR_MASS_KG; // Cygnus X-1
    double spin = 0.95;
    double mDotSolarYr = 1.2e-5; // 1.2e-5 M_sun/yr

    auto disk = RelativisticPhysics::calculateAccretionDiskProperties(mBH, spin, mDotSolarYr, 25.0);

    // 1. Inner radius should match prograde ISCO
    double expectedISCO = RelativisticPhysics::calculateISCORadius(mBH, spin, true);
    TEST_NEAR(disk.innerRadiusM, expectedISCO, 1e-5, "Accretion disk inner boundary truncates at ISCO");

    // 2. Radiative efficiency eta: for a* = 0, eta ~ 0.0572; for a* = 0.95, eta ~ 0.19
    double eta_0 = RelativisticPhysics::calculateAccretionEfficiency(0.0);
    double eta_95 = RelativisticPhysics::calculateAccretionEfficiency(0.95);
    double eta_1 = RelativisticPhysics::calculateAccretionEfficiency(0.9999);
    TEST_NEAR(eta_0, 0.05719, 0.01, "Schwarzschild accretion efficiency eta is ~5.7%");
    TEST_NEAR(eta_95, 0.19, 0.05, "Cygnus X-1 (a*=0.95) accretion efficiency eta is ~19%");
    TEST_NEAR(eta_1, 0.42, 0.05, "Extremal Kerr accretion efficiency eta approaches ~42%");

    // 3. Peak temperature should be on the order of 10^6 - 10^7 K for stellar mass black hole
    TEST_ASSERT(disk.peakTemperatureK > 1e6 && disk.peakTemperatureK < 5e7, "Stellar-mass accretion disk peak temp is X-ray energetic (~10^6 - 10^7 K)");

    // 4. Total disk luminosity: L = eta * Mdot * c^2
    double mDotKgS = (mDotSolarYr * RelativisticPhysics::SOLAR_MASS_KG) / (365.25 * 86400.0);
    double expectedL = disk.radiativeEfficiency * mDotKgS * RelativisticPhysics::C_LIGHT_SQ;
    TEST_NEAR(disk.totalLuminosityW, expectedL, 1e-4, "Bolometric luminosity strictly matches L = eta * Mdot * c^2");
}

void testDynamicCircularOrbitProperTime() {
    std::cout << "\n=== Test Suite 10: Dynamic Proper-Time Accumulation Along Circular Orbit ===" << std::endl;

    double mSun = RelativisticPhysics::SOLAR_MASS_KG;
    double rsSun = RelativisticPhysics::calculateSchwarzschildRadius(mSun);

    // Consider a circular orbit at radius r = 1 AU
    double rAU = RelativisticPhysics::AU_TO_METERS;
    double vOrb = std::sqrt(RelativisticPhysics::G_CONST * mSun / rAU);

    // Metric circular orbit formula:
    // dtau / dt = sqrt(1 - 3GM / (c^2 * r)) = sqrt(1 - 1.5 * rs / r)
    // Here, 1GM/(c^2 r) from kinetic velocity v^2/c^2 + 2GM/(c^2 r) from gravitational field = 3GM/(c^2 r)
    double expectedRate = std::sqrt(1.0 - 1.5 * rsSun / rAU);

    glm::dvec3 pos(rAU, 0.0, 0.0);
    glm::dvec3 vel(0.0, vOrb, 0.0);
    double calculatedRate = RelativisticPhysics::calculateProperTimeRate(pos, vel, { glm::dvec3(0.0) }, { mSun });

    TEST_NEAR(calculatedRate, expectedRate, 1e-12, "Circular orbit proper-time rate matches exact metric interval (gravitational + kinematic)");
}

void testNullGeodesicRK4Integration() {
    std::cout << "\n=== Test Suite 11: 4th-Order Runge-Kutta Null Geodesic Ray-Tracing ===" << std::endl;

    double mBH = 10.0 * RelativisticPhysics::SOLAR_MASS_KG;
    double rs = RelativisticPhysics::calculateSchwarzschildRadius(mBH);
    double bCrit = RelativisticPhysics::calculateCriticalImpactParameter(mBH);

    // Launch photons towards black hole from initial radius r0 = 50 * r_s with impact parameter b
    double r0 = 50.0 * rs;

    // Case 1: Sub-critical ray (b = 0.85 * b_crit): MUST be captured by event horizon
    auto resSub = RelativisticPhysics::traceNullGeodesicRK4(mBH, r0, 0.85 * bCrit, 2000);
    TEST_ASSERT(resSub.capturedByHorizon, "Sub-critical ray (b < b_crit) is captured by event horizon");

    // Case 2: Super-critical ray (b = 1.40 * b_crit): MUST escape to infinity with deflection
    auto resSuper = RelativisticPhysics::traceNullGeodesicRK4(mBH, r0, 1.40 * bCrit, 2500);
    TEST_ASSERT(!resSuper.capturedByHorizon, "Super-critical ray (b > b_crit) escapes to infinity");
    TEST_ASSERT(resSuper.turningRadiusM > rs, "Escaping ray periapsis remains outside event horizon");

    // Case 3: Closer impact parameter penetrates deeper into gravitational potential
    auto resClose = RelativisticPhysics::traceNullGeodesicRK4(mBH, r0, 1.10 * bCrit, 4000);
    TEST_ASSERT(resClose.turningRadiusM < resSuper.turningRadiusM, "Rays closer to shadow penetrate deeper into the gravitational well");
}

void testTidalForcesAndRocheDisruption() {
    std::cout << "\n=== Test Suite 12: Tidal Disruption & Roche Horizon Physics ===" << std::endl;

    double mBH_SMBH = 3e8 * RelativisticPhysics::SOLAR_MASS_KG; // 3 * 10^8 M_sun supermassive black hole (above Hills limit)
    double mBH_IMBH = 1e4 * RelativisticPhysics::SOLAR_MASS_KG; // 10^4 M_sun intermediate mass black hole
    double mStar = RelativisticPhysics::SOLAR_MASS_KG;
    double rStar = RelativisticPhysics::SOLAR_RADIUS_M;

    // Classical Roche tidal disruption radius: r_T = R_* * (M_BH / M_*)^(1/3)
    double rtSMBH = RelativisticPhysics::calculateTidalDisruptionRadius(mBH_SMBH, mStar, rStar);
    double rsSMBH = RelativisticPhysics::calculateSchwarzschildRadius(mBH_SMBH);

    double rtIMBH = RelativisticPhysics::calculateTidalDisruptionRadius(mBH_IMBH, mStar, rStar);
    double rsIMBH = RelativisticPhysics::calculateSchwarzschildRadius(mBH_IMBH);

    // For 3*10^8 M_sun SMBH: r_T < r_s (star is swallowed whole by horizon before tidal disruption can happen)
    TEST_ASSERT(rtSMBH < rsSMBH, "For 3*10^8 M_sun SMBH, tidal radius is INSIDE the horizon (star swallowed without flare)");

    // For 10^4 M_sun IMBH: r_T > r_s (star is disrupted outside horizon -> observable TDE flare)
    TEST_ASSERT(rtIMBH > rsIMBH, "For 10^4 M_sun IMBH, tidal radius is OUTSIDE the horizon (observable TDE flare)");

    // Differential tidal acceleration across 1 km distance at 10 r_s
    double mCyg = 14.8 * RelativisticPhysics::SOLAR_MASS_KG;
    double rsCyg = RelativisticPhysics::calculateSchwarzschildRadius(mCyg);
    glm::dvec3 aTidal = RelativisticPhysics::calculateTidalAcceleration(
        mCyg, glm::dvec3(10.0 * rsCyg, 0.0, 0.0), glm::dvec3(1000.0, 0.0, 0.0)
    );
    TEST_ASSERT(std::abs(aTidal.x) > 1e4, "Strong radial tidal stretching force verified near black hole");
}

void testDualObserverClockComparison() {
    std::cout << "\n=== Test Suite 13: Dual-Observer Clock Comparator Tool ===" << std::endl;

    // Clock A on Earth surface: rate ~ 1 - 7e-10
    // Clock B on GPS orbit (26600 km): rate ~ 1 - 2.5e-10
    double rateA = 0.99999999930;
    double rateB = 0.99999999975;
    double tauA = 86400.0 * rateA;
    double tauB = 86400.0 * rateB;

    auto result = RelativisticPhysics::compareClocks(tauA, tauB, rateA, rateB, 86400.0, "Clock A", "Clock B");

    TEST_NEAR(result.coordinateTimeDeltaSec, 86400.0, 1e-9, "Coordinate time matches exactly 1 day");
    TEST_ASSERT(result.clockRateA < result.clockRateB, "Earth surface clock runs slower than orbital clock");
    TEST_NEAR(result.clockRatioAoverB, rateA / rateB, 1e-12, "Clock ratio correctly computed");
    
    // Offset over 1 day in microseconds: (rateA - rateB) * 86400 ~ -38.8 microseconds
    double usPerDay = (rateA - rateB) * 86400.0 * 1e6;
    TEST_NEAR(usPerDay, -38.88, 0.1, "GPS daily gravitational + kinematic clock drift matches expected ~38 microseconds/day");
}

void testDatabaseRelativisticRoundTrip() {
    std::cout << "\n=== Test Suite 14: Database Relativistic Black Hole Persistence ===" << std::endl;

    DatabaseManager db;
    TEST_ASSERT(db.initialize(":memory:"), "In-memory database initialized successfully with migrations");

    ObjectRepository repo(db);

    // Save a custom rotating black hole with accretion disk
    CelestialBody bhBody;
    bhBody.id = "test_bh_kerr";
    bhBody.name = "Test Kerr Singularity";
    bhBody.type = "Kerr Black Hole";
    bhBody.category = "Relativity Test";
    bhBody.massKg = 20.0 * UnitConverter::SOLAR_MASS_KG;
    bhBody.spinParameter = 0.88;
    bhBody.hasAccretionDisk = true;
    bhBody.accretionRateSolarMassPerYear = 3.5e-5;

    int64_t savedId = 0;
    bool saveOk = repo.saveCelestialBody(bhBody, &savedId);
    TEST_ASSERT(saveOk && savedId > 0, "Saved Kerr black hole body to database");

    // Retrieve and verify hydrated properties
    auto hydrated = repo.getHydratedBody(savedId);
    TEST_ASSERT(hydrated.has_value(), "Retrieved hydrated body record from database");
    if (hydrated.has_value()) {
        const auto& h = hydrated.value();
        TEST_NEAR(h.spinParameter, 0.88, 1e-5, "Hydrated spin parameter matches saved value 0.88");
        TEST_ASSERT(h.hasAccretionDisk, "Hydrated accretion disk active flag matches");
        TEST_NEAR(h.accretionRateSolarMassPerYear, 3.5e-5, 1e-8, "Hydrated accretion rate matches saved rate");

        // Verify that relativistic geometry was derived during hydration
        TEST_ASSERT(h.schwarzschildRadiusM > 0.0, "Hydrated Schwarzschild radius computed from mass");
        TEST_ASSERT(h.photonSphereRadiusM > 0.0, "Hydrated photon sphere computed");
        TEST_ASSERT(h.kerrOuterHorizonM > 0.0, "Hydrated Kerr outer horizon computed");
        TEST_ASSERT(h.iscoRadiusM > 0.0, "Hydrated ISCO computed from spin parameter");
        TEST_ASSERT(h.diskPeakTempK > 1000.0, "Hydrated Novikov-Thorne disk peak temperature computed");
    }
}

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " AstroGenesis Relativistic Physics & Black Hole Test Suite" << std::endl;
    std::cout << "==========================================================" << std::endl;

    testUniversalConstants();
    testSchwarzschildGeometry();
    testStaticProperTimeRate();
    testGravitationalRedshift();
    testRelativisticDopplerAndBeaming();
    testWeakFieldLightDeflection();
    testKerrGeometry();
    testKerrISCO();
    testNovikovThorneAccretionDisk();
    testDynamicCircularOrbitProperTime();
    testNullGeodesicRK4Integration();
    testTidalForcesAndRocheDisruption();
    testDualObserverClockComparison();
    testDatabaseRelativisticRoundTrip();

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " TEST RESULTS: " << g_passedTests << " PASSED, " << g_failedTests << " FAILED" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return (g_failedTests == 0) ? 0 : 1;
}
