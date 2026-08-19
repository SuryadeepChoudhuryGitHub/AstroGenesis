#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <glm/glm.hpp>

namespace AstroGenesis {

struct CompositionItem {
    std::string name;
    float percentage;
    glm::vec4 color;
};

struct RingDisturbance {
    float normRadius = 0.5f;     // Normalized radius [0, 1] across ring (0 = inner, 1 = outer)
    float azimuthRad = 0.0f;     // Azimuthal angle theta on ring plane in radians
    float radialWidth = 0.08f;   // Gaussian radial width in normalized radius
    float angularWidth = 0.15f;  // Gaussian angular width in radians
    float intensity = 1.0f;      // 1.0 = fully carved hole/wake, 0.0 = fully healed
    float ageSeconds = 0.0f;     // Time since collision
    float decayRate = 0.02f;     // Viscous healing rate (1 / seconds)
    float keplerianOmega = 0.0f; // Orbital angular velocity at this radius (rad/s)
};

struct PlanetaryRing {
    bool hasRing = false;
    double innerRadiusM = 74500000.0;  // 74,500 km (D/C ring inner boundary)
    double outerRadiusM = 140220000.0; // 140,220 km (A/F ring outer boundary)
    double innerRadiusAU = 0.0;
    double outerRadiusAU = 0.0;
    float innerRadius3D = 0.0f;
    float outerRadius3D = 0.0f;
    double massKg = 1.5e19;             // Total ring mass in kg
    double thicknessM = 20.0;           // Average physical thickness (20 meters)
    glm::vec3 baseColor{0.88f, 0.82f, 0.70f}; // Saturn ring warm golden-tan ice color
    std::string texturePath = "assets/textures/saturn_ring_alpha.png";

    // Dynamic Hydrodynamic Perturbations & Self-Healing Field
    static constexpr int MAX_DISTURBANCES = 16;
    std::vector<RingDisturbance> disturbances;
};

struct CelestialBody {
    // Database and Provenance Identifiers
    int64_t dbId = 0;
    std::string id;           // Slug / unique code, e.g. "earth", "sol", "ceres", "trappist_1e"
    std::string name;         // Display name, e.g. "Earth"
    std::string type;         // e.g. "Terrestrial Planet", "G2V Star", "Gas Giant", "Asteroid"
    std::string category = "Solar System"; // "Solar System", "Asteroid Belt", "Exoplanet System", "Comet"
    bool isSynthetic = false; // Distinguishes real astronomical bodies from statistical particles
    std::optional<int64_t> parentObjectId;
    PlanetaryRing ring;

    // External Data Source Provenance & Verification Metadata
    std::string sourceName = "Bundled Seed Dataset"; // "JPL Horizons", "JPL SBDB", "NASA Exoplanet Archive", etc.
    std::string sourceObjectId;                       // e.g. "399", "2000001", "TRAPPIST-1 e"
    std::string importTimestamp;                      // e.g. "2026-08-19 22:00:00"
    std::string referenceFrame = "ICRF/Barycentric";  // "ICRF/Barycentric", "Ecliptic/J2000"
    double epochJd = 2451545.0;                       // Julian Date of state vector / elements
    std::string epochUtcStr = "2000-01-01 12:00:00 UTC";
    bool dataVerified = true;                         // Verified from external authority
    
    // Live Formatted Strings (dynamically updated in real-time)
    std::string distanceStr;       // e.g. "1.00 AU"
    std::string orbitalSpeedStr;   // e.g. "29.78 km/s"
    std::string radiusStr;         // e.g. "6,371 km"
    std::string massStr;           // e.g. "5.97 x 10^24 kg"
    std::string gravityStr;        // e.g. "9.81 m/s²"
    std::string tempStr;           // e.g. "287 K"
    std::string orbitalPeriodStr;  // e.g. "365.25 days"
    std::string rotationPeriodStr; // e.g. "23h 56m"
    std::string axialTiltStr;      // e.g. "23.44°"
    std::string atmosphereStr;     // e.g. "78% N₂, 21% O₂"
    int moons = 0;

    // Physical Overview Stats
    std::string escapeVelocityStr; // e.g. "11.19 km/s"
    std::string pressureStr;       // e.g. "101.3 kPa"
    std::string densityStr;        // e.g. "5,514 kg/m³"
    std::string yearLengthStr;     // e.g. "365.25 days"
    std::string surfaceAreaStr;    // e.g. "510.1 M km²"

    // Radiation, Environment & Relativity Stats
    std::string solarRadiationStr; // e.g. "1361 W/m²"
    std::string radLevelStr;       // e.g. "Low"
    std::string magneticFieldStr;  // e.g. "25.0–65.0 µT"
    std::string auroraActivityStr; // e.g. "Moderate"
    std::string timeDilationStr;   // e.g. "-14.8 µs/day"

