#include "simulation/PhysicsEngine.hpp"
#include "data/UnitConverter.hpp"
#include <ctime>
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace AstroGenesis {

static const double PI_DBL        = 3.14159265358979323846;
static const double G_CONST       = 6.67430e-11;      // Gravitational constant (m^3 kg^-1 s^-2)
static const double C_LIGHT       = 299792458.0;      // Speed of light (m/s)
static const double C_LIGHT_SQ    = C_LIGHT * C_LIGHT;
static const double AU_METERS     = 149597870700.0;   // 1 AU in meters
static const double AU_KM         = 149597870.7;      // 1 AU in km
static const double L_SUN         = 3.828e26;         // Solar luminosity in Watts
static const double SIGMA_SB      = 5.670374419e-8;   // Stefan-Boltzmann constant (W m^-2 K^-4)
static const double SEC_PER_DAY   = 86400.0;

PhysicsEngine::PhysicsEngine() {
    // Systems will be loaded dynamically from Database via loadFromDatabase()
}

bool PhysicsEngine::loadFromDatabase(ObjectRepository& repo, const std::string& systemCategory) {
    m_currentCategory = systemCategory;
    m_bodies.clear();

    m_bodies = repo.getSystemBodies(systemCategory);

    if (m_bodies.empty()) {
        std::cerr << "[PhysicsEngine] No bodies found for category: " << systemCategory << std::endl;
        return false;
    }

    // Apply Barycentric Frame transformation (Center of Mass & Zero Total Momentum)
    computeBarycenterTransform();

    // Default selection: Earth if present, else first body
    m_selectedBodyIndex = 0;
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        if (m_bodies[i].id == "earth" || m_bodies[i].name == "Earth") {
            m_selectedBodyIndex = (int)i;
            break;
        }
    }

    updateBodyScales();
    updatePhysicalQuantities();
    generateOrbitalTrails();

    // Initialize Particle System (Main Asteroid Belt + Extensible Particle Fields)
    m_particleSystem.initializeDefaultSystem();

    // Apply Asteroid Population Mode
    setAsteroidPopulationMode(m_asteroidPopulationMode, &repo);

    // Initialize Deformable Matter Simulation System
    m_matterSystem.initialize();

    std::cout << "[PhysicsEngine] Successfully loaded " << m_bodies.size() << " bodies for [" << systemCategory << "] from database." << std::endl;
    return true;
}

void PhysicsEngine::reloadCurrentSystem(ObjectRepository& repo) {
    loadFromDatabase(repo, m_currentCategory);
}

void PhysicsEngine::addBody(const CelestialBody& body) {
    m_bodies.push_back(body);
    updateBodyScales();
    updatePhysicalQuantities();
}

void PhysicsEngine::clearBodies() {
    m_bodies.clear();
    m_selectedBodyIndex = 0;
}

void PhysicsEngine::computeBarycenterTransform() {
    if (m_bodies.size() <= 1) return;

    double totalMass = 0.0;
    glm::dvec3 centerOfMassM(0.0);
    glm::dvec3 totalMomentumMps(0.0);

    for (const auto& b : m_bodies) {
        totalMass += b.massKg;
        centerOfMassM += b.massKg * b.positionM;
        totalMomentumMps += b.massKg * b.velocityMps;
    }

    if (totalMass <= 0.0) return;

    glm::dvec3 vBarycenter = totalMomentumMps / totalMass;
    glm::dvec3 rBarycenter = centerOfMassM / totalMass;

    for (auto& b : m_bodies) {
        b.positionM -= rBarycenter;
        b.velocityMps -= vBarycenter;
        b.position = glm::vec3((float)(b.positionM.x / AU_METERS), (float)(b.positionM.y / AU_METERS), (float)(b.positionM.z / AU_METERS));
        b.velocity = glm::vec3((float)(b.velocityMps.x / AU_METERS), (float)(b.velocityMps.y / AU_METERS), (float)(b.velocityMps.z / AU_METERS));
    }
}

