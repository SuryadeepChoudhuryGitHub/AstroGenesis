#pragma once

#include <string>
#include <vector>
#include <optional>
#include <glm/glm.hpp>

namespace AstroGenesis {

struct ObjectRecord {
    int64_t id = 0;
    std::string slug;           // Unique slug, e.g. "earth", "ceres", "trappist_1e"
    std::string name;           // Display name, e.g. "Earth"
    std::string type;           // e.g. "Terrestrial Planet", "Gas Giant", "G2V Star", "Asteroid"
    std::optional<int64_t> parentObjectId;
    std::string category = "Solar System"; // "Solar System", "Asteroid Belt", "Exoplanet System", "Comet", "Custom"
    std::string classification = "Unknown"; // Taxonomic classification e.g. "Terrestrial Planet", "Main Sequence Star"
    std::string provenanceStatus = "Verified / Observed"; // "Observed", "Derived", "Estimated", "User-Defined"
    bool isSynthetic = false;
    glm::vec3 color{0.0f, 0.83f, 1.0f};
    std::string texturePath;
    std::string createdAt;
    std::string updatedAt;
};

struct PhysicalPropertiesRecord {
    int64_t id = 0;
    int64_t objectId = 0;
    std::optional<double> massKg;
    std::optional<double> radiusM;
    std::optional<double> albedo;
    std::optional<double> greenhouseK;
    std::optional<double> luminosityW;
    std::optional<double> axialTiltDeg;
    std::optional<double> rotationPeriodHours;
    std::optional<double> meanDensityKgM3;
    std::optional<double> surfaceGravityMps2;
    std::optional<double> escapeVelocityMps;
    std::optional<double> surfaceTempK;
    std::optional<double> surfacePressureKpa;
    std::optional<std::string> magneticFieldStr;
    std::optional<std::string> atmosphereSummary;
    std::optional<std::string> ringsJson; // JSON string if has rings
    std::optional<std::string> spectralType; // e.g. "G2V", "M1V"
    std::optional<double> metallicityFeH;   // Stellar metallicity [Fe/H]
    bool isEstimated = false;               // True if values are procedurally estimated
    int64_t sourceId = 1;
    std::string sourceRecordId;
    std::string importTimestamp;
};

struct OrbitalElementsRecord {
    int64_t id = 0;
    int64_t objectId = 0;
    double epochJd = 2451545.0; // J2000.0 default
    std::optional<double> semiMajorAxisM;
    std::optional<double> semiMajorAxisAU;
    std::optional<double> eccentricity;
    std::optional<double> inclinationDeg;
    std::optional<double> longAscendingNodeDeg;
    std::optional<double> argPeriapsisDeg;
    std::optional<double> meanAnomalyDeg;
    std::optional<double> trueAnomalyDeg;
    std::optional<double> orbitalPeriodDays;
    std::string referenceFrame = "Ecliptic/J2000";
    int64_t sourceId = 1;
    std::string importTimestamp;
};

struct StateVectorRecord {
    int64_t id = 0;
    int64_t objectId = 0;
    double epochJd = 2451545.0;
    glm::dvec3 positionM{0.0};
    glm::dvec3 velocityMps{0.0};
    std::string referenceFrame = "ICRF/Barycentric";
    int64_t sourceId = 1;
    std::string importTimestamp;
};

struct EphemerisRecord {
    int64_t id = 0;
    int64_t objectId = 0;
    std::string targetName;
    std::string epochUtc;
    double epochJd = 2451545.0;
    glm::dvec3 positionM{0.0};
    glm::dvec3 velocityMps{0.0};
    std::string referenceFrame = "ICRF/Barycentric";
    int64_t sourceId = 1;
};

struct CompositionRecord {
    int64_t id = 0;
    int64_t objectId = 0;
    std::string elementOrCompound;
    float percentage = 0.0f;
    glm::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
};

struct DataSourceRecord {
    int64_t id = 0;
    std::string name;        // e.g. "JPL Horizons", "JPL SBDB", "NASA Exoplanet Archive", "Bundled Seed Dataset"
    std::string baseUrl;
    std::string description;
    bool isOfficial = true;
};

struct DataImportRecord {
    int64_t id = 0;
    int64_t sourceId = 1;
    std::string targetObject;
    std::string status = "SUCCESS"; // "SUCCESS", "FAILED", "PENDING"
    int recordsCount = 1;
    std::string details;
    std::string timestamp;
};

struct SimulationRunRecord {
    int64_t id = 0;
    std::string name;
    std::string integratorType = "Velocity-Verlet (1PN Einstein GR)";
    double startEpochJd = 2451545.0;
    double timeScale = 86400.0;
    bool grEnabled = true;
    std::string startTimestamp;
    std::string endTimestamp;
    double totalSimSeconds = 0.0;
};

struct SimulationStateRecord {
    int64_t id = 0;
    int64_t runId = 0;
    int64_t objectId = 0;
    double simTimeSeconds = 0.0;
    glm::dvec3 positionM{0.0};
    glm::dvec3 velocityMps{0.0};
    double energyJoules = 0.0;
    double angularMomentum = 0.0;
};

struct ValidationResultRecord {
    int64_t id = 0;
    int64_t runId = 0;
    int64_t objectId = 0;
    std::string objectName;
    double epochJd = 2451545.0;
    glm::dvec3 simPosM{0.0};
    glm::dvec3 realPosM{0.0};
    glm::dvec3 simVelMps{0.0};
    glm::dvec3 realVelMps{0.0};
    double posErrorM = 0.0;
    double posRelativeError = 0.0;
    double velErrorMps = 0.0;
    double energyDriftPct = 0.0;
    double angularMomentumDriftPct = 0.0;
    std::string evaluatedAt;
    bool grMode = true;
};

// Search Result from an External Provider
struct SearchResult {
    std::string sourceName;      // "JPL Horizons", "JPL SBDB", "NASA Exoplanet", "Stellar Catalogue"
    std::string sourceId;        // e.g. "399" (Earth), "2000001" (Ceres), "TRAPPIST-1 e", "star_sirius"
    std::string name;            // "Earth", "1 Ceres", "TRAPPIST-1 e", "Sirius A"
    std::string type;            // "Planet", "Asteroid", "Exoplanet", "Main Sequence Star", "Binary Star System"
    std::string details;         // Summary string e.g. "Mass: 2.06 M☉ | Rad: 1.71 R☉"
    bool alreadyInDatabase = false;
    float dataCompleteness = 1.0f; // Fractional completeness of observed parameters [0.0, 1.0]
    std::string aliases;         // Cross-catalog identifiers e.g. "HD 48915, HIP 32349, Alpha Canis Majoris"
};

// Combined aggregate record for importing or hydrating a full celestial body
struct CelestialBodyRecord {
    ObjectRecord object;
    PhysicalPropertiesRecord physical;
    OrbitalElementsRecord orbital;
    StateVectorRecord stateVector;
    std::vector<CompositionRecord> composition;
    std::string sourceName = "Bundled Seed Dataset";
    std::string hostStarName;
};

// Complete System Record representing an astronomical or custom star system
struct SystemRecord {
    int64_t id = 0;
    std::string name;                          // Display name, e.g. "Solar System", "TRAPPIST-1 System", "My Binary System"
    std::string type = "Custom";               // "Preset", "Imported", "Custom"
    std::string source = "User";               // "NASA Exoplanet Archive", "JPL Horizons", "Bundled Seed Dataset", "User"
    std::string description;
    std::string createdAt;
    std::string updatedAt;
    int objectCount = 0;
};

// Association between a system and its celestial objects with hierarchical relationship
struct SystemObjectRecord {
    int64_t id = 0;
    int64_t systemId = 0;
    int64_t objectId = 0;
    std::optional<int64_t> parentObjectId;
    int orbitalOrder = 0;
    std::string customOverridesJson;
};

// Physical plausibility and collision warning during custom system pre-flight checks
struct SystemValidationWarning {
    enum class Severity {
        Info,
        Warning,
        Error
    };
    Severity severity = Severity::Warning;
    std::string title;
    std::string message;
    std::string objectNameA;
    std::string objectNameB;
};

} // namespace AstroGenesis

