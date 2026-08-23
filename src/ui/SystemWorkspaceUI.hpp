#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <optional>
#include "data/AstronomicalModels.hpp"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "renderer/Camera.hpp"

namespace AstroGenesis {

class SystemWorkspaceUI {
public:
    SystemWorkspaceUI();

    void render(DataManager& dataManager, 
                ObjectRepository& objRepo, 
                PhysicsEngine& physics, 
                Camera& camera,
                int& activeTopTab,
                float winW, float winH);

    // External triggers
    void openCustomBuilderWithSystem(const std::string& systemName, ObjectRepository& objRepo);
    void openCustomBuilderNew();

private:
    // Tab Renders
    void drawHeaderAndModes(float winW);
    void drawImportMode(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH);
    void drawCustomBuilderMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH);
    void drawSavedSystemsMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH);
    void drawPresetsMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH);

    // Custom Builder Sub-Panels
    void drawBuilderSystemTree(ObjectRepository& objRepo, float panelW, float panelH);
    void drawBuilderSchematicCanvas(float panelW, float panelH);
    void drawBuilderObjectEditor(ObjectRepository& objRepo, float panelW, float panelH);
    void drawBuilderActionFooter(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab);
    void drawValidationModal(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab);

    // Helpers
    void recomputeDerivedProperties(CelestialBody& body, const CelestialBody* parent);
    void applyOrbitInitializer(CelestialBody& body, const CelestialBody& parent, double distAU, double ecc, double incDeg);
    void createNewDefaultObject(const std::string& type, std::optional<int64_t> parentId = std::nullopt);
    void loadSystemIntoBuilder(const std::string& systemName, ObjectRepository& objRepo);
    void runBuilderSimulation(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab);


    // State Variables
    int m_currentMode = 1; // 0: Import Existing, 1: Create Custom, 2: Saved Systems, 3: Presets

    // Mode A: Import State
    int m_selectedProviderIdx = 2; // 0: JPL Horizons, 1: JPL SBDB, 2: NASA Exoplanet, 3: Local Seed
    char m_searchBuffer[128] = "Kepler-90";
    std::string m_selectedImportSystemName;
    std::vector<CelestialBodyRecord> m_importPreviewBodies;
    std::vector<bool> m_importSelectionFlags;
    bool m_importLoading = false;
    std::string m_importStatusMsg;

    // Mode B: Builder State
    SystemRecord m_builderSystem;
    std::vector<CelestialBody> m_builderBodies;
    int m_selectedNodeIndex = 0;
    char m_systemNameBuf[128] = "My Custom Star System";
    char m_systemDescBuf[256] = "Custom multi-body celestial simulation";
    std::vector<SystemValidationWarning> m_currentValidationWarnings;
    bool m_showValidationWarningPopup = false;
    bool m_dismissWarningsAndRun = false;

    // Orbit Initializer Tool State
    int m_orbitInitParentIdx = 0;
    float m_orbitInitDistAU = 1.0f;
    float m_orbitInitEcc = 0.0f;
    float m_orbitInitIncDeg = 0.0f;

    // Unit conversion helpers for editor
    int m_massUnitIdx = 0; // 0: Earth Masses, 1: Solar Masses, 2: Jupiter Masses, 3: kg
    int m_radiusUnitIdx = 0; // 0: Earth Radii, 1: Solar Radii, 2: Jupiter Radii, 3: km
    int m_distUnitIdx = 0; // 0: AU, 1: km

    // Schematic Canvas State
    float m_schematicZoom = 1.0f;
    ImVec2 m_schematicPan{0.0f, 0.0f};

    // Feedback message
    std::string m_actionFeedbackMsg;
    float m_actionFeedbackTimer = 0.0f;
};

} // namespace AstroGenesis
