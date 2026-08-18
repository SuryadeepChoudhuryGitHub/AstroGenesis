#include "simulation/MaterialModel.hpp"
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

DerivedMaterialProperties MaterialModel::computeDerivedProperties(const MaterialDefinition& mat) {
    DerivedMaterialProperties p;

    double E = std::max(1.0e3, mat.youngsModulusPa);
    double nu = glm::clamp(mat.poissonsRatio, 0.0, 0.499);
    double rho = std::max(1.0, mat.referenceDensityKgM3);

    // Derived Shear Modulus G = E / [2(1 + nu)]
    p.shearModulusPa = E / (2.0 * (1.0 + nu));

    // Derived Bulk Modulus K = E / [3(1 - 2*nu)]
    p.bulkModulusPa = E / (3.0 * (1.0 - 2.0 * nu));

    // First Lamé Parameter lambda = E*nu / [(1+nu)(1-2*nu)]
    p.lameLambdaPa = (E * nu) / ((1.0 + nu) * (1.0 - 2.0 * nu));

    // P-wave Modulus M = K + 4/3*G
    p.pWaveModulusPa = p.bulkModulusPa + (4.0 / 3.0) * p.shearModulusPa;

    // Longitudinal Acoustic Speed c_s = sqrt(K / rho)
    p.soundSpeedMps = std::sqrt(p.bulkModulusPa / rho);

    // Thermal Diffusivity alpha = k / (rho * C_p)
    double cp = std::max(1.0, mat.specificHeatJPerKgK);
    p.thermalDiffusivityM2s = mat.thermalConductivityWPerMK / (rho * cp);

    return p;
}

void MaterialModel::evaluateCoupledProperties(
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
) {
    double T = std::max(0.1, temperatureK);
    double T_m = std::max(1.0, mat.meltingPointK);
    double T_b = std::max(T_m + 1.0, mat.boilingPointK);

    // 1. Determine Phase
    if (T >= T_b) {
        outPhase = MaterialPhase::VaporGas;
    } else if (T >= T_m) {
        outPhase = MaterialPhase::LiquidMolten;
    } else if (T >= 0.82 * T_m) {
        outPhase = MaterialPhase::SoftenedPlastic;
    } else {
        outPhase = MaterialPhase::Solid;
    }

    // 2. Temperature-Dependent Elastic Modulus E(T) & Yield Strength sigma_y(T)
    double thermalSofteningFactor = 1.0;
    if (T < T_m) {
        double tRatio = T / T_m;
        thermalSofteningFactor = std::max(0.01, 1.0 - (tRatio * tRatio));
    } else {
        thermalSofteningFactor = 0.005; // Molten state retains only minimal fluid shear stiffness
    }

    double E_T = mat.youngsModulusPa * thermalSofteningFactor;
    double sigY_T = mat.yieldStrengthPa * std::pow(thermalSofteningFactor, 0.75);

    // 3. Strain-Rate Dynamic Hardening (Cowper-Symonds model)
    double epsDot = std::max(0.0, strainRate);
    double strainRateMultiplier = 1.0 + 0.05 * std::log(1.0 + (epsDot / 0.001));
    double sigY_dynamic = sigY_T * glm::clamp(strainRateMultiplier, 1.0, 3.5);

    // 4. Continuous Damage Degradation D in [0, 1]
    double D = glm::clamp(damage, 0.0, 1.0);
    double effectiveDamageFactor = std::max(0.01, 1.0 - D);

    outEffectiveYoungsModulusPa = E_T * effectiveDamageFactor;
    double nu = glm::clamp(mat.poissonsRatio, 0.0, 0.499);
    outEffectiveShearModulusPa = outEffectiveYoungsModulusPa / (2.0 * (1.0 + nu));
    outEffectiveYieldStrengthPa = sigY_dynamic * effectiveDamageFactor;

    // 5. Pressure-Dependent Density (Murnaghan / Linear Compressibility Equation of State)
    double K = mat.youngsModulusPa / (3.0 * (1.0 - 2.0 * nu));
    double deltaP = std::max(0.0, pressurePa - 101325.0);
    double compressionRatio = 1.0 + (deltaP / std::max(1.0e6, K));

    // Thermal expansion correction: Delta V / V = 3 * alpha_th * (T - T0)
    double thermalVolumeFactor = 1.0 + 3.0 * mat.thermalExpansionCoeffPerK * (T - 293.15);
    thermalVolumeFactor = std::max(0.5, thermalVolumeFactor);

    outEffectiveDensityKgM3 = (mat.referenceDensityKgM3 * compressionRatio) / thermalVolumeFactor;
}

// =========================================================================
// Material Library Presets
// =========================================================================

