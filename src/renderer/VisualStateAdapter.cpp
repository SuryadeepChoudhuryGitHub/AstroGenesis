#include "renderer/VisualStateAdapter.hpp"
#include <cmath>
#include <algorithm>
#include <iostream>

namespace AstroGenesis {

static constexpr double G_CONST = 6.67430e-11;
static constexpr double C_LIGHT = 299792458.0;
static constexpr double AU_METERS = 149597870700.0;
static constexpr double KB_BOLTZMANN = 1.380649e-23;
static constexpr double SOL_LUMINOSITY = 3.828e26;

VisualStateAdapter::VisualStateAdapter() {
    m_visualBodies.reserve(64);
    m_starLights.reserve(8);
    m_impactEvents.reserve(32);
}

glm::vec3 VisualStateAdapter::temperatureToPlanckRGB(double kelvin) {
    // Analytic Tanner-Helland / Planckian Blackbody Chromaticity Approximation (1000 K - 50000 K)
    double temp = std::clamp(kelvin, 1000.0, 50000.0) / 100.0;
    double r, g, b;

    // Red component
    if (temp <= 66.0) {
        r = 255.0;
    } else {
        r = temp - 60.0;
        r = 329.698727446 * std::pow(r, -0.1332047592);
        r = std::clamp(r, 0.0, 255.0);
    }

    // Green component
    if (temp <= 66.0) {
        g = temp;
        g = 99.4708025861 * std::log(g) - 161.1195681661;
        g = std::clamp(g, 0.0, 255.0);
    } else {
        g = temp - 60.0;
        g = 288.1221695283 * std::pow(g, -0.0755148492);
        g = std::clamp(g, 0.0, 255.0);
    }

    // Blue component
    if (temp >= 66.0) {
        b = 255.0;
    } else if (temp <= 19.0) {
        b = 0.0;
    } else {
        b = temp - 10.0;
        b = 138.5177312231 * std::log(b) - 305.0447927307;
        b = std::clamp(b, 0.0, 255.0);
    }

    return glm::vec3((float)(r / 255.0), (float)(g / 255.0), (float)(b / 255.0));
}

glm::vec3 VisualStateAdapter::getSpectralClassColor(double kelvin, std::string& outSpectralType) {
    glm::vec3 col = temperatureToPlanckRGB(kelvin);
    if (kelvin >= 30000.0) {
        outSpectralType = "Class O (Deep Blue Supergiant)";
    } else if (kelvin >= 10000.0) {
        outSpectralType = "Class B (Blue-White Star)";
    } else if (kelvin >= 7500.0) {
        outSpectralType = "Class A (White Main Sequence)";
    } else if (kelvin >= 6000.0) {
        outSpectralType = "Class F (Yellow-White Star)";
    } else if (kelvin >= 5200.0) {
        outSpectralType = "Class G (Yellow Dwarf / Solar)";
    } else if (kelvin >= 3700.0) {
        outSpectralType = "Class K (Orange Dwarf)";
    } else if (kelvin >= 2400.0) {
        outSpectralType = "Class M (Red Dwarf / Giant)";
    } else {
        outSpectralType = "Class L/T (Cool Brown Dwarf)";
    }
    return col;
}

float VisualStateAdapter::calculateAtmosphericScaleHeightKm(double surfaceTempK, double surfaceGravityMps2, double meanMolarMassKgMol) {
    if (surfaceGravityMps2 <= 0.001 || meanMolarMassKgMol <= 0.0) return 8.5f;
    // H = (R_gas * T) / (M_molar * g)
    static constexpr double R_GAS = 8.314462618;
    double H_meters = (R_GAS * surfaceTempK) / (meanMolarMassKgMol * surfaceGravityMps2);
    return (float)(H_meters / 1000.0);
}

float VisualStateAdapter::calculateRenderRadius(double physicalRadiusM, double realRadiusAU, bool isTrueScale, float scaleMultiplier, float systemReferenceScale) {
    if (isTrueScale) {
        float r = (float)(realRadiusAU * scaleMultiplier);
        return std::max(0.00001f, r);
    }

    // Continuous, strictly monotonic power-law scale across ALL sizes (1 km -> 10,000,000 km)
    // Seamlessly and continuously maps physical radius to render radius with zero piecewise jumps
    double radiusKm = physicalRadiusM / 1000.0;
    if (radiusKm <= 0.0) radiusKm = realRadiusAU * (AU_METERS / 1000.0);
    if (radiusKm <= 0.0) radiusKm = 1000.0;

    double ratio = radiusKm / 6371.0;
    float visualR = (float)(0.030 * std::pow(ratio, 0.48));

    visualR *= scaleMultiplier * systemReferenceScale;
    return std::clamp(visualR, 0.002f, 0.85f);
}


void VisualStateAdapter::update(
    const std::vector<CelestialBody>& bodies,
    double deltaSimSeconds,
    bool isTrueScaleMode,
    float visualScaleMultiplier,
    VisualMode visualMode,
    DebugVisualOverlay debugOverlay
) {
    m_visualMode = visualMode;
    m_debugOverlay = debugOverlay;
    m_globalCloudRotationTimer += (float)deltaSimSeconds * 0.05f;

    m_visualBodies.clear();
    m_starLights.clear();

    // 1. Calculate system reference scale to dynamically prevent body overlap in tight compact systems (e.g. TRAPPIST-1)
    double minPlanetOrbitAU = 1.0;
    for (const auto& b : bodies) {
        if (b.id != "sol" && b.type.find("Star") == std::string::npos) {
            double r = (b.realOrbitRadiusAU > 0.0) ? b.realOrbitRadiusAU : (b.semiMajorAxisAU > 0.0 ? b.semiMajorAxisAU : (double)glm::length(b.position));
            if (r > 0.0001) minPlanetOrbitAU = std::min(minPlanetOrbitAU, r);
        }
    }
    float systemReferenceScale = (minPlanetOrbitAU < 0.2) ? (float)(minPlanetOrbitAU / 0.35) : 1.0f;
    systemReferenceScale = std::clamp(systemReferenceScale, 0.08f, 1.0f);

    // 2. Discover Star Light Sources (Multi-Star Lighting)
    for (const auto& b : bodies) {
        bool isStellar = (b.id == "sol" || b.type.find("Star") != std::string::npos || b.type.find("Dwarf") != std::string::npos || b.luminosityW > 1e20 || b.surfaceTempK >= 2000.0);
        if (isStellar && b.type.find("Black Hole") == std::string::npos) {
            StarLightSource light;
            light.positionAU = b.position;
            light.name = b.name;
            light.radiusAU = (float)(b.radiusM > 0.0 ? b.radiusM / AU_METERS : b.realRadiusAU);
            
            // Planckian emission color
            double effTemp = (b.surfaceTempK > 500.0) ? b.surfaceTempK : 5778.0;
            light.color = temperatureToPlanckRGB(effTemp);
            
            // Normalized intensity against Sol
            if (b.luminosityW > 0.0) {
                light.intensity = (float)std::clamp(b.luminosityW / SOL_LUMINOSITY, 0.05, 50.0);
            } else {
                light.intensity = (float)std::clamp(std::pow(effTemp / 5778.0, 4.0), 0.1, 10.0);
            }
            m_starLights.push_back(light);
        }
    }

    // Default primary light fallback if no star present in custom empty scene
    if (m_starLights.empty()) {
        StarLightSource defaultSun;
        defaultSun.positionAU = glm::vec3(0.0f);
        defaultSun.color = glm::vec3(1.0f, 0.96f, 0.89f);
        defaultSun.intensity = 1.0f;
        defaultSun.name = "Solar Reference";
        m_starLights.push_back(defaultSun);
    }

    // 3. Build Visual State for each Celestial Body
    for (const auto& b : bodies) {
        VisualBodyState vs;
        vs.dbId = b.dbId;
        vs.id = b.id;
        vs.name = b.name;
        vs.type = b.type;
        vs.positionAU = b.position;
        vs.velocityAU = b.velocity;
        
        // Geometry & Dimensions
        vs.physicalRadiusKm = (b.radiusM > 0.0) ? (b.radiusM / 1000.0) : (b.realRadiusAU * (AU_METERS / 1000.0));
        vs.trueRadiusAU = (float)(b.radiusM > 0.0 ? (b.radiusM / AU_METERS) : b.realRadiusAU);
        vs.renderRadius = calculateRenderRadius(b.radiusM, b.realRadiusAU, isTrueScaleMode, visualScaleMultiplier, systemReferenceScale);

        // Rotation & Axial Tilt
        vs.axialTiltDeg = b.axialTiltDeg;
        vs.rotationSpeedRadPerSec = (float)b.rotationSpeedRadPerSec;
        vs.currentRotationAngle = b.rotationAngle;

        glm::mat4 rot(1.0f);
        rot = glm::rotate(rot, glm::radians(b.axialTiltDeg), glm::vec3(0.0f, 0.0f, 1.0f));
        rot = glm::rotate(rot, b.rotationAngle, glm::vec3(0.0f, 1.0f, 0.0f));
        vs.rotationMatrix = rot;

        // Classification & Stellar / Relativistic Checks
        vs.isStar = (b.id == "sol" || b.type.find("Star") != std::string::npos || b.luminosityW > 1e22 || b.surfaceTempK >= 2400.0);
        vs.isBlackHole = (b.type.find("Black Hole") != std::string::npos || b.type.find("Singularity") != std::string::npos);
        vs.surfaceTempK = b.surfaceTempK;
        vs.luminosityWatts = b.luminosityW;

        // Black Hole Relativistic Radii
        if (vs.isBlackHole) {
            double massKg = (b.massKg > 0.0) ? b.massKg : 1.989e30;
            double rSchwM = (2.0 * G_CONST * massKg) / (C_LIGHT * C_LIGHT);
            vs.schwarzschildRadiusAU = (float)(rSchwM / AU_METERS);
            vs.photonSphereRadiusAU = (float)(1.5 * vs.schwarzschildRadiusAU);
        }

        // Photometry, Thermal Radiation & Emission
        vs.temperatureColor = temperatureToPlanckRGB(b.surfaceTempK);
        vs.baseAlbedo = b.color;

        if (vs.isStar) {
            std::string specClass;
            vs.temperatureColor = getSpectralClassColor(b.surfaceTempK, specClass);
            vs.emissionColor = vs.temperatureColor;
            vs.emissionIntensity = (float)std::clamp(std::pow(b.surfaceTempK / 5778.0, 2.0), 0.8, 5.0);
            vs.coronaIntensity = (m_visualMode == VisualMode::Cinematic) ? 2.5f : 1.5f;
            vs.thermalGlow = 0.0f;
        } else if (b.surfaceTempK >= 600.0) {
            // Incandescent hot planetary crust / Magma world
            float heatNorm = (float)std::clamp((b.surfaceTempK - 600.0) / 1800.0, 0.0, 1.0);
            vs.thermalGlow = heatNorm;
            vs.emissionColor = glm::mix(glm::vec3(0.85f, 0.12f, 0.01f), glm::vec3(1.0f, 0.75f, 0.25f), heatNorm);
            vs.emissionIntensity = heatNorm * 1.2f;
            vs.coronaIntensity = 0.0f;
        } else {
            vs.thermalGlow = 0.0f;
            vs.emissionIntensity = 0.0f;
            vs.coronaIntensity = 0.0f;
        }


        // Atmosphere & Cloud Physics
        bool isGasGiant = (b.type.find("Gas Giant") != std::string::npos || b.type.find("Ice Giant") != std::string::npos || b.id == "jupiter" || b.id == "saturn" || b.id == "uranus" || b.id == "neptune");
        bool hasAtmoData = (!b.atmosphereStr.empty() && b.atmosphereStr != "None" && b.atmosphereStr != "Trace");
        
        if ((hasAtmoData || isGasGiant || b.greenhouseK > 1.0) && m_enableAtmospheres) {
            vs.hasAtmosphere = true;
            vs.scaleHeightKm = calculateAtmosphericScaleHeightKm(b.surfaceTempK, b.surfaceGravityMps2);
            
            // Atmospheric outer shell thickness in visual space
            float atmoVisualThickness = isGasGiant ? (vs.renderRadius * 0.12f) : std::clamp((float)(vs.scaleHeightKm / vs.physicalRadiusKm * vs.renderRadius * 12.0f), vs.renderRadius * 0.025f, vs.renderRadius * 0.16f);
            vs.atmosphereThickness = atmoVisualThickness;
            vs.atmosphereRadius = vs.renderRadius + atmoVisualThickness;

            // Composition-based Rayleigh tint
            if (b.id == "earth" || b.atmosphereStr.find("N2") != std::string::npos || b.atmosphereStr.find("N₂") != std::string::npos) {
                vs.atmosphereColor = glm::vec3(0.18f, 0.45f, 0.95f); // Nitrogen-Oxygen Blue Rayleigh Sky
                vs.hasClouds = m_enableClouds;
                vs.cloudCoverage = 0.55f;
            } else if (b.id == "venus" || b.atmosphereStr.find("CO2") != std::string::npos || b.atmosphereStr.find("CO₂") != std::string::npos) {
                vs.atmosphereColor = glm::vec3(0.85f, 0.75f, 0.42f); // Sulfuric haze / Dense CO2
                vs.hasClouds = m_enableClouds;
                vs.cloudCoverage = 0.95f;
            } else if (b.id == "mars") {
                vs.atmosphereColor = glm::vec3(0.72f, 0.48f, 0.35f); // Thin mineral dust haze
                vs.hasClouds = false;
                vs.cloudCoverage = 0.08f;
            } else if (b.id == "titan") {
                vs.atmosphereColor = glm::vec3(0.88f, 0.58f, 0.22f); // Tholin organic orange haze
                vs.hasClouds = m_enableClouds;
                vs.cloudCoverage = 0.80f;
            } else if (isGasGiant) {
                vs.atmosphereColor = b.color * 1.1f;
                vs.hasClouds = m_enableClouds;
                vs.cloudCoverage = 1.0f;
            } else {
                vs.atmosphereColor = glm::vec3(0.35f, 0.60f, 0.90f);
            }

            vs.cloudRotationAngle = b.rotationAngle + m_globalCloudRotationTimer * 0.15f;
        }

        // Material Phase State
        if (vs.isStar) {
            vs.phase = MaterialPhase::VaporGas;
            vs.phaseFraction = 1.0f;
        } else if (b.surfaceTempK >= 1500.0) {
            vs.phase = MaterialPhase::LiquidMolten;
            vs.phaseFraction = (float)std::clamp((b.surfaceTempK - 1500.0) / 1000.0, 0.0, 1.0);
        } else if (b.surfaceTempK >= 1000.0) {
            vs.phase = MaterialPhase::SoftenedPlastic;
            vs.phaseFraction = (float)std::clamp((b.surfaceTempK - 1000.0) / 500.0, 0.0, 1.0);
        } else {
            vs.phase = MaterialPhase::Solid;
            vs.phaseFraction = 0.0f;
        }

        // Debug Physical Field False-Color Mapping
        switch (m_debugOverlay) {
            case DebugVisualOverlay::Temperature: {
                float normT = (float)std::clamp(b.surfaceTempK / 3000.0, 0.0, 1.0);
                vs.debugColor = glm::mix(glm::vec3(0.0f, 0.2f, 1.0f), glm::vec3(1.0f, 0.1f, 0.0f), normT);
                vs.debugScalar = normT;
                break;
            }
            case DebugVisualOverlay::VelocityVectors: {
                float speedNorm = (float)std::clamp(b.orbitalSpeedKmpS / 60.0, 0.0, 1.0);
                vs.debugColor = glm::mix(glm::vec3(0.1f, 0.9f, 0.3f), glm::vec3(1.0f, 0.8f, 0.0f), speedNorm);
                vs.debugScalar = speedNorm;
                break;
            }
            case DebugVisualOverlay::MaterialPhase: {
                if (vs.phase == MaterialPhase::VaporGas) vs.debugColor = glm::vec3(1.0f, 0.2f, 0.9f);
                else if (vs.phase == MaterialPhase::LiquidMolten) vs.debugColor = glm::vec3(1.0f, 0.4f, 0.0f);
                else if (vs.phase == MaterialPhase::SoftenedPlastic) vs.debugColor = glm::vec3(0.9f, 0.8f, 0.1f);
                else vs.debugColor = glm::vec3(0.2f, 0.6f, 1.0f);
                vs.debugScalar = (float)(int)vs.phase / 3.0f;
                break;
            }
            default:
                vs.debugColor = glm::vec3(0.0f);
                vs.debugScalar = 0.0f;
                break;
        }

        m_visualBodies.push_back(vs);
    }
}

const VisualBodyState* VisualStateAdapter::getVisualBody(const std::string& id) const {
    for (const auto& vb : m_visualBodies) {
        if (vb.id == id || vb.name == id) return &vb;
    }
    return nullptr;
}

const VisualBodyState* VisualStateAdapter::getVisualBody(int64_t dbId) const {
    for (const auto& vb : m_visualBodies) {
        if (vb.dbId == dbId) return &vb;
    }
    return nullptr;
}

void VisualStateAdapter::registerImpact(const glm::vec3& posAU, const glm::vec3& normal, double impactEnergyJ) {
    if (!m_enableImpactFX) return;
    
    VisualImpactEvent ev;
    ev.positionAU = posAU;
    ev.normal = glm::normalize(normal);
    ev.impactEnergyJoules = impactEnergyJ;
    
    // Scale flash size and intensity based on kinetic impact energy
    double logE = (impactEnergyJ > 1e12) ? std::log10(impactEnergyJ) : 12.0;
    ev.flashRadiusAU = (float)std::clamp((logE - 10.0) * 0.003, 0.005, 0.08);
    ev.intensity = 1.0f;
    ev.ageSeconds = 0.0f;
    ev.maxAgeSeconds = (float)std::clamp((logE - 10.0) * 0.25, 1.0, 5.0);
    
    float heat = (float)std::clamp((logE - 15.0) / 10.0, 0.0, 1.0);
    ev.thermalColor = glm::mix(glm::vec3(1.0f, 0.5f, 0.1f), glm::vec3(1.0f, 0.95f, 0.7f), heat);
    
    m_impactEvents.push_back(ev);
}

void VisualStateAdapter::updateImpactEvents(float deltaRealSeconds) {
    for (auto it = m_impactEvents.begin(); it != m_impactEvents.end(); ) {
        it->ageSeconds += deltaRealSeconds;
        it->intensity = std::max(0.0f, 1.0f - (it->ageSeconds / it->maxAgeSeconds));
        if (it->ageSeconds >= it->maxAgeSeconds || it->intensity <= 0.01f) {
            it = m_impactEvents.erase(it);
        } else {
            ++it;
        }
    }
}

} // namespace AstroGenesis