void PhysicsEngine::generateOrbitalTrails() {
    glm::dvec3 starPosM(0.0);
    for (const auto& b : m_bodies) {
        if (b.id == "sol" || b.type.find("Star") != std::string::npos) {
            starPosM = b.positionM;
            break;
        }
    }

    for (auto& body : m_bodies) {
        if (body.id != "sol" && body.type.find("Star") == std::string::npos) {
            body.trailHistory.clear();
            const int initialSteps = 240;
            glm::dvec3 relPosM = body.positionM - starPosM;
            double r = glm::length(relPosM) / AU_METERS;
            if (r <= 0.0001) continue;

            double curAngle = std::atan2(relPosM.z, relPosM.x);
            glm::vec3 starPosAU = glm::vec3((float)(starPosM.x / AU_METERS), (float)(starPosM.y / AU_METERS), (float)(starPosM.z / AU_METERS));
            for (int s = initialSteps; s >= 0; --s) {
                double angle = curAngle - (2.0 * PI_DBL * (double)s / (double)initialSteps);
                float px = (float)(r * std::cos(angle));
                float pz = (float)(r * std::sin(angle));
                body.trailHistory.push_back(starPosAU + glm::vec3(px, 0.0f, pz));
            }
        }
    }
}

void PhysicsEngine::setAsteroidPopulationMode(AsteroidPopulationMode mode, ObjectRepository* repo) {
    m_asteroidPopulationMode = mode;
    
    if (mode == AsteroidPopulationMode::RealOnly) {
        m_particleSystem.getAsteroidBelt().reseed(128, 256);
    } else if (mode == AsteroidPopulationMode::SyntheticOnly) {
        m_particleSystem.getAsteroidBelt().reseed(1024, 60000);
    } else if (mode == AsteroidPopulationMode::Hybrid) {
        m_particleSystem.getAsteroidBelt().reseed(512, 45000);
    }
}

void PhysicsEngine::computeAccelerations(const std::vector<glm::dvec3>& positions,
                                        const std::vector<glm::dvec3>& velocities,
                                        std::vector<glm::dvec3>& outAccelerations) {
    size_t n = m_bodies.size();
    outAccelerations.assign(n, glm::dvec3(0.0));

    for (size_t i = 0; i < n; ++i) {
        glm::dvec3 pos_i = positions[i];
        glm::dvec3 vel_i = velocities[i];
        double v_i_sq = glm::dot(vel_i, vel_i);

        for (size_t j = 0; j < n; ++j) {
            if (i == j) continue;

            glm::dvec3 r_vec = positions[j] - pos_i;
            double r_sq = glm::dot(r_vec, r_vec);
            double r = std::sqrt(r_sq);

            if (r < 1000.0) continue; // avoid singularity inside physical cores

            double r_cubed = r_sq * r;
            double mu_j = G_CONST * m_bodies[j].massKg;

            // 1. Classical Newtonian Gravity (Inverse-Square Law)
            glm::dvec3 a_newton = (mu_j / r_cubed) * r_vec;
            outAccelerations[i] += a_newton;

            // 2. Einstein 1PN Post-Newtonian Relativistic Correction
            if (m_enableGeneralRelativity) {
                glm::dvec3 vel_j = velocities[j];
                glm::dvec3 v_rel = vel_i - vel_j;
                double v_j_sq = glm::dot(vel_j, vel_j);
                double v_rel_sq = glm::dot(v_rel, v_rel);
                double r_dot_vi = glm::dot(r_vec, vel_i);
                double r_dot_vj = glm::dot(r_vec, vel_j);

                // Standard 1PN Post-Newtonian EIH (Einstein-Infeld-Hoffmann) acceleration formulation
                double factor1 = (4.0 * mu_j / r) + (4.0 * G_CONST * m_bodies[i].massKg / r)
                                 - v_i_sq + 4.0 * glm::dot(vel_i, vel_j) - 2.0 * v_j_sq
                                 + 1.5 * std::pow(r_dot_vj / r, 2.0);

                glm::dvec3 term1 = factor1 * r_vec;
                glm::dvec3 term2 = (4.0 * r_dot_vi - 3.0 * r_dot_vj) * v_rel;

                glm::dvec3 a_1PN = (mu_j / (C_LIGHT_SQ * r_cubed)) * (term1 + term2);
                outAccelerations[i] += a_1PN;
            }
        }
    }
}

