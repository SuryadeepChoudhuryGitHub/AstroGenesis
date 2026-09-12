#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>

namespace AstroGenesis {

enum class ChemicalCategory {
    LightGas,           // H2, He (thermal escape on small rocky bodies)
    AtmosphericGas,     // N2, O2, CO, Ar
    GreenhouseGas,      // CO2, CH4, H2O vapor, SO2
    CondensibleVolatile,// H2O, NH3, CH4 (forms ice or clouds)
    RefractoryMineral   // Silicates, Iron, Carbon
};

struct ChemicalSpecies {
    std::string id;                  // e.g. "N2", "CO2", "H2O"
    std::string formula;             // e.g. "N₂", "CO₂", "H₂O"
    std::string displayName;         // e.g. "Molecular Nitrogen"
    ChemicalCategory category;
    
    double molarMassKgMol = 0.028;   // kg/mol (e.g. 0.028 for N2, 0.044 for CO2)
    double greenhousePotency = 0.0;  // Relative infrared absorption strength
    double condensationTempK_1atm = 0.0; // Phase change condensation temperature at 1 atm
    glm::vec3 rayleighColor{0.2f, 0.5f, 0.9f}; // Intrinsic Rayleigh scattering optical tint
    double specificHeatJPerKgK = 1000.0;
    bool isAtmospheric = true;       // True for gas phase volatiles
    bool isCondensible = false;      // Can form surface frost / clouds
};

struct ChemicalAbundance {
    std::string speciesId;
    std::string formula;
    std::string name;
    float percentage = 0.0f;         // Mole / volume percentage (0.0 to 100.0)
    float massFraction = 0.0f;       // Mass fraction in mixture (0.0 to 1.0)
    glm::vec4 color{0.5f, 0.5f, 0.5f, 1.0f};
};

struct AtmosphericRetentionStatus {
    std::string speciesId;
    std::string formula;
    double thermalVelocityMps = 0.0;
    double escapeParameterLambda = 0.0; // v_esc^2 / v_th^2
    bool isRetained = true;
    std::string statusText;             // "Stable", "Gradual Loss", "Rapid Thermal Escape"
};

struct AtmosphereModel {
    bool hasAtmosphere = false;
    double surfacePressurePa = 0.0;     // Surface pressure in Pascals
    double surfacePressureKpa = 0.0;    // Surface pressure in kPa
    double surfacePressureAtm = 0.0;    // Surface pressure in standard atmospheres
    double columnMassKgM2 = 0.0;        // Total atmospheric column mass per m^2
    double meanMolarMassKgMol = 0.02896;// Mean atmospheric molar mass in kg/mol
    double scaleHeightKm = 8.5;         // Atmospheric scale height H = R*T / (mu * g)
    double opticalDepth = 0.0;          // Infrared optical depth tau
    double greenhouseDeltaK = 0.0;      // Greenhouse warming offset in Kelvin
    double surfaceTempK = 288.15;       // Resulting equilibrium surface temperature in Kelvin
    double cloudCoverage = 0.0;         // Fractional cloud cover [0, 1]
    double iceCoverage = 0.0;           // Surface ice / snow cover fraction [0, 1]
    double dynamicAlbedo = 0.30;        // Bond albedo coupled to clouds, ice, and atmosphere
    double transmittedRadiationFlux = 0.0; // Radiation reaching surface through atmosphere (W/m^2)
    double atmosphericShieldingFactor = 0.0; // Radiation attenuation fraction [0, 1]
    glm::vec3 rayleighScatteringColor{0.18f, 0.45f, 0.95f}; // Blended optical scattering color
    float visualDensityFactor = 1.0f;   // Normalized optical density for shader (Earth = 1.0)
    
    std::vector<ChemicalAbundance> composition;
    std::vector<AtmosphericRetentionStatus> retentionStatuses;
    std::vector<std::string> escapedGases;
};

class ChemicalSystem {
public:
    static void initialize();

    // Chemical Species Database
    static const ChemicalSpecies& getSpecies(const std::string& idOrName);
    static const std::unordered_map<std::string, ChemicalSpecies>& getAllSpecies();

    // Thermal / Jeans Atmospheric Escape Formulation
    static double calculateThermalVelocity(double molarMassKgMol, double temperatureK);
    static double calculateEscapeParameter(double escapeVelocityMps, double thermalVelocityMps);
    static bool isGasRetained(double escapeVelocityMps, double molarMassKgMol, double temperatureK);
    static AtmosphericRetentionStatus evaluateGasRetention(const ChemicalSpecies& species, double escapeVelocityMps, double exobaseTempK);

    // Milne-Eddington Grey Atmosphere Greenhouse Formulation
    static double calculateOpticalDepth(double surfacePressurePa, const std::vector<ChemicalAbundance>& gases);
    static double calculateGreenhouseDeltaK(double tEffectiveK, double opticalDepth);

    // Dynamic Albedo Coupling (Surface, Ice, Clouds, Rayleigh Scattering)
    static double calculateDynamicAlbedo(double baseAlbedo, double surfaceTempK, double cloudCoverage, bool hasWaterVolatiles);

    // Dynamic Rayleigh Optical Scattering Hue Synthesis
    static glm::vec3 calculateRayleighScatteringColor(const std::vector<ChemicalAbundance>& gases);

    // Atmospheric Column Mass, Pressure, and Scale Height
    static double calculateScaleHeightKm(double surfaceTempK, double surfaceGravityMps2, double meanMolarMassKgMol);

    // Evaluates the full coupled atmospheric & chemical state of a body
    static AtmosphereModel evaluateAtmosphericState(
        const std::string& bodyType,
        double massKg,
        double radiusM,
        double surfaceGravityMps2,
        double escapeVelocityMps,
        double incidentSolarFlux,
        double baseAlbedo,
        double currentSurfaceTempK,
        const std::vector<ChemicalAbundance>& rawComposition,
        double customPressurePa = -1.0
    );

    // Procedural baseline composition generator for astronomical bodies
    static std::vector<ChemicalAbundance> generateBaselineComposition(
        const std::string& bodyType,
        const std::string& bodyId,
        double massKg,
        double radiusM,
        double distanceAU,
        double surfaceTempK
    );

    // Format human-readable atmospheric chemical summary string
    static std::string formatAtmosphericSummary(const AtmosphereModel& model);

private:
    static std::unordered_map<std::string, ChemicalSpecies> s_speciesRegistry;
    static bool s_initialized;
};

} // namespace AstroGenesis
