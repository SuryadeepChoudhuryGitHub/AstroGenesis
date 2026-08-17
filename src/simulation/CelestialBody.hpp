#pragma once

#include <string>
#include <vector>
#include <deque>
#include <glm/glm.hpp>

namespace AstroGenesis {

struct CompositionItem {
    std::string name;
    float percentage;
    glm::vec4 color;
};

struct CelestialBody {
    std::string id;
    std::string name;
    std::string type;         // e.g. "Terrestrial Planet", "G2V Star", "Gas Giant"
    std::string distanceStr;  // e.g. "1.00 AU"
    double distanceAU;        // numeric distance
    
    std::string radiusStr;    // e.g. "6,371 km"
    std::string massStr;      // e.g. "5.97 x 10^24 kg"
    std::string gravityStr;   // e.g. "9.81 m/s²"
    std::string tempStr;      // e.g. "287 K"
    std::string orbitalPeriodStr;  // e.g. "365.25 days"
    std::string rotationPeriodStr; // e.g. "23h 56m"
    std::string axialTiltStr;      // e.g. "23.44°"
    std::string atmosphereStr;     // e.g. "78% N₂, 21% O₂"
    int moons;

    // Physical Overview Stats
    std::string escapeVelocityStr; // e.g. "11.19 km/s"
    std::string pressureStr;       // e.g. "101.3 kPa"
    std::string densityStr;        // e.g. "5,514 kg/m³"
    std::string yearLengthStr;     // e.g. "365.25 days"
    std::string surfaceAreaStr;    // e.g. "510.1 M km²"

    // Radiation & Environment
    std::string solarRadiationStr; // e.g. "1361 W/m²"
    std::string radLevelStr;       // e.g. "Low"
    std::string magneticFieldStr;  // e.g. "25.0–65.0 µT"
    std::string auroraActivityStr; // e.g. "Moderate"

    // Composition
    std::vector<CompositionItem> composition;

    // Rendering / 3D properties
    glm::vec3 position{0.0f};
    glm::vec3 velocity{0.0f};
    float radius3D = 1.0f;
    float rotationAngle = 0.0f;
    float rotationSpeed = 0.5f; // rad/s
    float axialTiltDeg = 23.44f;
    glm::vec3 color{0.0f, 0.83f, 1.0f}; // Default teal accent
    std::string texturePath;
    std::deque<glm::vec3> trailHistory;
    size_t maxTrailPoints = 600;

    // True Astronomical Scale Data (SI & AU units)
    double realRadiusAU = 0.0;
    double realOrbitRadiusAU = 0.0;
    double orbitalPeriodDays = 0.0;
    double rotationPeriodHours = 0.0;
    double orbitalAngleRad = 0.0;
    double orbitalSpeedRadPerSec = 0.0;
    double rotationSpeedRadPerSec = 0.0;
};

} // namespace AstroGenesis
