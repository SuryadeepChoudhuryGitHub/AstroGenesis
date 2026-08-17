#pragma once

#include "imgui.h"
#include "renderer/Camera.hpp"
#include "simulation/PhysicsEngine.hpp"

namespace AstroGenesis {

class UIManager {
public:
    UIManager();

    void initialize();
    void renderUI(PhysicsEngine& physics, Camera& camera, float windowWidth, float windowHeight, float fps);

    bool isViewportHovered() const { return m_viewportHovered; }
    void getViewportBounds(float& outX, float& outY, float& outW, float& outH) const;

private:
    void drawTopBar(float width);
    void drawLeftPanel(PhysicsEngine& physics, Camera& camera, float topBarH, float statusBarH, float winH);
    void drawInfoOverlay(const CelestialBody& body, float x, float y);
    void drawRightPanel(const CelestialBody& body, float topBarH, float winW, float winH, float statusBarH);
    void drawTimeControls(PhysicsEngine& physics, float x, float y, float w, float h);
    void drawSimMetrics(const PhysicsEngine& physics, float fps, float x, float y, float w, float h);
    void drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h);
    void drawAIAssistant(float x, float y, float w, float h);
    void drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH);

    bool m_viewportHovered = false;
    float m_viewportX = 210.0f;
    float m_viewportY = 48.0f;
    float m_viewportW = 1080.0f;
    float m_viewportH = 614.0f;
    float m_orbitVisZoom = 1.0f;  // 1.0 = fit all planets, >1 = zoom in
};

} // namespace AstroGenesis