void PhysicsEngine::integrateNBody(double deltaSeconds) {
    size_t n = m_bodies.size();
    if (n == 0) return;

    std::vector<glm::dvec3> positions(n);
    std::vector<glm::dvec3> velocities(n);
    std::vector<glm::dvec3> acc1(n);
    std::vector<glm::dvec3> acc2(n);

    for (size_t i = 0; i < n; ++i) {
        positions[i] = m_bodies[i].positionM;
        velocities[i] = m_bodies[i].velocityMps;
    }

    // Step 1: Compute initial accelerations a(t)
    computeAccelerations(positions, velocities, acc1);

    // Step 2: Symplectic Position update x(t + dt) = x(t) + v(t)*dt + 0.5*a(t)*dt^2
    for (size_t i = 0; i < n; ++i) {
        positions[i] += velocities[i] * deltaSeconds + 0.5 * acc1[i] * (deltaSeconds * deltaSeconds);
    }

    // Step 3: Compute updated accelerations a(t + dt)
    computeAccelerations(positions, velocities, acc2);

    // Step 4: Velocity update v(t + dt) = v(t) + 0.5*(a(t) + a(t + dt))*dt
    for (size_t i = 0; i < n; ++i) {
        velocities[i] += 0.5 * (acc1[i] + acc2[i]) * deltaSeconds;
        m_bodies[i].positionM = positions[i];
        m_bodies[i].velocityMps = velocities[i];
        m_bodies[i].accelerationMps2 = acc2[i];

        // Sync AU-space rendering positions
        m_bodies[i].position = glm::vec3((float)(positions[i].x / AU_METERS),
                                         (float)(positions[i].y / AU_METERS),
                                         (float)(positions[i].z / AU_METERS));
        m_bodies[i].velocity = glm::vec3((float)(velocities[i].x / AU_METERS),
                                         (float)(velocities[i].y / AU_METERS),
                                         (float)(velocities[i].z / AU_METERS));

        // Update 3D orbital trail history
        if (m_bodies[i].id != "sol" && m_bodies[i].type.find("Star") == std::string::npos) {
            glm::vec3 curPos = m_bodies[i].position;
            if (m_bodies[i].trailHistory.empty() ||
                glm::distance(m_bodies[i].trailHistory.back(), curPos) > 0.005f) {
                m_bodies[i].trailHistory.push_back(curPos);
                if (m_bodies[i].trailHistory.size() > m_bodies[i].maxTrailPoints) {
                    m_bodies[i].trailHistory.pop_front();
                }
            }
        }
    }
}

