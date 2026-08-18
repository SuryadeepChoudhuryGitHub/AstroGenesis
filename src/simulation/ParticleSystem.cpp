#include "simulation/ParticleSystem.hpp"
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

static const double G_CONST = 6.67430e-11;
static const double AU_METERS = 149597870700.0;
static const double PI_DBL = 3.14159265358979323846;
static const double C_LIGHT = 299792458.0;

// =========================================================================
// ParticleField Implementation
// =========================================================================

ParticleField::ParticleField() {
    m_histogram.counts.resize(ParticleHistogram::NUM_BINS, 0);
}

ParticleField::ParticleField(const ParticleFieldConfig& config) {
    m_histogram.counts.resize(ParticleHistogram::NUM_BINS, 0);
    initialize(config);
}

ParticleField::~ParticleField() {}

void ParticleField::initialize(const ParticleFieldConfig& config) {
    m_config = config;
    reseed(config.physicalCount, config.visualCount);
}

void ParticleField::reseed(int physicalCount, int visualCount) {
    m_rng.seed(1337);
    m_config.physicalCount = physicalCount;
    m_config.visualCount = visualCount;

    generatePhysicalPopulation(physicalCount);
    generateVisualPopulation(visualCount);

    m_histogram.counts.assign(ParticleHistogram::NUM_BINS, 0);
    m_diagnostics = {};
    m_diagnostics.totalPhysical = (int)m_physicalParticles.size();
    m_diagnostics.totalVisual = (int)m_visualParticles.size();
}

void ParticleField::generatePhysicalPopulation(int count) {
    m_physicalParticles.clear();
    m_physicalParticles.reserve(count);

    std::uniform_real_distribution<double> distSemiMajor(m_config.innerRadiusAU, m_config.outerRadiusAU);
    std::uniform_real_distribution<double> distAngle(0.0, 360.0);
    std::uniform_real_distribution<double> distUniform01(0.0, 1.0);
    std::normal_distribution<double> distInc(0.0, m_config.inclinationSigmaDeg);
    std::uniform_real_distribution<float> distColor(0.0f, 1.0f);

    double solMassKg = 1.9885e30;

    for (int i = 0; i < count; ++i) {
        PhysicalParticle p;

        p.osculating.semiMajorAxisAU = distSemiMajor(m_rng);

        // Rayleigh distribution for eccentricity: f(e) = (e / sigma^2) * exp(-e^2 / 2sigma^2)
        double u = std::max(1.0e-6, distUniform01(m_rng));
        double rayleighE = m_config.eccentricitySigma * std::sqrt(-2.0 * std::log(u));
        p.osculating.eccentricity = glm::clamp(rayleighE, 0.005, m_config.eccentricityMax);

        // Half-Gaussian for inclination
        p.osculating.inclinationDeg = glm::clamp(std::abs(distInc(m_rng)), 0.2, m_config.inclinationMaxDeg);

        p.osculating.longitudeAscendingNodeDeg = distAngle(m_rng);
        p.osculating.argumentOfPeriapsisDeg = distAngle(m_rng);
        p.osculating.meanAnomalyDeg = distAngle(m_rng);

        // Broken power law for size: dN/dr ~ r^-2.5
        double uSize = distUniform01(m_rng);
        double rM = m_config.minRadiusM * std::pow(1.0 - uSize * 0.99, -1.0 / 1.8);
        p.radiusM = glm::clamp(rM, m_config.minRadiusM, m_config.maxRadiusM);

        // Mass: spherical density
        p.massKg = (4.0 / 3.0) * PI_DBL * std::pow(p.radiusM, 3.0) * m_config.densityKgM3;
        p.colorSeed = distColor(m_rng);

        // Convert Keplerian elements to Cartesian (r, v) state vectors
        keplerianToCartesian(p.osculating, solMassKg, p.positionM, p.velocityMps);

        m_physicalParticles.push_back(p);
    }
}