MaterialLibrary& MaterialLibrary::instance() {
    static MaterialLibrary lib;
    return lib;
}

MaterialLibrary::MaterialLibrary() {
    initializeDefaultMaterials();
}

void MaterialLibrary::initializeDefaultMaterials() {
    // 1. Iron / Steel (Fe) - Ductile Metal
    {
        MaterialDefinition fe;
        fe.name = "Iron / Structural Steel";
        fe.category = MaterialCategory::Metal;
        fe.referenceDensityKgM3 = 7850.0;
        fe.youngsModulusPa = 2.1e11;             // 210 GPa
        fe.poissonsRatio = 0.29;
        fe.yieldStrengthPa = 2.5e8;              // 250 MPa
        fe.ultimateTensileStrengthPa = 4.0e8;    // 400 MPa
        fe.compressiveStrengthPa = 1.2e9;
        fe.fractureToughnessPaSqrtM = 5.0e7;     // 50 MPa*sqrt(m)
        fe.strainHardeningModulusPa = 1.5e9;
        fe.failureStrain = 0.18;                 // 18% ductile elongation
        fe.specificHeatJPerKgK = 450.0;
        fe.thermalConductivityWPerMK = 50.0;
        fe.thermalExpansionCoeffPerK = 1.2e-5;
        fe.meltingPointK = 1811.0;               // 1538 °C
        fe.boilingPointK = 3134.0;
        fe.latentHeatFusionJPerKg = 2.47e5;
        fe.latentHeatVaporizationJPerKg = 6.09e6;
        fe.dynamicViscosityPaS = 6.0e-3;
        fe.baseColor = glm::vec3(0.72f, 0.75f, 0.78f);
        fe.metallic = 0.9f;
        fe.roughness = 0.35f;
        registerMaterial(fe);
    }

    // 2. Basalt Rock - Silicate Planetary Crust & Asteroid Material
    {
        MaterialDefinition rock;
        rock.name = "Basalt Rock";
        rock.category = MaterialCategory::SilicateRock;
        rock.referenceDensityKgM3 = 2900.0;
        rock.youngsModulusPa = 6.5e10;            // 65 GPa
        rock.poissonsRatio = 0.23;
        rock.yieldStrengthPa = 1.2e8;             // 120 MPa
        rock.ultimateTensileStrengthPa = 1.4e8;   // 140 MPa
        rock.compressiveStrengthPa = 1.5e9;       // 1.5 GPa compressive strength
        rock.fractureToughnessPaSqrtM = 2.5e6;
        rock.strainHardeningModulusPa = 5.0e8;
        rock.failureStrain = 0.025;               // 2.5% brittle strain limit
        rock.specificHeatJPerKgK = 840.0;
        rock.thermalConductivityWPerMK = 2.2;
        rock.thermalExpansionCoeffPerK = 5.4e-6;
        rock.meltingPointK = 1450.0;
        rock.boilingPointK = 2800.0;
        rock.latentHeatFusionJPerKg = 4.2e5;
        fe_fallback:
        rock.baseColor = glm::vec3(0.35f, 0.34f, 0.33f);
        rock.metallic = 0.0f;
        rock.roughness = 0.85f;
        registerMaterial(rock);
    }

    // 3. Water Ice (H2O Solid) - Outer Solar System Bodies & Comets
    {
        MaterialDefinition ice;
        ice.name = "Water Ice";
        ice.category = MaterialCategory::VolatileIce;
        ice.referenceDensityKgM3 = 917.0;
        ice.youngsModulusPa = 9.0e9;              // 9 GPa
        ice.poissonsRatio = 0.33;
        ice.yieldStrengthPa = 3.5e6;              // 3.5 MPa
        ice.ultimateTensileStrengthPa = 5.0e6;    // 5.0 MPa
        ice.compressiveStrengthPa = 3.5e7;        // 35 MPa
        ice.fractureToughnessPaSqrtM = 1.1e5;
        ice.strainHardeningModulusPa = 1.0e8;
        ice.failureStrain = 0.015;                // 1.5% brittle
        ice.specificHeatJPerKgK = 2090.0;
        ice.thermalConductivityWPerMK = 2.22;
        ice.thermalExpansionCoeffPerK = 5.1e-5;
        ice.meltingPointK = 273.15;               // 0 °C
        ice.boilingPointK = 373.15;
        ice.latentHeatFusionJPerKg = 3.34e5;
        ice.latentHeatVaporizationJPerKg = 2.26e6;
        ice.dynamicViscosityPaS = 1.0e-3;
        ice.baseColor = glm::vec3(0.78f, 0.88f, 0.95f);
        ice.metallic = 0.0f;
        ice.roughness = 0.15f;
        registerMaterial(ice);
    }

    // 4. Titanium Alloy (Ti-6Al-4V) - Aerospace / Spacecraft Structure
    {
        MaterialDefinition ti;
        ti.name = "Titanium Alloy";
        ti.category = MaterialCategory::Metal;
        ti.referenceDensityKgM3 = 4430.0;
        ti.youngsModulusPa = 1.14e11;            // 114 GPa
        ti.poissonsRatio = 0.34;
        ti.yieldStrengthPa = 8.8e8;              // 880 MPa
        ti.ultimateTensileStrengthPa = 9.5e8;    // 950 MPa
        ti.compressiveStrengthPa = 1.8e9;
        ti.fractureToughnessPaSqrtM = 7.5e7;
        ti.strainHardeningModulusPa = 2.0e9;
        ti.failureStrain = 0.14;                 // 14% elongation
        ti.specificHeatJPerKgK = 526.0;
        ti.thermalConductivityWPerMK = 6.7;
        ti.thermalExpansionCoeffPerK = 8.6e-6;
        ti.meltingPointK = 1933.0;
        ti.boilingPointK = 3560.0;
        ti.latentHeatFusionJPerKg = 2.9e5;
        ti.baseColor = glm::vec3(0.68f, 0.70f, 0.73f);
        ti.metallic = 0.95f;
        ti.roughness = 0.28f;
        registerMaterial(ti);
    }

    // 5. Glass / Obsidian - Highly Brittle
    {
        MaterialDefinition glass;
        glass.name = "Glass / Obsidian";
        glass.category = MaterialCategory::BrittleMineral;
        glass.referenceDensityKgM3 = 2500.0;
        glass.youngsModulusPa = 7.0e10;            // 70 GPa
        glass.poissonsRatio = 0.22;
        glass.yieldStrengthPa = 5.0e7;             // 50 MPa
        glass.ultimateTensileStrengthPa = 6.0e7;   // 60 MPa
        glass.compressiveStrengthPa = 1.0e9;       // 1.0 GPa
        glass.fractureToughnessPaSqrtM = 7.5e5;    // Low fracture toughness
        glass.strainHardeningModulusPa = 0.0;      // Perfectly brittle
        glass.failureStrain = 0.008;               // 0.8% failure strain
        glass.specificHeatJPerKgK = 750.0;
        glass.thermalConductivityWPerMK = 1.0;
        glass.thermalExpansionCoeffPerK = 8.5e-6;
        glass.meltingPointK = 1700.0;
        glass.baseColor = glm::vec3(0.18f, 0.22f, 0.25f);
        glass.roughness = 0.08f;
        registerMaterial(glass);
    }

    // 6. Rubber / Elastomer - High Elastic Compliance
    {
        MaterialDefinition rubber;
        rubber.name = "Rubber / Elastomer";
        rubber.category = MaterialCategory::Polymer;
        rubber.referenceDensityKgM3 = 1100.0;
        rubber.youngsModulusPa = 5.0e7;             // 50 MPa (highly compliant)
        rubber.poissonsRatio = 0.48;               // Nearly incompressible
        rubber.yieldStrengthPa = 2.5e7;
        rubber.ultimateTensileStrengthPa = 3.5e7;
        rubber.compressiveStrengthPa = 1.0e8;
        rubber.fractureToughnessPaSqrtM = 5.0e5;
        rubber.strainHardeningModulusPa = 1.0e7;
        rubber.failureStrain = 3.50;                // 350% hyperelastic elongation
        rubber.specificHeatJPerKgK = 1800.0;
        rubber.thermalConductivityWPerMK = 0.15;
        rubber.thermalExpansionCoeffPerK = 2.2e-4;
        rubber.meltingPointK = 450.0;
        rubber.baseColor = glm::vec3(0.12f, 0.12f, 0.14f);
        rubber.roughness = 0.90f;
        registerMaterial(rubber);
    }

    m_fallbackMaterial = m_materials["Basalt Rock"];
}

void MaterialLibrary::registerMaterial(const MaterialDefinition& mat) {
    m_materials[mat.name] = mat;
}

const MaterialDefinition& MaterialLibrary::getMaterial(const std::string& name) const {
    auto it = m_materials.find(name);
    if (it != m_materials.end()) {
        return it->second;
    }
    return m_fallbackMaterial;
}

std::vector<std::string> MaterialLibrary::getMaterialNames() const {
    std::vector<std::string> names;
    names.reserve(m_materials.size());
    for (const auto& pair : m_materials) {
        names.push_back(pair.first);
    }
    return names;
}

} // namespace AstroGenesis