    // Dynamic Orbital Mechanics & Keplerian Elements Stats
    std::string semiMajorAxisStr;   // e.g. "1.000 AU (149.6M km)"
    std::string eccentricityStr;    // e.g. "0.0167"
    std::string periapsisStr;       // e.g. "0.983 AU (Perihelion)"
    std::string apoapsisStr;        // e.g. "1.017 AU (Aphelion)"
    std::string angularMomentumStr; // e.g. "4.45 × 10¹⁵ m²/s"
    std::string orbitalEnergyStr;   // e.g. "-443.8 MJ/kg"
    std::string grPrecessionStr;    // e.g. "+42.98\"/century"
    std::string trueAnomalyStr;     // e.g. "134.2°"

    // Composition
    std::vector<CompositionItem> composition;

    // Fundamental SI Physical State (Exact Double-Precision Dynamics)
    glm::dvec3 positionM{0.0};       // World position in meters
    glm::dvec3 velocityMps{0.0};     // World velocity in m/s
    glm::dvec3 accelerationMps2{0.0};// Net gravitational acceleration in m/s²
    double massKg = 0.0;             // Mass in kg
    double radiusM = 0.0;            // Physical radius in meters
    double albedo = 0.3;             // Bond / Geometric Albedo
    double greenhouseK = 0.0;        // Atmospheric greenhouse warming in Kelvin
    double luminosityW = 0.0;        // Stellar luminosity in Watts (Sol = 3.828e26 W)

    // Dynamic Live Physical Metrics
    double distanceAU = 0.0;         // Instantaneous distance to Sol in AU
    double distanceKm = 0.0;         // Instantaneous distance to Sol in km
    double orbitalSpeedKmpS = 0.0;   // Instantaneous orbital velocity in km/s
    double solarRadiationFlux = 0.0; // Instantaneous solar radiation flux in W/m²
    double surfaceTempK = 0.0;       // Instantaneous thermal equilibrium temp in K
    double surfaceGravityMps2 = 0.0; // Surface gravity g = GM/R² in m/s²
    double escapeVelocityKmpS = 0.0; // Escape velocity v_esc = sqrt(2GM/R) in km/s
    double meanDensityKgM3 = 0.0;    // Mean density in kg/m³
    double surfaceAreaKm2 = 0.0;     // Total surface area in km²
    double timeDilationShift = 0.0;  // Fractional proper time shift (1 - dt_proper / dt_coord)
    double timeDriftMicrosecPerDay = 0.0; // Relativistic clock drift in microseconds / Earth day

    // Dynamic Keplerian / Relativistic Orbital Parameters
    double semiMajorAxisAU = 0.0;
    double semiMajorAxisM = 0.0;
    double eccentricity = 0.0;
    double periapsisAU = 0.0;
    double periapsisM = 0.0;
    double apoapsisAU = 0.0;
    double apoapsisM = 0.0;
    double specificAngularMomentum = 0.0; // m²/s
    double specificOrbitalEnergy = 0.0;    // J/kg
    double grPrecessionArcsecCentury = 0.0;// arcseconds per Earth century
    double trueAnomalyDeg = 0.0;           // current orbital anomaly in degrees

    // Dynamic 3D Osculating Orbital Basis Vectors & Ellipse Points
    glm::dvec3 eccentricityVec{0.0};       // Runge-Lenz / Eccentricity vector pointing to periapsis
    glm::dvec3 angularMomentumVec{0.0};    // Specific angular momentum vector h = r x v (orbit normal)
    glm::dvec3 perifocalP{1.0, 0.0, 0.0};  // Unit vector pointing to periapsis in 3D space
    glm::dvec3 perifocalQ{0.0, 0.0, 1.0};  // In-plane transverse unit vector (W x P)
    std::vector<glm::vec3> dynamicOrbitCurve; // Live dynamic 3D Keplerian curve in AU space relative to star

    // Rendering / 3D Visualization properties (in AU space)
    glm::vec3 position{0.0f};        // Position in AU coordinates
    glm::vec3 velocity{0.0f};
    float radius3D = 1.0f;
    float rotationAngle = 0.0f;
    float rotationSpeed = 0.5f;      // rad/s
    float axialTiltDeg = 23.44f;
    glm::vec3 color{0.0f, 0.83f, 1.0f}; // Default teal accent
    std::string texturePath;
    std::deque<glm::vec3> trailHistory;
    size_t maxTrailPoints = 600;

    // Astronomical Scale Data (SI & AU units)
    double realRadiusAU = 0.0;
    double realOrbitRadiusAU = 0.0;
    double orbitalPeriodDays = 0.0;
    double rotationPeriodHours = 0.0;
    double orbitalAngleRad = 0.0;
    double orbitalSpeedRadPerSec = 0.0;
    double rotationSpeedRadPerSec = 0.0;
};

} // namespace AstroGenesis