void ParticleField::generateVisualPopulation(int count) {
    m_visualParticles.clear();
    m_visualParticles.reserve(count);

    std::uniform_real_distribution<float> distA((float)m_config.innerRadiusAU, (float)m_config.outerRadiusAU);
    std::uniform_real_distribution<float> distAngle(0.0f, (float)(2.0 * PI_DBL));
    std::uniform_real_distribution<float> distU(0.0f, 1.0f);
    std::normal_distribution<float> distInc(0.0f, (float)glm::radians(m_config.inclinationSigmaDeg));

    double solMassKg = 1.9885e30;

    for (int i = 0; i < count; ++i) {
        VisualParticle p;
        p.semiMajorAxisAU = distA(m_rng);

        // Rayleigh eccentricity
        float u = std::max(1.0e-5f, distU(m_rng));
        p.eccentricity = glm::clamp((float)m_config.eccentricitySigma * std::sqrt(-2.0f * std::log(u)), 0.005f, (float)m_config.eccentricityMax);

        // Inclination in radians
        p.inclinationRad = glm::clamp(std::abs(distInc(m_rng)), 0.005f, (float)glm::radians(m_config.inclinationMaxDeg));

        p.longitudeAscendingNodeRad = distAngle(m_rng);
        p.argumentOfPeriapsisRad = distAngle(m_rng);
        p.meanAnomaly0Rad = distAngle(m_rng);

        // Mean motion n = sqrt(G*M / a^3) in rad/sec
        double aMeters = (double)p.semiMajorAxisAU * AU_METERS;
        p.meanMotionRadPerSec = (float)std::sqrt((G_CONST * solMassKg) / std::pow(aMeters, 3.0));

        // Size scale (power law: mostly small particles, few larger boulders)
        float uS = distU(m_rng);
        p.sizeScale = 0.000015f + 0.000045f * std::pow(uS, 3.0f);
        p.colorSeed = distU(m_rng);

        m_visualParticles.push_back(p);
    }
}

void ParticleField::keplerianToCartesian(const KeplerianElements& elem, double centralMassKg,
                                        glm::dvec3& outPosM, glm::dvec3& outVelMps) {
    double mu = G_CONST * centralMassKg;
    double aMeters = elem.semiMajorAxisAU * AU_METERS;
    double e = glm::clamp(elem.eccentricity, 0.0, 0.999);

    double iRad = glm::radians(elem.inclinationDeg);
    double omegaNodeRad = glm::radians(elem.longitudeAscendingNodeDeg);
    double argPeriRad = glm::radians(elem.argumentOfPeriapsisDeg);
    double meanAnomRad = glm::radians(elem.meanAnomalyDeg);

    // 1. Solve Kepler's Equation for Eccentric Anomaly E: M = E - e*sin(E)
    double E = meanAnomRad;
    for (int iter = 0; iter < 5; ++iter) {
        double f = E - e * std::sin(E) - meanAnomRad;
        double fPrime = 1.0 - e * std::cos(E);
        E -= f / fPrime;
    }

    // 2. True Anomaly nu
    double cosE = std::cos(E);
    double sinE = std::sin(E);
    double sinNu = (std::sqrt(1.0 - e * e) * sinE) / (1.0 - e * cosE);
    double cosNu = (cosE - e) / (1.0 - e * cosE);
    double nu = std::atan2(sinNu, cosNu);

    // 3. Orbital radius r
    double r = aMeters * (1.0 - e * cosE);

    // 4. Position and velocity in the 2D orbital plane (P-Q frame)
    double xOrb = r * std::cos(nu);
    double zOrb = r * std::sin(nu);

    double vScale = std::sqrt(mu / (aMeters * (1.0 - e * e)));
    double vxOrb = -vScale * std::sin(nu);
    double vzOrb = vScale * (e + std::cos(nu));

    // 5. 3D Rotation to Heliocentric Ecliptic frame: R_z(OmegaNode) * R_x(i) * R_z(argPeri)
    double cosNode = std::cos(omegaNodeRad), sinNode = std::sin(omegaNodeRad);
    double cosInc  = std::cos(iRad),         sinInc  = std::sin(iRad);
    double cosPeri = std::cos(argPeriRad),  sinPeri = std::sin(argPeriRad);

    glm::dvec3 P_vec(
        cosNode * cosPeri - sinNode * sinPeri * cosInc,
        sinPeri * sinInc,
        sinNode * cosPeri + cosNode * sinPeri * cosInc
    );

    glm::dvec3 Q_vec(
        -cosNode * sinPeri - sinNode * cosPeri * cosInc,
        cosPeri * sinInc,
        -sinNode * sinPeri + cosNode * cosPeri * cosInc
    );

    outPosM = xOrb * P_vec + zOrb * Q_vec;
    outVelMps = vxOrb * P_vec + vzOrb * Q_vec;
}

