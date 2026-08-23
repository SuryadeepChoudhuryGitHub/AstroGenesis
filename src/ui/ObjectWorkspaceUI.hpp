#pragma once

#include "imgui.h"
#include <string>
#include <vector>
#include <optional>
#include "data/AstronomicalModels.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "renderer/Camera.hpp"

namespace AstroGenesis {

class ObjectWorkspaceUI {
public:
    ObjectWorkspaceUI();

    void render(ObjectRepository& objRepo, 
                PhysicsEngine& physics, 
                Camera& camera,
                int& activeTopTab,
                float winW, float winH);

    void setSelectedObjectBySlug(const std::string& slug, ObjectRepository& objRepo);

private:
    void drawLibraryPanel(ObjectRepository& objRepo, float panelW, float panelH);
    void drawEditorPanel(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float panelW, float panelH);

    // Context-sensitive editor sections
    void drawStarEditor(CelestialBody& body);
    void drawPlanetEditor(CelestialBody& body);
    void drawMoonEditor(CelestialBody& body, ObjectRepository& objRepo);
    void drawAsteroidCometEditor(CelestialBody& body);
    void drawBlackHoleEditor(CelestialBody& body);
    void drawCompositionEditor(CelestialBody& body);

    void recomputeDerived(CelestialBody& body);
    void createNewObjectTemplate(const std::string& objectType);

    // State
    int m_selectedCategoryFilter = 0; // 0: All, 1: Stars, 2: Planets, 3: Moons, 4: Asteroids & Comets, 5: Black Holes, 6: Custom
    char m_searchFilter[64] = "";
    int64_t m_selectedObjectId = 0;
    std::string m_selectedObjectSlug = "earth";
    CelestialBody m_editingBody;
    bool m_isDirty = false;
    std::string m_statusFeedbackMsg;
};

} // namespace AstroGenesis