void PhysicsEngine::updatePhysicalQuantities() {
    if (m_bodies.empty()) return;

    // Identify primary star (Sol)
    size_t solIdx = 0;
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        if (m_bodies[i].id == "sol" || m_bodies[i].type.find("Star") != std::string::npos) {
            solIdx = i;
            break;
        }
    }

    const auto& sol = m_bodies[solIdx];
    double solMass = (sol.massKg > 0.0) ? sol.massKg : 1.9885e30;
    double solLum = (sol.luminosityW > 0.0) ? sol.luminosityW : L_SUN;

    for (size_t i = 0; i < m_bodies.size(); ++i) {
        auto& b = m_bodies[i];
        if (i == solIdx) {
            b.distanceAU = 0.0;
            b.distanceKm = 0.0;
            b.distanceStr = "0.00 AU";
            b.orbitalSpeedKmpS = glm::length(b.velocityMps) / 1000.0;
            char spdBuf[64];
            snprintf(spdBuf, sizeof(spdBuf), "%.2f km/s", b.orbitalSpeedKmpS);
            b.orbitalSpeedStr = spdBuf;
            continue;
        }

        // Identify central attractor: parent planet if body is a moon, otherwise primary star
        size_t attractorIdx = solIdx;
        bool isMoon = (b.type.find("Moon") != std::string::npos || b.type.find("Satellite") != std::string::npos ||
                       b.id == "moon" || b.id == "ganymede" || b.id == "europa" || b.id == "io" || b.id == "callisto" || b.id == "titan" ||
                       b.id == "phobos" || b.id == "deimos" || b.id == "enceladus" || b.id == "triton" || b.id == "charon");

        if (isMoon) {
            for (size_t p = 0; p < m_bodies.size(); ++p) {
                if (p == i) continue;
                if ((b.id == "moon" && m_bodies[p].id == "earth") ||
                    ((b.id == "ganymede" || b.id == "europa" || b.id == "io" || b.id == "callisto") && m_bodies[p].id == "jupiter") ||
                    ((b.id == "titan" || b.id == "enceladus" || b.id == "mimas") && m_bodies[p].id == "saturn") ||
                    ((b.id == "phobos" || b.id == "deimos") && m_bodies[p].id == "mars") ||
                    ((b.id == "triton" || b.id == "proteus") && m_bodies[p].id == "neptune") ||
                    (b.id == "charon" && m_bodies[p].id == "pluto") ||
                    (b.parentObjectId.has_value() && b.parentObjectId.value() == m_bodies[p].dbId)) {
                    attractorIdx = p;
                    break;
                }
            }
        }
        const auto& attractor = m_bodies[attractorIdx];
        double attractorMass = (attractor.massKg > 0.0) ? attractor.massKg : solMass;

        // Relative vector to central attractor (parent planet for moons, sol for planets)
        glm::dvec3 rVec = b.positionM - attractor.positionM;
        glm::dvec3 vVec = b.velocityMps - attractor.velocityMps;
        double rM = glm::length(rVec);
        double vMps = glm::length(vVec);

        // Solar distance for solar radiation flux and thermal balance
        double rSolM = glm::length(b.positionM - sol.positionM);
        if (rSolM < 1000.0) rSolM = 1000.0;

        b.distanceAU = rM / AU_METERS;
        b.distanceKm = rM / 1000.0;
        b.orbitalSpeedKmpS = vMps / 1000.0;

        char distBuf[64], speedBuf[64];
        if (b.distanceAU >= 0.05) {
            snprintf(distBuf, sizeof(distBuf), "%.3f AU (%.1fM km)", b.distanceAU, (rM * 1e-9));
        } else {
            snprintf(distBuf, sizeof(distBuf), "%'.0f km", b.distanceKm);
        }
        b.distanceStr = distBuf;
        snprintf(speedBuf, sizeof(speedBuf), "%.2f km/s", b.orbitalSpeedKmpS);
        b.orbitalSpeedStr = speedBuf;

        // Instantaneous Solar Radiation Flux: F = L / (4 * pi * r_sol^2)
        b.solarRadiationFlux = solLum / (4.0 * PI_DBL * rSolM * rSolM);
        char fluxBuf[64];
        snprintf(fluxBuf, sizeof(fluxBuf), "%'.1f W/m²", b.solarRadiationFlux);
        b.solarRadiationStr = fluxBuf;

        if (b.solarRadiationFlux > 1500.0) b.radLevelStr = "Extreme";
        else if (b.solarRadiationFlux > 800.0) b.radLevelStr = "High";
        else if (b.solarRadiationFlux > 200.0) b.radLevelStr = "Moderate";
        else b.radLevelStr = "Low";

        // Thermal Equilibrium Temperature: T_eq = ( (F * (1 - A)) / (4 * sigma) )^(1/4) + T_greenhouse
        double tEffectiveK = std::pow((b.solarRadiationFlux * (1.0 - b.albedo)) / (4.0 * SIGMA_SB), 0.25);
        b.surfaceTempK = tEffectiveK + b.greenhouseK;
        char tempBuf[64];
        snprintf(tempBuf, sizeof(tempBuf), "%d K (%.1f °C)", (int)std::round(b.surfaceTempK), b.surfaceTempK - 273.15);
        b.tempStr = tempBuf;

        // Surface gravity: g = G * M / R^2
        if (b.radiusM > 0.0) {
            b.surfaceGravityMps2 = (G_CONST * b.massKg) / (b.radiusM * b.radiusM);
            char gravBuf[64];
            snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s² (%.2f g)", b.surfaceGravityMps2, b.surfaceGravityMps2 / 9.80665);
            b.gravityStr = gravBuf;

            b.escapeVelocityKmpS = std::sqrt((2.0 * G_CONST * b.massKg) / b.radiusM) / 1000.0;
            char escBuf[64];
            snprintf(escBuf, sizeof(escBuf), "%.2f km/s", b.escapeVelocityKmpS);
            b.escapeVelocityStr = escBuf;

            double volumeM3 = (4.0 / 3.0) * PI_DBL * std::pow(b.radiusM, 3.0);
            b.meanDensityKgM3 = b.massKg / volumeM3;
            char denBuf[64];
            snprintf(denBuf, sizeof(denBuf), "%'d kg/m³", (int)std::round(b.meanDensityKgM3));
            b.densityStr = denBuf;

            double surfaceAreaM2 = 4.0 * PI_DBL * std::pow(b.radiusM, 2.0);
            b.surfaceAreaKm2 = surfaceAreaM2 * 1e-6;
            char areaBuf[64];
            snprintf(areaBuf, sizeof(areaBuf), "%.1f M km²", b.surfaceAreaKm2 * 1e-6);
            b.surfaceAreaStr = areaBuf;
        }

        // Relativistic Time Dilation
        double phiPotential = - (G_CONST * attractorMass) / rM;
        double gravitationalShift = phiPotential / C_LIGHT_SQ;
        double kinematicShift = - 0.5 * (vMps * vMps) / C_LIGHT_SQ;
        b.timeDilationShift = gravitationalShift + kinematicShift;
        b.timeDriftMicrosecPerDay = b.timeDilationShift * SEC_PER_DAY * 1e6;

        char driftBuf[64];
        snprintf(driftBuf, sizeof(driftBuf), "%+.2f µs/day", b.timeDriftMicrosecPerDay);
        b.timeDilationStr = driftBuf;

        // Dynamic Keplerian Orbital Elements
        glm::dvec3 hVec = glm::cross(rVec, vVec);
        b.specificAngularMomentum = glm::length(hVec);
        char hBuf[64];
        snprintf(hBuf, sizeof(hBuf), "%.2e m²/s", b.specificAngularMomentum);
        b.angularMomentumStr = hBuf;

        double muTotal = G_CONST * (attractorMass + b.massKg);
        b.specificOrbitalEnergy = 0.5 * vMps * vMps - (muTotal / rM);
        char energyBuf[64];
        snprintf(energyBuf, sizeof(energyBuf), "%.1f MJ/kg", b.specificOrbitalEnergy * 1e-6);
        b.orbitalEnergyStr = energyBuf;

        if (b.specificOrbitalEnergy < 0.0) {
            b.semiMajorAxisM = - muTotal / (2.0 * b.specificOrbitalEnergy);
            b.semiMajorAxisAU = b.semiMajorAxisM / AU_METERS;

            glm::dvec3 eVec = (glm::cross(vVec, hVec) / muTotal) - (rVec / rM);
            b.eccentricity = glm::length(eVec);

            b.periapsisM = b.semiMajorAxisM * (1.0 - b.eccentricity);
            b.periapsisAU = b.periapsisM / AU_METERS;
            b.apoapsisM = b.semiMajorAxisM * (1.0 + b.eccentricity);
            b.apoapsisAU = b.apoapsisM / AU_METERS;

            char smaBuf[64], eccBuf[64], periBuf[64], apoBuf[64];
            snprintf(smaBuf, sizeof(smaBuf), "%.3f AU (%.1fM km)", b.semiMajorAxisAU, (b.semiMajorAxisM * 1e-9));
            b.semiMajorAxisStr = smaBuf;
            snprintf(eccBuf, sizeof(eccBuf), "%.4f", b.eccentricity);
            b.eccentricityStr = eccBuf;
            snprintf(periBuf, sizeof(periBuf), "%.3f AU", b.periapsisAU);
            b.periapsisStr = periBuf;
            snprintf(apoBuf, sizeof(apoBuf), "%.3f AU", b.apoapsisAU);
            b.apoapsisStr = apoBuf;

            // Einstein General Relativistic Perihelion Precession (arcsec / century)
            if (b.semiMajorAxisM > 0.0 && (1.0 - b.eccentricity * b.eccentricity) > 0.0) {
                double dSigmaRadPerOrbit = (6.0 * PI_DBL * G_CONST * solMass) / (C_LIGHT_SQ * b.semiMajorAxisM * (1.0 - b.eccentricity * b.eccentricity));
                double orbitalPeriodSec = 2.0 * PI_DBL * std::sqrt(std::pow(b.semiMajorAxisM, 3.0) / muTotal);
                double orbitsPerCentury = (100.0 * 365.25 * SEC_PER_DAY) / orbitalPeriodSec;
                b.grPrecessionArcsecCentury = dSigmaRadPerOrbit * orbitsPerCentury * (180.0 / PI_DBL) * 3600.0;

                char grBuf[64];
                snprintf(grBuf, sizeof(grBuf), "+%.2f\"/century", b.grPrecessionArcsecCentury);
                b.grPrecessionStr = grBuf;
            }

            // Current True Anomaly nu
            double cosNu = glm::dot(eVec, rVec) / (b.eccentricity * rM);
            cosNu = std::clamp(cosNu, -1.0, 1.0);
            double nuRad = std::acos(cosNu);
            if (glm::dot(rVec, vVec) < 0.0) nuRad = 2.0 * PI_DBL - nuRad;
            b.trueAnomalyDeg = nuRad * (180.0 / PI_DBL);

            char nuBuf[64];
            snprintf(nuBuf, sizeof(nuBuf), "%.1f°", b.trueAnomalyDeg);
            b.trueAnomalyStr = nuBuf;

            // 3D Perifocal Unit Vectors & Dynamic Keplerian Orbit Curve
            b.angularMomentumVec = hVec;
            b.eccentricityVec = eVec;

            glm::dvec3 P_hat(1.0, 0.0, 0.0);
            if (b.eccentricity > 1e-6) {
                P_hat = eVec / b.eccentricity;
            } else if (rM > 1e-6) {
                P_hat = rVec / rM;
            }
            b.perifocalP = P_hat;

            glm::dvec3 W_hat(0.0, 1.0, 0.0);
            double hLen = glm::length(hVec);
            if (hLen > 1e-6) {
                W_hat = hVec / hLen;
            }

            glm::dvec3 Q_hat = glm::cross(W_hat, P_hat);
            double qLen = glm::length(Q_hat);
            if (qLen > 1e-6) {
                Q_hat /= qLen;
            } else {
                Q_hat = glm::dvec3(0.0, 0.0, 1.0);
            }
            b.perifocalQ = Q_hat;

            // Generate full dynamic Keplerian ellipse (updating in real time with physical forces)
            b.dynamicOrbitCurve.clear();
            const int numOrbitSegments = 160;
            double pSemiLatusM = b.semiMajorAxisM * (1.0 - b.eccentricity * b.eccentricity);
            if (pSemiLatusM > 0.0) {
                b.dynamicOrbitCurve.reserve(numOrbitSegments + 1);
                for (int s = 0; s <= numOrbitSegments; ++s) {
                    double nu = (2.0 * PI_DBL * (double)s) / (double)numOrbitSegments;
                    double r_nu_M = pSemiLatusM / (1.0 + b.eccentricity * std::cos(nu));
                    glm::dvec3 posOrbM = r_nu_M * (std::cos(nu) * P_hat + std::sin(nu) * Q_hat);
                    glm::vec3 posOrbAU = glm::vec3((float)(posOrbM.x / AU_METERS),
                                                   (float)(posOrbM.y / AU_METERS),
                                                   (float)(posOrbM.z / AU_METERS));
                    b.dynamicOrbitCurve.push_back(posOrbAU);
                }
            }
        }
    }
}

