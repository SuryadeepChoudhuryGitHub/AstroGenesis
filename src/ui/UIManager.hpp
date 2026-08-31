#pragma once

#include "imgui.h"
#include "renderer/Camera.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "simulation/ValidationEngine.hpp"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "ui/DataManagerUI.hpp"
#include "ui/ValidationUI.hpp"
#include "ui/SystemWorkspaceUI.hpp"
#include "ui/ObjectWorkspaceUI.hpp"

namespace AstroGenesis {

struct EventLogEntry {
    std::string timeStr;
    std::string message;
};

class UIManager {
public:
    UIManager();

    void initialize();
    void renderUI(PhysicsEngine& physics, 
                  Camera& camera, 
                  ObjectRepository& objRepo,
                  DataManager& dataManager,
                  ValidationEngine& valEngine,
                  VisualStateAdapter& visualAdapter,
                  float windowWidth, float windowHeight, float fps);

    bool isViewportHovered() const { return m_viewportHovered; }
    int getHoveredBodyIndex() const { return m_hoveredBodyIndex; }
    void getViewportBounds(float& outX, float& outY, float& outW, float& outH) const;

    void addEventLog(const std::string& message);

    void openDataManager() { m_showDataManager = true; }
    void openValidationDashboard() { m_showValidationDashboard = true; }
    void setActiveTopTab(int tab) { m_activeTopTab = tab; }
    int getActiveTopTab() const { return m_activeTopTab; }

private:
    void drawTopBar(float width, PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo);
    void drawLeftPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH);
    void drawInfoOverlay(const CelestialBody& body, float x, float y);
    void drawRightPanel(PhysicsEngine& physics, CelestialBody& body, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, float topBarH, float winW, float winH, float statusBarH);
    void drawViewportHUD(PhysicsEngine& physics, Camera& camera, VisualStateAdapter& visualAdapter, float vpX, float vpY, float vpW, float vpH);
    void drawTimeControls(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float x, float y, float w, float h);
    void drawSimMetrics(PhysicsEngine& physics, float fps, float x, float y, float w, float h);
    void drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h);
    void drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH);
    void drawAsteroidBeltDiagnostics(PhysicsEngine& physics, ObjectRepository& objRepo, float winW, float winH);
    void drawMatterLab(PhysicsEngine& physics, float winW, float winH);

    // Extra Workspaces
    void drawExploreWorkspace(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, float winW, float winH);
    void drawSimulationWorkspace(PhysicsEngine& physics, Camera& camera, ValidationEngine& valEngine, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, float winW, float winH);
    void drawAIAssistantWorkspace(PhysicsEngine& physics, ObjectRepository& objRepo, float winW, float winH);

    bool m_viewportHovered = false;
    int m_hoveredBodyIndex = -1;
    bool m_showAsteroidBeltDiagnostics = false;
    bool m_showMatterLab = false;
    bool m_showDataManager = false;
    bool m_showValidationDashboard = false;
    int m_activeTopTab = 0; // 0: UNIVERSE, 1: SYSTEM, 2: OBJECTS, 3: EXPLORE, 4: SIMULATION, 5: AI ASSISTANT
    char m_searchQuery[64] = "";

    float m_viewportX = 210.0f;
    float m_viewportY = 48.0f;
    float m_viewportW = 1080.0f;
    float m_viewportH = 632.0f;
    float m_orbitVisZoom = 1.0f;

    std::vector<EventLogEntry> m_eventLogs;
    DataManagerUI m_dataManagerUI;
    ValidationUI m_validationUI;
    SystemWorkspaceUI m_systemWorkspaceUI;
    ObjectWorkspaceUI m_objectWorkspaceUI;
};

} // namespace AstroGenesis