void ParticleField::cartesianToKeplerian(const glm::dvec3& posM, const glm::dvec3& velMps,
                                        double centralMassKg, KeplerianElements& outElem) {
    double mu = G_CONST * centralMassKg;
    double r = glm::length(posM);
    double v = glm::length(velMps);
    if (r < 1000.0 || v < 0.001) return;

    glm::dvec3 h = glm::cross(posM, velMps);
    double hNorm = glm::length(h);
    if (hNorm < 1.0) return;

    // Specific orbital energy: E = v^2/2 - mu/r
    double energy = (v * v * 0.5) - (mu / r);
    if (std::abs(energy) < 1.0e-9) energy = -1.0e-9;

    // Semi-major axis: a = -mu / (2*E)
    double aMeters = -mu / (2.0 * energy);
    outElem.semiMajorAxisAU = aMeters / AU_METERS;

    // Laplace-Runge-Lenz eccentricity vector: e = (v x h)/mu - r/|r|
    glm::dvec3 eVec = (glm::cross(velMps, h) / mu) - (posM / r);
    outElem.eccentricity = glm::clamp(glm::length(eVec), 0.0, 0.999);

    // Inclination: i = arccos(h_y / |h|)
    outElem.inclinationDeg = glm::degrees(std::acos(glm::clamp(h.y / hNorm, -1.0, 1.0)));

    // Line of nodes: n = (0, 1, 0) x h = (h.z, 0, -h.x)
    glm::dvec3 nVec(h.z, 0.0, -h.x);
    double nNorm = glm::length(nVec);

    if (nNorm > 1.0e-7) {
        outElem.longitudeAscendingNodeDeg = glm::degrees(std::atan2(nVec.z, nVec.x));
        if (outElem.longitudeAscendingNodeDeg < 0.0) outElem.longitudeAscendingNodeDeg += 360.0;

        double cosOmega = glm::dot(nVec, eVec) / (nNorm * std::max(outElem.eccentricity, 1.0e-7));
        outElem.argumentOfPeriapsisDeg = glm::degrees(std::acos(glm::clamp(cosOmega, -1.0, 1.0)));
        if (eVec.y < 0.0) outElem.argumentOfPeriapsisDeg = 360.0 - outElem.argumentOfPeriapsisDeg;
    } else {
        outElem.longitudeAscendingNodeDeg = 0.0;
        outElem.argumentOfPeriapsisDeg = glm::degrees(std::atan2(eVec.z, eVec.x));
        if (outElem.argumentOfPeriapsisDeg < 0.0) outElem.argumentOfPeriapsisDeg += 360.0;
    }

    // True anomaly nu: cos(nu) = (e . r) / (|e|*r)
    double cosNu = glm::dot(eVec, posM) / (std::max(outElem.eccentricity, 1.0e-7) * r);
    outElem.trueAnomalyDeg = glm::degrees(std::acos(glm::clamp(cosNu, -1.0, 1.0)));
    if (glm::dot(posM, velMps) < 0.0) outElem.trueAnomalyDeg = 360.0 - outElem.trueAnomalyDeg;

    // Period via Kepler's Third Law
    if (outElem.semiMajorAxisAU > 0.0) {
        outElem.orbitalPeriodDays = (2.0 * PI_DBL * std::sqrt(std::pow(aMeters, 3.0) / mu)) / 86400.0;
    }
}

