#pragma once

#include "imgui.h"
#include "renderer/Camera.hpp"
#include "simulation/PhysicsEngine.hpp"

namespace AstroGenesis {

struct EventLogEntry {
    std::string timeStr;
    std::string message;
};

class UIManager {
public:
    UIManager();

    void initialize();
    void renderUI(PhysicsEngine& physics, Camera& camera, float windowWidth, float windowHeight, float fps);

    bool isViewportHovered() const { return m_viewportHovered; }
    int getHoveredBodyIndex() const { return m_hoveredBodyIndex; }
    void getViewportBounds(float& outX, float& outY, float& outW, float& outH) const;

    void addEventLog(const std::string& message);

private:
    void drawTopBar(float width);
    void drawLeftPanel(PhysicsEngine& physics, Camera& camera, float topBarH, float winH);
    void drawCenterViewportHeader(const CelestialBody& body, Camera& camera, float x, float y, float w);
    void drawFloatingInfoCards(const CelestialBody& body, float x, float y);
    void drawRightPanel(PhysicsEngine& physics, const CelestialBody& body, float topBarH, float winW, float winH);
    void drawViewportHUD(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH);
    void drawTimeControls(PhysicsEngine& physics, float x, float y, float w, float h);
    void drawSimMetrics(PhysicsEngine& physics, float fps, float x, float y, float w, float h);
    void drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h);
    void drawAsteroidBeltDiagnostics(PhysicsEngine& physics, float winW, float winH);
    void drawMatterLab(PhysicsEngine& physics, float winW, float winH);

    bool m_viewportHovered = false;
    int m_hoveredBodyIndex = -1;
    bool m_showAsteroidBeltDiagnostics = false;
    bool m_showMatterLab = false;
    int m_centerSubTab = 0; // 0: OVERVIEW, 1: INFO, 2: PHYSICAL, 3: ORBIT, etc.
    char m_searchQuery[64] = "";

    float m_viewportX = 230.0f;
    float m_viewportY = 48.0f;
    float m_viewportW = 1030.0f;
    float m_viewportH = 632.0f;
    float m_orbitVisZoom = 1.0f;

    std::vector<EventLogEntry> m_eventLogs;
};

} // namespace AstroGenesis