void PhysicsEngine::updateRingHydrodynamics(double deltaSeconds) {
    for (auto& body : m_bodies) {
        if (!body.ring.hasRing || body.ring.disturbances.empty()) continue;

        double muBody = G_CONST * body.massKg;
        for (auto it = body.ring.disturbances.begin(); it != body.ring.disturbances.end(); ) {
            it->ageSeconds += (float)deltaSeconds;
            it->intensity *= std::exp(-it->decayRate * (float)deltaSeconds);

            // Keplerian shear along azimuthal angle
            double currentRadiusM = body.ring.innerRadiusM + it->normRadius * (body.ring.outerRadiusM - body.ring.innerRadiusM);
            double omega = std::sqrt(muBody / std::pow(currentRadiusM, 3.0));
            it->azimuthRad += (float)(omega * deltaSeconds);
            it->azimuthRad = std::fmod(it->azimuthRad, (float)(2.0 * PI_DBL));

            if (it->intensity < 0.01f || it->ageSeconds > 300.0f) {
                it = body.ring.disturbances.erase(it);
            } else {
                ++it;
            }
        }
    }
}

void PhysicsEngine::computeSystemConservationStats() {
    size_t n = m_bodies.size();
    if (n == 0) return;

    double kineticTotal = 0.0;
    double potentialTotal = 0.0;
    glm::dvec3 totalAngMom(0.0);

    for (size_t i = 0; i < n; ++i) {
        double vSq = glm::dot(m_bodies[i].velocityMps, m_bodies[i].velocityMps);
        kineticTotal += 0.5 * m_bodies[i].massKg * vSq;

        for (size_t j = i + 1; j < n; ++j) {
            double r = glm::distance(m_bodies[i].positionM, m_bodies[j].positionM);
            if (r > 1000.0) {
                potentialTotal -= (G_CONST * m_bodies[i].massKg * m_bodies[j].massKg) / r;
            }
        }

        totalAngMom += m_bodies[i].massKg * glm::cross(m_bodies[i].positionM, m_bodies[i].velocityMps);
    }

    m_totalSystemKineticJ = kineticTotal;
    m_totalSystemPotentialJ = potentialTotal;
    m_totalSystemEnergyJ = kineticTotal + potentialTotal;
    m_totalSystemAngularMomentum = glm::length(totalAngMom);

    if (m_initialSystemEnergyJ == 0.0) {
        m_initialSystemEnergyJ = m_totalSystemEnergyJ;
    } else {
        m_energyConservationDriftPct = std::abs((m_totalSystemEnergyJ - m_initialSystemEnergyJ) / m_initialSystemEnergyJ) * 100.0;
    }
}

