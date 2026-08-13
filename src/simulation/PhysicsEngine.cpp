#include "simulation/PhysicsEngine.hpp"
#include <ctime>
#include <cstdio>
#include <cmath>

namespace AstroGenesis {

static const double PI_DBL = 3.14159265358979323846;
static const double AU_KM  = 149597870.7; // 1 Astronomical Unit in km

PhysicsEngine::PhysicsEngine() {
    initializeDefaultSolarSystem();
}

void PhysicsEngine::initializeDefaultSolarSystem() {
    m_bodies.clear();

    // 1. Sol (Sun)
    CelestialBody sol;
    sol.id = "sol"; sol.name = "Sol"; sol.type = "G2V Star";
    sol.distanceStr = "0.00 AU"; sol.distanceAU = 0.0;
    sol.radiusStr = "696,340 km"; sol.massStr = "1.989 x 10^30 kg";
    sol.gravityStr = "274.0 m/s²"; sol.tempStr = "5,778 K";
    sol.orbitalPeriodStr = "N/A"; sol.rotationPeriodStr = "25d 9h";
    sol.axialTiltStr = "7.25°"; sol.atmosphereStr = "73.46% H₂, 24.85% He"; sol.moons = 8;
    sol.escapeVelocityStr = "617.7 km/s"; sol.pressureStr = "N/A";
    sol.densityStr = "1,408 kg/m³"; sol.yearLengthStr = "N/A"; sol.surfaceAreaStr = "6.09 x 10^12 km²";
    sol.solarRadiationStr = "6.33 x 10^7 W/m²"; sol.radLevelStr = "Extreme";
    sol.magneticFieldStr = "100–300 µT"; sol.auroraActivityStr = "None";
    sol.composition = { {"Hydrogen", 73.46f, {1.0f, 0.8f, 0.2f, 1.0f}}, {"Helium", 24.85f, {0.9f, 0.5f, 0.1f, 1.0f}}, {"Oxygen", 0.77f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Carbon", 0.29f, {0.5f, 0.5f, 0.5f, 1.0f}} };
    sol.position = glm::vec3(0.0f);
    sol.axialTiltDeg = 7.25f;
    sol.color = glm::vec3(1.0f, 0.75f, 0.2f);
    sol.realRadiusAU = 696340.0 / AU_KM; // ~0.00465474 AU
    sol.realOrbitRadiusAU = 0.0;
    sol.orbitalPeriodDays = 0.0;
    sol.rotationPeriodHours = 609.12; // 25.38 days
    sol.orbitalAngleRad = 0.0;
    sol.orbitalSpeedRadPerSec = 0.0;
    sol.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (609.12 * 3600.0);
    m_bodies.push_back(sol);

    // 2. Mercury
    CelestialBody mercury;
    mercury.id = "mercury"; mercury.name = "Mercury"; mercury.type = "Terrestrial Planet";
    mercury.distanceStr = "0.387 AU"; mercury.distanceAU = 0.387098;
    mercury.radiusStr = "2,439.7 km"; mercury.massStr = "3.301 x 10^23 kg";
    mercury.gravityStr = "3.70 m/s²"; mercury.tempStr = "440 K";
    mercury.orbitalPeriodStr = "87.97 days"; mercury.rotationPeriodStr = "58d 15h";
    mercury.axialTiltStr = "0.034°"; mercury.atmosphereStr = "42% O₂, 29% Na, 22% H₂"; mercury.moons = 0;
    mercury.escapeVelocityStr = "4.25 km/s"; mercury.pressureStr = "10^-14 kPa";
    mercury.densityStr = "5,427 kg/m³"; mercury.yearLengthStr = "87.97 days"; mercury.surfaceAreaStr = "74.8 M km²";
    mercury.solarRadiationStr = "9,126 W/m²"; mercury.radLevelStr = "Very High";
    mercury.magneticFieldStr = "0.3 µT"; mercury.auroraActivityStr = "None";
    mercury.composition = { {"Oxygen", 42.0f, {0.2f, 0.8f, 0.4f, 1.0f}}, {"Sodium", 29.0f, {0.9f, 0.6f, 0.1f, 1.0f}}, {"Hydrogen", 22.0f, {0.4f, 0.7f, 1.0f, 1.0f}} };
    mercury.axialTiltDeg = 0.034f;
    mercury.color = glm::vec3(0.7f, 0.6f, 0.5f);
    mercury.texturePath = "assets/textures/mercury_surface.jpg";
    mercury.realRadiusAU = 2439.7 / AU_KM; // ~0.0000163084 AU
    mercury.realOrbitRadiusAU = 0.387098;
    mercury.orbitalPeriodDays = 87.969;
    mercury.rotationPeriodHours = 1407.5; // 58.646 days
    mercury.orbitalAngleRad = 0.2;
    mercury.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (87.969 * 86400.0);
    mercury.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (1407.5 * 3600.0);
    m_bodies.push_back(mercury);

    // 3. Venus (Surface texture used)
    CelestialBody venus;
    venus.id = "venus"; venus.name = "Venus"; venus.type = "Terrestrial Planet";
    venus.distanceStr = "0.723 AU"; venus.distanceAU = 0.723332;
    venus.radiusStr = "6,051.8 km"; venus.massStr = "4.867 x 10^24 kg";
    venus.gravityStr = "8.87 m/s²"; venus.tempStr = "737 K";
    venus.orbitalPeriodStr = "224.70 days"; venus.rotationPeriodStr = "243d 0h";
    venus.axialTiltStr = "177.36°"; venus.atmosphereStr = "96.5% CO₂, 3.5% N₂"; venus.moons = 0;
    venus.escapeVelocityStr = "10.36 km/s"; venus.pressureStr = "9,200 kPa";
    venus.densityStr = "5,243 kg/m³"; venus.yearLengthStr = "224.70 days"; venus.surfaceAreaStr = "460.2 M km²";
    venus.solarRadiationStr = "2,613 W/m²"; venus.radLevelStr = "High";
    venus.magneticFieldStr = "Induced"; venus.auroraActivityStr = "Weak";
    venus.composition = { {"Carbon Dioxide", 96.5f, {0.9f, 0.3f, 0.3f, 1.0f}}, {"Nitrogen", 3.5f, {0.0f, 0.7f, 0.9f, 1.0f}} };
    venus.axialTiltDeg = 177.36f;
    venus.color = glm::vec3(0.9f, 0.7f, 0.3f);
    venus.texturePath = "assets/textures/venus_surface.jpg";
    venus.realRadiusAU = 6051.8 / AU_KM; // ~0.0000404538 AU
    venus.realOrbitRadiusAU = 0.723332;
    venus.orbitalPeriodDays = 224.701;
    venus.rotationPeriodHours = -5832.6; // -243.025 days (retrograde)
    venus.orbitalAngleRad = 1.1;
    venus.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (224.701 * 86400.0);
    venus.rotationSpeedRadPerSec = -(2.0 * PI_DBL) / (5832.6 * 3600.0);
    m_bodies.push_back(venus);

    // 4. Earth (Default focus)
    CelestialBody earth;
    earth.id = "earth"; earth.name = "Earth"; earth.type = "Terrestrial Planet";
    earth.distanceStr = "1.000 AU"; earth.distanceAU = 1.000000;
    earth.radiusStr = "6,371.0 km"; earth.massStr = "5.972 x 10^24 kg";
    earth.gravityStr = "9.81 m/s²"; earth.tempStr = "287 K";
    earth.orbitalPeriodStr = "365.26 days"; earth.rotationPeriodStr = "23h 56m";
    earth.axialTiltStr = "23.44°"; earth.atmosphereStr = "78% N₂, 21% O₂"; earth.moons = 1;
    earth.escapeVelocityStr = "11.19 km/s"; earth.pressureStr = "101.3 kPa";
    earth.densityStr = "5,514 kg/m³"; earth.yearLengthStr = "365.26 days"; earth.surfaceAreaStr = "510.1 M km²";
    earth.solarRadiationStr = "1,361 W/m²"; earth.radLevelStr = "Low";
    earth.magneticFieldStr = "25.0–65.0 µT"; earth.auroraActivityStr = "Moderate";
    earth.composition = { {"Nitrogen", 78.08f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Oxygen", 20.95f, {0.0f, 0.85f, 0.4f, 1.0f}}, {"Argon", 0.93f, {0.94f, 0.75f, 0.12f, 1.0f}}, {"Carbon Dioxide", 0.04f, {0.86f, 0.31f, 0.31f, 1.0f}}, {"Others", 0.00f, {0.5f, 0.5f, 0.5f, 1.0f}} };
    earth.axialTiltDeg = 23.44f;
    earth.color = glm::vec3(0.0f, 0.83f, 1.0f);
    earth.texturePath = "assets/textures/earth_daymap.jpg";
    earth.realRadiusAU = 6371.0 / AU_KM; // ~0.0000425875 AU
    earth.realOrbitRadiusAU = 1.000000;
    earth.orbitalPeriodDays = 365.256;
    earth.rotationPeriodHours = 23.93446; // 23h 56m 4s
    earth.orbitalAngleRad = 2.4;
    earth.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (365.256 * 86400.0);
    earth.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (23.93446 * 3600.0);
    m_bodies.push_back(earth);

    // 5. Mars
    CelestialBody mars;
    mars.id = "mars"; mars.name = "Mars"; mars.type = "Terrestrial Planet";
    mars.distanceStr = "1.524 AU"; mars.distanceAU = 1.523679;
    mars.radiusStr = "3,389.5 km"; mars.massStr = "6.417 x 10^23 kg";
    mars.gravityStr = "3.72 m/s²"; mars.tempStr = "210 K";
    mars.orbitalPeriodStr = "686.98 days"; mars.rotationPeriodStr = "24h 37m";
    mars.axialTiltStr = "25.19°"; mars.atmosphereStr = "95.3% CO₂, 2.6% N₂"; mars.moons = 2;
    mars.escapeVelocityStr = "5.03 km/s"; mars.pressureStr = "0.61 kPa";
    mars.densityStr = "3,934 kg/m³"; mars.yearLengthStr = "686.98 days"; mars.surfaceAreaStr = "144.8 M km²";
    mars.solarRadiationStr = "586 W/m²"; mars.radLevelStr = "Moderate";
    mars.magneticFieldStr = "Remnant"; mars.auroraActivityStr = "Localized";
    mars.composition = { {"Carbon Dioxide", 95.32f, {0.9f, 0.3f, 0.2f, 1.0f}}, {"Nitrogen", 2.6f, {0.0f, 0.7f, 0.9f, 1.0f}}, {"Argon", 1.9f, {0.9f, 0.7f, 0.1f, 1.0f}} };
    mars.axialTiltDeg = 25.19f;
    mars.color = glm::vec3(0.95f, 0.35f, 0.2f);
    mars.texturePath = "assets/textures/mars_surface.jpg";
    mars.realRadiusAU = 3389.5 / AU_KM; // ~0.0000226574 AU
    mars.realOrbitRadiusAU = 1.523679;
    mars.orbitalPeriodDays = 686.980;
    mars.rotationPeriodHours = 24.6229; // 24h 37m 22s
    mars.orbitalAngleRad = 3.6;
    mars.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (686.980 * 86400.0);
    mars.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (24.6229 * 3600.0);
    m_bodies.push_back(mars);

    // 6. Jupiter
    CelestialBody jupiter;
    jupiter.id = "jupiter"; jupiter.name = "Jupiter"; jupiter.type = "Gas Giant";
    jupiter.distanceStr = "5.204 AU"; jupiter.distanceAU = 5.2044;
    jupiter.radiusStr = "69,911 km"; jupiter.massStr = "1.898 x 10^27 kg";
    jupiter.gravityStr = "24.79 m/s²"; jupiter.tempStr = "165 K";
    jupiter.orbitalPeriodStr = "11.86 years"; jupiter.rotationPeriodStr = "9h 55m";
    jupiter.axialTiltStr = "3.13°"; jupiter.atmosphereStr = "89% H₂, 10% He"; jupiter.moons = 95;
    jupiter.escapeVelocityStr = "59.5 km/s"; jupiter.pressureStr = "100 kPa";
    jupiter.densityStr = "1,326 kg/m³"; jupiter.yearLengthStr = "4,332.59 days"; jupiter.surfaceAreaStr = "6.14 x 10^10 km²";
    jupiter.solarRadiationStr = "50.5 W/m²"; jupiter.radLevelStr = "Extreme";
    jupiter.magneticFieldStr = "420 µT"; jupiter.auroraActivityStr = "Very High";
    jupiter.composition = { {"Hydrogen", 89.8f, {0.9f, 0.8f, 0.6f, 1.0f}}, {"Helium", 10.2f, {0.9f, 0.6f, 0.3f, 1.0f}} };
    jupiter.axialTiltDeg = 3.13f;
    jupiter.color = glm::vec3(0.85f, 0.65f, 0.45f);
    jupiter.texturePath = "assets/textures/jupiter_surface.jpg";
    jupiter.realRadiusAU = 69911.0 / AU_KM; // ~0.000467326 AU
    jupiter.realOrbitRadiusAU = 5.2044;
    jupiter.orbitalPeriodDays = 4332.59;
    jupiter.rotationPeriodHours = 9.925; // 9h 55m 30s
    jupiter.orbitalAngleRad = 4.5;
    jupiter.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (4332.59 * 86400.0);
    jupiter.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (9.925 * 3600.0);
    m_bodies.push_back(jupiter);

    // 7. Saturn
    CelestialBody saturn;
    saturn.id = "saturn"; saturn.name = "Saturn"; saturn.type = "Gas Giant";
    saturn.distanceStr = "9.583 AU"; saturn.distanceAU = 9.5826;
    saturn.radiusStr = "58,232 km"; saturn.massStr = "5.683 x 10^26 kg";
    saturn.gravityStr = "10.44 m/s²"; saturn.tempStr = "134 K";
    saturn.orbitalPeriodStr = "29.46 years"; saturn.rotationPeriodStr = "10h 39m";
    saturn.axialTiltStr = "26.73°"; saturn.atmosphereStr = "96% H₂, 3% He"; saturn.moons = 146;
    saturn.escapeVelocityStr = "35.5 km/s"; saturn.pressureStr = "100 kPa";
    saturn.densityStr = "687 kg/m³"; saturn.yearLengthStr = "10,759 days"; saturn.surfaceAreaStr = "4.27 x 10^10 km²";
    saturn.solarRadiationStr = "14.9 W/m²"; saturn.radLevelStr = "High";
    saturn.magneticFieldStr = "21 µT"; saturn.auroraActivityStr = "High";
    saturn.composition = { {"Hydrogen", 96.3f, {0.9f, 0.85f, 0.5f, 1.0f}}, {"Helium", 3.2f, {0.9f, 0.7f, 0.4f, 1.0f}} };
    saturn.axialTiltDeg = 26.73f;
    saturn.color = glm::vec3(0.9f, 0.8f, 0.5f);
    saturn.texturePath = "assets/textures/saturn_surface.jpg";
    saturn.realRadiusAU = 58232.0 / AU_KM; // ~0.000389257 AU
    saturn.realOrbitRadiusAU = 9.5826;
    saturn.orbitalPeriodDays = 10759.22;
    saturn.rotationPeriodHours = 10.656; // 10h 39m
    saturn.orbitalAngleRad = 5.2;
    saturn.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (10759.22 * 86400.0);
    saturn.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (10.656 * 3600.0);
    m_bodies.push_back(saturn);

    // 8. Uranus
    CelestialBody uranus;
    uranus.id = "uranus"; uranus.name = "Uranus"; uranus.type = "Ice Giant";
    uranus.distanceStr = "19.201 AU"; uranus.distanceAU = 19.2012;
    uranus.radiusStr = "25,362 km"; uranus.massStr = "8.681 x 10^25 kg";
    uranus.gravityStr = "8.69 m/s²"; uranus.tempStr = "76 K";
    uranus.orbitalPeriodStr = "84.02 years"; uranus.rotationPeriodStr = "17h 14m";
    uranus.axialTiltStr = "97.77°"; uranus.atmosphereStr = "83% H₂, 15% He, 2.3% CH₄"; uranus.moons = 28;
    uranus.escapeVelocityStr = "21.3 km/s"; uranus.pressureStr = "100 kPa";
    uranus.densityStr = "1,270 kg/m³"; uranus.yearLengthStr = "30,687 days"; uranus.surfaceAreaStr = "8.08 x 10^9 km²";
    uranus.solarRadiationStr = "3.7 W/m²"; uranus.radLevelStr = "Low";
    uranus.magneticFieldStr = "23 µT"; uranus.auroraActivityStr = "Moderate";
    uranus.composition = { {"Hydrogen", 83.0f, {0.3f, 0.8f, 0.9f, 1.0f}}, {"Helium", 15.0f, {0.5f, 0.7f, 0.9f, 1.0f}}, {"Methane", 2.3f, {0.1f, 0.5f, 0.8f, 1.0f}} };
    uranus.axialTiltDeg = 97.77f;
    uranus.color = glm::vec3(0.5f, 0.8f, 0.9f);
    uranus.texturePath = "assets/textures/uranus_surface.jpg";
    uranus.realRadiusAU = 25362.0 / AU_KM; // ~0.000169534 AU
    uranus.realOrbitRadiusAU = 19.2012;
    uranus.orbitalPeriodDays = 30687.15;
    uranus.rotationPeriodHours = -17.24; // -17h 14m (retrograde)
    uranus.orbitalAngleRad = 5.8;
    uranus.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (30687.15 * 86400.0);
    uranus.rotationSpeedRadPerSec = -(2.0 * PI_DBL) / (17.24 * 3600.0);
    m_bodies.push_back(uranus);

    // 9. Neptune
    CelestialBody neptune;
    neptune.id = "neptune"; neptune.name = "Neptune"; neptune.type = "Ice Giant";
    neptune.distanceStr = "30.047 AU"; neptune.distanceAU = 30.0472;
    neptune.radiusStr = "24,622 km"; neptune.massStr = "1.024 x 10^26 kg";
    neptune.gravityStr = "11.15 m/s²"; neptune.tempStr = "72 K";
    neptune.orbitalPeriodStr = "164.79 years"; neptune.rotationPeriodStr = "16h 6m";
    neptune.axialTiltStr = "28.32°"; neptune.atmosphereStr = "80% H₂, 19% He, 1.5% CH₄"; neptune.moons = 16;
    neptune.escapeVelocityStr = "23.5 km/s"; neptune.pressureStr = "100 kPa";
    neptune.densityStr = "1,638 kg/m³"; neptune.yearLengthStr = "60,190 days"; neptune.surfaceAreaStr = "7.61 x 10^9 km²";
    neptune.solarRadiationStr = "1.5 W/m²"; neptune.radLevelStr = "Low";
    neptune.magneticFieldStr = "14 µT"; neptune.auroraActivityStr = "Moderate";
    neptune.composition = { {"Hydrogen", 80.0f, {0.1f, 0.3f, 0.9f, 1.0f}}, {"Helium", 19.0f, {0.3f, 0.5f, 0.9f, 1.0f}}, {"Methane", 1.5f, {0.0f, 0.2f, 0.7f, 1.0f}} };
    neptune.axialTiltDeg = 28.32f;
    neptune.color = glm::vec3(0.2f, 0.4f, 0.9f);
    neptune.texturePath = "assets/textures/neptune_surface.jpg";
    neptune.realRadiusAU = 24622.0 / AU_KM; // ~0.000164588 AU
    neptune.realOrbitRadiusAU = 30.0472;
    neptune.orbitalPeriodDays = 60190.03;
    neptune.rotationPeriodHours = 16.11; // 16h 6m
    neptune.orbitalAngleRad = 0.5;
    neptune.orbitalSpeedRadPerSec = (2.0 * PI_DBL) / (60190.03 * 86400.0);
    neptune.rotationSpeedRadPerSec = (2.0 * PI_DBL) / (16.11 * 3600.0);
    m_bodies.push_back(neptune);

    m_selectedBodyIndex = 3; // Earth default
    updateBodyScales();
}

void PhysicsEngine::updateBodyScales() {
    for (auto& body : m_bodies) {
        if (m_isTrueScaleMode) {
            body.radius3D = (float)(body.realRadiusAU * (double)m_sizeMultiplier);
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
        }
    }
}

void PhysicsEngine::update(float deltaTime) {
    if (m_isPaused) return;

    double effectiveDelta = (double)deltaTime * (double)m_timeScale;
    m_simulatedTimeSeconds += effectiveDelta;

    for (auto& body : m_bodies) {
        if (body.id != "sol") {
            // Calculate orbital movement around Sol according to Keplerian velocity
            body.orbitalAngleRad += body.orbitalSpeedRadPerSec * effectiveDelta;
            if (body.orbitalAngleRad > 2.0 * PI_DBL) {
                body.orbitalAngleRad -= 2.0 * PI_DBL;
            }

            float x = (float)(body.realOrbitRadiusAU * std::cos(body.orbitalAngleRad));
            float z = (float)(body.realOrbitRadiusAU * std::sin(body.orbitalAngleRad));
            body.position = glm::vec3(x, 0.0f, z);
        } else {
            body.position = glm::vec3(0.0f, 0.0f, 0.0f);
        }

        // Calculate axial rotation
        body.rotationAngle += (float)(body.rotationSpeedRadPerSec * effectiveDelta);
        if (body.rotationAngle > 2.0 * PI_DBL) {
            body.rotationAngle -= (float)(2.0 * PI_DBL);
        } else if (body.rotationAngle < -2.0 * PI_DBL) {
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

} // namespace AstroGenesis