void ParticleField::computePhysicalAccelerations(const std::vector<CelestialBody>& attractors, bool enableGR) {
    size_t numParticles = m_physicalParticles.size();
    size_t numAttractors = attractors.size();

    for (size_t i = 0; i < numParticles; ++i) {
        auto& p = m_physicalParticles[i];
        if (p.isEscaped || p.collidedWithCentral) continue;

        glm::dvec3 totalAcc{0.0};
        glm::dvec3 pos = p.positionM;
        glm::dvec3 vel = p.velocityMps;

        for (size_t j = 0; j < numAttractors; ++j) {
            const auto& body = attractors[j];
            glm::dvec3 r_vec = body.positionM - pos;
            double r2 = glm::dot(r_vec, r_vec);
            double r = std::sqrt(r2);

            if (r < 1000.0) continue;

            // Newtonian gravitational acceleration: a = G * M / r^2 * r_hat
            double newtonCoeff = (G_CONST * body.massKg) / (r2 * r);
            glm::dvec3 acc = newtonCoeff * r_vec;

            // Einstein 1PN Post-Newtonian relativistic correction
            if (enableGR && body.id == "sol" && r > 1.0e6) {
                double c2 = C_LIGHT * C_LIGHT;
                double v2 = glm::dot(vel, vel);
                double rDotV = glm::dot(r_vec, vel);
                double grFactor1 = (4.0 * G_CONST * body.massKg / r) - v2;
                glm::dvec3 gr1 = (G_CONST * body.massKg / (c2 * r2 * r)) * (grFactor1 * r_vec);
                glm::dvec3 gr2 = (4.0 * G_CONST * body.massKg / (c2 * r2 * r)) * (rDotV * vel);
                acc += (gr1 + gr2);
            }

            totalAcc += acc;
        }

        p.accelerationMps2 = totalAcc;
    }
}

void ParticleField::integratePhysicalParticles(double dt, const std::vector<CelestialBody>& attractors, bool enableGR) {
    // Sub-stepped Symplectic Velocity-Verlet integrator
    const double maxSubStep = 3600.0 * 2.0; // 2 hour max physical sub-step
    int numSteps = std::max(1, (int)std::ceil(std::abs(dt) / maxSubStep));
    double subDt = dt / (double)numSteps;

    for (int step = 0; step < numSteps; ++step) {
        // Step 1: Position update r(t + dt) = r(t) + v(t)*dt + 0.5*a(t)*dt^2
        for (auto& p : m_physicalParticles) {
            if (p.isEscaped || p.collidedWithCentral) continue;
            p.positionM += p.velocityMps * subDt + 0.5 * p.accelerationMps2 * (subDt * subDt);
        }

        // Store old acceleration
        std::vector<glm::dvec3> oldAcc(m_physicalParticles.size());
        for (size_t i = 0; i < m_physicalParticles.size(); ++i) {
            oldAcc[i] = m_physicalParticles[i].accelerationMps2;
        }

        // Step 2: Calculate new acceleration a(t + dt)
        computePhysicalAccelerations(attractors, enableGR);

        // Step 3: Velocity update v(t + dt) = v(t) + 0.5 * (a(t) + a(t + dt)) * dt
        for (size_t i = 0; i < m_physicalParticles.size(); ++i) {
            auto& p = m_physicalParticles[i];
            if (p.isEscaped || p.collidedWithCentral) continue;

            p.velocityMps += 0.5 * (oldAcc[i] + p.accelerationMps2) * subDt;

            // Boundary checks:
            double distToSunM = glm::length(p.positionM);
            if (distToSunM < 7.0e8) { // Collided with Sol surface
                p.collidedWithCentral = true;
            } else if (distToSunM > 60.0 * AU_METERS) { // Ejected / Escaped Solar System
                p.isEscaped = true;
            }
        }
    }
}

void ParticleField::update(double deltaSeconds, const std::vector<CelestialBody>& attractors,
                          bool enableGR, double simulatedTimeSeconds) {
    if (deltaSeconds == 0.0 || m_physicalParticles.empty()) return;

    // Run physical gravitational integration
    integratePhysicalParticles(deltaSeconds, attractors, enableGR);

    // Update diagnostics, osculating elements, and N(a) histogram
    updateDiagnostics(attractors);
}