void PhysicsEngine::update(float deltaTime) {
    if (m_isPaused) return;

    // Advance real and simulated clocks
    m_realTimeElapsedSeconds += deltaTime;
    double dtSim = (double)deltaTime * (double)m_timeScale;
    m_simulatedTimeSeconds += dtSim;

    // Numerical Substepping for Symplectic Conservation
    const double maxSubstepSec = 1800.0; // 30 minutes max per substep
    int substeps = (int)std::ceil(std::abs(dtSim) / maxSubstepSec);
    substeps = std::clamp(substeps, 1, 64);
    double stepSize = dtSim / (double)substeps;

    for (int s = 0; s < substeps; ++s) {
        integrateNBody(stepSize);
    }

    // Dynamic physical stats & conservation
    updatePhysicalQuantities();
    updateRingHydrodynamics(dtSim);
    computeSystemConservationStats();

    // Advance 3D Axial Rotations
    for (auto& b : m_bodies) {
        b.rotationAngle += (float)(b.rotationSpeedRadPerSec * dtSim);
        b.rotationAngle = std::fmod(b.rotationAngle, (float)(2.0 * PI_DBL));
    }

    // Step Particle System (N-body gravitational perturbations & drag)
    m_particleSystem.update(dtSim, m_bodies, m_enableGeneralRelativity, m_simulatedTimeSeconds);

    // Step Deformable Matter System
    m_matterSystem.update(dtSim, m_bodies, m_enableGeneralRelativity);
}

