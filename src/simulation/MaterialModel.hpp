#pragma once

#include <string>
#include <unordered_map>
#include <vector>
#include <cmath>
#include <algorithm>
#include <glm/glm.hpp>

namespace AstroGenesis {

enum class MaterialCategory {
    Metal,
    SilicateRock,
    VolatileIce,
    Liquid,
    BrittleMineral,
    Polymer,
    Custom
};

enum class MaterialPhase {
    Solid,
    SoftenedPlastic,
    LiquidMolten,
    VaporGas
};

// Fundamental, authoritative reference material parameters
struct MaterialDefinition {
    std::string name = "Generic Rock";
    MaterialCategory category = MaterialCategory::SilicateRock;

    // Reference Mechanical Properties (at T0 = 293.15 K, P0 = 101.3 kPa)
    double referenceDensityKgM3 = 2700.0;       // rho0 (kg/m^3)
    double youngsModulusPa = 5.0e10;            // E0 (50 GPa)
    double poissonsRatio = 0.25;                // nu (dimensionless, 0.0 to 0.49)
    double yieldStrengthPa = 1.0e8;             // sigma_y0 (100 MPa)
    double ultimateTensileStrengthPa = 1.5e8;   // sigma_uts (150 MPa)
    double compressiveStrengthPa = 8.0e8;       // sigma_comp (800 MPa)
    double fractureToughnessPaSqrtM = 2.0e6;    // K_Ic (2.0 MPa*sqrt(m))
    double strainHardeningModulusPa = 2.0e9;     // H (plastic hardening slope)
    double failureStrain = 0.04;                // Tensile failure strain (4%)

    // Reference Thermal & Phase Properties
    double specificHeatJPerKgK = 800.0;         // C_p (J / kg*K)
    double thermalConductivityWPerMK = 2.5;     // k_th (W / m*K)
    double thermalExpansionCoeffPerK = 8.0e-6;  // alpha_th (1/K)
    double meltingPointK = 1473.15;             // T_melt (1200 °C)
    double boilingPointK = 3100.0;              // T_boil
    double latentHeatFusionJPerKg = 4.0e5;      // L_f (J/kg)
    double latentHeatVaporizationJPerKg = 6.0e6;// L_v (J/kg)
    double dynamicViscosityPaS = 1.0e3;         // eta0 (Pa*s when molten)

    // Visual Albedo & Emissivity
    glm::vec3 baseColor{0.65f, 0.60f, 0.55f};
    float metallic = 0.0f;
    float roughness = 0.8f;
    float emissivity = 0.92f;
};

// Physical properties derived from the authoritative source-of-truth
struct DerivedMaterialProperties {
    double shearModulusPa = 0.0;    // G = E / [2(1 + nu)]
    double bulkModulusPa = 0.0;     // K = E / [3(1 - 2nu)]
    double lameLambdaPa = 0.0;      // lambda = E*nu / [(1+nu)(1-2nu)]
    double pWaveModulusPa = 0.0;    // M = K + 4/3*G
    double soundSpeedMps = 0.0;     // c_s = sqrt(K / rho)
    double thermalDiffusivityM2s = 0.0; // alpha = k / (rho * C_p)
};

// Instantaneous physical and thermodynamic state of a material element
struct MaterialState {
    double temperatureK = 293.15;           // Current temperature
    double pressurePa = 101325.0;           // Hydrostatic pressure
    double currentDensityKgM3 = 2700.0;     // Density under compression/expansion
    double equivalentStrain = 0.0;          // Total mechanical strain
    double strainRatePerSec = 0.0;          // d(epsilon)/dt
    double equivalentStressPa = 0.0;        // von Mises equivalent stress
    double plasticStrain = 0.0;             // Accumulated irreversible plastic strain
    double damage = 0.0;                    // Continuous damage D in [0, 1]
    double internalEnergyJ = 0.0;           // Thermal internal energy
    MaterialPhase phase = MaterialPhase::Solid;
};

class MaterialModel {
public:
    // Derives dependent elastic constants from fundamental parameters
    static DerivedMaterialProperties computeDerivedProperties(const MaterialDefinition& mat);

    // Evaluates coupled temperature-, pressure-, and damage-dependent properties
    static void evaluateCoupledProperties(
        const MaterialDefinition& mat,
        double temperatureK,
        double pressurePa,
        double strainRate,
        double damage,
        double& outEffectiveYoungsModulusPa,
        double& outEffectiveShearModulusPa,
        double& outEffectiveYieldStrengthPa,
        double& outEffectiveDensityKgM3,
        MaterialPhase& outPhase
    );
};

// Central registry of standard astrophysical and engineering materials
class MaterialLibrary {
public:
    static MaterialLibrary& instance();

    const MaterialDefinition& getMaterial(const std::string& name) const;
    void registerMaterial(const MaterialDefinition& mat);
    std::vector<std::string> getMaterialNames() const;

private:
    MaterialLibrary();
    void initializeDefaultMaterials();

    std::unordered_map<std::string, MaterialDefinition> m_materials;
    MaterialDefinition m_fallbackMaterial;
};

} // namespace AstroGenesis