void ParticleField::updateDiagnostics(const std::vector<CelestialBody>& attractors) {
    double solMass = 1.9885e30;
    for (const auto& p : attractors) {
        if (p.id == "sol") {
            solMass = p.massKg;
            break;
        }
    }

    m_histogram.counts.assign(ParticleHistogram::NUM_BINS, 0);
    m_histogram.maxBinCount = 1;

    int active = 0;
    int escaped = 0;
    int sunCollided = 0;
    int excited = 0;

    double sumA = 0.0;
    double sumE = 0.0;
    double maxE = 0.0;
    double sumI = 0.0;
    double totalMass = 0.0;
    double totalEnergy = 0.0;

    for (auto& p : m_physicalParticles) {
        if (p.isEscaped) { escaped++; continue; }
        if (p.collidedWithCentral) { sunCollided++; continue; }

        active++;
        totalMass += p.massKg;

        // Recalculate osculating Keplerian orbital elements from current (r, v)
        cartesianToKeplerian(p.positionM, p.velocityMps, solMass, p.osculating);

        double a = p.osculating.semiMajorAxisAU;
        double e = p.osculating.eccentricity;
        double inc = p.osculating.inclinationDeg;

        sumA += a;
        sumE += e;
        maxE = std::max(maxE, e);
        sumI += inc;

        if (e > 0.25) excited++;

        // Mechanical energy
        double v = glm::length(p.velocityMps);
        double r = glm::length(p.positionM);
        totalEnergy += p.massKg * ((0.5 * v * v) - (G_CONST * solMass / r));

        // Bin into N(a) histogram
        if (a >= m_histogram.minAU && a < m_histogram.maxAU) {
            int bin = (int)((a - m_histogram.minAU) / m_histogram.binWidth);
            bin = glm::clamp(bin, 0, ParticleHistogram::NUM_BINS - 1);
            m_histogram.counts[bin]++;
            if (m_histogram.counts[bin] > m_histogram.maxBinCount) {
                m_histogram.maxBinCount = m_histogram.counts[bin];
            }
        }
    }

    m_diagnostics.totalPhysical = (int)m_physicalParticles.size();
    m_diagnostics.activePhysical = active;
    m_diagnostics.escapedPhysical = escaped;
    m_diagnostics.sunCollidedPhysical = sunCollided;
    m_diagnostics.totalVisual = (int)m_visualParticles.size();
    m_diagnostics.totalMassKg = totalMass;
    m_diagnostics.highlyExcitedCount = excited;

    if (active > 0) {
        m_diagnostics.meanSemiMajorAxisAU = sumA / (double)active;
        m_diagnostics.meanEccentricity = sumE / (double)active;
        m_diagnostics.maxEccentricity = maxE;
        m_diagnostics.meanInclinationDeg = sumI / (double)active;
    }

    if (m_diagnostics.initialEnergyJ == 0.0 && totalEnergy != 0.0) {
        m_diagnostics.initialEnergyJ = totalEnergy;
    }
    m_diagnostics.currentEnergyJ = totalEnergy;
    if (std::abs(m_diagnostics.initialEnergyJ) > 1.0e-9) {
        m_diagnostics.energyDriftPct = (std::abs(totalEnergy - m_diagnostics.initialEnergyJ) /
                                       std::abs(m_diagnostics.initialEnergyJ)) * 100.0;
    }
}

