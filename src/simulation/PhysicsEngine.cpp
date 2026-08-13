#include "simulation/PhysicsEngine.hpp"
#include <ctime>
#include <cstdio>
#include <cmath>

namespace AstroGenesis {

PhysicsEngine::PhysicsEngine() {
    initializeDefaultSolarSystem();
}

void PhysicsEngine::initializeDefaultSolarSystem() {
    m_bodies.clear();

    // 1. Sol
    CelestialBody sol;
    sol.id = "sol"; sol.name = "Sol"; sol.type = "G2V Star";
    sol.distanceStr = "0.00 AU"; sol.distanceAU = 0.0;
    sol.radiusStr = "696,340 km"; sol.massStr = "1.989 x 10^30 kg";
    sol.gravityStr = "274 m/s²"; sol.tempStr = "5,778 K";
    sol.orbitalPeriodStr = "N/A"; sol.rotationPeriodStr = "27d 6h";
    sol.axialTiltStr = "7.25°"; sol.atmosphereStr = "73% H₂, 25% He"; sol.moons = 8;
    sol.escapeVelocityStr = "617.7 km/s"; sol.pressureStr = "N/A";
    sol.densityStr = "1,408 kg/m³"; sol.yearLengthStr = "N/A"; sol.surfaceAreaStr = "6.09 x 10^12 km²";
    sol.solarRadiationStr = "6.33 x 10^7 W/m²"; sol.radLevelStr = "Extreme";
    sol.magneticFieldStr = "100–300 µT"; sol.auroraActivityStr = "None";
    sol.composition = { {"Hydrogen", 73.46f, {1.0f, 0.8f, 0.2f, 1.0f}}, {"Helium", 24.85f, {0.9f, 0.5f, 0.1f, 1.0f}}, {"Oxygen", 0.77f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Carbon", 0.29f, {0.5f, 0.5f, 0.5f, 1.0f}} };
    sol.position = glm::vec3(-15.0f, 0.0f, 0.0f);
    sol.radius3D = 2.5f;
    sol.color = glm::vec3(1.0f, 0.75f, 0.2f);
    m_bodies.push_back(sol);

    // 2. Mercury
    CelestialBody mercury;
    mercury.id = "mercury"; mercury.name = "Mercury"; mercury.type = "Terrestrial Planet";
    mercury.distanceStr = "0.39 AU"; mercury.distanceAU = 0.39;
    mercury.radiusStr = "2,439.7 km"; mercury.massStr = "3.301 x 10^23 kg";
    mercury.gravityStr = "3.7 m/s²"; mercury.tempStr = "440 K";
    mercury.orbitalPeriodStr = "87.97 days"; mercury.rotationPeriodStr = "58d 15h";
    mercury.axialTiltStr = "0.034°"; mercury.atmosphereStr = "42% O₂, 29% Na, 22% H₂"; mercury.moons = 0;
    mercury.escapeVelocityStr = "4.25 km/s"; mercury.pressureStr = "10^-14 kPa";
    mercury.densityStr = "5,427 kg/m³"; mercury.yearLengthStr = "87.97 days"; mercury.surfaceAreaStr = "74.8 M km²";
    mercury.solarRadiationStr = "9126 W/m²"; mercury.radLevelStr = "Very High";
    mercury.magneticFieldStr = "0.3 µT"; mercury.auroraActivityStr = "None";
    mercury.composition = { {"Oxygen", 42.0f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Sodium", 29.0f, {0.9f, 0.6f, 0.1f, 1.0f}}, {"Hydrogen", 22.0f, {0.4f, 0.7f, 1.0f, 1.0f}} };
    mercury.position = glm::vec3(-6.0f, 0.0f, 0.0f);
    mercury.radius3D = 0.6f;
    mercury.color = glm::vec3(0.7f, 0.6f, 0.5f);
    m_bodies.push_back(mercury);

    // 3. Venus
    CelestialBody venus;
    venus.id = "venus"; venus.name = "Venus"; venus.type = "Terrestrial Planet";
    venus.distanceStr = "0.72 AU"; venus.distanceAU = 0.72;
    venus.radiusStr = "6,051.8 km"; venus.massStr = "4.867 x 10^24 kg";
    venus.gravityStr = "8.87 m/s²"; venus.tempStr = "737 K";
    venus.orbitalPeriodStr = "224.7 days"; venus.rotationPeriodStr = "243d 0h";
    venus.axialTiltStr = "177.3°"; venus.atmosphereStr = "96.5% CO₂, 3.5% N₂"; venus.moons = 0;
    venus.escapeVelocityStr = "10.36 km/s"; venus.pressureStr = "9200 kPa";
    venus.densityStr = "5,243 kg/m³"; venus.yearLengthStr = "224.7 days"; venus.surfaceAreaStr = "460.2 M km²";
    venus.solarRadiationStr = "2613 W/m²"; venus.radLevelStr = "High";
    venus.magneticFieldStr = "Induced"; venus.auroraActivityStr = "Weak";
    venus.composition = { {"Carbon Dioxide", 96.5f, {0.9f, 0.3f, 0.3f, 1.0f}}, {"Nitrogen", 3.5f, {0.0f, 0.7f, 0.9f, 1.0f}} };
    venus.position = glm::vec3(-3.0f, 0.0f, 0.0f);
    venus.radius3D = 0.95f;
    venus.color = glm::vec3(0.9f, 0.7f, 0.3f);
    m_bodies.push_back(venus);

    // 4. Earth (Default focus)
    CelestialBody earth;
    earth.id = "earth"; earth.name = "Earth"; earth.type = "Terrestrial Planet";
    earth.distanceStr = "1.00 AU"; earth.distanceAU = 1.00;
    earth.radiusStr = "6,371 km"; earth.massStr = "5.97 x 10^24 kg";
    earth.gravityStr = "9.81 m/s²"; earth.tempStr = "287 K";
    earth.orbitalPeriodStr = "365.25 days"; earth.rotationPeriodStr = "23h 56m";
    earth.axialTiltStr = "23.44°"; earth.atmosphereStr = "78% N₂, 21% O₂"; earth.moons = 1;
    earth.escapeVelocityStr = "11.19 km/s"; earth.pressureStr = "101.3 kPa";
    earth.densityStr = "5,514 kg/m³"; earth.yearLengthStr = "365.25 days"; earth.surfaceAreaStr = "510.1 M km²";
    earth.solarRadiationStr = "1361 W/m²"; earth.radLevelStr = "Low";
    earth.magneticFieldStr = "25.0–65.0 µT"; earth.auroraActivityStr = "Moderate";
    earth.composition = { {"Nitrogen", 78.08f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Oxygen", 20.95f, {0.0f, 0.85f, 0.4f, 1.0f}}, {"Argon", 0.93f, {0.94f, 0.75f, 0.12f, 1.0f}}, {"Carbon Dioxide", 0.04f, {0.86f, 0.31f, 0.31f, 1.0f}}, {"Others", 0.00f, {0.5f, 0.5f, 0.5f, 1.0f}} };
    earth.position = glm::vec3(0.0f, 0.0f, 0.0f);
    earth.radius3D = 1.0f;
    earth.color = glm::vec3(0.0f, 0.83f, 1.0f);
    m_bodies.push_back(earth);

    // 5. Mars
    CelestialBody mars;
    mars.id = "mars"; mars.name = "Mars"; mars.type = "Terrestrial Planet";
    mars.distanceStr = "1.52 AU"; mars.distanceAU = 1.52;
    mars.radiusStr = "3,389.5 km"; mars.massStr = "6.417 x 10^23 kg";
    mars.gravityStr = "3.72 m/s²"; mars.tempStr = "210 K";
    mars.orbitalPeriodStr = "686.98 days"; mars.rotationPeriodStr = "24h 37m";
    mars.axialTiltStr = "25.19°"; mars.atmosphereStr = "95.3% CO₂, 2.6% N₂"; mars.moons = 2;
    mars.escapeVelocityStr = "5.03 km/s"; mars.pressureStr = "0.61 kPa";
    mars.densityStr = "3,934 kg/m³"; mars.yearLengthStr = "686.98 days"; mars.surfaceAreaStr = "144.8 M km²";
    mars.solarRadiationStr = "586 W/m²"; mars.radLevelStr = "Moderate";
    mars.magneticFieldStr = "Remnant"; mars.auroraActivityStr = "Localized";
    mars.composition = { {"Carbon Dioxide", 95.32f, {0.9f, 0.3f, 0.2f, 1.0f}}, {"Nitrogen", 2.6f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Argon", 1.9f, {0.9f, 0.7f, 0.1f, 1.0f}} };
    mars.position = glm::vec3(3.5f, 0.0f, 0.0f);
    mars.radius3D = 0.75f;
    mars.color = glm::vec3(0.95f, 0.35f, 0.2f);
    m_bodies.push_back(mars);

    m_selectedBodyIndex = 3; // Earth
}

void PhysicsEngine::update(float deltaTime) {
    if (m_isPaused) return;

    float effectiveDelta = deltaTime * m_timeScale;
    m_simulatedTimeSeconds += effectiveDelta;

    for (auto& body : m_bodies) {
        body.rotationAngle += body.rotationSpeed * effectiveDelta;
        if (body.rotationAngle > 6.28318530718f) {
            body.rotationAngle -= 6.28318530718f;
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

} // namespace AstroGenesis
