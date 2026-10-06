#pragma once

#include <string>
#include <vector>
#include <deque>
#include <optional>
#include <glm/glm.hpp>
#include "simulation/ChemicalComposition.hpp"

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
    enum class CelestialClass {
        Star_MainSequence,
        Star_RedDwarf,
        Star_Giant,
        Star_Supergiant,
        Star_WhiteDwarf,
        Star_NeutronStar,
        Star_Pulsar,
        Star_Magnetar,
        Planet_Terrestrial,
        Planet_SuperEarth,
        Planet_GasGiant,
        Planet_IceGiant,
        DwarfPlanet,
        Moon,
        Asteroid,
        Comet,
        BlackHole,
        Other,
        Unknown
    };

    // Database and Provenance Identifiers
    int64_t dbId = 0;
    std::string id;           // Slug / unique code, e.g. "earth", "sol", "ceres", "trappist_1e"
    std::string name;         // Display name, e.g. "Earth"
    std::string type;         // e.g. "Terrestrial Planet", "G2V Star", "Gas Giant", "Asteroid"
    std::string category = "Solar System"; // "Solar System", "Asteroid Belt", "Exoplanet System", "Comet"
    bool isSynthetic = false; // Distinguishes real astronomical bodies from statistical particles
    std::optional<int64_t> parentObjectId;
    PlanetaryRing ring;

    // Classification Taxonomy
    CelestialClass classification = CelestialClass::Unknown;
    std::string classificationStr = "Unknown";

    // External Data Source Provenance & Verification Metadata
    std::string sourceName = "Bundled Seed Dataset"; // "JPL Horizons", "JPL SBDB", "NASA Exoplanet Archive", etc.
    std::string sourceObjectId;                       // e.g. "399", "2000001", "TRAPPIST-1 e"
    std::string datasetVersion = "NASA/JPL Baseline";
    std::string provenanceStatus = "Verified / Observed"; // "Observed", "Derived", "Estimated", "User-Defined"
    std::string hostStarName;                         // Host star name for exoplanetary systems
    std::string spectralType;                         // e.g. "G2V", "M1V", "O9.7 Iab"
    std::string appearanceType = "Procedural Estimate"; // "Photographic Texture", "Procedural Atmosphere", "Blackbody Emission", "Relativistic Singularity", etc.
    bool isEstimated = false;                         // True if physical parameters or appearance are procedurally estimated
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
    std::string gravityStr;        // e.g. "9.81 m/s^2"
    std::string tempStr;           // e.g. "287 K"
    std::string orbitalPeriodStr;  // e.g. "365.25 days"
    std::string rotationPeriodStr; // e.g. "23h 56m"
    std::string axialTiltStr;      // e.g. "23.44 deg"
    std::string atmosphereStr;     // e.g. "78% N2, 21% O2"
    int moons = 0;

    // Physical Overview Stats
    std::string escapeVelocityStr; // e.g. "11.19 km/s"
    std::string pressureStr;       // e.g. "101.3 kPa"
    std::string densityStr;        // e.g. "5,514 kg/m^3"
    std::string yearLengthStr;     // e.g. "365.25 days"
    std::string surfaceAreaStr;    // e.g. "510.1 M km^2"

    // Radiation, Environment & Relativity Stats
    std::string solarRadiationStr; // e.g. "1361 W/m^2"
    std::string radLevelStr;       // e.g. "Low"
    std::string magneticFieldStr;  // e.g. "25.0-65.0 uT"
    std::string auroraActivityStr; // e.g. "Moderate"
    std::string timeDilationStr;   // e.g. "-14.8 us/day"

    // Dynamic Orbital Mechanics & Keplerian Elements Stats
    std::string semiMajorAxisStr;   // e.g. "1.000 AU (149.6M km)"
    std::string eccentricityStr;    // e.g. "0.0167"
    std::string periapsisStr;       // e.g. "0.983 AU (Perihelion)"
    std::string apoapsisStr;        // e.g. "1.017 AU (Aphelion)"
    std::string angularMomentumStr; // e.g. "4.45 x 10^15 m^2/s"
    std::string orbitalEnergyStr;   // e.g. "-443.8 MJ/kg"
    std::string grPrecessionStr;    // e.g. "+42.98\"/century"
    std::string trueAnomalyStr;     // e.g. "134.2 deg"

    // Composition & Chemical Inventory
    std::vector<CompositionItem> composition;
    std::vector<ChemicalAbundance> chemicalInventory;
    AtmosphereModel atmosphere;

    // Fundamental SI Physical State (Exact Double-Precision Dynamics)
    glm::dvec3 positionM{0.0};       // World position in meters
    glm::dvec3 velocityMps{0.0};     // World velocity in m/s
    glm::dvec3 accelerationMps2{0.0};// Net gravitational acceleration in m/s^2
    double massKg = 0.0;             // Mass in kg
    double radiusM = 0.0;            // Physical radius in meters
    double baseAlbedo = 0.3;         // Intrinsic bare surface albedo
    double albedo = 0.3;             // Dynamic Bond / Geometric Albedo (coupled to ice, clouds, and atmosphere)
    double greenhouseK = 0.0;        // Dynamic atmospheric greenhouse warming in Kelvin
    double luminosityW = 0.0;        // Stellar luminosity in Watts (Sol = 3.828e26 W)
    bool hasCustomTemp = false;      // True if user manually tuned surface temperature (prevents radiative equilibrium overwrite)
    bool hasAtmosphereCustom = false;// True if user explicitly enabled/disabled atmosphere
    bool hasAtmosphere = true;       // Per-body atmosphere state
    double surfacePressurePa = 0.0;  // Dynamic atmospheric surface pressure in Pa
    double surfacePressureKpa = 0.0; // Dynamic atmospheric surface pressure in kPa
    double opticalDepth = 0.0;       // Infrared opacity tau
    double scaleHeightKm = 8.5;      // Dynamic atmospheric scale height in km
    double cloudCoverage = 0.0;      // Dynamic fractional cloud cover [0, 1]
    double iceCoverage = 0.0;        // Dynamic snow/ice surface coverage [0, 1]
    glm::vec3 rayleighColor{0.18f, 0.45f, 0.95f}; // Dynamic Rayleigh scattering tint

    // Dynamic Live Physical Metrics
    double distanceAU = 0.0;         // Instantaneous distance to Sol in AU
    double distanceKm = 0.0;         // Instantaneous distance to Sol in km
    double orbitalSpeedKmpS = 0.0;   // Instantaneous orbital velocity in km/s
    double solarRadiationFlux = 0.0; // Instantaneous solar radiation flux in W/m^2
    double surfaceTempK = 0.0;       // Instantaneous thermal equilibrium temp in K
    double surfaceGravityMps2 = 0.0; // Surface gravity g = GM/R^2 in m/s^2
    double escapeVelocityKmpS = 0.0; // Escape velocity v_esc = sqrt(2GM/R) in km/s
    double meanDensityKgM3 = 0.0;    // Mean density in kg/m^3
    double surfaceAreaKm2 = 0.0;     // Total surface area in km^2
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
    double specificAngularMomentum = 0.0; // m^2/s
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

    static std::string getClassName(CelestialClass cls) {
        switch (cls) {
            case CelestialClass::Star_MainSequence: return "Main Sequence Star";
            case CelestialClass::Star_RedDwarf:     return "Red Dwarf Star";
            case CelestialClass::Star_Giant:        return "Giant Star";
            case CelestialClass::Star_Supergiant:   return "Supergiant Star";
            case CelestialClass::Star_WhiteDwarf:   return "White Dwarf";
            case CelestialClass::Star_NeutronStar:  return "Neutron Star";
            case CelestialClass::Star_Pulsar:       return "Pulsar";
            case CelestialClass::Star_Magnetar:     return "Magnetar";
            case CelestialClass::Planet_Terrestrial:return "Terrestrial Planet";
            case CelestialClass::Planet_SuperEarth: return "Super-Earth";
            case CelestialClass::Planet_GasGiant:   return "Gas Giant";
            case CelestialClass::Planet_IceGiant:   return "Ice Giant";
            case CelestialClass::DwarfPlanet:       return "Dwarf Planet";
            case CelestialClass::Moon:              return "Natural Satellite (Moon)";
            case CelestialClass::Asteroid:          return "Asteroid";
            case CelestialClass::Comet:             return "Comet";
            case CelestialClass::BlackHole:         return "Black Hole";
            case CelestialClass::Other:             return "Other";
            default:                                return "Unknown";
        }
    }

    std::string getClassName() const {
        return getClassName(classification);
    }

    static CelestialClass parseClass(const std::string& str) {
        std::string s = str;
        for (auto& c : s) c = (char)tolower((unsigned char)c);
        if (s.empty()) return CelestialClass::Unknown;
        if (s.find("main sequence") != std::string::npos) return CelestialClass::Star_MainSequence;
        if (s.find("red dwarf") != std::string::npos) return CelestialClass::Star_RedDwarf;
        if (s.find("supergiant") != std::string::npos) return CelestialClass::Star_Supergiant;
        if (s.find("giant") != std::string::npos && s.find("gas") == std::string::npos && s.find("ice") == std::string::npos) return CelestialClass::Star_Giant;
        if (s.find("white dwarf") != std::string::npos) return CelestialClass::Star_WhiteDwarf;
        if (s.find("neutron") != std::string::npos) return CelestialClass::Star_NeutronStar;
        if (s.find("pulsar") != std::string::npos) return CelestialClass::Star_Pulsar;
        if (s.find("magnetar") != std::string::npos) return CelestialClass::Star_Magnetar;
        if (s.find("black hole") != std::string::npos || s.find("singularity") != std::string::npos) return CelestialClass::BlackHole;
        if (s.find("super-earth") != std::string::npos || s.find("superearth") != std::string::npos) return CelestialClass::Planet_SuperEarth;
        if (s.find("terrestrial") != std::string::npos || s.find("rocky") != std::string::npos) return CelestialClass::Planet_Terrestrial;
        if (s.find("gas giant") != std::string::npos || s.find("jovian") != std::string::npos) return CelestialClass::Planet_GasGiant;
        if (s.find("ice giant") != std::string::npos || s.find("neptun") != std::string::npos) return CelestialClass::Planet_IceGiant;
        if (s.find("dwarf planet") != std::string::npos) return CelestialClass::DwarfPlanet;
        if (s.find("moon") != std::string::npos || s.find("satellite") != std::string::npos) return CelestialClass::Moon;
        if (s.find("comet") != std::string::npos) return CelestialClass::Comet;
        if (s.find("asteroid") != std::string::npos) return CelestialClass::Asteroid;
        if (s.find("star") != std::string::npos) return CelestialClass::Star_MainSequence;
        if (s.find("planet") != std::string::npos) return CelestialClass::Planet_Terrestrial;
        if (s.find("other") != std::string::npos) return CelestialClass::Other;
        return CelestialClass::Unknown;
    }

    static CelestialClass classify(const std::string& typeStr, double massKg, double radiusM, double tempK, double lumW, const std::string& category = "") {
        std::string t = typeStr;
        for (auto& c : t) c = (char)tolower((unsigned char)c);

        if (t.find("black hole") != std::string::npos || t.find("singularity") != std::string::npos) {
            return CelestialClass::BlackHole;
        }
        if (t.find("pulsar") != std::string::npos) return CelestialClass::Star_Pulsar;
        if (t.find("magnetar") != std::string::npos) return CelestialClass::Star_Magnetar;
        if (t.find("neutron") != std::string::npos) return CelestialClass::Star_NeutronStar;
        if (t.find("white dwarf") != std::string::npos) return CelestialClass::Star_WhiteDwarf;
        if (t.find("supergiant") != std::string::npos) return CelestialClass::Star_Supergiant;
        if (t.find("red dwarf") != std::string::npos || (t.find("star") != std::string::npos && tempK > 0.0 && tempK < 3900.0 && massKg < 0.6 * 1.989e30)) {
            return CelestialClass::Star_RedDwarf;
        }
        if (t.find("giant star") != std::string::npos || (t.find("star") != std::string::npos && radiusM > 10.0 * 6.9634e8)) {
            return CelestialClass::Star_Giant;
        }
        if (t.find("star") != std::string::npos || lumW > 1e23 || (tempK >= 2400.0 && massKg > 1e29)) {
            return CelestialClass::Star_MainSequence;
        }

        if (t.find("moon") != std::string::npos || t.find("satellite") != std::string::npos) {
            return CelestialClass::Moon;
        }
        if (t.find("comet") != std::string::npos) {
            return CelestialClass::Comet;
        }
        if (t.find("dwarf planet") != std::string::npos) {
            return CelestialClass::DwarfPlanet;
        }
        if (t.find("asteroid") != std::string::npos || category == "Asteroid Belt") {
            return CelestialClass::Asteroid;
        }
        if (t.find("ice giant") != std::string::npos || t.find("neptun") != std::string::npos) {
            return CelestialClass::Planet_IceGiant;
        }
        if (t.find("gas giant") != std::string::npos || t.find("jovian") != std::string::npos || massKg >= 50.0 * 5.972e24) {
            return CelestialClass::Planet_GasGiant;
        }
        if (t.find("super-earth") != std::string::npos || (massKg >= 2.0 * 5.972e24 && massKg < 15.0 * 5.972e24)) {
            return CelestialClass::Planet_SuperEarth;
        }
        if (t.find("terrestrial") != std::string::npos || t.find("planet") != std::string::npos || (massKg >= 0.01 * 5.972e24 && massKg <= 2.0 * 5.972e24)) {
            return CelestialClass::Planet_Terrestrial;
        }

        return CelestialClass::Unknown;
    }

    void classify() {
        classification = classify(type, massKg, radiusM, surfaceTempK, luminosityW, category);
        classificationStr = getClassName(classification);
    }

    bool isStar() const {
        return classification == CelestialClass::Star_MainSequence ||
               classification == CelestialClass::Star_RedDwarf ||
               classification == CelestialClass::Star_Giant ||
               classification == CelestialClass::Star_Supergiant ||
               classification == CelestialClass::Star_WhiteDwarf ||
               classification == CelestialClass::Star_NeutronStar ||
               classification == CelestialClass::Star_Pulsar ||
               classification == CelestialClass::Star_Magnetar;
    }

    bool isPlanet() const {
        return classification == CelestialClass::Planet_Terrestrial ||
               classification == CelestialClass::Planet_SuperEarth ||
               classification == CelestialClass::Planet_GasGiant ||
               classification == CelestialClass::Planet_IceGiant ||
               classification == CelestialClass::DwarfPlanet;
    }

    bool isGasOrIceGiant() const {
        return classification == CelestialClass::Planet_GasGiant ||
               classification == CelestialClass::Planet_IceGiant;
    }

    bool isBlackHole() const {
        return classification == CelestialClass::BlackHole;
    }

    bool isMinorBody() const {
        return classification == CelestialClass::Asteroid ||
               classification == CelestialClass::Comet;
    }

    bool isMoon() const {
        return classification == CelestialClass::Moon;
    }
};

using CelestialClass = CelestialBody::CelestialClass;

} // namespace AstroGenesis