void ParticleField::updateVisualInstanceBuffer(double simulatedTimeSeconds, const glm::vec3& cameraTarget,
                                               float visualSizeMultiplier) {
    size_t totalCount = m_physicalParticles.size() + m_visualParticles.size();
    m_instanceData.resize(totalCount);

    size_t outIdx = 0;

    // 1. Pack physical particles
    for (const auto& p : m_physicalParticles) {
        if (p.isEscaped || p.collidedWithCentral) continue;

        glm::vec3 posAU = glm::vec3(
            (float)(p.positionM.x / AU_METERS),
            (float)(p.positionM.y / AU_METERS),
            (float)(p.positionM.z / AU_METERS)
        ) - cameraTarget;

        float baseScale = (float)(p.radiusM / 1000.0) * 0.00008f * visualSizeMultiplier;
        baseScale = glm::clamp(baseScale, 0.00004f, 0.003f);

        glm::vec3 rockColor = glm::mix(m_config.colorA, m_config.colorB, p.colorSeed);
        m_instanceData[outIdx++] = { posAU, baseScale, glm::vec4(rockColor, 1.0f) };
    }

    // 2. Vectorized analytical Kepler propagation for high-density visual particles
    for (const auto& p : m_visualParticles) {
        // Instantaneous Mean Anomaly: M(t) = M0 + n*t
        float M = p.meanAnomaly0Rad + p.meanMotionRadPerSec * (float)simulatedTimeSeconds;
        M = std::fmod(M, (float)(2.0 * PI_DBL));
        if (M < 0.0f) M += (float)(2.0 * PI_DBL);

        // Fast 2-iteration Kepler equation solver for Eccentric Anomaly E
        float e = p.eccentricity;
        float E = M + e * std::sin(M);
        for (int k = 0; k < 2; ++k) {
            float f = E - e * std::sin(E) - M;
            float fP = 1.0f - e * std::cos(E);
            E -= f / fP;
        }

        // True anomaly and radius in AU
        float cosE = std::cos(E);
        float sinE = std::sin(E);
        float rAU = p.semiMajorAxisAU * (1.0f - e * cosE);

        float sinNu = (std::sqrt(1.0f - e * e) * sinE) / (1.0f - e * cosE);
        float cosNu = (cosE - e) / (1.0f - e * cosE);
        float nu = std::atan2(sinNu, cosNu);

        // Orbital plane coordinates (x, z)
        float xOrb = rAU * std::cos(nu);
        float zOrb = rAU * std::sin(nu);

        // 3D Rotation to Heliocentric Ecliptic frame
        float cosNode = std::cos(p.longitudeAscendingNodeRad), sinNode = std::sin(p.longitudeAscendingNodeRad);
        float cosInc  = std::cos(p.inclinationRad),            sinInc  = std::sin(p.inclinationRad);
        float cosPeri = std::cos(p.argumentOfPeriapsisRad),     sinPeri = std::sin(p.argumentOfPeriapsisRad);

        glm::vec3 P_vec(
            cosNode * cosPeri - sinNode * sinPeri * cosInc,
            sinPeri * sinInc,
            sinNode * cosPeri + cosNode * sinPeri * cosInc
        );

        glm::vec3 Q_vec(
            -cosNode * sinPeri - sinNode * cosPeri * cosInc,
            cosPeri * sinInc,
            -sinNode * sinPeri + cosNode * cosPeri * cosInc
        );

        glm::vec3 posAU = (xOrb * P_vec + zOrb * Q_vec) - cameraTarget;

        float finalScale = p.sizeScale * visualSizeMultiplier;
        glm::vec3 rockColor = glm::mix(m_config.colorA, m_config.colorB, p.colorSeed);

        m_instanceData[outIdx++] = { posAU, finalScale, glm::vec4(rockColor, 0.85f) };
    }

    m_instanceData.resize(outIdx);
}

void ParticleField::triggerResonanceImpulseTest() {
    for (auto& p : m_physicalParticles) {
        if (std::abs(p.osculating.semiMajorAxisAU - 2.50) < 0.08 ||
            std::abs(p.osculating.semiMajorAxisAU - 3.27) < 0.08) {
            p.velocityMps *= 1.15; // Gravitational boost
        }
    }
}

// =========================================================================
// ParticleSystem Implementation
// =========================================================================

ParticleSystem::ParticleSystem() {}

ParticleSystem::~ParticleSystem() {}

void ParticleSystem::initializeDefaultSystem() {
    m_fields.clear();

    // 1. Initialize Main Asteroid Belt
    m_asteroidBelt.initialize(ParticleFieldConfig::createMainAsteroidBelt());

    // Wrap in fields list for multi-field traversal
    m_fields.push_back(std::make_shared<ParticleField>(m_asteroidBelt));
}

void ParticleSystem::update(double deltaSeconds, const std::vector<CelestialBody>& attractors,
                            bool enableGR, double simulatedTimeSeconds) {
    m_asteroidBelt.update(deltaSeconds, attractors, enableGR, simulatedTimeSeconds);
}

} // namespace AstroGenesis
