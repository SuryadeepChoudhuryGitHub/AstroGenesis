#include "simulation/ChemicalComposition.hpp"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>

namespace AstroGenesis {

static constexpr double R_GAS = 8.314462618;          // J / (mol * K)
static constexpr double P_REF_PA = 101325.0;          // Standard 1 atmosphere in Pa
static constexpr double G_CONST = 6.67430e-11;        // m^3 / (kg * s^2)
static constexpr double SIGMA_SB = 5.670374419e-8;    // W / (m^2 * K^4)

std::unordered_map<std::string, ChemicalSpecies> ChemicalSystem::s_speciesRegistry;
bool ChemicalSystem::s_initialized = false;

void ChemicalSystem::initialize() {
    if (s_initialized) return;

    auto reg = [](const ChemicalSpecies& s) {
        s_speciesRegistry[s.id] = s;
        s_speciesRegistry[s.formula] = s;
        s_speciesRegistry[s.displayName] = s;
    };

    // 1. Molecular Hydrogen (H2)
    {
        ChemicalSpecies s;
        s.id = "H2";
        s.formula = "H₂";
        s.displayName = "Molecular Hydrogen";
        s.category = ChemicalCategory::LightGas;
        s.molarMassKgMol = 0.002016;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 20.28;
        s.rayleighColor = glm::vec3(0.35f, 0.55f, 0.90f);
        s.specificHeatJPerKgK = 14304.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 2. Helium (He)
    {
        ChemicalSpecies s;
        s.id = "He";
        s.formula = "He";
        s.displayName = "Helium";
        s.category = ChemicalCategory::LightGas;
        s.molarMassKgMol = 0.004003;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 4.22;
        s.rayleighColor = glm::vec3(0.40f, 0.60f, 0.85f);
        s.specificHeatJPerKgK = 5193.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 3. Molecular Nitrogen (N2)
    {
        ChemicalSpecies s;
        s.id = "N2";
        s.formula = "N₂";
        s.displayName = "Molecular Nitrogen";
        s.category = ChemicalCategory::AtmosphericGas;
        s.molarMassKgMol = 0.028013;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 77.36;
        s.rayleighColor = glm::vec3(0.18f, 0.45f, 0.95f); // Earth Azure Blue
        s.specificHeatJPerKgK = 1040.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 4. Molecular Oxygen (O2)
    {
        ChemicalSpecies s;
        s.id = "O2";
        s.formula = "O₂";
        s.displayName = "Molecular Oxygen";
        s.category = ChemicalCategory::AtmosphericGas;
        s.molarMassKgMol = 0.031999;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 90.20;
        s.rayleighColor = glm::vec3(0.15f, 0.50f, 0.98f);
        s.specificHeatJPerKgK = 918.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 5. Carbon Dioxide (CO2)
    {
        ChemicalSpecies s;
        s.id = "CO2";
        s.formula = "CO₂";
        s.displayName = "Carbon Dioxide";
        s.category = ChemicalCategory::GreenhouseGas;
        s.molarMassKgMol = 0.044010;
        s.greenhousePotency = 1.0; // Baseline greenhouse unit
        s.condensationTempK_1atm = 194.7; // Sublimation point
        s.rayleighColor = glm::vec3(0.85f, 0.75f, 0.42f); // Venus Amber / Golden Haze
        s.specificHeatJPerKgK = 846.0;
        s.isAtmospheric = true;
        s.isCondensible = true;
        reg(s);
    }

    // 6. Methane (CH4)
    {
        ChemicalSpecies s;
        s.id = "CH4";
        s.formula = "CH₄";
        s.displayName = "Methane";
        s.category = ChemicalCategory::GreenhouseGas;
        s.molarMassKgMol = 0.016043;
        s.greenhousePotency = 25.0; // 25x stronger GWP than CO2
        s.condensationTempK_1atm = 111.6;
        s.rayleighColor = glm::vec3(0.20f, 0.75f, 0.85f); // Titan / Uranus Cyan
        s.specificHeatJPerKgK = 2226.0;
        s.isAtmospheric = true;
        s.isCondensible = true;
        reg(s);
    }

    // 7. Water Vapor (H2O)
    {
        ChemicalSpecies s;
        s.id = "H2O";
        s.formula = "H₂O";
        s.displayName = "Water Vapor";
        s.category = ChemicalCategory::CondensibleVolatile;
        s.molarMassKgMol = 0.018015;
        s.greenhousePotency = 4.0;
        s.condensationTempK_1atm = 373.15;
        s.rayleighColor = glm::vec3(0.30f, 0.65f, 0.95f);
        s.specificHeatJPerKgK = 1864.0;
        s.isAtmospheric = true;
        s.isCondensible = true;
        reg(s);
    }

    // 8. Carbon Monoxide (CO)
    {
        ChemicalSpecies s;
        s.id = "CO";
        s.formula = "CO";
        s.displayName = "Carbon Monoxide";
        s.category = ChemicalCategory::AtmosphericGas;
        s.molarMassKgMol = 0.028010;
        s.greenhousePotency = 0.15;
        s.condensationTempK_1atm = 81.6;
        s.rayleighColor = glm::vec3(0.45f, 0.50f, 0.60f);
        s.specificHeatJPerKgK = 1040.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 9. Ammonia (NH3)
    {
        ChemicalSpecies s;
        s.id = "NH3";
        s.formula = "NH₃";
        s.displayName = "Ammonia";
        s.category = ChemicalCategory::GreenhouseGas;
        s.molarMassKgMol = 0.017031;
        s.greenhousePotency = 5.0;
        s.condensationTempK_1atm = 239.8;
        s.rayleighColor = glm::vec3(0.70f, 0.75f, 0.82f);
        s.specificHeatJPerKgK = 2060.0;
        s.isAtmospheric = true;
        s.isCondensible = true;
        reg(s);
    }

    // 10. Sulfur Dioxide (SO2)
    {
        ChemicalSpecies s;
        s.id = "SO2";
        s.formula = "SO₂";
        s.displayName = "Sulfur Dioxide";
        s.category = ChemicalCategory::GreenhouseGas;
        s.molarMassKgMol = 0.064066;
        s.greenhousePotency = 2.2;
        s.condensationTempK_1atm = 263.1;
        s.rayleighColor = glm::vec3(0.90f, 0.82f, 0.35f); // Volcanic Pale Yellow
        s.specificHeatJPerKgK = 620.0;
        s.isAtmospheric = true;
        s.isCondensible = true;
        reg(s);
    }

    // 11. Argon (Ar)
    {
        ChemicalSpecies s;
        s.id = "Ar";
        s.formula = "Ar";
        s.displayName = "Argon";
        s.category = ChemicalCategory::AtmosphericGas;
        s.molarMassKgMol = 0.039948;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 87.3;
        s.rayleighColor = glm::vec3(0.50f, 0.55f, 0.70f);
        s.specificHeatJPerKgK = 520.0;
        s.isAtmospheric = true;
        s.isCondensible = false;
        reg(s);
    }

    // 12. Silicate Minerals / Basalt Crust
    {
        ChemicalSpecies s;
        s.id = "Silicates";
        s.formula = "SiO₂/Silicates";
        s.displayName = "Silicate Minerals & Rock";
        s.category = ChemicalCategory::RefractoryMineral;
        s.molarMassKgMol = 0.06008;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 1900.0;
        s.rayleighColor = glm::vec3(0.50f, 0.48f, 0.45f);
        s.specificHeatJPerKgK = 840.0;
        s.isAtmospheric = false;
        s.isCondensible = false;
        reg(s);
    }

    // 13. Metallic Iron / Nickel Core
    {
        ChemicalSpecies s;
        s.id = "Iron";
        s.formula = "Fe/Ni";
        s.displayName = "Iron & Metallic Core";
        s.category = ChemicalCategory::RefractoryMineral;
        s.molarMassKgMol = 0.055845;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 3134.0;
        s.rayleighColor = glm::vec3(0.65f, 0.65f, 0.70f);
        s.specificHeatJPerKgK = 450.0;
        s.isAtmospheric = false;
        s.isCondensible = false;
        reg(s);
    }

    // 14. Water Ice (Surface Lithosphere / Glaciers)
    {
        ChemicalSpecies s;
        s.id = "WaterIce";
        s.formula = "H₂O (Ice)";
        s.displayName = "Water Ice";
        s.category = ChemicalCategory::CondensibleVolatile;
        s.molarMassKgMol = 0.018015;
        s.greenhousePotency = 0.0;
        s.condensationTempK_1atm = 273.15;
        s.rayleighColor = glm::vec3(0.85f, 0.92f, 0.98f);
        s.specificHeatJPerKgK = 2090.0;
        s.isAtmospheric = false;
        s.isCondensible = true;
        reg(s);
    }

    // Common database and colloquial name aliases for seamless DB hydration
    s_speciesRegistry["Nitrogen"] = s_speciesRegistry["N2"];
    s_speciesRegistry["Oxygen"] = s_speciesRegistry["O2"];
    s_speciesRegistry["Carbon Dioxide"] = s_speciesRegistry["CO2"];
    s_speciesRegistry["Water Vapor"] = s_speciesRegistry["H2O"];
    s_speciesRegistry["Water"] = s_speciesRegistry["H2O"];
    s_speciesRegistry["Methane"] = s_speciesRegistry["CH4"];
    s_speciesRegistry["Hydrogen"] = s_speciesRegistry["H2"];
    s_speciesRegistry["Helium"] = s_speciesRegistry["He"];
    s_speciesRegistry["Argon"] = s_speciesRegistry["Ar"];
    s_speciesRegistry["Sulfur Dioxide"] = s_speciesRegistry["SO2"];
    s_speciesRegistry["Ammonia"] = s_speciesRegistry["NH3"];
    s_speciesRegistry["Carbon Monoxide"] = s_speciesRegistry["CO"];

    s_initialized = true;
}

const ChemicalSpecies& ChemicalSystem::getSpecies(const std::string& idOrName) {
    initialize();
    auto it = s_speciesRegistry.find(idOrName);
    if (it != s_speciesRegistry.end()) {
        return it->second;
    }
    // Search by displayName or formula
    for (const auto& pair : s_speciesRegistry) {
        if (pair.second.displayName == idOrName ||
            pair.second.formula == idOrName ||
            idOrName.find(pair.second.displayName) != std::string::npos ||
            pair.second.displayName.find(idOrName) != std::string::npos) {
            return pair.second;
        }
    }
    static ChemicalSpecies fallback = s_speciesRegistry["N2"];
    return fallback;
}

const std::unordered_map<std::string, ChemicalSpecies>& ChemicalSystem::getAllSpecies() {
    initialize();
    return s_speciesRegistry;
}

double ChemicalSystem::calculateThermalVelocity(double molarMassKgMol, double temperatureK) {
    if (molarMassKgMol <= 0.0 || temperatureK <= 0.0) return 0.0;
    // v_th = sqrt(3 * R_gas * T / mu)
    return std::sqrt((3.0 * R_GAS * temperatureK) / molarMassKgMol);
}

double ChemicalSystem::calculateEscapeParameter(double escapeVelocityMps, double thermalVelocityMps) {
    if (thermalVelocityMps <= 1e-4) return 100.0;
    double ratio = escapeVelocityMps / thermalVelocityMps;
    return ratio * ratio;
}

bool ChemicalSystem::isGasRetained(double escapeVelocityMps, double molarMassKgMol, double temperatureK) {
    if (escapeVelocityMps <= 10.0) return false;
    double v_th = calculateThermalVelocity(molarMassKgMol, temperatureK);
    // Standard Jeans stability criterion: v_esc >= 5.5 * v_th (lambda >= 30)
    return (escapeVelocityMps >= 5.0 * v_th);
}

AtmosphericRetentionStatus ChemicalSystem::evaluateGasRetention(
    const ChemicalSpecies& species,
    double escapeVelocityMps,
    double exobaseTempK
) {
    AtmosphericRetentionStatus status;
    status.speciesId = species.id;
    status.formula = species.formula;
    status.thermalVelocityMps = calculateThermalVelocity(species.molarMassKgMol, exobaseTempK);
    status.escapeParameterLambda = calculateEscapeParameter(escapeVelocityMps, status.thermalVelocityMps);

    if (escapeVelocityMps <= 50.0) {
        status.isRetained = false;
        status.statusText = "Rapid Thermal Escape (Sub-orbital)";
    } else if (status.escapeParameterLambda >= 36.0) { // v_esc >= 6 * v_th
        status.isRetained = true;
        status.statusText = "Stable (> 4 Gyr)";
    } else if (status.escapeParameterLambda >= 25.0) { // 5 * v_th <= v_esc < 6 * v_th
        status.isRetained = true;
        status.statusText = "Gradual Depletion (~100 Myr - 1 Gyr)";
    } else {
        status.isRetained = false;
        status.statusText = "Rapid Thermal Escape";
    }

    return status;
}

double ChemicalSystem::calculateOpticalDepth(double surfacePressurePa, const std::vector<ChemicalAbundance>& gases) {
    if (surfacePressurePa <= 1.0 || gases.empty()) return 0.0;

    double pNorm = surfacePressurePa / P_REF_PA; // Normalized to 1 atm
    double greenhouseSum = 0.0;

    for (const auto& g : gases) {
        const auto& spec = getSpecies(g.speciesId);
        if (!spec.isAtmospheric || spec.greenhousePotency <= 0.0) continue;
        
        double moleFrac = (double)g.percentage / 100.0;
        greenhouseSum += moleFrac * spec.greenhousePotency;
    }

    // Pressure-broadened infrared absorption opacity
    double rawTau = std::pow(pNorm, 0.82) * greenhouseSum * 18.0;

    // Convective lapse rate saturation constraint at extreme optical depths
    double effectiveTau = rawTau / (1.0 + rawTau / 145.0);
    return std::clamp(effectiveTau, 0.0, 160.0);
}

double ChemicalSystem::calculateGreenhouseDeltaK(double tEffectiveK, double opticalDepth) {
    if (opticalDepth <= 0.0001 || tEffectiveK <= 1.0) return 0.0;
    
    // Milne-Eddington grey atmosphere radiative equilibrium:
    // T_surf^4 = T_eff^4 * (1 + 0.75 * tau)
    // T_surf = T_eff * (1 + 0.75 * tau)^0.25
    double tSurfaceK = tEffectiveK * std::pow(1.0 + 0.75 * opticalDepth, 0.25);
    double deltaK = tSurfaceK - tEffectiveK;
    return std::clamp(deltaK, 0.0, 1200.0);
}

double ChemicalSystem::calculateDynamicAlbedo(double baseAlbedo, double surfaceTempK, double cloudCoverage, bool hasWaterVolatiles) {
    double bareAlbedo = baseAlbedo;

    // 1. Ice Condensation & Snow Coverage (T < 273.15 K)
    if (hasWaterVolatiles) {
        if (surfaceTempK < 273.15) {
            double iceFraction = std::clamp((273.15 - surfaceTempK) / 60.0, 0.0, 0.85);
            bareAlbedo = bareAlbedo * (1.0 - iceFraction) + 0.72 * iceFraction; // Fresh ice albedo ~0.72
        } else if (surfaceTempK > 1400.0) {
            // Molten magma crust is dark and basaltic
            double meltFraction = std::clamp((surfaceTempK - 1400.0) / 600.0, 0.0, 0.90);
            bareAlbedo = bareAlbedo * (1.0 - meltFraction) + 0.08 * meltFraction;
        }
    }

    // 2. Cloud Layer Reflectivity (Albedo of thick planetary clouds ~0.68 to 0.80)
    double effectiveCloud = std::clamp(cloudCoverage, 0.0, 1.0);
    double cloudAlbedo = 0.72;

    // Combined top-of-atmosphere albedo
    double netAlbedo = bareAlbedo * (1.0 - 0.80 * effectiveCloud) + cloudAlbedo * (0.80 * effectiveCloud);
    return std::clamp(netAlbedo, 0.04, 0.92);
}

glm::vec3 ChemicalSystem::calculateRayleighScatteringColor(const std::vector<ChemicalAbundance>& gases) {
    if (gases.empty()) return glm::vec3(0.20f, 0.50f, 0.95f);

    glm::vec3 blendedColor(0.0f);
    float totalWeight = 0.0f;

    for (const auto& g : gases) {
        const auto& spec = getSpecies(g.speciesId);
        if (!spec.isAtmospheric || g.percentage <= 0.0f) continue;

        float weight = g.percentage;
        blendedColor += spec.rayleighColor * weight;
        totalWeight += weight;
    }

    if (totalWeight <= 0.0f) {
        return glm::vec3(0.20f, 0.50f, 0.95f);
    }

    return blendedColor / totalWeight;
}

double ChemicalSystem::calculateScaleHeightKm(double surfaceTempK, double surfaceGravityMps2, double meanMolarMassKgMol) {
    if (surfaceGravityMps2 <= 0.01 || meanMolarMassKgMol <= 0.0001 || surfaceTempK <= 1.0) {
        return 8.5;
    }
    // H = (R_gas * T) / (mu * g) in meters
    double hMeters = (R_GAS * surfaceTempK) / (meanMolarMassKgMol * surfaceGravityMps2);
    return std::clamp(hMeters / 1000.0, 0.1, 500.0);
}

AtmosphereModel ChemicalSystem::evaluateAtmosphericState(
    const std::string& bodyType,
    double massKg,
    double radiusM,
    double surfaceGravityMps2,
    double escapeVelocityMps,
    double incidentSolarFlux,
    double baseAlbedo,
    double currentSurfaceTempK,
    const std::vector<ChemicalAbundance>& rawComposition,
    double customPressurePa
) {
    initialize();
    AtmosphereModel model;

    bool isStar = (bodyType.find("Star") != std::string::npos || bodyType.find("Dwarf") != std::string::npos);
    bool isGasGiant = (bodyType.find("Gas Giant") != std::string::npos || bodyType.find("Ice Giant") != std::string::npos);
    bool isMoonOrAsteroid = (bodyType.find("Moon") != std::string::npos || bodyType.find("Asteroid") != std::string::npos || bodyType.find("Comet") != std::string::npos);

    // Filter and evaluate Jeans thermal retention for each gas in inventory
    // Exobase / thermosphere temperature is driven by solar UV/EUV flux (~2.5x surface T for Earth-like illumination)
    double exobaseT = isGasGiant ? std::max(currentSurfaceTempK, 150.0) : std::max(currentSurfaceTempK * 2.8, 750.0);
    double retainedSumPct = 0.0;
    double weightedMolarMass = 0.0;
    bool hasWater = false;
    bool hasVolatiles = false;

    std::vector<ChemicalAbundance> retainedGases;

    for (const auto& g : rawComposition) {
        const auto& spec = getSpecies(g.speciesId);
        if (!spec.isAtmospheric) continue;

        auto ret = evaluateGasRetention(spec, escapeVelocityMps, exobaseT);
        model.retentionStatuses.push_back(ret);

        if (ret.isRetained || isStar || isGasGiant) {
            retainedGases.push_back(g);
            retainedSumPct += g.percentage;
            weightedMolarMass += (double)g.percentage * spec.molarMassKgMol;
            hasVolatiles = true;
            if (spec.id == "H2O") hasWater = true;
        } else {
            model.escapedGases.push_back(spec.formula + " (" + spec.displayName + ")");
        }
    }

    // If warm terrestrial planet has no water vapor explicitly listed, add equilibrium hydrological vapor
    if (!hasWater && !isGasGiant && !isStar && currentSurfaceTempK >= 240.0 && currentSurfaceTempK <= 350.0 && escapeVelocityMps > 7000.0) {
        const auto& h2oSpec = getSpecies("H2O");
        ChemicalAbundance vapor;
        vapor.speciesId = h2oSpec.id;
        vapor.formula = h2oSpec.formula;
        vapor.name = h2oSpec.displayName;
        vapor.percentage = 1.2f;
        vapor.massFraction = 0.012f;
        vapor.color = glm::vec4(h2oSpec.rayleighColor, 1.0f);
        retainedGases.push_back(vapor);
        retainedSumPct += 1.2;
        weightedMolarMass += 1.2 * h2oSpec.molarMassKgMol;
        hasWater = true;
    }

    // Normalize retained gas percentages to 100%
    if (retainedSumPct > 0.0) {
        for (auto& rg : retainedGases) {
            rg.percentage = (float)((rg.percentage / retainedSumPct) * 100.0);
        }
        model.meanMolarMassKgMol = weightedMolarMass / retainedSumPct;
    } else {
        model.meanMolarMassKgMol = 0.02896;
    }
    model.composition = retainedGases;

    // Atmospheric Pressure Model:
    // If body cannot retain atmosphere (e.g. Moon, Ceres, Mercury with v_esc < thermal velocity of all heavy gases), pressure vanishes
    if (retainedGases.empty() || (!isGasGiant && !isStar && escapeVelocityMps < 2500.0 && isMoonOrAsteroid && customPressurePa <= 0.0)) {
        model.hasAtmosphere = false;
        model.surfacePressurePa = 0.0;
        model.surfacePressureKpa = 0.0;
        model.surfacePressureAtm = 0.0;
        model.columnMassKgM2 = 0.0;
        model.opticalDepth = 0.0;
        model.greenhouseDeltaK = 0.0;
        model.cloudCoverage = 0.0;
        model.dynamicAlbedo = calculateDynamicAlbedo(baseAlbedo, currentSurfaceTempK, 0.0, false);
        model.transmittedRadiationFlux = incidentSolarFlux;
        model.atmosphericShieldingFactor = 0.0;
        model.visualDensityFactor = 0.0f;
        model.surfaceTempK = std::pow(std::max(0.1, (incidentSolarFlux * (1.0 - model.dynamicAlbedo)) / (4.0 * SIGMA_SB)), 0.25);
        return model;
    }

    model.hasAtmosphere = true;

    // Set or compute surface pressure
    if (customPressurePa >= 0.0) {
        model.surfacePressurePa = customPressurePa;
    } else if (isGasGiant) {
        model.surfacePressurePa = 1.0e7; // Nominal 100 bar deep envelope
    } else {
        // Compute pressure from volatile column mass: P = columnMass * g
        // Scale with mass and retained volatile fraction
        double massEarth = massKg / 5.972e24;
        double colMass = 10330.0 * std::pow(std::max(0.01, massEarth), 1.2) * (retainedSumPct / 100.0);
        model.surfacePressurePa = colMass * surfaceGravityMps2;
    }

    model.surfacePressureKpa = model.surfacePressurePa / 1000.0;
    model.surfacePressureAtm = model.surfacePressurePa / P_REF_PA;
    model.columnMassKgM2 = (surfaceGravityMps2 > 0.01) ? (model.surfacePressurePa / surfaceGravityMps2) : 0.0;

    // Compute Scale Height
    model.scaleHeightKm = calculateScaleHeightKm(currentSurfaceTempK, surfaceGravityMps2, model.meanMolarMassKgMol);

    // Compute Optical Depth
    model.opticalDepth = calculateOpticalDepth(model.surfacePressurePa, model.composition);

    // Compute Cloud Coverage
    if (isGasGiant) {
        model.cloudCoverage = 1.0;
    } else if (hasWater && currentSurfaceTempK >= 220.0 && currentSurfaceTempK <= 380.0) {
        // Water planet with hydrological cycle
        double waterPct = 0.0;
        for (const auto& g : model.composition) {
            if (g.speciesId == "H2O") waterPct = g.percentage;
        }
        model.cloudCoverage = std::clamp((waterPct / 3.0) * 0.55 + 0.20, 0.05, 0.85);
    } else if (model.surfacePressureAtm > 10.0) {
        model.cloudCoverage = 0.95; // Heavy overcast (Venus-like)
    } else {
        model.cloudCoverage = 0.05;
    }

    // Ice coverage on surface
    if (hasWater && currentSurfaceTempK < 273.15) {
        model.iceCoverage = std::clamp((273.15 - currentSurfaceTempK) / 50.0, 0.0, 1.0);
    } else {
        model.iceCoverage = 0.0;
    }

    // Dynamic Albedo
    model.dynamicAlbedo = calculateDynamicAlbedo(baseAlbedo, currentSurfaceTempK, model.cloudCoverage, hasWater);

    // Dynamic equilibrium surface temperature with coupled albedo & greenhouse warming
    double tEffCoupled = std::pow(std::max(0.1, (incidentSolarFlux * (1.0 - model.dynamicAlbedo)) / (4.0 * SIGMA_SB)), 0.25);
    model.greenhouseDeltaK = calculateGreenhouseDeltaK(tEffCoupled, model.opticalDepth);
    model.surfaceTempK = tEffCoupled + model.greenhouseDeltaK;

    // Atmospheric Radiation Attenuation & Shielding
    double tauShield = model.opticalDepth + (model.surfacePressureAtm * 0.4);
    double transmission = std::exp(- std::min(tauShield, 15.0) / 2.0);
    model.transmittedRadiationFlux = incidentSolarFlux * transmission;
    model.atmosphericShieldingFactor = 1.0 - transmission;

    // Rayleigh Scattering Tint
    model.rayleighScatteringColor = calculateRayleighScatteringColor(model.composition);

    // Visual Density Factor for shader [0.1, 3.0]
    model.visualDensityFactor = (float)std::clamp(std::pow(model.surfacePressureAtm, 0.45), 0.15, 3.5);

    return model;
}

std::vector<ChemicalAbundance> ChemicalSystem::generateBaselineComposition(
    const std::string& bodyType,
    const std::string& bodyId,
    double massKg,
    double radiusM,
    double distanceAU,
    double surfaceTempK
) {
    initialize();
    std::vector<ChemicalAbundance> comp;

    auto add = [&](const std::string& id, float pct, const glm::vec4& col) {
        const auto& spec = getSpecies(id);
        comp.push_back({ spec.id, spec.formula, spec.displayName, pct, pct / 100.0f, col });
    };

    if (bodyType.find("Star") != std::string::npos || bodyId == "sol") {
        add("H2", 73.46f, glm::vec4(1.0f, 0.8f, 0.2f, 1.0f));
        add("He", 24.85f, glm::vec4(0.9f, 0.5f, 0.1f, 1.0f));
        add("O2", 0.77f,  glm::vec4(0.2f, 0.8f, 0.4f, 1.0f));
        add("CO", 0.29f,  glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
        add("Iron", 0.16f,glm::vec4(0.7f, 0.6f, 0.5f, 1.0f));
    } else if (bodyType.find("Gas Giant") != std::string::npos || bodyId == "jupiter" || bodyId == "saturn") {
        add("H2",  89.8f, glm::vec4(0.9f, 0.75f, 0.55f, 1.0f));
        add("He",  10.2f, glm::vec4(0.85f, 0.65f, 0.45f, 1.0f));
        add("CH4", 0.3f,  glm::vec4(0.2f, 0.8f, 0.9f, 1.0f));
        add("NH3", 0.03f, glm::vec4(0.8f, 0.8f, 0.9f, 1.0f));
    } else if (bodyType.find("Ice Giant") != std::string::npos || bodyId == "uranus" || bodyId == "neptune") {
        add("H2",  82.5f, glm::vec4(0.4f, 0.7f, 0.9f, 1.0f));
        add("He",  15.2f, glm::vec4(0.5f, 0.8f, 0.85f, 1.0f));
        add("CH4", 2.3f,  glm::vec4(0.1f, 0.9f, 0.8f, 1.0f));
        add("H2O", 0.5f,  glm::vec4(0.3f, 0.6f, 0.95f, 1.0f));
    } else if (bodyId == "earth") {
        add("N2",  78.08f, glm::vec4(0.2f, 0.5f, 0.9f, 1.0f));
        add("O2",  20.95f, glm::vec4(0.3f, 0.8f, 0.4f, 1.0f));
        add("Ar",  0.93f,  glm::vec4(0.6f, 0.6f, 0.7f, 1.0f));
        add("H2O", 1.20f,  glm::vec4(0.4f, 0.7f, 1.0f, 1.0f));
        add("CO2", 0.04f,  glm::vec4(0.8f, 0.4f, 0.2f, 1.0f));
    } else if (bodyId == "venus") {
        add("CO2", 96.5f, glm::vec4(0.85f, 0.75f, 0.42f, 1.0f));
        add("N2",  3.5f,  glm::vec4(0.4f, 0.6f, 0.8f, 1.0f));
        add("SO2", 0.015f,glm::vec4(0.9f, 0.85f, 0.3f, 1.0f));
    } else if (bodyId == "mars") {
        add("CO2", 95.32f, glm::vec4(0.85f, 0.45f, 0.3f, 1.0f));
        add("N2",  2.6f,   glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
        add("Ar",  1.9f,   glm::vec4(0.5f, 0.5f, 0.6f, 1.0f));
        add("O2",  0.13f,  glm::vec4(0.3f, 0.8f, 0.4f, 1.0f));
    } else if (bodyId == "titan") {
        add("N2",  94.2f, glm::vec4(0.88f, 0.58f, 0.22f, 1.0f));
        add("CH4", 5.65f, glm::vec4(0.2f, 0.8f, 0.85f, 1.0f));
        add("H2",  0.15f, glm::vec4(0.4f, 0.6f, 0.9f, 1.0f));
    } else {
        // General rocky planet / moon / exoplanet
        double vEsc = (radiusM > 0.0) ? std::sqrt((2.0 * G_CONST * massKg) / radiusM) : 0.0;
        if (vEsc > 8000.0 && surfaceTempK < 350.0) {
            // Super-Earth with retained secondary atmosphere
            add("N2",  65.0f, glm::vec4(0.2f, 0.5f, 0.9f, 1.0f));
            add("CO2", 25.0f, glm::vec4(0.8f, 0.5f, 0.3f, 1.0f));
            add("H2O", 8.0f,  glm::vec4(0.3f, 0.7f, 1.0f, 1.0f));
            add("Ar",  2.0f,  glm::vec4(0.5f, 0.5f, 0.6f, 1.0f));
        } else if (vEsc > 4000.0 && surfaceTempK > 500.0) {
            // Hot dry terrestrial (Venus-like)
            add("CO2", 92.0f, glm::vec4(0.85f, 0.6f, 0.3f, 1.0f));
            add("N2",  6.0f,  glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
            add("SO2", 2.0f,  glm::vec4(0.9f, 0.8f, 0.3f, 1.0f));
        } else if (vEsc < 3000.0) {
            // Low-gravity airless body (Moon, Mercury, Asteroids)
            add("Silicates", 70.0f, glm::vec4(0.5f, 0.5f, 0.5f, 1.0f));
            add("Iron",      30.0f, glm::vec4(0.6f, 0.6f, 0.65f, 1.0f));
        } else {
            // Intermediate terrestrial world
            add("CO2", 80.0f, glm::vec4(0.8f, 0.5f, 0.3f, 1.0f));
            add("N2",  18.0f, glm::vec4(0.3f, 0.5f, 0.8f, 1.0f));
            add("Ar",  2.0f,  glm::vec4(0.5f, 0.5f, 0.6f, 1.0f));
        }
    }

    return comp;
}

std::string ChemicalSystem::formatAtmosphericSummary(const AtmosphereModel& model) {
    if (!model.hasAtmosphere || model.composition.empty()) {
        return "Airless (Vacuum)";
    }
    std::string summary;
    for (size_t i = 0; i < model.composition.size(); ++i) {
        if (i > 0) summary += ", ";
        char b[64];
        if (model.composition[i].percentage >= 1.0f) {
            snprintf(b, sizeof(b), "%s %.1f%%", model.composition[i].formula.c_str(), model.composition[i].percentage);
        } else if (model.composition[i].percentage >= 0.01f) {
            snprintf(b, sizeof(b), "%s %.2f%%", model.composition[i].formula.c_str(), model.composition[i].percentage);
        } else {
            snprintf(b, sizeof(b), "%s <0.01%%", model.composition[i].formula.c_str());
        }
        summary += b;
        if (i >= 3 && model.composition.size() > 4) {
            summary += ", ...";
            break;
        }
    }
    return summary;
}

} // namespace AstroGenesis
