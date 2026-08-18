#include "simulation/PhysicsEngine.hpp"
#include <ctime>
#include <cstdio>
#include <cmath>
#include <algorithm>

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
    initializeDefaultSolarSystem();
}

void PhysicsEngine::initializeDefaultSolarSystem() {
    m_bodies.clear();

    // 1. Sol (Sun)
    CelestialBody sol;
    sol.id = "sol"; sol.name = "Sol"; sol.type = "G2V Star";
    sol.massKg = 1.9885e30;
    sol.radiusM = 696340000.0; // 696,340 km
    sol.luminosityW = L_SUN;
    sol.albedo = 0.0;
    sol.greenhouseK = 0.0;
    sol.moons = 8;
    sol.axialTiltDeg = 7.25f;
    sol.axialTiltStr = "7.25°";
    sol.rotationPeriodHours = 609.12; // 25.38 days
    sol.rotationPeriodStr = "25d 9h";
    sol.atmosphereStr = "73.46% H₂, 24.85% He";
    sol.pressureStr = "N/A";
    sol.magneticFieldStr = "100–300 µT";
    sol.auroraActivityStr = "None";
    sol.orbitalPeriodStr = "N/A";
    sol.yearLengthStr = "N/A";
    sol.composition = { {"Hydrogen", 73.46f, {1.0f, 0.8f, 0.2f, 1.0f}}, {"Helium", 24.85f, {0.9f, 0.5f, 0.1f, 1.0f}}, {"Oxygen", 0.77f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Carbon", 0.29f, {0.5f, 0.5f, 0.5f, 1.0f}} };
    sol.positionM = glm::dvec3(0.0);
    sol.velocityMps = glm::dvec3(0.0);
    sol.color = glm::vec3(1.0f, 0.75f, 0.2f);
    sol.realRadiusAU = sol.radiusM / AU_METERS;
    sol.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (sol.rotationPeriodHours * 3600.0);
    m_bodies.push_back(sol);

    // Lambda to configure planet initial physical orbital state (perihelion velocity)
    auto addPlanet = [&](const std::string& id, const std::string& name, const std::string& type,
                         double massKg, double radiusM, double semiMajorAU, double eccentricity,
                         double initialAngleRad, double albedo, double greenhouseK,
                         float axialTiltDeg, const std::string& axialTiltStr,
                         double rotPeriodHours, const std::string& rotPeriodStr,
                         const std::string& atmStr, const std::string& pressStr,
                         const std::string& magStr, const std::string& auroraStr,
                         int moons, const std::vector<CompositionItem>& comp,
                         const glm::vec3& color, const std::string& texPath) {
        CelestialBody b;
        b.id = id; b.name = name; b.type = type;
        b.massKg = massKg;
        b.radiusM = radiusM;
        b.albedo = albedo;
        b.greenhouseK = greenhouseK;
        b.axialTiltDeg = axialTiltDeg;
        b.axialTiltStr = axialTiltStr;
        b.rotationPeriodHours = rotPeriodHours;
        b.rotationPeriodStr = rotPeriodStr;
        b.atmosphereStr = atmStr;
        b.pressureStr = pressStr;
        b.magneticFieldStr = magStr;
        b.auroraActivityStr = auroraStr;
        b.moons = moons;
        b.composition = comp;
        b.color = color;
        b.texturePath = texPath;
        b.realRadiusAU = radiusM / AU_METERS;
        b.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (std::abs(rotPeriodHours) * 3600.0);
        if (rotPeriodHours < 0.0) b.rotationSpeedRadPerSec = -b.rotationSpeedRadPerSec;

        // Calculate initial Keplerian/relativistic state vector at perihelion (or specified true anomaly)
        double aM = semiMajorAU * AU_METERS;
        double e = eccentricity;
        double rM = aM * (1.0 - e); // Perihelion distance in meters
        double vPerihelion = std::sqrt((G_CONST * (sol.massKg + massKg) / aM) * ((1.0 + e) / (1.0 - e)));

        // Position & velocity vectors oriented by initial angle
        double cosA = std::cos(initialAngleRad);
        double sinA = std::sin(initialAngleRad);
        b.positionM = glm::dvec3(rM * cosA, 0.0, rM * sinA);
        b.velocityMps = glm::dvec3(-vPerihelion * sinA, 0.0, vPerihelion * cosA);

        b.position = glm::vec3((float)(b.positionM.x / AU_METERS), 0.0f, (float)(b.positionM.z / AU_METERS));
        b.velocity = glm::vec3((float)(b.velocityMps.x / AU_METERS), 0.0f, (float)(b.velocityMps.z / AU_METERS));

        // Theoretical period (Kepler's Third Law)
        b.orbitalPeriodDays = (2.0 * PI_DBL * std::sqrt(std::pow(aM, 3.0) / (G_CONST * sol.massKg))) / SEC_PER_DAY;
        char yearBuf[64];
        if (b.orbitalPeriodDays >= 365.25 * 1.5) {
            snprintf(yearBuf, sizeof(yearBuf), "%.2f years", b.orbitalPeriodDays / 365.256);
        } else {
            snprintf(yearBuf, sizeof(yearBuf), "%.2f days", b.orbitalPeriodDays);
        }
        b.orbitalPeriodStr = yearBuf;
        b.yearLengthStr = yearBuf;

        char radBuf[64], massBuf[64];
        snprintf(radBuf, sizeof(radBuf), "%'.1f km", radiusM / 1000.0);
        b.radiusStr = radBuf;
        if (massKg > 1.0e26) snprintf(massBuf, sizeof(massBuf), "%.3f × 10²⁶ kg", massKg / 1.0e26);
        else if (massKg > 1.0e24) snprintf(massBuf, sizeof(massBuf), "%.3f × 10²⁴ kg", massKg / 1.0e24);
        else snprintf(massBuf, sizeof(massBuf), "%.3f × 10²³ kg", massKg / 1.0e23);
        b.massStr = massBuf;

        m_bodies.push_back(b);
    };

    // 2. Mercury
    addPlanet("mercury", "Mercury", "Terrestrial Planet",
              3.3011e23, 2439700.0, 0.387098, 0.205630, 0.2,
              0.088, 0.0, 0.034f, "0.034°", 1407.5, "58d 15h",
              "42% O₂, 29% Na, 22% H₂", "10⁻¹⁴ kPa", "0.3 µT", "None", 0,
              { {"Oxygen", 42.0f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Sodium", 29.0f, {0.9f, 0.6f, 0.1f, 1.0f}}, {"Hydrogen", 22.0f, {0.4f, 0.7f, 1.0f, 1.0f}} },
              glm::vec3(0.7f, 0.6f, 0.5f), "assets/textures/mercury_surface.jpg");

    // 3. Venus
    addPlanet("venus", "Venus", "Terrestrial Planet",
              4.8675e24, 6051800.0, 0.723332, 0.006772, 1.1,
              0.760, 480.0, 177.36f, "177.36°", -5832.6, "243d 0h",
              "96.5% CO₂, 3.5% N₂", "9,200 kPa", "Induced", "Weak", 0,
              { {"Carbon Dioxide", 96.5f, {0.9f, 0.3f, 0.3f, 1.0f}}, {"Nitrogen", 3.5f, {0.0f, 0.7f, 0.9f, 1.0f}} },
              glm::vec3(0.9f, 0.7f, 0.3f), "assets/textures/venus_surface.jpg");

    // 4. Earth
    addPlanet("earth", "Earth", "Terrestrial Planet",
              5.9722e24, 6371000.0, 1.000000, 0.0167086, 2.4,
              0.306, 33.0, 23.44f, "23.44°", 23.93446, "23h 56m",
              "78% N₂, 21% O₂", "101.3 kPa", "25.0–65.0 µT", "Moderate", 1,
              { {"Nitrogen", 78.08f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Oxygen", 20.95f, {0.0f, 0.85f, 0.4f, 1.0f}}, {"Argon", 0.93f, {0.94f, 0.75f, 0.12f, 1.0f}}, {"Carbon Dioxide", 0.04f, {0.86f, 0.31f, 0.31f, 1.0f}} },
              glm::vec3(0.0f, 0.83f, 1.0f), "assets/textures/earth_daymap.jpg");

    // 5. Mars
    addPlanet("mars", "Mars", "Terrestrial Planet",
              6.4171e23, 3389500.0, 1.523679, 0.093400, 3.6,
              0.250, 5.0, 25.19f, "25.19°", 24.6229, "24h 37m",
              "95.3% CO₂, 2.6% N₂", "0.61 kPa", "Remnant", "Localized", 2,
              { {"Carbon Dioxide", 95.32f, {0.9f, 0.3f, 0.2f, 1.0f}}, {"Nitrogen", 2.6f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Argon", 1.9f, {0.9f, 0.7f, 0.1f, 1.0f}} },
              glm::vec3(0.95f, 0.35f, 0.2f), "assets/textures/mars_surface.jpg");

    // 6. Jupiter
    addPlanet("jupiter", "Jupiter", "Gas Giant",
              1.8982e27, 69911000.0, 5.204400, 0.048900, 4.5,
              0.503, 0.0, 3.13f, "3.13°", 9.925, "9h 55m",
              "89% H₂, 10% He", "100 kPa", "420 µT", "Very High", 95,
              { {"Hydrogen", 89.8f, {0.9f, 0.8f, 0.6f, 1.0f}}, {"Helium", 10.2f, {0.9f, 0.6f, 0.3f, 1.0f}} },
              glm::vec3(0.85f, 0.65f, 0.45f), "assets/textures/jupiter_surface.jpg");

    // 7. Saturn
    addPlanet("saturn", "Saturn", "Gas Giant",
              5.6834e26, 58232000.0, 9.582600, 0.056500, 5.2,
              0.342, 0.0, 26.73f, "26.73°", 10.656, "10h 39m",
              "96% H₂, 3% He", "100 kPa", "21 µT", "High", 146,
              { {"Hydrogen", 96.3f, {0.9f, 0.85f, 0.5f, 1.0f}}, {"Helium", 3.2f, {0.9f, 0.7f, 0.4f, 1.0f}} },
              glm::vec3(0.9f, 0.8f, 0.5f), "assets/textures/saturn_surface.jpg");

    // 8. Uranus
    addPlanet("uranus", "Uranus", "Ice Giant",
              8.6810e25, 25362000.0, 19.20120, 0.047170, 5.8,
              0.300, 0.0, 97.77f, "97.77°", -17.24, "17h 14m",
              "83% H₂, 15% He, 2.3% CH₄", "100 kPa", "23 µT", "Moderate", 28,
              { {"Hydrogen", 83.0f, {0.3f, 0.8f, 0.9f, 1.0f}}, {"Helium", 15.0f, {0.5f, 0.7f, 0.9f, 1.0f}}, {"Methane", 2.3f, {0.1f, 0.5f, 0.8f, 1.0f}} },
              glm::vec3(0.5f, 0.8f, 0.9f), "assets/textures/uranus_surface.jpg");

    // 9. Neptune
    addPlanet("neptune", "Neptune", "Ice Giant",
              1.02413e26, 24622000.0, 30.04720, 0.008678, 0.5,
              0.290, 0.0, 28.32f, "28.32°", 16.11, "16h 6m",
              "80% H₂, 19% He, 1.5% CH₄", "100 kPa", "14 µT", "Moderate", 16,
              { {"Hydrogen", 80.0f, {0.1f, 0.3f, 0.9f, 1.0f}}, {"Helium", 19.0f, {0.3f, 0.5f, 0.9f, 1.0f}}, {"Methane", 1.5f, {0.0f, 0.2f, 0.7f, 1.0f}} },
              glm::vec3(0.2f, 0.4f, 0.9f), "assets/textures/neptune_surface.jpg");

    // Configure Saturn's planetary ring physical system
    for (auto& b : m_bodies) {
        if (b.id == "saturn") {
            b.ring.hasRing = true;
            b.ring.innerRadiusM = 74500000.0;  // 74,500 km
            b.ring.outerRadiusM = 140220000.0; // 140,220 km
            b.ring.innerRadiusAU = b.ring.innerRadiusM / AU_METERS;
            b.ring.outerRadiusAU = b.ring.outerRadiusM / AU_METERS;
            b.ring.baseColor = glm::vec3(0.88f, 0.82f, 0.70f);
        }
    }

    // 10. Transform into Solar System Barycentric Frame (Center of Mass & Zero Total Momentum)
    // Scientifically standard (NASA JPL Barycentric Frame): Net momentum P_total = 0
    // and Center of Mass R_COM = 0 guarantees the Solar System never drifts through space.
    double totalMass = 0.0;
    glm::dvec3 centerOfMassM(0.0);
    glm::dvec3 totalMomentumMps(0.0);

    for (const auto& b : m_bodies) {
        totalMass += b.massKg;
        centerOfMassM += b.massKg * b.positionM;
        totalMomentumMps += b.massKg * b.velocityMps;
    }

    glm::dvec3 vBarycenter = totalMomentumMps / totalMass;
    glm::dvec3 rBarycenter = centerOfMassM / totalMass;

    for (auto& b : m_bodies) {
        b.positionM -= rBarycenter;
        b.velocityMps -= vBarycenter;
        b.position = glm::vec3((float)(b.positionM.x / AU_METERS), 0.0f, (float)(b.positionM.z / AU_METERS));
        b.velocity = glm::vec3((float)(b.velocityMps.x / AU_METERS), 0.0f, (float)(b.velocityMps.z / AU_METERS));
    }

    m_selectedBodyIndex = 3; // Earth default
    updateBodyScales();
    updatePhysicalQuantities();

    // Pre-seed smooth 3D trails along initial orbits relative to the barycenter
    for (auto& body : m_bodies) {
        if (body.id != "sol") {
            body.trailHistory.clear();
            const int initialSteps = 240;
            double r = glm::length(body.positionM) / AU_METERS;
            double curAngle = std::atan2(body.positionM.z, body.positionM.x);

            for (int s = initialSteps; s >= 0; --s) {
                double angle = curAngle - (2.0 * PI_DBL * (double)s / (double)initialSteps);
                float px = (float)(r * std::cos(angle));
                float pz = (float)(r * std::sin(angle));
                body.trailHistory.push_back(glm::vec3(px, 0.0f, pz));
            }
        }
    }

    // Initialize Particle System (Main Asteroid Belt + Extensible Particle Fields)
    m_particleSystem.initializeDefaultSystem();
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

            glm::dvec3 pos_j = positions[j];
            glm::dvec3 r_ij = pos_j - pos_i; // Vector from body i towards body j
            double dist_sq = glm::dot(r_ij, r_ij);
            double dist = std::sqrt(dist_sq);
            if (dist < 1000.0) continue; // Softening to prevent singularity

            double dist_cubed = dist_sq * dist;
            double GM_j = G_CONST * m_bodies[j].massKg;

            // 1. Classical Newtonian Gravitational Acceleration
            glm::dvec3 a_newton = (GM_j / dist_cubed) * r_ij;
            outAccelerations[i] += a_newton;

            // 2. Einstein General Relativity 1PN (Post-Newtonian) Correction
            // EIH Post-Newtonian term producing Mercury perihelion precession & frame-dragging
            if (m_enableGeneralRelativity) {
                double r_dot_v = glm::dot(r_ij, vel_i);
                double gr_scalar = (4.0 * GM_j / dist) - v_i_sq;
                glm::dvec3 a_gr = (GM_j / (C_LIGHT_SQ * dist_cubed)) * (gr_scalar * r_ij + 4.0 * r_dot_v * vel_i);
                outAccelerations[i] += a_gr;
            }
        }
    }
}

void PhysicsEngine::integrateNBody(double deltaSeconds) {
    if (deltaSeconds == 0.0) return;

    // Sub-stepping for symplectic stability: max 3600s (1 hr) per sub-step
    double maxSubDt = 3600.0;
    int numSubSteps = std::max(1, (int)std::ceil(std::abs(deltaSeconds) / maxSubDt));
    numSubSteps = std::min(numSubSteps, 400); // Cap for 60fps real-time interactive rendering
    double subDt = deltaSeconds / (double)numSubSteps;

    size_t n = m_bodies.size();
    std::vector<glm::dvec3> positions(n), velocities(n), accelerations(n);
    for (size_t i = 0; i < n; ++i) {
        positions[i] = m_bodies[i].positionM;
        velocities[i] = m_bodies[i].velocityMps;
    }

    computeAccelerations(positions, velocities, accelerations);

    for (int step = 0; step < numSubSteps; ++step) {
        // Velocity-Verlet Step 1: v(t + dt/2) = v(t) + 0.5 * a(t) * dt
        for (size_t i = 0; i < n; ++i) {
            velocities[i] += 0.5 * accelerations[i] * subDt;
        }

        // Velocity-Verlet Step 2: x(t + dt) = x(t) + v(t + dt/2) * dt
        for (size_t i = 0; i < n; ++i) {
            positions[i] += velocities[i] * subDt;
        }

        // Velocity-Verlet Step 3: Compute new accelerations a(t + dt)
        computeAccelerations(positions, velocities, accelerations);

        // Velocity-Verlet Step 4: v(t + dt) = v(t + dt/2) + 0.5 * a(t + dt) * dt
        for (size_t i = 0; i < n; ++i) {
            velocities[i] += 0.5 * accelerations[i] * subDt;
        }

        // Record dynamic sub-step trail points for perfectly continuous curves at high speeds
        if (numSubSteps > 1) {
            for (size_t i = 1; i < n; ++i) {
                glm::vec3 subPos(
                    (float)(positions[i].x / AU_METERS),
                    (float)(positions[i].y / AU_METERS),
                    (float)(positions[i].z / AU_METERS)
                );
                if (m_bodies[i].trailHistory.empty() || glm::distance(m_bodies[i].trailHistory.back(), subPos) > 0.0003f) {
                    m_bodies[i].trailHistory.push_back(subPos);
                    while (m_bodies[i].trailHistory.size() > m_bodies[i].maxTrailPoints) {
                        m_bodies[i].trailHistory.pop_front();
                    }
                }
            }
        }
    }

    // Barycentric zero-drift preservation (astrophysical conservation of linear momentum)
    double totalMass = 0.0;
    glm::dvec3 comPos(0.0);
    glm::dvec3 comVel(0.0);
    for (size_t i = 0; i < n; ++i) {
        totalMass += m_bodies[i].massKg;
        comPos += m_bodies[i].massKg * positions[i];
        comVel += m_bodies[i].massKg * velocities[i];
    }
    glm::dvec3 driftPos = comPos / totalMass;
    glm::dvec3 driftVel = comVel / totalMass;

    // Commit physical state & map to AU visualization coordinates
    for (size_t i = 0; i < n; ++i) {
        positions[i] -= driftPos;
        velocities[i] -= driftVel;

        m_bodies[i].positionM = positions[i];
        m_bodies[i].velocityMps = velocities[i];
        m_bodies[i].accelerationMps2 = accelerations[i];

        // AU coordinates for rendering
        m_bodies[i].position = glm::vec3(
            (float)(positions[i].x / AU_METERS),
            (float)(positions[i].y / AU_METERS),
            (float)(positions[i].z / AU_METERS)
        );
        m_bodies[i].velocity = glm::vec3(
            (float)(velocities[i].x / AU_METERS),
            (float)(velocities[i].y / AU_METERS),
            (float)(velocities[i].z / AU_METERS)
        );
    }
}

void PhysicsEngine::updatePhysicalQuantities() {
    glm::dvec3 solPosM = m_bodies.empty() ? glm::dvec3(0.0) : m_bodies[0].positionM;
    double solMass = m_bodies.empty() ? 1.989e30 : m_bodies[0].massKg;

    for (size_t i = 0; i < m_bodies.size(); ++i) {
        auto& body = m_bodies[i];

        // 1. Real-time Distance to Sol
        glm::dvec3 relToSol = body.positionM - solPosM;
        double distM = glm::length(relToSol);
        body.distanceKm = distM / 1000.0;
        body.distanceAU = distM / AU_METERS;
        body.realOrbitRadiusAU = body.distanceAU;

        if (body.id == "sol") {
            body.distanceStr = "0.00 AU";
            body.orbitalSpeedStr = "0.00 km/s";
        } else {
            char distBuf[64];
            if (body.distanceAU < 0.1) {
                snprintf(distBuf, sizeof(distBuf), "%.4f AU", body.distanceAU);
            } else if (body.distanceAU < 10.0) {
                snprintf(distBuf, sizeof(distBuf), "%.3f AU", body.distanceAU);
            } else {
                snprintf(distBuf, sizeof(distBuf), "%.2f AU", body.distanceAU);
            }
            body.distanceStr = distBuf;
        }

        // 2. Real-time Orbital Velocity
        double speedMps = glm::length(body.velocityMps);
        body.orbitalSpeedKmpS = speedMps / 1000.0;
        char spdBuf[64];
        snprintf(spdBuf, sizeof(spdBuf), "%.2f km/s", body.orbitalSpeedKmpS);
        body.orbitalSpeedStr = spdBuf;

        // 3. Real-time Surface Gravity: g = GM / R^2
        if (body.radiusM > 0.0) {
            body.surfaceGravityMps2 = (G_CONST * body.massKg) / (body.radiusM * body.radiusM);
            char gravBuf[64];
            snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s² (%.2fg)", body.surfaceGravityMps2, body.surfaceGravityMps2 / 9.80665);
            body.gravityStr = gravBuf;

            // 4. Real-time Escape Velocity: v_esc = sqrt(2GM / R)
            body.escapeVelocityKmpS = std::sqrt(2.0 * G_CONST * body.massKg / body.radiusM) / 1000.0;
            char escBuf[64];
            snprintf(escBuf, sizeof(escBuf), "%.2f km/s", body.escapeVelocityKmpS);
            body.escapeVelocityStr = escBuf;

            // 5. Mean Density: rho = M / (4/3 * pi * R^3)
            double volM3 = (4.0 / 3.0) * PI_DBL * std::pow(body.radiusM, 3.0);
            body.meanDensityKgM3 = body.massKg / volM3;
            char denBuf[64];
            snprintf(denBuf, sizeof(denBuf), "%.0f kg/m³", body.meanDensityKgM3);
            body.densityStr = denBuf;

            // 6. Surface Area: A = 4 * pi * R^2
            body.surfaceAreaKm2 = (4.0 * PI_DBL * std::pow(body.radiusM / 1000.0, 2.0));
            char areaBuf[64];
            if (body.surfaceAreaKm2 > 1.0e9) {
                snprintf(areaBuf, sizeof(areaBuf), "%.2f B km²", body.surfaceAreaKm2 / 1.0e9);
            } else if (body.surfaceAreaKm2 > 1.0e6) {
                snprintf(areaBuf, sizeof(areaBuf), "%.1f M km²", body.surfaceAreaKm2 / 1.0e6);
            } else {
                snprintf(areaBuf, sizeof(areaBuf), "%.0f km²", body.surfaceAreaKm2);
            }
            body.surfaceAreaStr = areaBuf;
        }

        // 7. Real-time Solar Radiation Flux & Surface Temperature (Stefan-Boltzmann Law)
        if (body.id == "sol") {
            body.solarRadiationFlux = 6.33e7;
            body.surfaceTempK = 5778.0;
            body.tempStr = "5,778 K (5,505 °C)";
            body.solarRadiationStr = "6.33 × 10⁷ W/m²";
            body.radLevelStr = "Extreme";
        } else {
            if (distM > 0.0) {
                // F = L_sun / (4 * pi * r^2)
                body.solarRadiationFlux = L_SUN / (4.0 * PI_DBL * distM * distM);
                char radBuf[64];
                if (body.solarRadiationFlux >= 10000.0) {
                    snprintf(radBuf, sizeof(radBuf), "%.0f W/m²", body.solarRadiationFlux);
                } else if (body.solarRadiationFlux >= 10.0) {
                    snprintf(radBuf, sizeof(radBuf), "%.1f W/m²", body.solarRadiationFlux);
                } else {
                    snprintf(radBuf, sizeof(radBuf), "%.2f W/m²", body.solarRadiationFlux);
                }
                body.solarRadiationStr = radBuf;

                if (body.solarRadiationFlux > 5000.0) body.radLevelStr = "Extreme";
                else if (body.solarRadiationFlux > 1500.0) body.radLevelStr = "Very High";
                else if (body.solarRadiationFlux > 800.0) body.radLevelStr = "High";
                else if (body.solarRadiationFlux > 100.0) body.radLevelStr = "Moderate";
                else body.radLevelStr = "Low";

                // Equilibrium Temperature: T_eq = (( (1 - A) * F ) / (4 * sigma))^0.25 + T_greenhouse
                double t_eq = std::pow(((1.0 - body.albedo) * body.solarRadiationFlux) / (4.0 * SIGMA_SB), 0.25);
                body.surfaceTempK = t_eq + body.greenhouseK;
                double tempC = body.surfaceTempK - 273.15;
                char tempBuf[64];
                snprintf(tempBuf, sizeof(tempBuf), "%.0f K (%.0f °C)", body.surfaceTempK, tempC);
                body.tempStr = tempBuf;
            }
        }

        // 8. General Relativistic Time Dilation & Gravitational Redshift
        if (body.id != "sol" && distM > 0.0) {
            // dt_proper / dt_coord = 1 - (GM_sun / (c^2 * r)) - (v^2 / (2 * c^2))
            double gravPotentialShift = (G_CONST * solMass) / (C_LIGHT_SQ * distM);
            double kinematicShift = (speedMps * speedMps) / (2.0 * C_LIGHT_SQ);
            body.timeDilationShift = gravPotentialShift + kinematicShift;
            // Shift in microseconds per Earth day (86,400 s)
            body.timeDriftMicrosecPerDay = -body.timeDilationShift * SEC_PER_DAY * 1.0e6;
            char tdBuf[64];
            snprintf(tdBuf, sizeof(tdBuf), "%.1f µs/day (GR+SR)", body.timeDriftMicrosecPerDay);
            body.timeDilationStr = tdBuf;
        } else {
            body.timeDilationStr = "Reference (Sol)";
        }

        // 9. Dynamic Keplerian Elements & Orbital Mechanics (Relative to Sol)
        if (body.id != "sol" && distM > 0.0) {
            glm::dvec3 rVec = relToSol;
            glm::dvec3 vVec = body.velocityMps;
            double mu = G_CONST * (solMass + body.massKg);

            // Specific Angular Momentum: h = r x v
            glm::dvec3 hVec = glm::cross(rVec, vVec);
            body.specificAngularMomentum = glm::length(hVec);
            char hBuf[64];
            snprintf(hBuf, sizeof(hBuf), "%.3e m²/s", body.specificAngularMomentum);
            body.angularMomentumStr = hBuf;

            // Specific Orbital Energy: E = v^2/2 - mu/r
            body.specificOrbitalEnergy = (speedMps * speedMps * 0.5) - (mu / distM);
            char eBuf[64];
            snprintf(eBuf, sizeof(eBuf), "%.1f MJ/kg", body.specificOrbitalEnergy / 1.0e6);
            body.orbitalEnergyStr = eBuf;

            // Semi-Major Axis: a = -mu / (2 * E)
            if (std::abs(body.specificOrbitalEnergy) > 1.0e-9) {
                body.semiMajorAxisM = -mu / (2.0 * body.specificOrbitalEnergy);
                body.semiMajorAxisAU = body.semiMajorAxisM / AU_METERS;
                char aBuf[64];
                snprintf(aBuf, sizeof(aBuf), "%.3f AU (%.1fM km)", body.semiMajorAxisAU, body.semiMajorAxisM / 1.0e9);
                body.semiMajorAxisStr = aBuf;
            }

            // Eccentricity Vector: e = (v x h)/mu - r/|r|
            glm::dvec3 eVec = (glm::cross(vVec, hVec) / mu) - (rVec / distM);
            body.eccentricity = glm::length(eVec);
            char eccBuf[64];
            snprintf(eccBuf, sizeof(eccBuf), "%.4f", body.eccentricity);
            body.eccentricityStr = eccBuf;

            // Periapsis (Perihelion) & Apoapsis (Aphelion)
            if (body.semiMajorAxisM > 0.0) {
                body.periapsisM = body.semiMajorAxisM * (1.0 - body.eccentricity);
                body.periapsisAU = body.periapsisM / AU_METERS;
                body.apoapsisM = body.semiMajorAxisM * (1.0 + body.eccentricity);
                body.apoapsisAU = body.apoapsisM / AU_METERS;

                char pBuf[64], apBuf[64];
                snprintf(pBuf, sizeof(pBuf), "%.3f AU (%.1fM km)", body.periapsisAU, body.periapsisM / 1.0e9);
                snprintf(apBuf, sizeof(apBuf), "%.3f AU (%.1fM km)", body.apoapsisAU, body.apoapsisM / 1.0e9);
                body.periapsisStr = pBuf;
                body.apoapsisStr = apBuf;
            }

            // True Anomaly: angle between eccentricity vector and position vector
            double eLen = glm::length(eVec);
            if (eLen > 1.0e-7) {
                double cosNu = glm::clamp(glm::dot(eVec, rVec) / (eLen * distM), -1.0, 1.0);
                double nuRad = std::acos(cosNu);
                if (glm::dot(rVec, vVec) < 0.0) nuRad = 2.0 * PI_DBL - nuRad;
                body.trueAnomalyDeg = nuRad * 180.0 / PI_DBL;
                char nuBuf[64];
                snprintf(nuBuf, sizeof(nuBuf), "%.1f°", body.trueAnomalyDeg);
                body.trueAnomalyStr = nuBuf;
            }

            // Einstein General Relativistic Perihelion Precession Rate per Century
            // Delta phi = 6 * pi * G * M / (c^2 * a * (1 - e^2)) radians/revolution
            if (body.semiMajorAxisM > 0.0 && body.eccentricity < 0.999) {
                double term1 = (6.0 * PI_DBL * G_CONST * solMass);
                double term2 = (C_LIGHT_SQ * body.semiMajorAxisM * (1.0 - body.eccentricity * body.eccentricity));
                double deltaPhiRadPerRev = term1 / term2;
                double periodSec = 2.0 * PI_DBL * std::sqrt(std::pow(body.semiMajorAxisM, 3.0) / mu);
                double revsPerCentury = (100.0 * 365.256 * SEC_PER_DAY) / periodSec;
                double radToArcsec = 206264.806247;
                body.grPrecessionArcsecCentury = deltaPhiRadPerRev * revsPerCentury * radToArcsec;

                char precBuf[64];
                snprintf(precBuf, sizeof(precBuf), "+%.2f\"/century (GR)", body.grPrecessionArcsecCentury);
                body.grPrecessionStr = precBuf;
            }
        } else {
            body.semiMajorAxisStr = "Central Star (Sol)";
            body.eccentricityStr = "0.0000";
            body.periapsisStr = "0.00 AU";
            body.apoapsisStr = "0.00 AU";
            body.angularMomentumStr = "Reference (Sol)";
            body.orbitalEnergyStr = "Reference (Sol)";
            body.grPrecessionStr = "Reference Source";
            body.trueAnomalyStr = "N/A";
        }
    }
}

void PhysicsEngine::computeSystemConservationStats() {
    size_t n = m_bodies.size();
    if (n == 0) return;

    double kinetic = 0.0;
    double potential = 0.0;
    glm::dvec3 totalAngMom(0.0);

    for (size_t i = 0; i < n; ++i) {
        double mass = m_bodies[i].massKg;
        glm::dvec3 pos = m_bodies[i].positionM;
        glm::dvec3 vel = m_bodies[i].velocityMps;

        // Kinetic Energy: 0.5 * m * v^2
        kinetic += 0.5 * mass * glm::dot(vel, vel);

        // Angular Momentum: r x (m * v)
        totalAngMom += mass * glm::cross(pos, vel);

        // Potential Energy: - G * m_i * m_j / r_ij
        for (size_t j = i + 1; j < n; ++j) {
            double dist = glm::length(pos - m_bodies[j].positionM);
            if (dist > 1000.0) {
                potential -= (G_CONST * mass * m_bodies[j].massKg) / dist;
            }
        }
    }

    m_totalSystemKineticJ = kinetic;
    m_totalSystemPotentialJ = potential;
    m_totalSystemEnergyJ = kinetic + potential;
    m_totalSystemAngularMomentum = glm::length(totalAngMom);

    if (m_initialSystemEnergyJ == 0.0) {
        m_initialSystemEnergyJ = m_totalSystemEnergyJ;
    }

    if (std::abs(m_initialSystemEnergyJ) > 1.0e-9) {
        m_energyConservationDriftPct = (std::abs(m_totalSystemEnergyJ - m_initialSystemEnergyJ) / std::abs(m_initialSystemEnergyJ)) * 100.0;
    }
}

void PhysicsEngine::clearTrails() {
    for (auto& body : m_bodies) {
        body.trailHistory.clear();
    }
}

void PhysicsEngine::updateBodyScales() {
    for (auto& body : m_bodies) {
        if (m_isTrueScaleMode) {
            body.radius3D = (float)(body.realRadiusAU * (double)m_sizeMultiplier);
            if (body.ring.hasRing) {
                body.ring.innerRadius3D = (float)(body.ring.innerRadiusAU * (double)m_sizeMultiplier);
                body.ring.outerRadius3D = (float)(body.ring.outerRadiusAU * (double)m_sizeMultiplier);
            }
        } else {
            // Simplified visual scale fallback
            if (body.id == "sol") body.radius3D = 2.5f;
            else if (body.id == "jupiter") body.radius3D = 1.6f;
            else if (body.id == "saturn") body.radius3D = 1.35f;
            else if (body.id == "uranus") body.radius3D = 1.1f;
            else if (body.id == "neptune") body.radius3D = 1.05f;
            else if (body.id == "earth") body.radius3D = 1.0f;
            else if (body.id == "venus") body.radius3D = 0.95f;
            else if (body.id == "mars") body.radius3D = 0.75f;
            else if (body.id == "mercury") body.radius3D = 0.6f;

            if (body.ring.hasRing) {
                body.ring.innerRadius3D = body.radius3D * 1.28f;
                body.ring.outerRadius3D = body.radius3D * 2.35f;
            }
        }
    }
}

void PhysicsEngine::update(float deltaTime) {
    if (m_isPaused) return;

    m_realTimeElapsedSeconds += (double)deltaTime;
    double effectiveDelta = (double)deltaTime * (double)m_timeScale;
    m_simulatedTimeSeconds += effectiveDelta;

    // Run symplectic N-body integration (Newtonian + Einstein 1PN General Relativity)
    integrateNBody(effectiveDelta);

    // Update real-time reacting astrophysical properties & Keplerian orbital elements
    updatePhysicalQuantities();

    // Update planetary ring hydrodynamics: differential Keplerian shear & viscous self-healing
    updateRingHydrodynamics(effectiveDelta);

    // Update Particle System: N-body gravitational integration, resonance dynamics & GPU visual instances
    m_particleSystem.update(effectiveDelta, m_bodies, m_enableGeneralRelativity, m_simulatedTimeSeconds);

    // Compute global energy and angular momentum conservation
    computeSystemConservationStats();

    // Record dynamic 3D trail points along true gravitational trajectory
    for (auto& body : m_bodies) {
        if (body.id != "sol") {
            if (body.trailHistory.empty() || glm::distance(body.trailHistory.back(), body.position) > 0.0002f) {
                body.trailHistory.push_back(body.position);
                while (body.trailHistory.size() > body.maxTrailPoints) {
                    body.trailHistory.pop_front();
                }
            }
        }

        // Calculate axial rotation
        body.rotationAngle = (float)std::fmod((double)body.rotationAngle + (double)body.rotationSpeedRadPerSec * effectiveDelta, 2.0 * PI_DBL);
        if (body.rotationAngle < 0.0f) {
            body.rotationAngle += (float)(2.0 * PI_DBL);
        }
    }
}

void PhysicsEngine::stepFrameForward() {
    update(0.0166667f);
}

void PhysicsEngine::stepFrameBackward() {
    update(-0.0166667f);
}

void PhysicsEngine::selectBody(int index) {
    if (index >= 0 && index < (int)m_bodies.size()) {
        m_selectedBodyIndex = index;
    }
}

const CelestialBody& PhysicsEngine::getSelectedBody() const {
    return m_bodies[m_selectedBodyIndex];
}

std::string PhysicsEngine::getSimulationTimeStr() const {
    time_t baseTime = 1716215742; // May 20, 2024 14:35:42 UTC
    time_t curTime = baseTime + (time_t)m_simulatedTimeSeconds;
    struct tm t;
    #ifdef _WIN32
    localtime_s(&t, &curTime);
    #else
    localtime_r(&curTime, &t);
    #endif
    char buf[64];
    strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S UTC", &t);
    return std::string(buf);
}

std::string PhysicsEngine::getTotalEnergyStr() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3e J (ΔE: %.4f%%)", m_totalSystemEnergyJ, m_energyConservationDriftPct);
    return std::string(buf);
}

std::string PhysicsEngine::getTotalAngularMomentumStr() const {
    char buf[64];
    snprintf(buf, sizeof(buf), "%.3e kg·m²/s", m_totalSystemAngularMomentum);
    return std::string(buf);
}

std::string PhysicsEngine::getSimVsRealTimeStr() const {
    char buf[128];
    double simYears = m_simulatedTimeSeconds / (365.256 * SEC_PER_DAY);
    int realMins = (int)(m_realTimeElapsedSeconds / 60.0);
    int realSecs = (int)m_realTimeElapsedSeconds % 60;
    if (simYears >= 1.0) {
        snprintf(buf, sizeof(buf), "Sim: %.2f yrs | Real: %02d:%02d (%s)", simYears, realMins, realSecs,
                 (m_timeScale >= 86400.0f) ? "Fast-Forward" : "Real-time");
    } else {
        double simDays = m_simulatedTimeSeconds / SEC_PER_DAY;
        snprintf(buf, sizeof(buf), "Sim: %.1f days | Real: %02d:%02d (%.0fx)", simDays, realMins, realSecs, m_timeScale);
    }
    return std::string(buf);
}

void PhysicsEngine::triggerRingImpact(const std::string& planetId, float normRadius, float azimuthRad, float impactRadiusM) {
    for (auto& body : m_bodies) {
        if (body.id == planetId && body.ring.hasRing) {
            normRadius = glm::clamp(normRadius, 0.05f, 0.95f);
            double ringWidth = body.ring.outerRadiusM - body.ring.innerRadiusM;
            double rM = body.ring.innerRadiusM + (double)normRadius * ringWidth;

            // Keplerian orbital angular velocity at this exact radial distance (rad/s)
            double omega = std::sqrt((G_CONST * body.massKg) / std::pow(rM, 3.0));

            RingDisturbance dist;
            dist.normRadius = normRadius;
            dist.azimuthRad = azimuthRad;
            dist.radialWidth = (float)glm::clamp((double)impactRadiusM / ringWidth, 0.03, 0.20);
            dist.angularWidth = (float)glm::clamp((double)impactRadiusM / rM * 2.5, 0.08, 0.35);
            dist.intensity = 1.0f;
            dist.ageSeconds = 0.0f;
            dist.decayRate = 0.015f; // Viscous healing rate
            dist.keplerianOmega = (float)omega;

            if (body.ring.disturbances.size() >= (size_t)PlanetaryRing::MAX_DISTURBANCES) {
                body.ring.disturbances.erase(body.ring.disturbances.begin());
            }
            body.ring.disturbances.push_back(dist);
            break;
        }
    }
}

void PhysicsEngine::triggerSaturnRingImpact() {
    static float testAngle = 0.5f;
    testAngle += 1.1f;
    triggerRingImpact("saturn", 0.55f, testAngle, 6000000.0f);
}

void PhysicsEngine::updateRingHydrodynamics(double deltaSeconds) {
    if (deltaSeconds <= 0.0) return;

    for (size_t i = 0; i < m_bodies.size(); ++i) {
        auto& host = m_bodies[i];
        if (!host.ring.hasRing) continue;

        double rIn = host.ring.innerRadiusM;
        double rOut = host.ring.outerRadiusM;
        double ringWidth = rOut - rIn;

        // Saturn equatorial tilt basis vectors (tilt around (0,0,1))
        double tiltRad = glm::radians((double)host.axialTiltDeg);
        glm::dvec3 normal(-std::sin(tiltRad), std::cos(tiltRad), 0.0);
        glm::dvec3 basisX(std::cos(tiltRad), std::sin(tiltRad), 0.0);
        glm::dvec3 basisZ(0.0, 0.0, 1.0);

        // 1. Physical Collision & Gravitational Wake Detection from Passing Celestial Objects
        for (size_t j = 0; j < m_bodies.size(); ++j) {
            if (i == j) continue;
            const auto& intruder = m_bodies[j];

            glm::dvec3 dPos = intruder.positionM - host.positionM;
            double zDist = glm::dot(dPos, normal);
            double xPlane = glm::dot(dPos, basisX);
            double zPlane = glm::dot(dPos, basisZ);
            double rM = std::sqrt(xPlane * xPlane + zPlane * zPlane);

            // Check if intruder penetrates the ring's physical disk
            double interactionRadius = intruder.radiusM * 3.0 + 1.0e6;
            if (std::abs(zDist) <= interactionRadius && rM >= rIn * 0.90 && rM <= rOut * 1.10) {
                float normR = (float)((rM - rIn) / ringWidth);
                normR = glm::clamp(normR, 0.02f, 0.98f);
                float azimuth = (float)std::atan2(zPlane, xPlane);

                // Check if a disturbance already exists close to this position to avoid duplicates
                bool alreadyTracking = false;
                for (const auto& d : host.ring.disturbances) {
                    if (std::abs(d.normRadius - normR) < 0.05f && std::abs(d.azimuthRad - azimuth) < 0.2f && d.ageSeconds < 3600.0f) {
                        alreadyTracking = true;
                        break;
                    }
                }

                if (!alreadyTracking) {
                    double omega = std::sqrt((G_CONST * host.massKg) / std::pow(rM, 3.0));
                    RingDisturbance dist;
                    dist.normRadius = normR;
                    dist.azimuthRad = azimuth;
                    dist.radialWidth = (float)glm::clamp(interactionRadius / ringWidth, 0.04, 0.25);
                    dist.angularWidth = (float)glm::clamp((interactionRadius * 2.0) / rM, 0.08, 0.40);
                    dist.intensity = 1.0f;
                    dist.ageSeconds = 0.0f;
                    dist.decayRate = 0.012f;
                    dist.keplerianOmega = (float)omega;

                    if (host.ring.disturbances.size() >= (size_t)PlanetaryRing::MAX_DISTURBANCES) {
                        host.ring.disturbances.erase(host.ring.disturbances.begin());
                    }
                    host.ring.disturbances.push_back(dist);
                }
            }
        }

        // 2. Differential Keplerian Shear Advection & Viscous Self-Healing (Fluid Relaxation)
        for (auto it = host.ring.disturbances.begin(); it != host.ring.disturbances.end(); ) {
            it->ageSeconds += (float)deltaSeconds;

            // Keplerian orbital advection: each radial band orbits at its physical Omega(r)
            it->azimuthRad += it->keplerianOmega * (float)deltaSeconds;

            // Normalize angle to [-PI, PI]
            while (it->azimuthRad > PI_DBL) it->azimuthRad -= (float)(2.0 * PI_DBL);
            while (it->azimuthRad < -PI_DBL) it->azimuthRad += (float)(2.0 * PI_DBL);

            // Keplerian shear stretches disturbance into an elongated spiral wake
            it->angularWidth = glm::clamp(it->angularWidth * (1.0f + (float)deltaSeconds * 0.00005f), 0.05f, 1.2f);

            // Viscous diffusion: inelastic collisions dissipate the perturbation
            // Disturbance heals smoothly across simulation time
            float decayFactor = std::exp(-it->decayRate * (float)(deltaSeconds / 86400.0 * 2.0));
            it->intensity *= decayFactor;

            // Once disturbance is healed (< 2% void intensity), remove from active list
            if (it->intensity < 0.02f || it->ageSeconds > 86400.0f * 365.0f) {
                it = host.ring.disturbances.erase(it);
            } else {
                ++it;
            }
        }
    }
}

} // namespace AstroGenesis