void PhysicsEngine::updateBodyScales() {
    double minPlanetOrbitAU = 1.0;
    for (const auto& b : m_bodies) {
        if (b.id != "sol" && b.type.find("Star") == std::string::npos) {
            double r = (b.realOrbitRadiusAU > 0.0) ? b.realOrbitRadiusAU : (b.semiMajorAxisAU > 0.0 ? b.semiMajorAxisAU : (double)glm::length(b.position));
            if (r > 0.0001) minPlanetOrbitAU = std::min(minPlanetOrbitAU, r);
        }
    }

    float systemVisualScale = (minPlanetOrbitAU < 0.2) ? (float)(minPlanetOrbitAU / 0.35) : 1.0f;
    systemVisualScale = std::clamp(systemVisualScale, 0.08f, 1.0f);

    for (auto& b : m_bodies) {
        if (m_isTrueScaleMode) {
            b.radius3D = (float)(b.realRadiusAU * m_sizeMultiplier);
            if (b.radius3D < 0.00001f) b.radius3D = 0.00001f;
        } else {
            // Enhanced visual exaggeration scale
            if (b.id == "sol" || b.type.find("Star") != std::string::npos) {
                b.radius3D = 0.15f * m_sizeMultiplier * systemVisualScale;
                if (b.radius3D < 0.006f) b.radius3D = 0.006f;
            } else if (b.id == "jupiter" || b.id == "saturn") {
                b.radius3D = 0.06f * m_sizeMultiplier * systemVisualScale;
            } else if (b.id == "uranus" || b.id == "neptune") {
                b.radius3D = 0.045f * m_sizeMultiplier * systemVisualScale;
            } else if (b.type.find("Asteroid") != std::string::npos) {
                b.radius3D = 0.015f * m_sizeMultiplier * systemVisualScale;
            } else {
                b.radius3D = 0.03f * m_sizeMultiplier * systemVisualScale;
            }
        }

        if (b.ring.hasRing) {
            b.ring.innerRadius3D = (float)(b.ring.innerRadiusAU * (m_isTrueScaleMode ? m_sizeMultiplier : 1.0f));
            b.ring.outerRadius3D = (float)(b.ring.outerRadiusAU * (m_isTrueScaleMode ? m_sizeMultiplier : 1.0f));
        }
    }
}

