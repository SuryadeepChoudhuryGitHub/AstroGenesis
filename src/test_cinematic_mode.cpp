#include <iostream>
#include <cassert>
#include <cmath>
#include "renderer/Camera.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "simulation/CelestialBody.hpp"

using namespace AstroGenesis;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " AstroGenesis Cinematic Mode & Photo Mode Verification" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Verify Camera System Extensions (Roll, FOV, DoF, Planes)
    {
        Camera cam;
        assert(std::abs(cam.getRoll()) < 1e-5f);
        
        // Roll manipulation
        cam.roll(glm::radians(45.0f));
        assert(std::abs(glm::degrees(cam.getRoll()) - 45.0f) < 1e-3f);
        cam.setRoll(glm::radians(-30.0f));
        assert(std::abs(glm::degrees(cam.getRoll()) - (-30.0f)) < 1e-3f);
        cam.resetRoll();
        assert(std::abs(cam.getRoll()) < 1e-5f);

        // FOV clamping
        cam.setFOV(55.0f);
        assert(std::abs(cam.getFOV() - 55.0f) < 1e-3f);
        cam.setFOV(5.0f); // Should clamp to 10.0
        assert(std::abs(cam.getFOV() - 10.0f) < 1e-3f);
        cam.setFOV(150.0f); // Should clamp to 120.0
        assert(std::abs(cam.getFOV() - 120.0f) < 1e-3f);
        cam.setFOV(45.0f);

        // Near / Far planes
        assert(cam.getNearPlane() > 0.0f);
        assert(cam.getFarPlane() >= 500.0f);

        // View Matrix with Roll
        glm::mat4 viewNoRoll = cam.getViewMatrix();
        cam.setRoll(glm::radians(90.0f));
        glm::mat4 viewWithRoll = cam.getViewMatrix();
        assert(viewNoRoll != viewWithRoll);

        std::cout << "[Test 1] Camera Roll, FOV Clamping & View Matrix -> PASS" << std::endl;
    }

    // 2. Verify Visual State Presets
    {
        VisualStateAdapter adapter;
        assert(!adapter.isCinematicModeEnabled());
        assert(adapter.getPreset() == VisualPreset::Normal);

        // Apply Realistic Preset
        adapter.applyPreset(VisualPreset::Realistic);
        assert(adapter.getPreset() == VisualPreset::Realistic);
        assert(adapter.isCinematicModeEnabled());
        assert(!adapter.isDoFEnabled());
        assert(!adapter.isVignetteEnabled());
        assert(!adapter.areOrbitLinesEnabled()); // Clean cinematic space view
        assert(!adapter.areMotionTrailsEnabled());

        // Apply Cinematic Preset
        adapter.applyPreset(VisualPreset::Cinematic);
        assert(adapter.getPreset() == VisualPreset::Cinematic);
        assert(adapter.isCinematicModeEnabled());
        assert(!adapter.areOrbitLinesEnabled());
        assert(adapter.getBloomIntensity() >= 0.35f && adapter.getBloomIntensity() <= 0.60f);

        // Apply Ultra Preset
        adapter.applyPreset(VisualPreset::Ultra);
        assert(adapter.getPreset() == VisualPreset::Ultra);
        assert(adapter.isCinematicModeEnabled());
        assert(adapter.getBloomIntensity() >= 0.50f);

        // Test DoF toggle
        adapter.setDoFEnabled(true);
        assert(adapter.isDoFEnabled());

        // Apply Normal Preset
        adapter.applyPreset(VisualPreset::Normal);
        assert(!adapter.isCinematicModeEnabled());
        assert(!adapter.isVignetteEnabled());
        assert(adapter.areOrbitLinesEnabled()); // Scientific mode preserves orbit lines
        assert(adapter.areMotionTrailsEnabled());

        std::cout << "[Test 2] Visual Quality Presets (Normal, Realistic, Cinematic, Ultra) -> PASS" << std::endl;
    }

    // 3. Verify Photo Mode & UI Visibility State Toggles
    {
        VisualStateAdapter adapter;
        assert(!adapter.isPhotoModeActive());
        assert(!adapter.isUIHidden());

        adapter.setPhotoModeActive(true);
        assert(adapter.isPhotoModeActive());

        adapter.toggleUIHidden();
        assert(adapter.isUIHidden());
        adapter.toggleUIHidden();
        assert(!adapter.isUIHidden());

        adapter.setExposure(2.5f);
        assert(std::abs(adapter.getExposure() - 2.5f) < 1e-4f);

        adapter.setFocusDistance(1.496f);
        assert(std::abs(adapter.getFocusDistance() - 1.496f) < 1e-4f);

        std::cout << "[Test 3] Photo Mode & Clean Screenshot UI Toggles -> PASS" << std::endl;
    }

    // 4. Verify Physical Parameter Derivation for Shaders
    {
        VisualStateAdapter adapter;

        std::vector<CelestialBody> bodies;
        
        // Sun
        CelestialBody sun;
        sun.id = "sol";
        sun.name = "Sun";
        sun.type = "Star (G2V)";
        sun.position = glm::vec3(0.0f);
        sun.radius3D = 0.285f;
        sun.radiusM = 696340000.0;
        sun.surfaceTempK = 5778.0;
        sun.luminosityW = 3.828e26;
        bodies.push_back(sun);

        // Earth
        CelestialBody earth;
        earth.id = "earth";
        earth.name = "Earth";
        earth.type = "Terrestrial Planet";
        earth.position = glm::vec3(1.0f, 0.0f, 0.0f);
        earth.radius3D = 0.03f;
        earth.radiusM = 6371000.0;
        earth.surfaceTempK = 288.0;
        earth.hasAtmosphere = true;
        earth.surfacePressurePa = 101325.0;
        earth.surfacePressureKpa = 101.325;
        earth.scaleHeightKm = 8.4;
        earth.cloudCoverage = 0.45;
        earth.iceCoverage = 0.10;
        ChemicalAbundance h2o; h2o.speciesId = "H2O"; h2o.name = "Water Vapor"; h2o.percentage = 1.2f;
        earth.chemicalInventory.push_back(h2o);
        ChemicalAbundance n2; n2.speciesId = "N2"; n2.name = "Nitrogen"; n2.percentage = 78.0f;
        earth.chemicalInventory.push_back(n2);
        ChemicalAbundance o2; o2.speciesId = "O2"; o2.name = "Oxygen"; o2.percentage = 20.8f;
        earth.chemicalInventory.push_back(o2);
        bodies.push_back(earth);

        // Saturn with Ring
        CelestialBody saturn;
        saturn.id = "saturn";
        saturn.name = "Saturn";
        saturn.type = "Gas Giant";
        saturn.position = glm::vec3(9.5f, 0.0f, 0.0f);
        saturn.radius3D = 0.08f;
        saturn.radiusM = 58232000.0;
        saturn.surfaceTempK = 134.0;
        saturn.ring.hasRing = true;
        saturn.ring.innerRadius3D = 0.11f;
        saturn.ring.outerRadius3D = 0.20f;
        saturn.ring.baseColor = glm::vec3(0.85f, 0.78f, 0.65f);
        bodies.push_back(saturn);

        adapter.update(bodies, 0.016, false, 1.0f, VisualMode::Realistic, DebugVisualOverlay::None);

        const auto* vEarth = adapter.getVisualBody("earth");
        assert(vEarth != nullptr);
        assert(vEarth->waterFraction > 0.0f);
        assert(vEarth->hasAtmosphere);
        assert(!vEarth->hasClouds); // Procedural clouds disabled by default
        assert(vEarth->scaleHeightKm > 7.0 && vEarth->scaleHeightKm < 9.0);
        assert(vEarth->surfaceRoughness < 0.6f); // Ocean specularity reduces average roughness

        const auto* vSaturn = adapter.getVisualBody("saturn");
        assert(vSaturn != nullptr);
        assert(!vSaturn->hasClouds); // Gas giants never receive generic clouds
        assert(vSaturn->hasRing);
        assert(vSaturn->ringInnerRadiusAU > 0.0f);
        assert(vSaturn->ringOuterRadiusAU > vSaturn->ringInnerRadiusAU);

        // Explicit enable test
        adapter.setCloudsEnabled(true);
        adapter.update(bodies, 0.016, false, 1.0f, VisualMode::Realistic, DebugVisualOverlay::None);
        assert(adapter.getVisualBody("earth")->hasClouds);
        assert(!adapter.getVisualBody("saturn")->hasClouds); // Gas giants still protected

        // Star Light extraction
        const auto& starLights = adapter.getStarLightSources();
        assert(!starLights.empty());
        assert(starLights[0].intensity > 0.0f);

        std::cout << "[Test 4] Physical-to-Visual Parameter Mapping (Ocean, Ice, Scale Height, Rings, Lights) -> PASS" << std::endl;
    }

    // 5. Verify Orbit Visualization & User Preference Persistence across Modes
    {
        VisualStateAdapter adapter;

        // TEST 1: Initial startup in Normal Mode -> Orbit lines & trails MUST be enabled
        assert(adapter.getPreset() == VisualPreset::Normal);
        assert(!adapter.isCinematicModeEnabled());
        assert(adapter.areOrbitLinesEnabled());
        assert(adapter.areMotionTrailsEnabled());

        // TEST 2: Switch Normal -> Realistic / Cinematic -> Orbit lines & trails MUST be hidden
        adapter.setCinematicMode(true);
        assert(adapter.isCinematicModeEnabled());
        assert(!adapter.areOrbitLinesEnabled());
        assert(!adapter.areMotionTrailsEnabled());

        // TEST 3: Switch back Cinematic -> Normal -> Orbit lines & trails MUST automatically return
        adapter.setCinematicMode(false);
        assert(!adapter.isCinematicModeEnabled());
        assert(adapter.areOrbitLinesEnabled());
        assert(adapter.areMotionTrailsEnabled());

        // Test with setVisualMode directly
        adapter.setVisualMode(VisualMode::Cinematic);
        assert(!adapter.areOrbitLinesEnabled());
        assert(!adapter.areMotionTrailsEnabled());

        adapter.setVisualMode(VisualMode::Scientific);
        assert(adapter.areOrbitLinesEnabled());
        assert(adapter.areMotionTrailsEnabled());

        // TEST 4: User toggle preferences in Normal Mode are respected
        adapter.setOrbitLinesEnabled(false);
        assert(!adapter.areOrbitLinesEnabled());

        // Entering cinematic mode hides it regardless
        adapter.applyPreset(VisualPreset::Realistic);
        assert(!adapter.areOrbitLinesEnabled());

        // Returning to normal mode restores the user's manual preference (false)
        adapter.applyPreset(VisualPreset::Normal);
        assert(!adapter.areOrbitLinesEnabled());

        // User turns it back on
        adapter.setOrbitLinesEnabled(true);
        assert(adapter.areOrbitLinesEnabled());

        adapter.applyPreset(VisualPreset::Cinematic);
        assert(!adapter.areOrbitLinesEnabled());

        adapter.applyPreset(VisualPreset::Normal);
        assert(adapter.areOrbitLinesEnabled());

        // Verification of Dwarf Planet vs Star illumination (Sol is only star light)
        std::vector<CelestialBody> bodies;
        CelestialBody sol; sol.id = "sol"; sol.name = "Sun"; sol.type = "Star (G2V)"; sol.surfaceTempK = 5778.0; sol.luminosityW = 3.828e26;
        CelestialBody earth; earth.id = "earth"; earth.name = "Earth"; earth.type = "Terrestrial Planet"; earth.surfaceTempK = 288.0;
        CelestialBody ceres; ceres.id = "ceres"; ceres.name = "Ceres"; ceres.type = "Dwarf Planet"; ceres.surfaceTempK = 168.0;
        CelestialBody pluto; pluto.id = "pluto"; pluto.name = "Pluto"; pluto.type = "Dwarf Planet"; pluto.surfaceTempK = 44.0;
        bodies.push_back(sol);
        bodies.push_back(earth);
        bodies.push_back(ceres);
        bodies.push_back(pluto);

        adapter.update(bodies, 0.016, false, 1.0f, VisualMode::Realistic, DebugVisualOverlay::None);
        const auto& lights = adapter.getStarLightSources();
        assert(lights.size() == 1);
        assert(lights[0].name == "Sun");

        std::cout << "[Test 5] Orbit Lines Visibility, Mode Persistence & Dwarf Planet Star Light Check -> PASS" << std::endl;
    }

    std::cout << "==========================================================" << std::endl;
    std::cout << " ALL CINEMATIC GRAPHICS & PHOTO MODE TESTS PASSED!" << std::endl;
    std::cout << "==========================================================" << std::endl;
    return 0;
}