void PhysicsEngine::selectBody(int index) {
    if (index >= 0 && index < (int)m_bodies.size()) {
        m_selectedBodyIndex = index;
    }
}

void PhysicsEngine::selectBodyById(const std::string& id) {
    for (size_t i = 0; i < m_bodies.size(); ++i) {
        if (m_bodies[i].id == id || m_bodies[i].name == id) {
            m_selectedBodyIndex = (int)i;
            return;
        }
    }
}

const CelestialBody& PhysicsEngine::getSelectedBody() const {
    if (!m_bodies.empty()) {
        if (m_selectedBodyIndex >= 0 && m_selectedBodyIndex < (int)m_bodies.size()) {
            return m_bodies[m_selectedBodyIndex];
        }
        return m_bodies[0];
    }
    static CelestialBody dummy;
    return dummy;
}

void PhysicsEngine::clearTrails() {
    for (auto& b : m_bodies) {
        b.trailHistory.clear();
    }
}

void PhysicsEngine::triggerRingImpact(const std::string& planetId, float normRadius, float azimuthRad, float impactRadiusM) {
    for (auto& b : m_bodies) {
        if (b.id == planetId && b.ring.hasRing) {
            RingDisturbance dist;
            dist.normRadius = std::clamp(normRadius, 0.05f, 0.95f);
            dist.azimuthRad = azimuthRad;
            dist.radialWidth = (float)(impactRadiusM / (b.ring.outerRadiusM - b.ring.innerRadiusM));
            dist.radialWidth = std::clamp(dist.radialWidth, 0.02f, 0.25f);
            dist.angularWidth = 0.18f;
            dist.intensity = 1.0f;
            dist.ageSeconds = 0.0f;
            dist.decayRate = 0.015f;

            if (b.ring.disturbances.size() >= PlanetaryRing::MAX_DISTURBANCES) {
                b.ring.disturbances.erase(b.ring.disturbances.begin());
            }
            b.ring.disturbances.push_back(dist);
            return;
        }
    }
}

void PhysicsEngine::triggerSaturnRingImpact() {
    float normR = 0.2f + 0.6f * ((float)rand() / (float)RAND_MAX);
    float azRad = ((float)rand() / (float)RAND_MAX) * (float)(2.0 * PI_DBL);
    triggerRingImpact("saturn", normR, azRad, 5000000.0f);
}

void PhysicsEngine::stepFrameForward() {
    update(1.0f / 60.0f);
}

void PhysicsEngine::stepFrameBackward() {
    float savedScale = m_timeScale;
    m_timeScale = -std::abs(m_timeScale);
    update(1.0f / 60.0f);
    m_timeScale = savedScale;
}

std::string PhysicsEngine::getSimulationTimeStr() const {
    double days = m_simulatedTimeSeconds / SEC_PER_DAY;
    if (std::abs(days) >= 365.25) {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.2f yr", days / 365.25);
        return std::string(buf);
    } else {
        char buf[64];
        snprintf(buf, sizeof(buf), "%.1f days", days);
        return std::string(buf);
    }
}

std::string PhysicsEngine::getSimVsRealTimeStr() const {
    char buf[64];
    if (m_timeScale >= 86400.0f) {
        snprintf(buf, sizeof(buf), "%.1f days/sec", m_timeScale / 86400.0f);
    } else if (m_timeScale >= 3600.0f) {
        snprintf(buf, sizeof(buf), "%.1f hrs/sec", m_timeScale / 3600.0f);
    } else {
        snprintf(buf, sizeof(buf), "%.0fx", m_timeScale);
    }
    return std::string(buf);
}

std::string PhysicsEngine::getTotalEnergyStr() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3e J", m_totalSystemEnergyJ);
    return std::string(buf);
}

std::string PhysicsEngine::getTotalAngularMomentumStr() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3e kg·m²/s", m_totalSystemAngularMomentum);
    return std::string(buf);
}

} // namespace AstroGenesis
