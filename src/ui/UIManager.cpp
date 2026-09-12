#include "ui/UIManager.hpp"
#include "ai/AIManager.hpp"
#include "simulation/MaterialModel.hpp"
#include "data/UnitConverter.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

namespace Col {
    static ImVec4 BgDark       {0.024f, 0.035f, 0.065f, 1.00f};
    static ImVec4 BgPanel      {0.035f, 0.050f, 0.090f, 0.96f};
    static ImVec4 BgChild      {0.045f, 0.065f, 0.115f, 0.90f};
    static ImVec4 BgPopup      {0.050f, 0.075f, 0.130f, 0.98f};
    static ImVec4 Border       {0.120f, 0.180f, 0.280f, 0.60f};
    static ImVec4 BorderLight  {0.180f, 0.280f, 0.420f, 0.75f};
    static ImVec4 Accent       {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 AccentDim    {0.000f, 0.500f, 0.700f, 0.70f};
    static ImVec4 AccentHover  {0.200f, 0.920f, 1.000f, 1.00f};
    static ImVec4 TextPrimary  {0.900f, 0.930f, 0.970f, 1.00f};
    static ImVec4 TextSecondary{0.460f, 0.540f, 0.680f, 1.00f};
    static ImVec4 SelectedBg   {0.000f, 0.600f, 0.850f, 0.22f};
    static ImVec4 SelectedBorder{0.000f, 0.850f, 1.000f, 0.85f};
    static ImVec4 TabActive    {0.000f, 0.500f, 0.750f, 0.35f};
    static ImVec4 Green        {0.150f, 0.880f, 0.450f, 1.00f};
    static ImVec4 Yellow       {0.980f, 0.780f, 0.120f, 1.00f};
    static ImVec4 Orange       {0.980f, 0.550f, 0.150f, 1.00f};
    static ImVec4 Red          {0.950f, 0.250f, 0.200f, 1.00f};
}

static bool SectionHeader(const char* label, bool defaultOpen = true) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(Col::Accent.x, Col::Accent.y, Col::Accent.z, 0.10f));
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    bool open = ImGui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    ImGui::PopStyleColor(3);
    return open;
}

static void StatItem(const char* icon, const char* label, const char* value) {
    ImGui::BeginGroup();
    ImGui::TextColored(Col::Accent, "%s", icon);
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "%s", label);
    ImGui::TextColored(Col::TextPrimary, " %s", value);
    ImGui::EndGroup();
}

static void StatCard2Col(const char* l1, const char* v1, const char* l2, const char* v2, float halfW) {
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "%s", l1);
    ImGui::TextColored(Col::TextPrimary, "%s", v1);
    ImGui::EndGroup();

    ImGui::SameLine(halfW);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "%s", l2);
    ImGui::TextColored(Col::TextPrimary, "%s", v2);
    ImGui::EndGroup();
    ImGui::Spacing();
}

UIManager::UIManager() {}

void UIManager::initialize() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding    = 8.0f;
    s.ChildRounding     = 6.0f;
    s.FrameRounding     = 5.0f;
    s.GrabRounding      = 4.0f;
    s.PopupRounding     = 6.0f;
    s.TabRounding       = 4.0f;
    s.ScrollbarRounding = 6.0f;
    s.WindowBorderSize  = 1.0f;
    s.ChildBorderSize   = 1.0f;
    s.FrameBorderSize   = 0.0f;
    s.WindowPadding     = ImVec2(12, 10);
    s.FramePadding      = ImVec2(8, 5);
    s.ItemSpacing       = ImVec2(8, 6);
    s.ItemInnerSpacing  = ImVec2(6, 4);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg]             = Col::BgPanel;
    c[ImGuiCol_ChildBg]              = Col::BgChild;
    c[ImGuiCol_PopupBg]              = Col::BgPopup;
    c[ImGuiCol_Border]               = Col::Border;
    c[ImGuiCol_FrameBg]              = ImVec4(0.08f, 0.12f, 0.20f, 0.75f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.12f, 0.18f, 0.28f, 0.85f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.00f, 0.50f, 0.70f, 0.45f);
    c[ImGuiCol_TitleBg]              = Col::BgDark;
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.06f, 0.09f, 0.16f, 1.00f);
    c[ImGuiCol_CheckMark]            = Col::Accent;
    c[ImGuiCol_SliderGrab]           = Col::Accent;
    c[ImGuiCol_SliderGrabActive]     = Col::AccentHover;
    c[ImGuiCol_Button]               = ImVec4(0.09f, 0.13f, 0.22f, 0.85f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.00f, 0.50f, 0.70f, 0.55f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.00f, 0.65f, 0.85f, 0.75f);
    c[ImGuiCol_Header]               = Col::SelectedBg;
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 0.60f, 0.80f, 0.25f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 0.70f, 0.90f, 0.35f);
    c[ImGuiCol_Text]                 = Col::TextPrimary;
    c[ImGuiCol_TextDisabled]         = Col::TextSecondary;

    m_eventLogs.clear();
    m_eventLogs.push_back({ "00:00:01", "AstroGenesis engine initialized" });
    m_eventLogs.push_back({ "00:00:02", "SQLite astronomical database connected" });
    m_eventLogs.push_back({ "00:00:03", "Einstein 1PN Post-Newtonian GR active" });
}

void UIManager::addEventLog(const std::string& message) {
    char buf[16];
    static int logSec = 4;
    snprintf(buf, sizeof(buf), "00:%02d:%02d", logSec / 60, logSec % 60);
    logSec++;
    m_eventLogs.push_back({ buf, message });
    if (m_eventLogs.size() > 50) {
        m_eventLogs.erase(m_eventLogs.begin());
    }
}

void UIManager::getViewportBounds(float& outX, float& outY, float& outW, float& outH) const {
    outX = m_viewportX;
    outY = m_viewportY;
    outW = m_viewportW;
    outH = m_viewportH;
}

void UIManager::renderUI(PhysicsEngine& physics, 
                         Camera& camera, 
                         ObjectRepository& objRepo,
                         DataManager& dataManager,
                         ValidationEngine& valEngine,
                         VisualStateAdapter& visualAdapter,
                         ai::AIManager& aiManager,
                         float windowWidth, float windowHeight, float fps) {
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();

    // 1. Clean Screenshot Mode (Hotkey: F12)
    if (visualAdapter.isUIHidden()) {
        m_viewportX = 0.0f;
        m_viewportY = 0.0f;
        m_viewportW = windowWidth;
        m_viewportH = windowHeight;
        m_viewportHovered = !io.WantCaptureMouse;
        drawHiddenUIOverlay(visualAdapter, windowWidth, windowHeight);
        return;
    }

    // 2. SpaceEngine-style Photo Mode Active (Hotkey: P / F11)
    if (visualAdapter.isPhotoModeActive()) {
        m_viewportX = 0.0f;
        m_viewportY = 0.0f;
        m_viewportW = windowWidth;
        m_viewportH = windowHeight;
        m_viewportHovered = !io.WantCaptureMouse;
        drawPhotoModeToolbar(physics, camera, visualAdapter, windowWidth, windowHeight);
        return;
    }

    // 3. Standard Workspace Layout
    float topBarH    = 48.0f;
    float statusBarH = 28.0f;
    float leftPanelW = 210.0f;
    float rightPanelW = 310.0f;
    float bottomH    = 180.0f;

    m_viewportX = leftPanelW;
    m_viewportY = topBarH;
    m_viewportW = windowWidth - leftPanelW - rightPanelW;
    m_viewportH = windowHeight - topBarH - bottomH - statusBarH;

    m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                         mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH) && !io.WantCaptureMouse;

    // 1. Top Bar (Global Navigation & Workspace Switcher)
    drawTopBar(windowWidth, physics, camera, objRepo, visualAdapter);

    // 2. Route Top-Level Workspaces
    if (m_activeTopTab == 0) {
        // ── UNIVERSE WORKSPACE (Primary Live Simulation & Controls) ────────────
        drawLeftPanel(physics, camera, objRepo, topBarH, statusBarH, windowHeight);
        drawViewportHUD(physics, camera, visualAdapter, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        float bottomY = windowHeight - statusBarH - bottomH;
        float bpW = m_viewportW / 3.0f;
        drawTimeControls(physics, camera, objRepo, leftPanelW, bottomY, bpW, bottomH);
        drawSimMetrics  (physics, fps, leftPanelW + bpW,      bottomY, bpW, bottomH);
        drawOrbitVis    (physics, camera, leftPanelW + bpW * 2, bottomY, bpW, bottomH);

        const auto& currentBodies = physics.getBodies();
        int selIdx = physics.getSelectedBodyIndex();
        if (selIdx >= 0 && selIdx < (int)currentBodies.size()) {
            CelestialBody& currentBody = physics.getBodies()[selIdx];
            drawInfoOverlay(currentBody, m_viewportX, m_viewportY);
            drawRightPanel(physics, currentBody, dataManager, objRepo, visualAdapter, aiManager, topBarH, windowWidth, windowHeight, statusBarH);
        }

        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 1) {
        // ── SYSTEM WORKSPACE (Import Existing & Custom System Builder) ─────────
        m_systemWorkspaceUI.render(dataManager, objRepo, physics, camera, m_activeTopTab, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 2) {
        // ── OBJECTS WORKSPACE (Celestial Object Library & Editor) ──────────────
        m_objectWorkspaceUI.render(objRepo, physics, camera, m_activeTopTab, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 3) {
        // ── EXPLORE WORKSPACE (Discovery & Catalog Browser) ────────────────────
        drawExploreWorkspace(objRepo, physics, camera, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 4) {
        // ── SIMULATION WORKSPACE (Physics Config, Integrator, Validation) ──────
        drawSimulationWorkspace(physics, camera, valEngine, objRepo, visualAdapter, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 5) {
        // ── AI ASSISTANT WORKSPACE (Astrophysics & Orbital Stability Studio) ───
        drawAIAssistantWorkspace(physics, objRepo, aiManager, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    }


    // Modals / Overlay Windows
    if (m_showAsteroidBeltDiagnostics) {
        drawAsteroidBeltDiagnostics(physics, objRepo, windowWidth, windowHeight);
    }
    if (m_showMatterLab) {
        drawMatterLab(physics, camera, windowWidth, windowHeight);
    }
    if (m_showDataManager) {
        m_dataManagerUI.render(m_showDataManager, dataManager, objRepo, physics, windowWidth, windowHeight);
    }
    if (m_showValidationDashboard) {
        m_validationUI.render(m_showValidationDashboard, valEngine, objRepo, physics, windowWidth, windowHeight);
    }
}


void UIManager::drawTopBar(float width, PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, 48));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.05f, 0.09f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, Col::Border);

    ImGui::Begin("##TopBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    // Logo & Title
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("\xE2\x97\x86"); // Diamond icon
    ImGui::SameLine();
    ImGui::Text("ASTROGENESIS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "SPACE SIMULATION ENGINE");
    ImGui::SameLine(0, 24);

    // Top Navigation Tabs
    const char* tabs[] = { "UNIVERSE", "SYSTEM", "OBJECTS", "EXPLORE", "SIMULATION", "AI ASSISTANT" };
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool isActive = (i == m_activeTopTab);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Col::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::Button(tabs[i], ImVec2(0, 28))) m_activeTopTab = i;
        ImGui::PopStyleColor(2);
    }

    // Top Bar Action Buttons: Cinematic Toggle, Photo Mode, Reset Workspace, Data Manager, Validation, Asteroids, Matter Lab
    float rightOffset = std::max(width - 1120.0f, 630.0f);
    ImGui::SameLine(rightOffset);

    // 1. CINEMATIC MODE TOGGLE
    bool cineOn = visualAdapter.isCinematicModeEnabled();
    if (cineOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.85f, 0.60f, 0.05f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.95f, 0.70f, 0.15f, 1.00f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.05f, 0.05f, 0.05f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.16f, 0.24f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.18f, 0.24f, 0.35f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button(cineOn ? "🎬 CINEMATIC: ON" : "🎬 CINEMATIC: OFF", ImVec2(140, 28))) {
        visualAdapter.toggleCinematicMode();
        addEventLog(visualAdapter.isCinematicModeEnabled() ? "Cinematic Graphics Mode enabled (ACES Filmic + Bloom)" : "Standard Graphics Mode active");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Toggle Realistic Cinematic Post-Processing Pipeline (ACES Filmic Tone Mapping, HDR Bloom, Rayleigh Scattering) (Hotkey: F10)");
    }
    ImGui::PopStyleColor(3);

    // 2. PHOTO MODE BUTTON
    ImGui::SameLine(0, 5);
    bool photoOn = visualAdapter.isPhotoModeActive();
    if (photoOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.20f, 0.85f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.12f, 0.26f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 0.70f, 1.0f, 1.0f));
    }
    if (ImGui::Button("📷 PHOTO MODE", ImVec2(120, 28))) {
        visualAdapter.setPhotoModeActive(!photoOn);
        addEventLog(visualAdapter.isPhotoModeActive() ? "Photo Mode activated (Hotkey: P / F11)" : "Exited Photo Mode");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open SpaceEngine-inspired Photo Mode Toolbar (Camera Roll, FOV, Exposure EV, DoF Focus, Clean Shots) (Hotkey: P / F11)");
    }
    ImGui::PopStyleColor(2);

    // 3. RESET WORKSPACE (Clean Start)
    ImGui::SameLine(0, 5);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.16f, 0.16f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.70f, 0.22f, 0.22f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.9f, 0.9f, 1.0f));
    if (ImGui::Button("↺ RESET WORKSPACE", ImVec2(150, 28))) {
        physics.resetSimulation(objRepo);
        camera.resetOverview(glm::vec3(0.0f), 6.0f);
        addEventLog("Simulation workspace reset to fresh start");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear workspace, remove all custom loaded objects, and reset simulation to a fresh default start (Hotkey: R / Ctrl+R)");
    }
    ImGui::PopStyleColor(3);

    // 4. DATA MANAGER
    ImGui::SameLine(0, 5);
    if (m_showDataManager) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.65f, 0.85f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.20f, 0.32f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button("⛃ DATA MANAGER", ImVec2(135, 28))) {
        m_showDataManager = !m_showDataManager;
    }
    ImGui::PopStyleColor(2);

    // 5. VALIDATION
    ImGui::SameLine(0, 5);
    if (m_showValidationDashboard) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.35f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.14f, 0.10f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.75f, 0.3f, 1.0f));
    }
    if (ImGui::Button("⚖ VALIDATION", ImVec2(120, 28))) {
        m_showValidationDashboard = !m_showValidationDashboard;
    }
    ImGui::PopStyleColor(2);

    // 6. ASTEROID BELT
    ImGui::SameLine(0, 5);
    if (m_showAsteroidBeltDiagnostics) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.75f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.16f, 0.26f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button("☄ ASTEROID BELT", ImVec2(145, 28))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    ImGui::PopStyleColor(2);

    // 7. DEFORMABLE MATTER LAB
    ImGui::SameLine(0, 5);
    if (m_showMatterLab) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.65f, 0.35f, 0.15f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.16f, 0.12f, 0.22f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.9f, 0.7f, 0.4f, 1.0f));
    }
    if (ImGui::Button("⬡ MATTER LAB", ImVec2(120, 28))) {
        m_showMatterLab = !m_showMatterLab;
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::drawPhotoModeToolbar(PhysicsEngine& physics, Camera& camera, VisualStateAdapter& visualAdapter, float winW, float winH) {
    float toolbarW = 340.0f;
    float toolbarH = std::min(winH - 40.0f, 680.0f);
    ImGui::SetNextWindowPos(ImVec2(20.0f, 20.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(toolbarW, toolbarH), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.045f, 0.080f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.60f, 0.30f, 0.90f, 0.80f));

    if (ImGui::Begin("📷 PHOTO MODE STUDIO##PhotoModeWin", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Header & Exit
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 1.0f, 1.0f), "SPACE PHOTO MODE");
        ImGui::SameLine(toolbarW - 90.0f);
        if (ImGui::Button("✖ Exit (Esc)", ImVec2(80, 22))) {
            visualAdapter.setPhotoModeActive(false);
        }
        ImGui::Separator();

        // 1. Visual Preset Selector
        ImGui::TextColored(Col::Accent, "VISUAL QUALITY PRESET");
        int curPreset = (int)visualAdapter.getPreset();
        const char* presets[] = { "Normal (Baseline)", "Realistic (Photometric)", "Cinematic (Bloom + Flare)", "Ultra (Full ACES & FX)" };
        if (ImGui::Combo("##PresetCombo", &curPreset, presets, 4)) {
            visualAdapter.applyPreset((VisualPreset)curPreset);
            addEventLog(std::string("Applied visual preset: ") + presets[curPreset]);
        }

        bool cine = visualAdapter.isCinematicModeEnabled();
        if (ImGui::Checkbox("Cinematic Graphics Pipeline (F10)", &cine)) {
            visualAdapter.setCinematicMode(cine);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 2. Camera Lens & Roll Controls
        ImGui::TextColored(Col::Accent, "CAMERA LENS & ANGLE");

        // FOV Slider
        float fov = camera.getFOV();
        if (ImGui::SliderFloat("FOV (Lens)##PhotoFOV", &fov, 15.0f, 105.0f, "%.1f°")) {
            camera.setFOV(fov);
        }
        // FOV quick presets
        if (ImGui::SmallButton("24mm (42°)##Fov24")) { camera.setFOV(42.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("50mm (28°)##Fov50")) { camera.setFOV(28.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("85mm (18°)##Fov85")) { camera.setFOV(18.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Wide (65°)##FovWide")) { camera.setFOV(65.0f); }

        // Camera Roll Slider (radians -> degrees)
        float rollDeg = glm::degrees(camera.getRoll());
        if (ImGui::SliderFloat("Roll (Tilt)##PhotoRoll", &rollDeg, -180.0f, 180.0f, "%.1f°")) {
            camera.setRoll(glm::radians(rollDeg));
        }
        ImGui::TextDisabled("Hold [Q] / [E] to roll camera smoothly");
        if (ImGui::Button("↺ Reset Roll (0°)##ResetRollBtn", ImVec2(140, 22))) {
            camera.resetRoll();
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Photographic Exposure & Tone Mapping
        ImGui::TextColored(Col::Accent, "EXPOSURE & COLOR SCIENCE");

        float expVal = visualAdapter.getExposure();
        float ev = std::log2(std::max(expVal, 0.05f));
        if (ImGui::SliderFloat("Exposure (EV)##PhotoEV", &ev, -3.0f, 3.0f, "%.2f EV")) {
            visualAdapter.setExposure(std::pow(2.0f, ev));
        }
        if (ImGui::SmallButton("Reset EV (0.0)##ResetEV")) {
            visualAdapter.setExposure(1.0f);
        }

        int toneMode = visualAdapter.getToneMappingMode();
        const char* toneNames[] = { "ACES Filmic (Hollywood Standard)", "Reinhard (Natural Highlight Softening)", "Filmic (High Contrast Space)" };
        if (ImGui::Combo("Tone Mapping##PhotoTone", &toneMode, toneNames, 3)) {
            visualAdapter.setToneMappingMode(toneMode);
        }

        float bloom = visualAdapter.getBloomIntensity();
        if (ImGui::SliderFloat("HDR Bloom Glow##PhotoBloom", &bloom, 0.0f, 2.5f, "%.2fx")) {
            visualAdapter.setBloomIntensity(bloom);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 4. Depth of Field (DoF) & Focus Distance
        ImGui::TextColored(Col::Accent, "DEPTH OF FIELD (CINEMATIC BLUR)");
        bool dof = visualAdapter.isDoFEnabled();
        if (ImGui::Checkbox("Enable Depth of Field##PhotoDoF", &dof)) {
            visualAdapter.setDoFEnabled(dof);
            camera.setDoFEnabled(dof);
        }

        if (dof) {
            float focusDist = visualAdapter.getFocusDistance();
            if (ImGui::DragFloat("Focus Distance (AU)##PhotoFocus", &focusDist, 0.05f, 0.001f, 100.0f, "%.4f AU")) {
                visualAdapter.setFocusDistance(focusDist);
                camera.setFocusDistance(focusDist);
            }

            if (ImGui::Button("🎯 Auto-Focus on Target Object##FocusTgt", ImVec2(220, 24))) {
                float dist = camera.getDistance();
                visualAdapter.setFocusDistance(dist);
                camera.setFocusDistance(dist);
            }

            float aperture = visualAdapter.getDoFAperture();
            if (ImGui::SliderFloat("Aperture / CoC##PhotoAp", &aperture, 0.002f, 0.12f, "f/%.3f")) {
                visualAdapter.setDoFAperture(aperture);
                camera.setDoFAperture(aperture);
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 5. Optics & Lens Effects
        ImGui::TextColored(Col::Accent, "OPTICAL EFFECTS");
        bool vig = visualAdapter.isVignetteEnabled();
        if (ImGui::Checkbox("Lens Vignette##PhotoVig", &vig)) {
            visualAdapter.setVignetteEnabled(vig);
        }
        ImGui::SameLine();
        bool ca = visualAdapter.isCAEnabled();
        if (ImGui::Checkbox("Chromatic Aberration##PhotoCA", &ca)) {
            visualAdapter.setCAEnabled(ca);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 6. Scene Overlays & Elements
        ImGui::TextColored(Col::Accent, "SCENE ELEMENTS");
        bool orbits = visualAdapter.areOrbitLinesEnabled();
        if (ImGui::Checkbox("Orbit Lines##PhotoOrbits", &orbits)) {
            visualAdapter.setOrbitLinesEnabled(orbits);
        }
        ImGui::SameLine();
        bool trails = visualAdapter.areMotionTrailsEnabled();
        if (ImGui::Checkbox("Motion Trails##PhotoTrails", &trails)) {
            visualAdapter.setMotionTrailsEnabled(trails);
        }

        bool atmo = visualAdapter.areAtmospheresEnabled();
        if (ImGui::Checkbox("Atmosphere Glow##PhotoAtmo", &atmo)) {
            visualAdapter.setAtmospheresEnabled(atmo);
        }
        ImGui::SameLine();
        bool clouds = visualAdapter.areCloudsEnabled();
        if (ImGui::Checkbox("Clouds##PhotoClouds", &clouds)) {
            visualAdapter.setCloudsEnabled(clouds);
        }

        ImGui::Spacing();
        ImGui::Separator();

        // 7. Time Control & Capture
        ImGui::TextColored(Col::Accent, "CAPTURE & CONTROLS");
        bool paused = physics.isPaused();
        if (ImGui::Button(paused ? "▶ Resume Motion##PhotoPlay" : "⏸ Freeze Motion##PhotoPause", ImVec2(150, 26))) {
            physics.togglePause();
        }
        ImGui::SameLine();
        if (ImGui::Button("📸 HIDE UI (F12)##HideUIBtn", ImVec2(140, 26))) {
            visualAdapter.setUIHidden(true);
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Hide all UI and menus for clean screenshot capture! (Press F12 again to bring UI back)");
        }

        ImGui::Spacing();
        ImGui::TextDisabled("Controls: [Q/E] Roll | [Scroll] Zoom/Distance | [F12] Hide UI | [Esc] Exit");
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void UIManager::drawHiddenUIOverlay(VisualStateAdapter& visualAdapter, float winW, float winH) {
    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const char* hint = "● PHOTO CAPTURE MODE  |  [F12] Show UI  |  [Esc] Exit";
    ImVec2 textSize = ImGui::CalcTextSize(hint);
    float pad = 8.0f;
    float boxW = textSize.x + pad * 2.0f;
    float boxH = textSize.y + pad * 1.5f;
    float boxX = winW - boxW - 16.0f;
    float boxY = 16.0f;

    // Translucent glass badge (40% opacity so it doesn't ruin the view)
    dl->AddRectFilled(ImVec2(boxX, boxY), ImVec2(boxX + boxW, boxY + boxH),
                      ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.06f, 0.40f)), 4.0f);
    dl->AddRect(ImVec2(boxX, boxY), ImVec2(boxX + boxW, boxY + boxH),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.40f, 0.60f, 0.90f, 0.35f)), 4.0f);
    dl->AddText(ImVec2(boxX + pad, boxY + pad * 0.75f),
                ImGui::ColorConvertFloat4ToU32(ImVec4(0.80f, 0.85f, 0.95f, 0.70f)), hint);

    // If clicked on the badge, unhide UI
    ImVec2 mousePos = ImGui::GetMousePos();
    if (mousePos.x >= boxX && mousePos.x <= boxX + boxW && mousePos.y >= boxY && mousePos.y <= boxY + boxH) {
        if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            visualAdapter.setUIHidden(false);
        }
    }
}

void UIManager::drawLeftPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH) {
    float panelW = 210.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##LeftPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Search Bar
    ImGui::PushItemWidth(-30);
    ImGui::InputTextWithHint("##search", "Search Anything...", m_searchQuery, sizeof(m_searchQuery));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "\xE2\x96\xBC");
    ImGui::Separator();

    // Active System Hierarchy (Place 1: Selection from Left List)
    const std::string curCat = physics.getCurrentCategory();
    std::string headerLabel = curCat.empty() ? "SOLAR SYSTEM" : curCat;
    std::transform(headerLabel.begin(), headerLabel.end(), headerLabel.begin(), ::toupper);

    ImGui::TextColored(Col::Accent, "%s", headerLabel.c_str());
    ImGui::SameLine(panelW - 68.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.14f, 0.14f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.85f, 1.0f));
    if (ImGui::Button("↺ Reset", ImVec2(56, 18))) {
        physics.resetSimulation(objRepo);
        camera.resetOverview(glm::vec3(0.0f), 6.0f);
        addEventLog("Simulation workspace reset to fresh start");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset workspace to clean start (Hotkey: R)");
    }
    ImGui::PopStyleColor(2);

    if (true) {
        const auto& bodies = physics.getBodies();
        int selectedIndex = physics.getSelectedBodyIndex();

        for (int i = 0; i < (int)bodies.size(); ++i) {
            if (m_searchQuery[0] != '\0') {
                std::string bName = bodies[i].name;
                std::string q = m_searchQuery;
                std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                if (bName.find(q) == std::string::npos) continue;
            }

            bool isSelected = (i == selectedIndex);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, Col::SelectedBg);
                ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
            }

            if (ImGui::Selectable(("##body" + std::to_string(i)).c_str(), isSelected, 0, ImVec2(0, 36))) {
                physics.selectBody(i);
                camera.focusOnBody(bodies[i].position, bodies[i].radius3D, 0.85f);
                addEventLog(bodies[i].name + " selected");
            }

            ImVec2 p = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(ImVec2(p.x + 28, p.y + 2));
            ImGui::Text("%s", bodies[i].name.c_str());
            ImGui::SetCursorScreenPos(ImVec2(p.x + 28, p.y + 18));
            ImGui::TextColored(Col::TextSecondary, "%s", bodies[i].distanceStr.c_str());

            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2(p.x + 14, p.y + 18), 8.0f,
                isSelected ? ImGui::ColorConvertFloat4ToU32(Col::Accent)
                           : ImGui::ColorConvertFloat4ToU32(ImVec4(bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 0.8f)));

            if (isSelected) ImGui::PopStyleColor(2);
        }
    }

    ImGui::Separator();

    // Additional Database System Categories
    auto categories = objRepo.getAvailableCategories();
    for (const auto& cat : categories) {
        if (cat == curCat) continue;
        std::string upperCat = cat;
        std::transform(upperCat.begin(), upperCat.end(), upperCat.begin(), ::toupper);
        
        if (SectionHeader(upperCat.c_str(), false)) {
            auto catObjs = objRepo.getAllObjects(cat, false);
            for (const auto& obj : catObjs) {
                if (ImGui::Selectable(obj.name.c_str())) {
                    physics.loadFromDatabase(objRepo, cat);
                    physics.selectBodyById(obj.slug);
                    addEventLog("Switched system to " + cat + " (" + obj.name + ")");
                }
            }
        }
    }

    const char* staticSections[] = { "STAR CLUSTERS", "GALAXIES", "FAVORITES" };
    for (auto& sec : staticSections) {
        SectionHeader(sec, false);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::drawInfoOverlay(const CelestialBody& body, float x, float y) {
    ImGui::SetNextWindowPos(ImVec2(x + 12, y + 12));
    ImGui::SetNextWindowSize(ImVec2(280, 0));
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::Begin("##CelestialInfoOverlay", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(Col::Accent, "%s", body.name.c_str());
    ImGui::SameLine();
    ImGui::TextColored(Col::Yellow, "\xE2\x98\x85");
    ImGui::TextColored(Col::TextSecondary, "%s", body.type.c_str());
    ImGui::Separator();

    auto InfoRow = [](const char* label, const std::string& value) {
        ImGui::TextColored(Col::TextSecondary, "%-18s", label);
        ImGui::SameLine(125);
        ImGui::TextColored(Col::TextPrimary, "%s", value.c_str());
    };

    InfoRow("Distance (Sol)",   (body.id == "sol" || body.type.find("Star") != std::string::npos) ? "0.00 AU" : body.distanceStr);
    if (body.id != "sol") {
        InfoRow("Orbital Velocity", body.orbitalSpeedStr);
        InfoRow("Semi-Major Axis",  body.semiMajorAxisStr);
        InfoRow("Eccentricity",     body.eccentricityStr);
        InfoRow("Perihelion",       body.periapsisStr);
        InfoRow("Aphelion",         body.apoapsisStr);
        InfoRow("GR Precession",    body.grPrecessionStr);
    }
    InfoRow("Radius",           body.radiusStr);
    InfoRow("Mass",             body.massStr);
    InfoRow("Surface Gravity",  body.gravityStr);
    InfoRow("Surface Temp.",    body.tempStr);
    InfoRow("Solar Flux",       body.solarRadiationStr);
    if (body.id != "sol") {
        InfoRow("Time Dilation", body.timeDilationStr);
    }
    InfoRow("Axial Tilt",       body.axialTiltStr);
    InfoRow("Atmosphere",       body.atmosphereStr);
    InfoRow("Moons",            std::to_string(body.moons));
    InfoRow("Data Source",      body.sourceName);

    ImGui::End();
}

void UIManager::drawRightPanel(PhysicsEngine& physics, CelestialBody& body, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, float topBarH, float winW, float winH, float statusBarH) {
    float panelW = 310.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(winW - panelW, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##RightPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // ── VISUAL & PHYSICAL STATE INSPECTOR ──────────────────────────────────────
    if (SectionHeader("VISUAL & PHYSICAL STATE")) {
        const VisualBodyState* vBody = visualAdapter.getVisualBody(body.id);
        if (!vBody) vBody = visualAdapter.getVisualBody(body.dbId);

        float halfW = (panelW - 40) / 2.0f;
        char rBuf[32];
        if (vBody) snprintf(rBuf, sizeof(rBuf), "%.4f AU", vBody->renderRadius);
        else snprintf(rBuf, sizeof(rBuf), "%.4f AU", body.radius3D);

        ImGui::BeginGroup();
        StatItem("📐", "Render Scale", rBuf);
        ImGui::SameLine(halfW);
        StatItem("📏", "Physical Radius", body.radiusStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("🌡", "Blackbody Temp", body.tempStr.c_str());
        ImGui::SameLine(halfW);
        std::string phaseStr = "Solid Rock/Ice";
        if (vBody) {
            if (vBody->phase == MaterialPhase::VaporGas) phaseStr = vBody->isStar ? "Stellar Plasma" : "Vapor / Gas";
            else if (vBody->phase == MaterialPhase::LiquidMolten) phaseStr = "Molten Magma";
            else if (vBody->phase == MaterialPhase::SoftenedPlastic) phaseStr = "Softened Plastic";
            else if (vBody->phase == MaterialPhase::Solid) phaseStr = "Solid Rock/Ice";
        }
        StatItem("⬡", "Material Phase", phaseStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("☁", "Atmosphere", (vBody && vBody->hasAtmosphere) ? "Scattering Active" : "None/Thin");
        ImGui::SameLine(halfW);
        StatItem("💨", "Cloud Cover", (vBody && vBody->hasClouds) ? "Dynamic Clouds" : "Clear");
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("🔄", "Rotation Speed", body.rotationPeriodStr.c_str());
        ImGui::SameLine(halfW);
        StatItem("📐", "Axial Tilt", body.axialTiltStr.c_str());
        ImGui::EndGroup();

        if (vBody && vBody->isBlackHole) {
            ImGui::Spacing();
            ImGui::TextColored(Col::Yellow, "Relativistic Singular Event Horizon:");
            ImGui::Text("  • Schwarzschild: %.5f AU", vBody->schwarzschildRadiusAU);
            ImGui::Text("  • Photon Ring:    %.5f AU", vBody->photonSphereRadiusAU);
        }
    }

    ImGui::Separator();

    if (SectionHeader("PHYSICAL OVERVIEW")) {
        float halfW = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem("\xE2\x86\x93", "Gravity", body.gravityStr.c_str());
        ImGui::SameLine(halfW);
        StatItem("\xE2\x86\x97", "Escape Velocity", body.escapeVelocityStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x97\x8B", "Surface Temp.", body.tempStr.c_str());
        ImGui::SameLine(halfW);
        StatItem("\xE2\x97\x8B", "Atmospheric Pressure", body.pressureStr.c_str());
        ImGui::EndGroup();

        char hBuf[32], tauBuf[32];
        snprintf(hBuf, sizeof(hBuf), "%.1f km", body.scaleHeightKm);
        snprintf(tauBuf, sizeof(tauBuf), "%.2f (+%.0f K)", body.opticalDepth, body.greenhouseK);
        ImGui::BeginGroup();
        StatItem("\xE2\x96\xB3", "Scale Height", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? hBuf : "N/A");
        ImGui::SameLine(halfW);
        StatItem("\xE2\x97\x86", "Optical Depth (τ)", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? tauBuf : "0.00 (+0 K)");
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x97\x8F", "Mean Density", body.densityStr.c_str());
        ImGui::SameLine(halfW);
        StatItem("\xE2\x97\x8F", "Day Length", body.rotationPeriodStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x97\x89", "Year Length", body.yearLengthStr.c_str());
        ImGui::SameLine(halfW);
        StatItem("\xE2\x97\x89", "Surface Area", body.surfaceAreaStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("LIVE SIMULATION TUNING & EDITING")) {
        int selIdx = physics.getSelectedBodyIndex();
        if (selIdx >= 0 && selIdx < (int)physics.getBodies().size()) {
            CelestialBody& mutBody = physics.getBodies()[selIdx];
            bool isPlanetOrMinor = (mutBody.type.find("Planet") != std::string::npos || 
                                   mutBody.type.find("Moon") != std::string::npos || 
                                   mutBody.type.find("Asteroid") != std::string::npos || 
                                   mutBody.type.find("Comet") != std::string::npos);
            bool isDwarfStar = (mutBody.type.find("Dwarf") != std::string::npos && !isPlanetOrMinor);
            bool isStar = !isPlanetOrMinor && (mutBody.id == "sol" || mutBody.type.find("Star") != std::string::npos || isDwarfStar);
            bool isBlackHole = (mutBody.type.find("Black Hole") != std::string::npos);

            ImGui::TextColored(physics.isPaused() ? Col::Yellow : Col::Green, 
                               physics.isPaused() ? "⏸ Paused (Live Real-Time Tuning Active)" : "▶ Running (Live Real-Time Tuning Active)");

            // 1. Physical Radius Slider (km, Earth Radii, Solar Radii)
            if (isStar) {
                float rSun = (float)(mutBody.radiusM / UnitConverter::SOLAR_RADIUS_M);
                if (ImGui::DragFloat("Radius (R☉)##LiveEditR", &rSun, 0.02f, 0.01f, 1500.0f, "%.3f R☉")) {
                    mutBody.radiusM = (double)rSun * UnitConverter::SOLAR_RADIUS_M;
                    mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                    char rBuf[64];
                    snprintf(rBuf, sizeof(rBuf), "%'.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            } else {
                float rEarth = (float)(mutBody.radiusM / UnitConverter::EARTH_RADIUS_M);
                if (ImGui::DragFloat("Radius (R⊕)##LiveEditR", &rEarth, 0.02f, 0.005f, 250.0f, "%.3f R⊕")) {
                    mutBody.radiusM = (double)rEarth * UnitConverter::EARTH_RADIUS_M;
                    mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                    char rBuf[64];
                    snprintf(rBuf, sizeof(rBuf), "%'.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            }

            float rKm = (float)(mutBody.radiusM / 1000.0);
            if (ImGui::DragFloat("Radius (km)##LiveEditRkm", &rKm, 10.0f, 10.0f, 5000000.0f, "%.1f km")) {
                mutBody.radiusM = (double)rKm * 1000.0;
                mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                char rBuf[64];
                snprintf(rBuf, sizeof(rBuf), "%'.1f km", mutBody.radiusM / 1000.0);
                mutBody.radiusStr = rBuf;
                physics.updateBodyScales();
            }

            // 2. Physical Mass Slider
            double curM = mutBody.massKg;
            if (isStar || isBlackHole) {
                float mSun = (float)(curM / UnitConverter::SOLAR_MASS_KG);
                if (ImGui::DragFloat("Mass (M☉)##LiveEditM", &mSun, 0.05f, 0.01f, 50000.0f, "%.3f M☉")) {
                    mutBody.massKg = (double)mSun * UnitConverter::SOLAR_MASS_KG;
                    mutBody.massStr = UnitConverter::formatMass(mutBody.massKg);
                }
            } else {
                float mEarth = (float)(curM / UnitConverter::EARTH_MASS_KG);
                if (ImGui::DragFloat("Mass (M⊕)##LiveEditM", &mEarth, 0.05f, 0.001f, 10000.0f, "%.3f M⊕")) {
                    mutBody.massKg = (double)mEarth * UnitConverter::EARTH_MASS_KG;
                    mutBody.massStr = UnitConverter::formatMass(mutBody.massKg);
                }
            }

            // 3. Surface Temperature Slider (Triggers Magma / Incandescence / Star visuals)
            float tempK = (float)mutBody.surfaceTempK;
            if (ImGui::DragFloat("Surface Temp (K)##LiveEditT", &tempK, 15.0f, 10.0f, 50000.0f, "%.0f K")) {
                physics.setBodyCustomTemperature(selIdx, (double)tempK);
            }
            if (mutBody.hasCustomTemp) {
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset Eq##ResetEqT")) {
                    physics.resetBodyToThermalEquilibrium(selIdx);
                }
                if (ImGui::IsItemHovered()) {
                    ImGui::SetTooltip("Reset temperature to radiative blackbody equilibrium");
                }
            }

            // Atmosphere & Greenhouse (Planets) / Stellar Luminosity (Stars)
            if (!isStar && !isBlackHole) {
                bool atmo = mutBody.hasAtmosphere;
                if (ImGui::Checkbox("Atmosphere Present##LiveAtmo", &atmo)) {
                    physics.setBodyAtmosphere(selIdx, atmo);
                }
                if (atmo) {
                    float pressKpa = (float)mutBody.surfacePressureKpa;
                    if (ImGui::DragFloat("Pressure (kPa)##LiveP", &pressKpa, 0.5f, 0.0f, 15000.0f, "%.1f kPa")) {
                        physics.setBodySurfacePressureKpa(selIdx, (double)pressKpa);
                    }

                    float baseAlb = (float)mutBody.baseAlbedo;
                    if (ImGui::SliderFloat("Bare Albedo##LiveBaseAlb", &baseAlb, 0.01f, 0.95f, "%.2f")) {
                        physics.setBodyBareAlbedo(selIdx, (double)baseAlb);
                    }

                    // Chemical Species Gas Sliders
                    ImGui::Spacing();
                    ImGui::TextColored(Col::Accent, "Atmospheric Volatiles & Greenhouse Gases:");
                    
                    float co2Pct = 0.0f;
                    for (const auto& ab : mutBody.chemicalInventory) {
                        if (ab.speciesId == "CO2") co2Pct = ab.percentage;
                    }
                    if (ImGui::SliderFloat("CO₂ (%)##LiveCO2", &co2Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "CO2", co2Pct);
                    }

                    float ch4Pct = 0.0f;
                    for (const auto& ab : mutBody.chemicalInventory) {
                        if (ab.speciesId == "CH4") ch4Pct = ab.percentage;
                    }
                    if (ImGui::SliderFloat("CH₄ (%)##LiveCH4", &ch4Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "CH4", ch4Pct);
                    }

                    float h2oPct = 0.0f;
                    for (const auto& ab : mutBody.chemicalInventory) {
                        if (ab.speciesId == "H2O") h2oPct = ab.percentage;
                    }
                    if (ImGui::SliderFloat("H₂O Vapor (%)##LiveH2O", &h2oPct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "H2O", h2oPct);
                    }

                    float n2Pct = 0.0f;
                    for (const auto& ab : mutBody.chemicalInventory) {
                        if (ab.speciesId == "N2") n2Pct = ab.percentage;
                    }
                    if (ImGui::SliderFloat("N₂ (%)##LiveN2", &n2Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "N2", n2Pct);
                    }

                    ImGui::TextDisabled("Greenhouse warming: +%.1f K (τ = %.2f)", mutBody.greenhouseK, mutBody.opticalDepth);
                    ImGui::TextDisabled("Rayleigh Sky: (%.2f, %.2f, %.2f) Clouds: %.0f%%", 
                        mutBody.rayleighColor.r, mutBody.rayleighColor.g, mutBody.rayleighColor.b, mutBody.cloudCoverage * 100.0f);

                    if (ImGui::SmallButton("Reset Atmosphere to Baseline##ResetAtmo")) {
                        physics.resetBodyAtmosphereToBaseline(selIdx);
                    }
                }
            } else if (isStar) {
                float lum = (float)(mutBody.luminosityW / 3.828e26);
                if (ImGui::DragFloat("Luminosity (L☉)##LiveLum", &lum, 0.02f, 0.0001f, 100000.0f, "%.3f L☉")) {
                    physics.setStarLuminositySolar(selIdx, (double)lum);
                }
            }

            // 4. Color Tint & Albedo
            float col[3] = { mutBody.color.r, mutBody.color.g, mutBody.color.b };
            if (ImGui::ColorEdit3("Albedo Tint##LiveEditCol", col)) {
                mutBody.color = glm::vec3(col[0], col[1], col[2]);
            }

            // 5. Axial Tilt & Rotation
            float tilt = mutBody.axialTiltDeg;
            if (ImGui::SliderFloat("Axial Tilt (°)##LiveEditTilt", &tilt, 0.0f, 180.0f, "%.1f°")) {
                mutBody.axialTiltDeg = tilt;
                char tiltBuf[32];
                snprintf(tiltBuf, sizeof(tiltBuf), "%.2f°", mutBody.axialTiltDeg);
                mutBody.axialTiltStr = tiltBuf;
            }

            // Update derived physical quantities
            if (mutBody.radiusM > 0.0 && mutBody.massKg > 0.0) {
                mutBody.surfaceGravityMps2 = (UnitConverter::G_CONST * mutBody.massKg) / (mutBody.radiusM * mutBody.radiusM);
                char gravBuf[64];
                snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s² (%.2f g)", mutBody.surfaceGravityMps2, mutBody.surfaceGravityMps2 / 9.80665);
                mutBody.gravityStr = gravBuf;

                double vol = (4.0 / 3.0) * UnitConverter::PI * std::pow(mutBody.radiusM, 3.0);
                mutBody.meanDensityKgM3 = mutBody.massKg / vol;
                char densBuf[64];
                snprintf(densBuf, sizeof(densBuf), "%'.1f kg/m³", mutBody.meanDensityKgM3);
                mutBody.densityStr = densBuf;

                mutBody.escapeVelocityKmpS = std::sqrt(2.0 * UnitConverter::G_CONST * mutBody.massKg / mutBody.radiusM) / 1000.0;
                char escBuf[64];
                snprintf(escBuf, sizeof(escBuf), "%.2f km/s", mutBody.escapeVelocityKmpS);
                mutBody.escapeVelocityStr = escBuf;
            }

            ImGui::Spacing();
            if (ImGui::Button("✏ Open in Object Editor Workspace", ImVec2(panelW - 20, 24))) {
                m_objectWorkspaceUI.setSelectedObjectBySlug(mutBody.id, objRepo);
                m_activeTopTab = 2; // OBJECTS workspace
            }
        }
    }

    ImGui::Separator();

    if (SectionHeader("ORBITAL DYNAMICS & VELOCITY TUNING")) {
        int selIdx = physics.getSelectedBodyIndex();
        if (selIdx >= 0 && selIdx < (int)physics.getBodies().size()) {
            CelestialBody& mutBody = physics.getBodies()[selIdx];
            bool isStar = (mutBody.type.find("Star") != std::string::npos || mutBody.id == "sol");

            if (isStar) {
                ImGui::TextDisabled("Central gravitational attractor");
            } else {
                double vMag = glm::length(mutBody.velocityMps) / 1000.0;
                ImGui::Text("Orbital Speed: %.2f km/s (%s)", vMag, mutBody.orbitalSpeedStr.c_str());

                // Quick velocity multipliers
                ImGui::TextColored(Col::TextSecondary, "Scale Orbital Speed:");
                if (ImGui::Button("0.5x##Vel05", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 0.5); }
                ImGui::SameLine();
                if (ImGui::Button("0.9x##Vel09", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 0.9); }
                ImGui::SameLine();
                if (ImGui::Button("1.1x##Vel11", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 1.1); }
                ImGui::SameLine();
                if (ImGui::Button("1.5x##Vel15", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 1.5); }
                ImGui::SameLine();
                if (ImGui::Button("Reverse##VelRev", ImVec2(64, 22))) { physics.scaleBodyVelocity(selIdx, -1.0); }

                // Prograde / Retrograde Impulse
                ImGui::Spacing();
                ImGui::TextColored(Col::TextSecondary, "Impulse Maneuver (Δv):");
                if (ImGui::Button("-5 km/s##Ret5", ImVec2(70, 22))) { physics.applyProgradeDeltaV(selIdx, -5.0); }
                ImGui::SameLine();
                if (ImGui::Button("-1 km/s##Ret1", ImVec2(70, 22))) { physics.applyProgradeDeltaV(selIdx, -1.0); }
                ImGui::SameLine();
                if (ImGui::Button("+1 km/s##Pro1", ImVec2(70, 22))) { physics.applyProgradeDeltaV(selIdx, +1.0); }
                ImGui::SameLine();
                if (ImGui::Button("+5 km/s##Pro5", ImVec2(70, 22))) { physics.applyProgradeDeltaV(selIdx, +5.0); }

                static float customDv = 0.0f;
                ImGui::PushItemWidth(panelW - 130.0f);
                ImGui::DragFloat("##CustomDv", &customDv, 0.1f, -100.0f, 100.0f, "Δv: %+.2f km/s");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Apply Δv##ApplyDvBtn", ImVec2(80, 22))) {
                    physics.applyProgradeDeltaV(selIdx, (double)customDv);
                    customDv = 0.0f;
                }

                // Normal (inclination change)
                static float customNormDv = 0.0f;
                ImGui::PushItemWidth(panelW - 130.0f);
                ImGui::DragFloat("##CustomNormDv", &customNormDv, 0.1f, -50.0f, 50.0f, "Norm: %+.2f km/s");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Apply Norm##ApplyNormBtn", ImVec2(80, 22))) {
                    physics.applyNormalDeltaV(selIdx, (double)customNormDv);
                    customNormDv = 0.0f;
                }

                // Orbital Shape Modifiers
                ImGui::Spacing();
                if (ImGui::Button("🎯 Circularize Orbit at Current Distance", ImVec2(panelW - 20, 24))) {
                    physics.circularizeOrbit(selIdx);
                }

                float curA = (float)mutBody.semiMajorAxisAU;
                if (curA <= 0.0f) curA = (float)glm::length(mutBody.position);
                if (ImGui::DragFloat("Semi-Major Axis (AU)##LiveSMA", &curA, 0.02f, 0.05f, 150.0f, "%.3f AU")) {
                    physics.setBodyOrbitRadiusAU(selIdx, (double)curA);
                }

                float curEcc = (float)mutBody.eccentricity;
                if (ImGui::SliderFloat("Eccentricity (e)##LiveEcc", &curEcc, 0.0f, 0.95f, "%.3f")) {
                    physics.setBodyEccentricity(selIdx, (double)curEcc);
                }
            }
        }
    }


    ImGui::Separator();

    if (SectionHeader("ORBITAL MECHANICS & KEPLERIAN ELEMENTS")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem("\xE2\x97\x86", "Semi-Major Axis", body.semiMajorAxisStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x97\x87", "Eccentricity", body.eccentricityStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x86\x98", "Perihelion (Closest)", body.periapsisStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x86\x97", "Aphelion (Farthest)", body.apoapsisStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x86\xBB", "Angular Momentum", body.angularMomentumStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9A\xA1", "Orbital Energy", body.orbitalEnergyStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x8C\x9B", "GR Precession", body.grPrecessionStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x88\xA0", "True Anomaly", body.trueAnomalyStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("AI ORBITAL STABILITY")) {
        const auto& pred = aiManager.getCurrentPrediction();
        float hw = (panelW - 40) / 2.0f;

        // Prediction Status Badge
        ImVec4 statusCol = Col::Green;
        const char* statusPrefix = "[ STABLE ]";
        if (pred.prediction == "UNSTABLE") {
            statusCol = Col::Red;
            statusPrefix = "[ UNSTABLE ]";
        } else if (pred.prediction == "MARGINAL") {
            statusCol = Col::Yellow;
            statusPrefix = "[ MARGINAL ]";
        } else if (pred.prediction == "UNAVAILABLE") {
            statusCol = Col::TextSecondary;
            statusPrefix = "[ UNAVAILABLE ]";
        }

        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Prediction:");
        ImGui::SameLine();
        ImGui::TextColored(statusCol, "%s", statusPrefix);
        ImGui::SameLine(hw + 20);
        ImGui::TextColored(Col::TextSecondary, "Confidence:");
        ImGui::SameLine();
        ImGui::TextColored(pred.confidence == "HIGH" ? Col::Green : (pred.confidence == "MEDIUM" ? Col::Yellow : Col::Orange),
                           "%s", pred.confidence.c_str());
        ImGui::EndGroup();

        // Progress bars for probabilities
        float pStable = pred.stableProbability;
        float pUnstable = pred.unstableProbability;
        
        char stableBuf[32], unstableBuf[32];
        snprintf(stableBuf, sizeof(stableBuf), "Stable: %.1f%%", pStable * 100.0f);
        snprintf(unstableBuf, sizeof(unstableBuf), "Unstable: %.1f%%", pUnstable * 100.0f);

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col::Green);
        ImGui::ProgressBar(pStable, ImVec2(panelW - 20, 16), stableBuf);
        ImGui::PopStyleColor();

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col::Orange);
        ImGui::ProgressBar(pUnstable, ImVec2(panelW - 20, 16), unstableBuf);
        ImGui::PopStyleColor();

        ImGui::BeginGroup();
        StatItem("🪐", "Bodies", std::to_string(pred.analyzedBodyCount).c_str());
        ImGui::SameLine(hw);
        char sepBuf[32];
        snprintf(sepBuf, sizeof(sepBuf), "%.2f R_H", pred.minMutualHillSep);
        StatItem("📐", "Min Sep", sepBuf);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::TextColored(Col::TextSecondary, "Primary Risk Factor:");
        ImGui::TextWrapped("%s", pred.primaryRiskFactor.c_str());

        ImGui::Spacing();
        ImGui::TextDisabled("ℹ ML-based estimate of orbital stability.");

        if (ImGui::Button("🤖 Open AI Analysis Studio", ImVec2(panelW - 20, 24))) {
            m_activeTopTab = 5;
        }
    }


    if (body.ring.hasRing) {
        ImGui::Separator();
        if (SectionHeader("PLANETARY RING ASTROPHYSICS & SHEAR")) {
            float hw = (panelW - 40) / 2.0f;
            
            ImGui::BeginGroup();
            StatItem("\xE2\x9C\xA8", "Inner Speed (74.5k km)", "23.1 km/s (5.6h)");
            ImGui::SameLine(hw);
            StatItem("\xE2\x9C\xA8", "Outer Speed (140.2k km)", "16.8 km/s (14.9h)");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem("\xE2\x86\x93", "Local Gravity (g)", "6.84 → 1.93 m/s²");
            ImGui::SameLine(hw);
            StatItem("\xE2\x86\x97", "Escape Velocity", "32.7 → 23.8 km/s");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem("\xE2\x97\x8B", "Ring Temp. (Ice)", "85 K (-188 °C)");
            ImGui::SameLine(hw);
            StatItem("\xE2\x8F\xB1", "Relativistic Drift", "-1.35 × 10⁻⁸");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem("\xE2\x9A\x96", "Total Ring Mass", "1.50 × 10¹⁹ kg");
            ImGui::SameLine(hw);
            char actBuf[32];
            snprintf(actBuf, sizeof(actBuf), "%zu Active", body.ring.disturbances.size());
            StatItem("\xE2\x8F\xB3", "Fluid State", body.ring.disturbances.empty() ? "Equilibrium" : actBuf);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.65f, 0.9f));
            if (ImGui::Button("☄ Trigger Asteroid Ring Impact", ImVec2(panelW - 20, 24))) {
                physics.triggerSaturnRingImpact();
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Carve a physical void/wake in Saturn's rings and watch Keplerian shear and viscous self-healing in real time!");
            }
            ImGui::PopStyleColor();
        }
    }

    ImGui::Separator();

    if (SectionHeader("RADIATION & GENERAL RELATIVITY")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem("\xE2\x98\x80", "Solar Radiation", body.solarRadiationStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9A\xA0", "Radiation Level", body.radLevelStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x8F\xB1", "Relativistic Drift", body.timeDilationStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9C\xA8", "Orbital Velocity", body.orbitalSpeedStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x97\x86", "Magnetic Field", body.magneticFieldStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9C\xA8", "Aurora Activity", body.auroraActivityStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("DATA SOURCE & VERIFICATION")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem("🏛", "Authority", body.sourceName.c_str());
        ImGui::SameLine(hw);
        StatItem("🆔", "Target ID", body.sourceObjectId.empty() ? body.id.c_str() : body.sourceObjectId.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("🧭", "Ref Frame", body.referenceFrame.c_str());
        ImGui::SameLine(hw);
        StatItem("📅", "Epoch", body.epochUtcStr.c_str());
        ImGui::EndGroup();

        ImGui::Spacing();
        if (ImGui::Button("⛃ Open Data Manager", ImVec2(panelW - 20, 24))) {
            m_showDataManager = true;
        }
    }

    ImGui::Separator();

    if (SectionHeader("COMPOSITION")) {
        float totalPct = 0.0f;
        for (const auto& item : body.composition) totalPct += item.percentage;

        if (totalPct > 0.0f && !body.composition.empty()) {
            float chartRadius = 38.0f;
            float innerRadius = 22.0f;
            ImVec2 curPos = ImGui::GetCursorScreenPos();
            ImVec2 chartCenter = ImVec2(curPos.x + chartRadius + 8.0f, curPos.y + chartRadius + 4.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();

            float startAngle = -3.14159f / 2.0f;
            for (const auto& item : body.composition) {
                float sweep = (item.percentage / totalPct) * 2.0f * 3.14159f;
                if (sweep < 0.01f) continue;
                ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(item.color.r, item.color.g, item.color.b, 1.0f));

                int segments = std::max(4, (int)(sweep * 22.0f));
                for (int s = 0; s < segments; ++s) {
                    float a0 = startAngle + sweep * (float)s / (float)segments;
                    float a1 = startAngle + sweep * (float)(s + 1) / (float)segments;
                    dl->AddTriangleFilled(
                        chartCenter,
                        ImVec2(chartCenter.x + chartRadius * cosf(a0), chartCenter.y + chartRadius * sinf(a0)),
                        ImVec2(chartCenter.x + chartRadius * cosf(a1), chartCenter.y + chartRadius * sinf(a1)),
                        col);
                }
                startAngle += sweep;
            }

            dl->AddCircleFilled(chartCenter, innerRadius, ImGui::ColorConvertFloat4ToU32(Col::BgChild), 32);

            float legendX = chartCenter.x + chartRadius + 14.0f;
            float legendY = chartCenter.y - chartRadius + 2.0f;

            for (const auto& item : body.composition) {
                ImVec2 dotPos = ImVec2(legendX + 4.0f, legendY + 5.0f);
                dl->AddCircleFilled(dotPos, 4.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(item.color.r, item.color.g, item.color.b, 1.0f)));

                char pctBuf[32];
                snprintf(pctBuf, sizeof(pctBuf), "%5.2f%%", item.percentage);
                dl->AddText(ImVec2(legendX + 12.0f, legendY), ImGui::ColorConvertFloat4ToU32(Col::Accent), pctBuf);
                dl->AddText(ImVec2(legendX + 68.0f, legendY), ImGui::ColorConvertFloat4ToU32(Col::TextPrimary), item.name.c_str());

                legendY += 16.0f;
            }

            ImGui::SetCursorScreenPos(ImVec2(curPos.x, curPos.y + chartRadius * 2.0f + 10.0f));
        }
    }

    ImGui::Separator();

    ImGui::TextColored(Col::Accent, "EVENT LOG");
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.035f, 0.065f, 0.90f));
    ImGui::BeginChild("##EventLogChild", ImVec2(panelW - 20, 75), true);

    for (const auto& log : m_eventLogs) {
        ImGui::TextColored(Col::Accent, "%s", log.timeStr.c_str());
        ImGui::SameLine(0, 8);
        ImGui::TextColored(Col::TextSecondary, "%s", log.message.c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar();
}

// ------------------------------------------------------------------------------------------------
// SELECTION PLACE 2: DIRECT HOVER & CLICK IN 3D VIEWPORT (HUD Screen Projection)
// ------------------------------------------------------------------------------------------------
void UIManager::drawViewportHUD(PhysicsEngine& physics, Camera& camera, VisualStateAdapter& visualAdapter, float vpX, float vpY, float vpW, float vpH) {
    if (vpW <= 0.0f || vpH <= 0.0f) return;

    ImVec2 mousePos = ImGui::GetMousePos();
    const auto& bodies = physics.getBodies();
    int selectedIdx = physics.getSelectedBodyIndex();

    struct ProjectedBody {
        int index;
        glm::vec2 screenPos;
        float screenRadius;
        float distToMouse;
        bool visible;
    };

    std::vector<ProjectedBody> projectedBodies;
    int bestHoverIdx = -1;
    float bestDist = 1e9f;

    // Minimum adaptive hitbox radius in pixels relative to viewport size
    float baseHitbox = std::max(22.0f, vpH * 0.035f);

    for (int i = 0; i < (int)bodies.size(); ++i) {
        glm::vec2 sPos(0.0f);
        float sRadius = 0.0f;
        bool inFrustum = camera.projectToScreen(bodies[i].position, camera.getTargetPosition(),
                                                vpX, vpY, vpW, vpH, sPos, sRadius, bodies[i].radius3D);

        bool inViewport = (inFrustum && sPos.x >= vpX && sPos.x <= vpX + vpW && sPos.y >= vpY && sPos.y <= vpY + vpH);

        float hitboxRadius = std::max(baseHitbox, sRadius + 12.0f);
        float distToMouse = 1e9f;

        if (m_viewportHovered && inViewport) {
            float dx = mousePos.x - sPos.x;
            float dy = mousePos.y - sPos.y;
            distToMouse = std::sqrt(dx * dx + dy * dy);

            if (distToMouse <= hitboxRadius && distToMouse < bestDist) {
                bestDist = distToMouse;
                bestHoverIdx = i;
            }
        }

        projectedBodies.push_back({ i, sPos, sRadius, distToMouse, inViewport });
    }

    m_hoveredBodyIndex = bestHoverIdx;

    // Direct Left Click in 3D Viewport on body selects it
    if (m_viewportHovered && m_hoveredBodyIndex >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(m_hoveredBodyIndex);
        camera.focusOnBody(bodies[m_hoveredBodyIndex].position, bodies[m_hoveredBodyIndex].radius3D, 0.85f);
        addEventLog(bodies[m_hoveredBodyIndex].name + " selected via 3D Viewport");
    }

    // Draw HUD hover targeting reticle clipped to the 3D viewport area
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    fg->PushClipRect(ImVec2(vpX, vpY), ImVec2(vpX + vpW, vpY + vpH), true);

    if (m_hoveredBodyIndex >= 0 && m_hoveredBodyIndex != selectedIdx) {
        for (const auto& pb : projectedBodies) {
            if (pb.index != m_hoveredBodyIndex || !pb.visible) continue;

            const auto& body = bodies[pb.index];
            float ringR = std::max(13.0f, pb.screenRadius + 4.0f);
            ImVec2 center(pb.screenPos.x, pb.screenPos.y);

            ImU32 bodyCol = ImGui::ColorConvertFloat4ToU32(ImVec4(body.color.r, body.color.g, body.color.b, 1.0f));
            ImU32 glowCol = ImGui::ColorConvertFloat4ToU32(ImVec4(body.color.r, body.color.g, body.color.b, 0.25f));

            // Compact hover circle ring + subtle glow
            fg->AddCircle(center, ringR, bodyCol, 32, 1.5f);
            fg->AddCircle(center, ringR + 2.5f, glowCol, 32, 1.0f);

            // 4 Corner / cardinal HUD tick brackets
            float tickLen = 4.0f;
            fg->AddLine(ImVec2(center.x - ringR - 2.0f, center.y), ImVec2(center.x - ringR - 2.0f - tickLen, center.y), bodyCol, 1.2f);
            fg->AddLine(ImVec2(center.x + ringR + 2.0f, center.y), ImVec2(center.x + ringR + 2.0f + tickLen, center.y), bodyCol, 1.2f);
            fg->AddLine(ImVec2(center.x, center.y - ringR - 2.0f), ImVec2(center.x, center.y - ringR - 2.0f - tickLen), bodyCol, 1.2f);
            fg->AddLine(ImVec2(center.x, center.y + ringR + 2.0f), ImVec2(center.x, center.y + ringR + 2.0f + tickLen), bodyCol, 1.2f);

            // Hover Info Pill Badge (Name + Distance)
            std::string label = body.name + "  •  " + (body.id == "sol" ? "0.00 AU" : body.distanceStr);
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());

            float pillW = textSize.x + 14.0f;
            float pillH = textSize.y + 6.0f;
            float pillX = center.x + ringR + 8.0f;
            float pillY = center.y - pillH * 0.5f;

            if (pillX + pillW > vpX + vpW - 10.0f) {
                pillX = center.x - ringR - 8.0f - pillW;
            }

            fg->AddRectFilled(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                              ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.06f, 0.12f, 0.92f)), 4.0f);
            fg->AddRect(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                        bodyCol, 4.0f, 0, 1.0f);
            fg->AddText(ImVec2(pillX + 7.0f, pillY + 3.0f), bodyCol, label.c_str());
        }
    }

    fg->PopClipRect();

    // ── FLOATING VIEW / VISUALIZATION HUD CONTROLS ────────────────────────────
    ImVec2 visBtnPos(vpX + vpW - 180.0f, vpY + 10.0f);
    ImGui::SetCursorScreenPos(visBtnPos);
    static bool showVisPopup = false;
    if (ImGui::Button("⚙ VISUALIZATION", ImVec2(170, 26))) {
        showVisPopup = !showVisPopup;
    }

    if (showVisPopup) {
        ImGui::SetNextWindowPos(ImVec2(visBtnPos.x - 140.0f, visBtnPos.y + 32.0f), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(340.0f, 580.0f));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.045f, 0.085f, 0.98f));
        ImGui::PushStyleColor(ImGuiCol_Border, Col::Accent);
        if (ImGui::Begin("##VisPopup", &showVisPopup, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse)) {
            ImGui::TextColored(Col::Accent, "CINEMATIC & VISUAL PRESETS");
            int curPreset = (int)visualAdapter.getPreset();
            const char* presets[] = { "Normal (Baseline)", "Realistic (Photometric)", "Cinematic (Bloom + Flare)", "Ultra (Full ACES & FX)" };
            if (ImGui::Combo("Quality Preset##VisPreset", &curPreset, presets, 4)) {
                visualAdapter.applyPreset((VisualPreset)curPreset);
            }

            bool cineOn = visualAdapter.isCinematicModeEnabled();
            if (ImGui::Checkbox("Cinematic Graphics Pipeline (F10)", &cineOn)) {
                visualAdapter.setCinematicMode(cineOn);
            }

            if (cineOn) {
                float expVal = visualAdapter.getExposure();
                float ev = std::log2(std::max(expVal, 0.05f));
                if (ImGui::SliderFloat("Exposure (EV)", &ev, -3.0f, 3.0f, "%.2f EV")) {
                    visualAdapter.setExposure(std::pow(2.0f, ev));
                }

                float bloom = visualAdapter.getBloomIntensity();
                if (ImGui::SliderFloat("HDR Bloom Glow", &bloom, 0.0f, 2.5f, "%.2fx")) {
                    visualAdapter.setBloomIntensity(bloom);
                }

                int toneMode = visualAdapter.getToneMappingMode();
                const char* toneNames[] = { "ACES Filmic", "Reinhard", "Filmic" };
                if (ImGui::Combo("Tone Mapping", &toneMode, toneNames, 3)) {
                    visualAdapter.setToneMappingMode(toneMode);
                }
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(Col::Accent, "VISUALIZATION PIPELINE");

            int vMode = (int)visualAdapter.getVisualMode();
            const char* vModes[] = { "Realistic (PBR Photometry)", "Scientific (High-Contrast)", "Cinematic (Bloom & Flare)", "Debug (Physical Overlays)" };
            ImGui::Text("Visual Mode:");
            if (ImGui::Combo("##VModeCombo", &vMode, vModes, 4)) {
                visualAdapter.setVisualMode((VisualMode)vMode);
            }

            int dOverlay = (int)visualAdapter.getDebugOverlay();
            const char* dOverlays[] = { "None", "Von Mises Stress (Pa)", "Plastic Strain", "Damage / Fracture", "Surface Temperature (K)", "Velocity Vectors", "Gravitational Field", "Material Phase" };
            ImGui::Text("Debug Physical Field:");
            if (ImGui::Combo("##DOverlayCombo", &dOverlay, dOverlays, 8)) {
                visualAdapter.setDebugOverlay((DebugVisualOverlay)dOverlay);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(Col::Accent, "RENDERING FEATURES");

            bool showOrbits = visualAdapter.areOrbitLinesEnabled();
            if (ImGui::Checkbox("Keplerian Orbit Lines", &showOrbits)) {
                visualAdapter.setOrbitLinesEnabled(showOrbits);
            }

            bool showTrails = visualAdapter.areMotionTrailsEnabled();
            if (ImGui::Checkbox("N-Body Motion Trails", &showTrails)) {
                visualAdapter.setMotionTrailsEnabled(showTrails);
            }

            if (ImGui::Button("Clear Trails (C)##ClearTrailsBtn", ImVec2(180, 22))) {
                physics.clearTrails();
            }

            bool atmo = visualAdapter.areAtmospheresEnabled();
            if (ImGui::Checkbox("Atmospheric Rim Scattering", &atmo)) {
                visualAdapter.setAtmospheresEnabled(atmo);
            }

            bool clouds = visualAdapter.areCloudsEnabled();
            if (ImGui::Checkbox("Dynamic Rotating Clouds", &clouds)) {
                visualAdapter.setCloudsEnabled(clouds);
            }

            bool multiLight = visualAdapter.isMultiStarLightingEnabled();
            if (ImGui::Checkbox("Multi-Star Planetary Lighting", &multiLight)) {
                visualAdapter.setMultiStarLightingEnabled(multiLight);
            }

            bool impacts = visualAdapter.areImpactFXEnabled();
            if (ImGui::Checkbox("Collision Shockwave Ejecta", &impacts)) {
                visualAdapter.setImpactFXEnabled(impacts);
            }

            ImGui::Spacing();
            ImGui::Separator();
            ImGui::TextColored(Col::Accent, "SCALE CONTROLS");

            bool isTrueScale = physics.isTrueScaleMode();
            if (ImGui::Checkbox("True 1:1 Scale", &isTrueScale)) {
                physics.setTrueScaleMode(isTrueScale);
            }
            if (!isTrueScale) {
                float mult = physics.getSizeMultiplier();
                if (ImGui::SliderFloat("Visual Scale", &mult, 0.2f, 5.0f, "%.1fx")) {
                    physics.setSizeMultiplier(mult);
                }
            }

            ImGui::End();
        }
        ImGui::PopStyleColor(2);
    }
}


void UIManager::drawTimeControls(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##TimeControls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "TIME CONTROLS");
    ImGui::Separator();

    bool isPaused = physics.isPaused();
    if (ImGui::Button("|<", ImVec2(28, 24))) { physics.stepFrameBackward(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Step Backward (Frame)"); }
    ImGui::SameLine();
    if (ImGui::Button(isPaused ? " > " : " || ", ImVec2(28, 24))) { physics.togglePause(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip(isPaused ? "Play (Space)" : "Pause (Space)"); }
    ImGui::SameLine();
    if (ImGui::Button(">|", ImVec2(28, 24))) { physics.stepFrameForward(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Step Forward (Frame)"); }

    ImGui::SameLine(0, 6);
    if (ImGui::Button("1s/s", ImVec2(32, 24))) { physics.setTimeScale(1.0f); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("1d/s", ImVec2(32, 24))) { physics.setTimeScale(86400.0f); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("1m/s", ImVec2(32, 24))) { physics.setTimeScale(2592000.0f); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("1y/s", ImVec2(32, 24))) { physics.setTimeScale(31536000.0f); }

    ImGui::SameLine(0, 6);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.48f, 0.16f, 0.16f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.68f, 0.22f, 0.22f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.88f, 0.88f, 1.0f));
    if (ImGui::Button("↺ Reset", ImVec2(56, 24))) {
        physics.resetSimulation(objRepo);
        camera.resetOverview(glm::vec3(0.0f), 6.0f);
        addEventLog("Simulation workspace reset to fresh start");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear workspace, remove all custom loaded objects, and reset simulation to a fresh default start (Hotkey: R / Ctrl+R)");
    }
    ImGui::PopStyleColor(3);

    float scale = physics.getTimeScale();
    ImGui::PushItemWidth(w - 20);
    if (ImGui::SliderFloat("##speed", &scale, 1.0f, 31536000.0f, "Speed: %.0f sec/s", ImGuiSliderFlags_Logarithmic)) {
        physics.setTimeScale(scale);
    }
    ImGui::PopItemWidth();

    ImGui::TextColored(Col::TextSecondary, "%s", physics.getSimulationTimeStr().c_str());

    ImGui::End();
}

void UIManager::drawSimMetrics(PhysicsEngine& physics, float fps, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##SimMetrics", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "PHYSICS & RELATIVITY ENGINE");
    ImGui::Separator();

    float colW = (w - 30) / 4.0f;
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "FPS");
    ImGui::TextColored(Col::Green, "%.0f", fps);
    ImGui::EndGroup();

    ImGui::SameLine(colW);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Bodies");
    ImGui::TextColored(Col::TextPrimary, "%d", physics.getObjectCount());
    ImGui::EndGroup();

    ImGui::SameLine(colW * 2);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Step Time");
    ImGui::TextColored(Col::TextPrimary, "%.2f ms", physics.getPhysicsStepTimeMs());
    ImGui::EndGroup();

    ImGui::SameLine(colW * 3);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Integrator");
    ImGui::TextColored(Col::Accent, "Verlet (Sym)");
    ImGui::EndGroup();

    ImGui::Spacing();
    float halfW = (w - 30) / 2.0f;

    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Total System Energy");
    ImGui::TextColored(Col::TextPrimary, "%s", physics.getTotalEnergyStr().c_str());
    ImGui::EndGroup();

    ImGui::SameLine(halfW);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "System Angular Momentum");
    ImGui::TextColored(Col::TextPrimary, "%s", physics.getTotalAngularMomentumStr().c_str());
    ImGui::EndGroup();

    ImGui::Spacing();
    ImGui::TextColored(Col::TextSecondary, "Time Flow: ");
    ImGui::SameLine();
    ImGui::TextColored(Col::Accent, "%s", physics.getSimVsRealTimeStr().c_str());

    ImGui::Spacing();
    ImGui::Separator();

    bool grOn = physics.isGeneralRelativityEnabled();
    if (grOn) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.1f, 0.45f, 0.25f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.85f, 1.0f, 0.85f, 1.0f));
        if (ImGui::Button("EINSTEIN GR (1PN): ACTIVE", ImVec2(w - 20, 24))) {
            physics.toggleGeneralRelativity();
        }
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.2f, 0.1f, 0.9f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.75f, 1.0f));
        if (ImGui::Button("GRAVITY: NEWTONIAN ONLY", ImVec2(w - 20, 24))) {
            physics.toggleGeneralRelativity();
        }
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
}

// ------------------------------------------------------------------------------------------------
// SELECTION PLACE 3: 2D ORBIT VISUALIZATION SCHEMATIC (Click Planet to Select & Focus)
// ------------------------------------------------------------------------------------------------
void UIManager::drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##OrbitVis", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored(Col::Accent, "ORBIT VISUALIZATION");
    ImGui::Separator();

    ImVec2 contentMin = ImGui::GetCursorScreenPos();
    ImVec2 contentMax = ImVec2(x + w - 10.0f, y + h - 10.0f);
    float areaW = contentMax.x - contentMin.x;
    float areaH = contentMax.y - contentMin.y;
    float halfSize = std::min(areaW, areaH) * 0.45f;
    ImVec2 center = ImVec2(contentMin.x + areaW * 0.5f, contentMin.y + areaH * 0.5f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();
    bool panelHovered = (mousePos.x >= x && mousePos.x <= x + w && mousePos.y >= y && mousePos.y <= y + h);

    if (panelHovered && io.MouseWheel != 0.0f) {
        m_orbitVisZoom *= (io.MouseWheel > 0) ? 1.15f : 0.87f;
        m_orbitVisZoom = std::clamp(m_orbitVisZoom, 0.15f, 50.0f);
    }

    const auto& bodies = physics.getBodies();
    int selectedIdx = physics.getSelectedBodyIndex();
    float hitRadius = 12.0f;

    // Dynamically calculate system scale based on bodies in the current system
    float maxDistInSystem = 0.05f;
    for (const auto& b : bodies) {
        if (b.id != "sol" && b.type.find("Star") == std::string::npos) {
            double r = (b.realOrbitRadiusAU > 0.0) ? b.realOrbitRadiusAU : (b.semiMajorAxisAU > 0.0 ? b.semiMajorAxisAU : (double)glm::length(b.position));
            maxDistInSystem = std::max(maxDistInSystem, (float)r);
        }
    }
    float baseSystemAU = (maxDistInSystem < 0.2f) ? (maxDistInSystem * 1.35f) : ((maxDistInSystem < 5.5f) ? (maxDistInSystem * 1.25f) : 32.0f);
    float maxAU = baseSystemAU / m_orbitVisZoom;
    float scale = halfSize / maxAU;

    // Draw dynamic Keplerian osculating orbit tracks
    for (const auto& body : bodies) {
        if (body.id == "sol" || body.type.find("Star") != std::string::npos) continue;

        ImU32 orbitLineCol = ImGui::ColorConvertFloat4ToU32(
            ImVec4(body.color.r * 0.55f, body.color.g * 0.55f, body.color.b * 0.55f, 0.45f));

        if (body.dynamicOrbitCurve.size() >= 2) {
            for (size_t s = 0; s < body.dynamicOrbitCurve.size() - 1; ++s) {
                const auto& pA = body.dynamicOrbitCurve[s];
                const auto& pB = body.dynamicOrbitCurve[s + 1];
                if (std::isnan(pA.x) || std::isnan(pA.z) || std::isnan(pB.x) || std::isnan(pB.z)) continue;
                if (glm::distance(pA, pB) > 50.0f) continue; // Don't draw across jump discontinuities

                ImVec2 pt1(center.x + pA.x * scale, center.y + pA.z * scale);
                ImVec2 pt2(center.x + pB.x * scale, center.y + pB.z * scale);
                if (std::abs(pt1.x - center.x) < halfSize * 3.0f && std::abs(pt1.y - center.y) < halfSize * 3.0f &&
                    std::abs(pt2.x - center.x) < halfSize * 3.0f && std::abs(pt2.y - center.y) < halfSize * 3.0f) {
                    dl->AddLine(pt1, pt2, orbitLineCol, 1.2f);
                }
            }
        } else {
            double orbitRadiusAU = (body.realOrbitRadiusAU > 0.0) ? body.realOrbitRadiusAU : (body.semiMajorAxisAU > 0.0 ? body.semiMajorAxisAU : (double)glm::length(body.position));
            if (orbitRadiusAU > 0.00001 && orbitRadiusAU < 500.0) {
                float ringRadius = (float)orbitRadiusAU * scale;
                if (ringRadius >= 2.0f && ringRadius <= halfSize * 3.5f) {
                    dl->AddCircle(center, ringRadius, orbitLineCol, 64, 1.0f);
                }
            }
        }

    }

    struct BodyScreenInfo { int index; float px, py, dotR; bool visible; };
    std::vector<BodyScreenInfo> screenBodies;

    // Find star position in 3D AU coordinates
    glm::vec3 starPosAU{0.0f};
    for (const auto& b : bodies) {
        if (b.id == "sol" || b.type.find("Star") != std::string::npos) {
            starPosAU = b.position;
            break;
        }
    }

    for (int i = 0; i < (int)bodies.size(); ++i) {
        float px, py;
        if (i == 0 || bodies[i].id == "sol" || bodies[i].type.find("Star") != std::string::npos) {
            px = center.x;
            py = center.y;
        } else {
            glm::vec3 relPos = bodies[i].position - starPosAU;
            px = center.x + relPos.x * scale;
            py = center.y + relPos.z * scale;
        }
        bool visible = (px >= x - 20 && px <= x + w + 20 && py >= y - 20 && py <= y + h + 20);
        bool isSelected = (i == selectedIdx);
        float dotR = (i == 0 || bodies[i].type.find("Star") != std::string::npos) ? 6.0f : (isSelected ? 5.0f : 3.5f);
        screenBodies.push_back({i, px, py, dotR, visible});
    }

    int hoveredIdx = -1;
    if (panelHovered) {
        float closestDist = hitRadius;
        for (auto& sb : screenBodies) {
            if (!sb.visible) continue;
            float dx = mousePos.x - sb.px;
            float dy = mousePos.y - sb.py;
            float dist = sqrtf(dx * dx + dy * dy);
            if (dist < closestDist) {
                closestDist = dist;
                hoveredIdx = sb.index;
            }
        }
    }

    // Direct Left Click on Orbit Vis Radar selects the body
    if (hoveredIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(hoveredIdx);
        camera.focusOnBody(bodies[hoveredIdx].position, bodies[hoveredIdx].radius3D, 0.85f);
        addEventLog(bodies[hoveredIdx].name + " selected via Orbit Radar");
    }

    // Draw central star
    {
        dl->AddCircleFilled(center, 7.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.3f, 1.0f)), 32);
        dl->AddCircle(center, 10.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.3f, 0.35f)), 32, 1.5f);
    }

    // Draw planets on 2D radar
    for (const auto& sb : screenBodies) {
        if (!sb.visible || sb.index == 0 || bodies[sb.index].type.find("Star") != std::string::npos) continue;
        const auto& body = bodies[sb.index];
        bool isSel = (sb.index == selectedIdx);
        bool isHov = (sb.index == hoveredIdx);

        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(body.color.r, body.color.g, body.color.b, 1.0f));

        if (isSel) {
            dl->AddCircle(ImVec2(sb.px, sb.py), sb.dotR + 4.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent), 16, 1.5f);
        }
        if (isHov && !isSel) {
            dl->AddCircle(ImVec2(sb.px, sb.py), sb.dotR + 3.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(1, 1, 1, 0.6f)), 16, 1.0f);
        }

        dl->AddCircleFilled(ImVec2(sb.px, sb.py), sb.dotR, col, 16);

        // Tooltip on Hover
        if (isHov) {
            std::string label = body.name + " (" + body.distanceStr + ")";
            ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
            float tX = sb.px + sb.dotR + 6.0f;
            float tY = sb.py - textSize.y * 0.5f;

            dl->AddRectFilled(ImVec2(tX - 4, tY - 2), ImVec2(tX + textSize.x + 4, tY + textSize.y + 2),
                              ImGui::ColorConvertFloat4ToU32(ImVec4(0.04f, 0.06f, 0.12f, 0.90f)), 3.0f);
            dl->AddRect(ImVec2(tX - 4, tY - 2), ImVec2(tX + textSize.x + 4, tY + textSize.y + 2),
                        col, 3.0f, 0, 1.0f);
            dl->AddText(ImVec2(tX, tY), col, label.c_str());
        }
    }

    ImGui::End();
}

void UIManager::drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH) {
    ImGui::SetNextWindowPos(ImVec2(0, winH - barH));
    ImGui::SetNextWindowSize(ImVec2(winW, barH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));

    ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    const CelestialBody& sel = physics.getSelectedBody();
    ImGui::TextColored(Col::Accent, "TARGET: %s (%s)", sel.name.c_str(), sel.type.c_str());
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "| Dist: %s", sel.distanceStr.c_str());
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "| Cam: %.2f AU (fov %.0f°)", camera.getDistance(), camera.getFOV());
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "| Engine: %s", physics.isGeneralRelativityEnabled() ? "Einstein 1PN GR" : "Newtonian");
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::Green, "● Database: Active");

    ImGui::SameLine(winW - 200.0f);
    ImGui::TextColored(Col::TextSecondary, "Epoch: %s", sel.epochUtcStr.c_str());

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void UIManager::drawAsteroidBeltDiagnostics(PhysicsEngine& physics, ObjectRepository& objRepo, float winW, float winH) {
    auto& belt = physics.getAsteroidBelt();
    const auto& hist = belt.getHistogram();
    const auto& diag = belt.getDiagnostics();

    float w = 720.0f;
    float h = 540.0f;
    ImGui::SetNextWindowPos(ImVec2((winW - w) * 0.5f, (winH - h) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.09f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, Col::AccentDim);

    if (ImGui::Begin("ASTEROID BELT POPULATION & KIRKWOOD GAPS MONITOR##BeltDiag", &m_showAsteroidBeltDiagnostics, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(Col::Accent, "RADIAL DISTRIBUTION N(a) & MEAN-MOTION ORBITAL RESONANCES");
        ImGui::Separator();

        // Population Mode Switcher
        int curMode = (int)physics.getAsteroidPopulationMode();
        const char* modeNames[] = {
            "Real SBDB Population (Major Asteroids from DB)",
            "Synthetic Statistical (Kirkwood Gaps Simulation)",
            "Hybrid (Real Major Asteroids + Synthetic Swarm)"
        };

        ImGui::Text("Population Mode:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(360.0f);
        if (ImGui::Combo("##PopModeCombo", &curMode, modeNames, 3)) {
            physics.setAsteroidPopulationMode((AsteroidPopulationMode)curMode, &objRepo);
            addEventLog("Asteroid population mode set to: " + std::string(modeNames[curMode]));
        }

        ImGui::Spacing();

        // Histogram of Kirkwood Gaps N(a)
        ImVec2 plotMin = ImGui::GetCursorScreenPos();
        float plotW = w - 40.0f;
        float plotH = 160.0f;
        ImVec2 plotMax(plotMin.x + plotW, plotMin.y + plotH);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(plotMin, plotMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.06f, 0.9f)), 4.0f);
        dl->AddRect(plotMin, plotMax, ImGui::ColorConvertFloat4ToU32(Col::Border), 4.0f);

        if (!hist.counts.empty() && hist.maxBinCount > 0) {
            float barW = plotW / (float)hist.counts.size();
            for (size_t i = 0; i < hist.counts.size(); ++i) {
                float normH = (float)hist.counts[i] / (float)hist.maxBinCount;
                float bx0 = plotMin.x + (float)i * barW;
                float bx1 = bx0 + barW - 1.0f;
                float by0 = plotMax.y;
                float by1 = plotMax.y - normH * (plotH - 12.0f);

                ImU32 barCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.12f, 0.55f, 0.85f, 0.85f));
                dl->AddRectFilled(ImVec2(bx0, by1), ImVec2(bx1, by0), barCol);
            }

            // Resonance markers
            struct ResMarker { float au; const char* name; };
            ResMarker markers[] = {
                { ParticleHistogram::RES_4_1, "4:1" },
                { ParticleHistogram::RES_3_1, "3:1" },
                { ParticleHistogram::RES_5_2, "5:2" },
                { ParticleHistogram::RES_7_3, "7:3" },
                { ParticleHistogram::RES_2_1, "2:1" }
            };

            for (const auto& rm : markers) {
                if (rm.au < hist.minAU || rm.au > hist.maxAU) continue;
                float normX = (rm.au - hist.minAU) / (hist.maxAU - hist.minAU);
                float rx = plotMin.x + normX * plotW;

                dl->AddLine(ImVec2(rx, plotMin.y), ImVec2(rx, plotMax.y),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.35f, 0.25f, 0.85f)), 1.2f);
                dl->AddText(ImVec2(rx + 2.0f, plotMin.y + 4.0f),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.65f, 0.45f, 1.0f)), rm.name);
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(plotMin.x, plotMax.y + 10.0f));
        ImGui::Separator();

        float halfW = (w - 40.0f) * 0.5f;
        StatCard2Col("Active Physical Asteroids", std::to_string(diag.activePhysical).c_str(),
                     "Visual Asteroids (GPU)", std::to_string(diag.totalVisual).c_str(), halfW);
        StatCard2Col("Mean Semi-Major Axis", (std::to_string(diag.meanSemiMajorAxisAU).substr(0, 5) + " AU").c_str(),
                     "Mean Eccentricity (e)", std::to_string(diag.meanEccentricity).substr(0, 5).c_str(), halfW);
        StatCard2Col("Resonance Excitation Count", std::to_string(diag.highlyExcitedCount).c_str(),
                     "Energy Conservation Drift", (std::to_string(diag.energyDriftPct).substr(0, 6) + " %").c_str(), halfW);

        ImGui::Spacing();
        if (ImGui::Button("⚡ Jupiter Flyby Perturbation Impulse Test", ImVec2(320, 26))) {
            belt.triggerResonanceImpulseTest();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Simulates strong gravitational kicks at Jupiter resonances (3:1 and 2:1) to demonstrate orbital excitation and gap depletion!");
        }

        ImGui::SameLine(w - 120.0f);
        if (ImGui::Button("Close##Belt", ImVec2(90, 26))) {
            m_showAsteroidBeltDiagnostics = false;
        }
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

void UIManager::drawMatterLab(PhysicsEngine& physics, Camera& camera, float winW, float winH) {
    auto& matter = physics.getMatterSystem();
    const auto& diag = matter.getDiagnostics();
    auto& lib = MaterialLibrary::instance();

    float w = 780.0f;
    float h = 640.0f;
    ImGui::SetNextWindowPos(ImVec2((winW - w) * 0.5f, (winH - h) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.05f, 0.09f, 0.97f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.65f, 0.35f, 0.15f, 0.7f));

    if (ImGui::Begin("DEFORMABLE MATTER & MATERIALS PHYSICS LABORATORY##MatterLab", &m_showMatterLab, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0.95f, 0.7f, 0.35f, 1.0f), "COUPLED CONTINUUM MECHANICS, XPBD, PLASTICITY & FRACTURE ENGINE");
        ImGui::Separator();

        // 1. Scientific Field Visualization Mode Selector
        ImGui::TextColored(Col::Accent, "SCIENTIFIC VISUALIZATION FIELD SELECTOR");
        int currentMode = (int)matter.getVisualizationMode();
        const char* modeNames[] = {
            "Realistic Material Surface",
            "Von Mises Stress Field (Turbo Colormap)",
            "Mechanical Strain & Deformation",
            "Temperature Heatmap & Incandescence",
            "Continuous Damage & Micro-cracks",
            "Plastic Deformation Field",
            "Differential Tidal Gravity Vectors"
        };

        ImGui::PushItemWidth(340.0f);
        if (ImGui::Combo("##VisMode", &currentMode, modeNames, 7)) {
            matter.setVisualizationMode((MatterVisualizationMode)currentMode);
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();
        ImGui::Separator();

        // 2. Material Property Inspector & Derived Physics
        ImGui::TextColored(Col::Accent, "MATERIAL PROPERTY & DERIVED ELASTIC CONSTANTS INSPECTOR");
        static int selectedMatIdx = 1;
        std::vector<std::string> matNames = lib.getMaterialNames();

        std::vector<const char*> matNameCstrs;
        for (const auto& name : matNames) matNameCstrs.push_back(name.c_str());

        ImGui::PushItemWidth(260.0f);
        if (selectedMatIdx >= (int)matNames.size()) selectedMatIdx = 0;
        ImGui::Combo("Material Preset", &selectedMatIdx, matNameCstrs.data(), (int)matNameCstrs.size());
        ImGui::PopItemWidth();

        const auto& selMat = lib.getMaterial(matNames[selectedMatIdx]);
        auto derived = MaterialModel::computeDerivedProperties(selMat);

        float halfW = (w - 40.0f) * 0.5f;

        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Fundamental Parameters:");
        ImGui::Text("Density (rho0): %.0f kg/m3", selMat.referenceDensityKgM3);
        ImGui::Text("Young's Modulus (E): %.2f GPa", selMat.youngsModulusPa / 1.0e9);
        ImGui::Text("Poisson's Ratio (nu): %.2f", selMat.poissonsRatio);
        ImGui::Text("Yield Strength (sig_y): %.1f MPa", selMat.yieldStrengthPa / 1.0e6);
        ImGui::Text("UTS (Tensile limit): %.1f MPa", selMat.ultimateTensileStrengthPa / 1.0e6);
        ImGui::EndGroup();

        ImGui::SameLine(halfW);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Derived Source-of-Truth Properties:");
        ImGui::Text("Shear Modulus (G): %.2f GPa", derived.shearModulusPa / 1.0e9);
        ImGui::Text("Bulk Modulus (K): %.2f GPa", derived.bulkModulusPa / 1.0e9);
        ImGui::Text("Acoustic Sound Speed: %.0f m/s", derived.soundSpeedMps);
        ImGui::Text("Thermal Conductivity: %.1f W/m*K", selMat.thermalConductivityWPerMK);
        ImGui::Text("Melting Point: %.1f K (%.0f °C)", selMat.meltingPointK, selMat.meltingPointK - 273.15);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Continuum Mechanics & Conservation Monitor
        ImGui::TextColored(Col::Accent, "PHYSICS STATE & CONSERVATION MONITOR");
        float quadW = (w - 50.0f) / 4.0f;

        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Active Bodies/Fragments");
        ImGui::TextColored(Col::Green, "%d Bodies", diag.totalDeformableBodies);
        ImGui::TextColored(Col::TextSecondary, "Nodes: %d", diag.totalNodes);
        ImGui::EndGroup();

        ImGui::SameLine(quadW);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Constraints / Fractures");
        ImGui::TextColored(Col::Accent, "%d Active", diag.totalConstraints - diag.totalBrokenConstraints);
        ImGui::TextColored(Col::Red, "Broken: %d", diag.totalBrokenConstraints);
        ImGui::EndGroup();

        ImGui::SameLine(quadW * 2);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Max Stress & Temp");
        ImGui::TextColored(Col::TextPrimary, "%.1f MPa", diag.maxVonMisesStressPa / 1.0e6);
        ImGui::TextColored(Col::TextSecondary, "Temp: %.1f K", diag.maxTemperatureK);
        ImGui::EndGroup();

        ImGui::SameLine(quadW * 3);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Energy Conservation");
        ImGui::TextColored(Col::TextPrimary, "Drift: %.4f %%", diag.energyConservationDriftPct);
        ImGui::TextColored(Col::TextSecondary, "Max Damage: %.2f", diag.maxDamage);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();

        // 4. Sandbox Scenario Presets
        ImGui::TextColored(Col::Accent, "DEFORMABLE ASTROPHYSICAL SCENARIOS & EXPERIMENTS");

        if (ImGui::Button("\xF0\x9F\x8C\x8C Black Hole Tidal Disruption Laboratory", ImVec2(340, 28))) {
            matter.spawnBlackHoleTidalDisruptionLab();
            camera.resetOverview(glm::vec3(0.0468f, 0.0f, 0.0f), 0.12f);
            addEventLog("Black Hole Tidal Disruption spawned (Camera centered at 0.047 AU)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Spawns a 150 km asteroid on an extreme periapsis trajectory near a massive gravitational attractor. Watch differential gravity stretch, yield, and fragment the object into a tidal debris stream!");
        }

        ImGui::SameLine();
        if (ImGui::Button("\xE2\x98\x84 Hypervelocity Impact & Crater Fracture", ImVec2(340, 28))) {
            matter.spawnHypervelocityCollision();
            camera.resetOverview(glm::vec3(0.0f), 0.00025f);
            addEventLog("Hypervelocity Collision spawned (Camera focused on impact origin)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Collides a high-speed Iron impactor with a Basalt rock target, producing realistic contact stress, plastic deformation, impact heating, and fragmentation!");
        }

        if (ImGui::Button("\xE2\x9A\xA1 Tensile Stress & Necking / Ductile Failure", ImVec2(340, 28))) {
            matter.spawnTensileTest();
            camera.resetOverview(glm::vec3(0.0f), 0.00010f);
            addEventLog("Tensile Test specimen spawned (Camera focused on test specimen)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Anchors a specimen on one end while applying tensile velocity to the other. Demonstrates linear elasticity, von Mises yielding, necking, and ductile fracture!");
        }

        ImGui::SameLine();
        if (ImGui::Button("\xF0\x9F\x94\xA5 Thermal Heating & Melting Phase Change", ImVec2(340, 28))) {
            matter.spawnThermalMeltingLab();
            camera.resetOverview(glm::vec3(0.0f), 0.00012f);
            addEventLog("Thermal Melting specimen spawned (Camera focused on melting ice block)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Applies intense heat flux to an ice/metal block, demonstrating thermal conduction, thermal softening, melting, and fluid drop relaxation!");
        }

        ImGui::Spacing();
        if (ImGui::Button("✖ Clear All Deformable Bodies", ImVec2(220, 26))) {
            matter.clearAllBodies();
            addEventLog("Cleared deformable bodies");
        }
        ImGui::SameLine(w - 120.0f);
        if (ImGui::Button("Close##Matter", ImVec2(90, 26))) {
            m_showMatterLab = false;
        }
    }

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

// ------------------------------------------------------------------------------------------------
// TOP-LEVEL WORKSPACE: EXPLORE (Catalog & Discovery Browser)
// ------------------------------------------------------------------------------------------------
void UIManager::drawExploreWorkspace(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, float winW, float winH) {
    float topBarH = 48.0f;
    float statusBarH = 28.0f;
    float contentW = winW;
    float contentH = winH - topBarH - statusBarH;

    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(contentW, contentH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Col::BgDark);

    ImGui::Begin("##ExploreWorkspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "🌌 ASTRONOMICAL EXPLORER & CATALOG DISCOVERY");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "| Explore Verified NASA/JPL Baseline Celestial Catalog");
    ImGui::Separator();
    ImGui::Spacing();

    static char searchFilter[64] = "";
    static int catFilter = 0; // 0: All, 1: Solar System, 2: Exoplanet Systems, 3: Asteroids

    ImGui::TextColored(Col::TextSecondary, "Catalog Filter:");
    const char* cats[] = { "All Astronomical Objects", "Solar System", "Exoplanet Systems", "Asteroid Belt" };
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine(0, 8);
        bool isSel = (catFilter == i);
        if (isSel) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.80f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.20f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::Button(cats[i], ImVec2(180, 26))) catFilter = i;
        ImGui::PopStyleColor(2);
    }

    ImGui::SameLine(contentW - 320.0f);
    ImGui::PushItemWidth(300);
    ImGui::InputTextWithHint("##ExploreSearch", "Search catalog...", searchFilter, sizeof(searchFilter));
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    std::string catStr = (catFilter == 1) ? "Solar System" : (catFilter == 2) ? "Exoplanet System" : (catFilter == 3) ? "Asteroid Belt" : "";
    auto objects = objRepo.getAllObjects(catStr, false, searchFilter);

    if (ImGui::BeginTable("##ExploreTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("Name", ImGuiTableColumnFlags_WidthFixed, 200);
        ImGui::TableSetupColumn("Classification", ImGuiTableColumnFlags_WidthFixed, 180);
        ImGui::TableSetupColumn("Category", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn("Mass", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("Radius", ImGuiTableColumnFlags_WidthFixed, 140);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableHeadersRow();

        for (const auto& obj : objects) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            bool isStar = obj.type.find("Star") != std::string::npos;
            ImGui::TextColored(isStar ? Col::Yellow : Col::Accent, "%s %s", isStar ? "★" : "●", obj.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(obj.type.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(Col::TextSecondary, "%s", obj.category.c_str());

            auto phys = objRepo.getPhysicalProperties(obj.id);
            ImGui::TableSetColumnIndex(3);
            if (phys.has_value() && phys->massKg.has_value()) {
                ImGui::TextUnformatted(UnitConverter::formatMass(phys->massKg.value()).c_str());
            } else {
                ImGui::TextColored(Col::TextSecondary, "N/A");
            }

            ImGui::TableSetColumnIndex(4);
            if (phys.has_value() && phys->radiusM.has_value()) {
                ImGui::Text("%'.1f km", phys->radiusM.value() / 1000.0);
            } else {
                ImGui::TextColored(Col::TextSecondary, "N/A");
            }

            ImGui::TableSetColumnIndex(5);
            ImGui::PushID((int)obj.id);
            if (ImGui::SmallButton("🔍 Inspect in Object Editor")) {
                m_objectWorkspaceUI.setSelectedObjectBySlug(obj.slug, objRepo);
                m_activeTopTab = 2; // OBJECTS workspace
            }
            ImGui::SameLine();
            if (ImGui::SmallButton("🚀 Test in Universe")) {
                auto bOpt = objRepo.getHydratedBody(obj.id);
                if (bOpt.has_value()) {
                    physics.clearBodies();
                    physics.addBody(bOpt.value());
                    camera.resetOverview(glm::vec3(0.0f), 4.0f);
                    m_activeTopTab = 0; // UNIVERSE
                }
            }
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void UIManager::drawSimulationWorkspace(PhysicsEngine& physics, Camera& camera, ValidationEngine& valEngine, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, float winW, float winH) {
    float topBarH = 48.0f;
    float statusBarH = 28.0f;
    float contentW = winW;
    float contentH = winH - topBarH - statusBarH;

    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(contentW, contentH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Col::BgDark);

    ImGui::Begin("##SimulationWorkspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "⚙ PHYSICS ENGINE, TIME SYSTEM & DIAGNOSTICS");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "| Integrator, Gravitation & Conservation Diagnostics");
    ImGui::Separator();
    ImGui::Spacing();

    float colW = (contentW - 48.0f) / 2.0f;

    // Left Column: Physical Integrator & Time Configuration
    ImGui::BeginChild("##SimLeftCol", ImVec2(colW, contentH - 50.0f), true);
    ImGui::TextColored(Col::Accent, "GRAVITATION & INTEGRATOR CONFIGURATION");
    ImGui::Separator();
    ImGui::Spacing();

    bool gr = physics.isGeneralRelativityEnabled();
    if (ImGui::Checkbox("Enable Einstein 1PN Post-Newtonian General Relativity (GR)", &gr)) {
        physics.setGeneralRelativityEnabled(gr);
    }
    ImGui::TextColored(Col::TextSecondary, "Computes 1PN relativistic perihelion precession and gravitational time dilation.");

    ImGui::Spacing();
    bool paused = physics.isPaused();
    if (ImGui::Checkbox("Pause Simulation Dynamics", &paused)) {
        physics.setPaused(paused);
    }

    ImGui::Spacing();
    float timeScale = physics.getTimeScale();
    if (ImGui::DragFloat("Simulation Time Warp (sec/sec)", &timeScale, 100.0f, 0.1f, 86400.0f * 365.0f, "%.1f x")) {
        physics.setTimeScale(timeScale);
    }

    ImGui::Spacing();
    bool trueScale = physics.isTrueScaleMode();
    if (ImGui::Checkbox("True 1:1 Astronomical Scale", &trueScale)) {
        physics.setTrueScaleMode(trueScale);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Accent, "DIAGNOSTIC & VALIDATION LABORATORIES");
    if (ImGui::Button("⚖ Open Real-Data Validation Dashboard", ImVec2(-1, 28))) {
        m_showValidationDashboard = true;
    }
    if (ImGui::Button("☄ Open Asteroid Belt Statistical Tool (N(a))", ImVec2(-1, 28))) {
        m_showAsteroidBeltDiagnostics = true;
    }
    if (ImGui::Button("⬡ Open Deformable Matter Impact Lab", ImVec2(-1, 28))) {
        m_showMatterLab = true;
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // Right Column: System Conservation & Visualization Pipeline
    ImGui::BeginChild("##SimRightCol", ImVec2(colW, contentH - 50.0f), true);
    ImGui::TextColored(Col::Accent, "GLOBAL SYSTEM CONSERVATION DIAGNOSTICS");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::TextSecondary, "Total System Energy:");
    ImGui::TextColored(Col::TextPrimary, "%s", physics.getTotalEnergyStr().c_str());

    ImGui::TextColored(Col::TextSecondary, "Total Angular Momentum:");
    ImGui::TextColored(Col::TextPrimary, "%s", physics.getTotalAngularMomentumStr().c_str());

    ImGui::TextColored(Col::TextSecondary, "Energy Conservation Drift:");
    ImGui::TextColored((physics.getEnergyConservationDriftPct() < 0.01) ? Col::Green : Col::Orange, 
                       "%.6f %%", physics.getEnergyConservationDriftPct());

    ImGui::TextColored(Col::TextSecondary, "Simulated Epoch Time:");
    ImGui::TextColored(Col::TextPrimary, "%s", physics.getSimulationTimeStr().c_str());

    ImGui::TextColored(Col::TextSecondary, "Active Gravitational Bodies:");
    ImGui::TextColored(Col::Accent, "%d bodies", physics.getObjectCount());

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Accent, "GRAPHICS & VISUALIZATION PIPELINE");
    ImGui::Separator();

    int vMode = (int)visualAdapter.getVisualMode();
    const char* vModes[] = { "Realistic (PBR Photometry)", "Scientific (High-Contrast)", "Cinematic (Bloom & Flare)", "Debug (Physical Overlays)" };
    ImGui::Text("Visual Mode:");
    if (ImGui::Combo("##SimVModeCombo", &vMode, vModes, 4)) {
        visualAdapter.setVisualMode((VisualMode)vMode);
    }

    int dOverlay = (int)visualAdapter.getDebugOverlay();
    const char* dOverlays[] = { "None", "Von Mises Stress (Pa)", "Plastic Strain", "Damage / Fracture", "Surface Temperature (K)", "Velocity Vectors", "Gravitational Field", "Material Phase" };
    ImGui::Text("Debug Physical Field:");
    if (ImGui::Combo("##SimDOverlayCombo", &dOverlay, dOverlays, 8)) {
        visualAdapter.setDebugOverlay((DebugVisualOverlay)dOverlay);
    }

    bool atmo = visualAdapter.areAtmospheresEnabled();
    if (ImGui::Checkbox("Atmospheric Rim Scattering", &atmo)) {
        visualAdapter.setAtmospheresEnabled(atmo);
    }

    bool clouds = visualAdapter.areCloudsEnabled();
    if (ImGui::Checkbox("Dynamic Rotating Clouds", &clouds)) {
        visualAdapter.setCloudsEnabled(clouds);
    }

    bool multiLight = visualAdapter.isMultiStarLightingEnabled();
    if (ImGui::Checkbox("Multi-Star Lighting", &multiLight)) {
        visualAdapter.setMultiStarLightingEnabled(multiLight);
    }

    bool impacts = visualAdapter.areImpactFXEnabled();
    if (ImGui::Checkbox("Collision Shockwave Particles", &impacts)) {
        visualAdapter.setImpactFXEnabled(impacts);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("🚀 Return to UNIVERSE View", ImVec2(-1, 32))) {
        m_activeTopTab = 0;
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}


// ------------------------------------------------------------------------------------------------
// TOP-LEVEL WORKSPACE: AI ASSISTANT (Astrophysics & Orbital Calculator)
// ------------------------------------------------------------------------------------------------
void UIManager::drawAIAssistantWorkspace(PhysicsEngine& physics, ObjectRepository& objRepo, ai::AIManager& aiManager, float winW, float winH) {
    float topBarH = 48.0f;
    float statusBarH = 28.0f;
    float contentW = winW;
    float contentH = winH - topBarH - statusBarH;

    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(contentW, contentH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Col::BgDark);

    ImGui::Begin("##AIAssistantWorkspace", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    const auto& pred = aiManager.getCurrentPrediction();
    const auto& feat = aiManager.getCurrentFeatures();

    // ── TOP HEADER & DIAGNOSTICS BAR ──
    ImGui::TextColored(Col::Accent, "🤖 ASTROGENESIS AI & MACHINE LEARNING STUDIO");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "| Local Machine Learning Orbital Stability Predictor");
    ImGui::SameLine(contentW - 320.0f);
    if (pred.modelLoaded) {
        ImGui::TextColored(Col::Green, "● MODEL ACTIVE (Local, No API)");
    } else {
        ImGui::TextColored(Col::Orange, "○ MODEL UNAVAILABLE");
    }

    ImGui::Separator();
    ImGui::Spacing();

    float colW = (contentW - 48.0f) / 3.0f;

    // ── COLUMN 1: LIVE STABILITY ANALYZER ──
    ImGui::BeginChild("##AICol1", ImVec2(colW, contentH - 50.0f), true);
    ImGui::TextColored(Col::Accent, "1. LIVE ORBITAL STABILITY PREDICTOR");
    ImGui::Separator();
    ImGui::Spacing();

    // Large Banner
    ImVec4 predBadgeCol = Col::Green;
    const char* predBadgeTitle = "SYSTEM PREDICTED: STABLE";
    if (pred.prediction == "UNSTABLE") {
        predBadgeCol = Col::Red;
        predBadgeTitle = "SYSTEM PREDICTED: UNSTABLE";
    } else if (pred.prediction == "MARGINAL") {
        predBadgeCol = Col::Yellow;
        predBadgeTitle = "SYSTEM PREDICTED: MARGINAL";
    } else if (pred.prediction == "UNAVAILABLE") {
        predBadgeCol = Col::TextSecondary;
        predBadgeTitle = "SYSTEM STATUS: MODEL UNAVAILABLE";
    }

    ImGui::PushStyleColor(ImGuiCol_Button, predBadgeCol);
    ImGui::Button(predBadgeTitle, ImVec2(-1, 32));
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::TextColored(Col::TextSecondary, "Model Architecture: ");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextPrimary, "%s", pred.modelArchitecture.c_str());

    ImGui::TextColored(Col::TextSecondary, "Prediction Confidence: ");
    ImGui::SameLine();
    ImGui::TextColored(pred.confidence == "HIGH" ? Col::Green : (pred.confidence == "MEDIUM" ? Col::Yellow : Col::Orange),
                       "%s", pred.confidence.c_str());

    ImGui::Spacing();
    ImGui::Text("Stable Probability:   %.1f%%", pred.stableProbability * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col::Green);
    ImGui::ProgressBar(pred.stableProbability, ImVec2(-1, 16));
    ImGui::PopStyleColor();

    ImGui::Text("Unstable Probability: %.1f%%", pred.unstableProbability * 100.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, Col::Orange);
    ImGui::ProgressBar(pred.unstableProbability, ImVec2(-1, 16));
    ImGui::PopStyleColor();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Accent, "Extracted Celestial Mechanics Features:");
    ImGui::Text("  • Primary Host Star Mass: %.3f M☉", feat.starMassKg / 1.9885e30);
    ImGui::Text("  • Number of Orbiting Bodies: %d", feat.bodyCount);
    ImGui::Text("  • Min Mutual Hill Separation: %.2f R_Hill", feat.minMutualHillSep);
    ImGui::Text("  • Max Planetary Eccentricity: %.4f", feat.maxEccentricity);
    ImGui::Text("  • Angular Momentum Deficit (AMD): %.5f", feat.angularMomentumDeficit);
    ImGui::Text("  • Planetary Orbit Crossing: %s", feat.hasOrbitCrossing ? "YES (CRITICAL RISK)" : "NO (CLEAR)");
    ImGui::Text("  • Inference Latency: %.1f µs", pred.inferenceTimeUs);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Yellow, "Risk Factor Diagnostics:");
    if (pred.riskFactors.empty()) {
        ImGui::TextColored(Col::Green, "  ✓ All orbital separation criteria satisfied.");
    } else {
        for (const auto& risk : pred.riskFactors) {
            ImGui::TextColored(Col::Orange, "  ⚠ %s", risk.c_str());
        }
    }

    ImGui::Spacing();
    if (ImGui::Button("🔄 Force Re-Evaluate Simulation State", ImVec2(-1, 26))) {
        aiManager.forceRecompute(physics);
    }

    ImGui::Spacing();
    ImGui::TextDisabled("ℹ Scientific Honesty: ML-based estimate of stability from trained dynamical patterns. Does not replace symplectic physics integrator.");

    ImGui::EndChild();

    ImGui::SameLine();

    // ── COLUMN 2: MUTUAL HILL SPHERE & ORBITAL DYNAMICS INSPECTOR ──
    ImGui::BeginChild("##AICol2", ImVec2(colW, contentH - 50.0f), true);
    ImGui::TextColored(Col::Accent, "2. DYNAMICAL HIERARCHY & HILL SPHERES");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::TextSecondary, "Gladman Hill Stability Criterion (Δ > 3.46):");
    ImGui::Spacing();

    if (feat.pairMetrics.empty()) {
        ImGui::TextDisabled("Need at least 2 co-orbiting bodies to evaluate mutual Hill spheres.");
    } else {
        if (ImGui::BeginTable("##HillTable", 4, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Pair");
            ImGui::TableSetupColumn("Δ (R_H)");
            ImGui::TableSetupColumn("P_ratio");
            ImGui::TableSetupColumn("Status");
            ImGui::TableHeadersRow();

            for (const auto& pair : feat.pairMetrics) {
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::Text("%s - %s", pair.innerName.c_str(), pair.outerName.c_str());

                ImGui::TableNextColumn();
                if (pair.isHillUnstable) {
                    ImGui::TextColored(Col::Red, "%.2f", pair.deltaHill);
                } else {
                    ImGui::TextColored(Col::Green, "%.2f", pair.deltaHill);
                }

                ImGui::TableNextColumn();
                ImGui::Text("%.2f:1", pair.periodRatio);

                ImGui::TableNextColumn();
                if (pair.isOrbitCrossing) {
                    ImGui::TextColored(Col::Red, "CROSSING");
                } else if (pair.isHillUnstable) {
                    ImGui::TextColored(Col::Orange, "CHAOTIC");
                } else {
                    ImGui::TextColored(Col::Green, "STABLE");
                }
            }
            ImGui::EndTable();
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Habitability Estimator (Step 10)
    ImGui::TextColored(Col::Accent, "Planetary Habitability Estimator (PHL Model):");
    ImGui::Spacing();

    const auto& bodies = physics.getBodies();
    static int selectedPlanetIdx = 0;
    if (selectedPlanetIdx >= (int)bodies.size()) selectedPlanetIdx = 0;

    std::vector<const char*> bodyNames;
    for (const auto& b : bodies) bodyNames.push_back(b.name.c_str());

    if (!bodyNames.empty()) {
        ImGui::Combo("Select Target Body##HabBody", &selectedPlanetIdx, bodyNames.data(), (int)bodyNames.size());
        const auto& targetBody = bodies[selectedPlanetIdx];

        auto hab = aiManager.evaluateHabitability(targetBody);

        ImGui::Text("Habitability Score: ");
        ImGui::SameLine();
        ImVec4 habCol = (hab.score >= 70.0f) ? Col::Green : (hab.score >= 45.0f ? Col::Yellow : Col::Orange);
        ImGui::TextColored(habCol, "%.1f / 100", hab.score);

        ImGui::Text("Classification:     ");
        ImGui::SameLine();
        ImGui::TextColored(habCol, "%s", hab.classification.c_str());

        ImGui::PushStyleColor(ImGuiCol_PlotHistogram, habCol);
        ImGui::ProgressBar(hab.score / 100.0f, ImVec2(-1, 14));
        ImGui::PopStyleColor();

        ImGui::Text("  • Temp ESI:    %.2f  (T = %s)", hab.temperatureESI, targetBody.tempStr.c_str());
        ImGui::Text("  • Radius ESI:  %.2f  (R = %s)", hab.radiusESI, targetBody.radiusStr.c_str());
        ImGui::Text("  • Gravity ESI: %.2f  (g = %s)", hab.gravityESI, targetBody.gravityStr.c_str());
        ImGui::Text("  • Flux ESI:    %.2f  (F = %s)", hab.fluxESI, targetBody.solarRadiationStr.c_str());
        ImGui::TextWrapped("Diagnostic: %s", hab.diagnostic.c_str());
    }

    ImGui::EndChild();

    ImGui::SameLine();

    // ── COLUMN 3: INTERACTIVE WHAT-IF PERTURBATION LAB ──
    ImGui::BeginChild("##AICol3", ImVec2(colW, contentH - 50.0f), true);
    ImGui::TextColored(Col::Accent, "3. INTERACTIVE WHAT-IF PERTURBATION LAB");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::TextSecondary, "Hypothetically perturb parameters to test ML sensitivity:");
    ImGui::Spacing();

    static int perturbIdx = 0;
    static float whatIfEcc = 0.05f;
    static float whatIfSmaAU = 1.0f;
    static float whatIfMassM = 1.0f;
    static bool whatIfInitialized = false;

    if (!whatIfInitialized && !bodies.empty()) {
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].id != "sol" && bodies[i].type.find("Star") == std::string::npos) {
                perturbIdx = (int)i;
                whatIfEcc = (float)bodies[i].eccentricity;
                whatIfSmaAU = (float)((bodies[i].semiMajorAxisAU > 0.0) ? bodies[i].semiMajorAxisAU : bodies[i].distanceAU);
                whatIfMassM = (float)(bodies[i].massKg / UnitConverter::EARTH_MASS_KG);
                whatIfInitialized = true;
                break;
            }
        }
    }

    if (perturbIdx >= (int)bodies.size()) perturbIdx = 0;

    if (!bodyNames.empty()) {
        if (ImGui::Combo("Body to Perturb##WhatIfTarget", &perturbIdx, bodyNames.data(), (int)bodyNames.size())) {
            whatIfEcc = (float)bodies[perturbIdx].eccentricity;
            whatIfSmaAU = (float)((bodies[perturbIdx].semiMajorAxisAU > 0.0) ? bodies[perturbIdx].semiMajorAxisAU : bodies[perturbIdx].distanceAU);
            whatIfMassM = (float)(bodies[perturbIdx].massKg / UnitConverter::EARTH_MASS_KG);
        }

        ImGui::SliderFloat("Hypothetical Eccentricity##WhatIfEcc", &whatIfEcc, 0.0f, 0.85f, "e = %.3f");
        ImGui::DragFloat("Hypothetical Orbit (AU)##WhatIfSma", &whatIfSmaAU, 0.05f, 0.05f, 50.0f, "%.3f AU");
        ImGui::DragFloat("Hypothetical Mass (M⊕)##WhatIfMass", &whatIfMassM, 0.1f, 0.01f, 1000.0f, "%.2f M⊕");

        // Clone bodies and apply what-if perturbation
        std::vector<CelestialBody> whatIfBodies = bodies;
        if (perturbIdx >= 0 && perturbIdx < (int)whatIfBodies.size()) {
            whatIfBodies[perturbIdx].eccentricity = (double)whatIfEcc;
            whatIfBodies[perturbIdx].semiMajorAxisAU = (double)whatIfSmaAU;
            whatIfBodies[perturbIdx].semiMajorAxisM = (double)whatIfSmaAU * UnitConverter::AU_TO_METERS;
            whatIfBodies[perturbIdx].massKg = (double)whatIfMassM * UnitConverter::EARTH_MASS_KG;
        }

        ai::StabilityPrediction whatIfPred = aiManager.evaluateWhatIf(whatIfBodies);

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "What-If Model Prediction:");

        ImVec4 whatIfCol = (whatIfPred.prediction == "STABLE") ? Col::Green : (whatIfPred.prediction == "MARGINAL" ? Col::Yellow : Col::Red);
        ImGui::TextColored(whatIfCol, "Status: %s (Confidence: %s)", whatIfPred.prediction.c_str(), whatIfPred.confidence.c_str());
        ImGui::Text("Stable Prob:   %.1f%%", whatIfPred.stableProbability * 100.0f);
        ImGui::Text("Unstable Prob: %.1f%%", whatIfPred.unstableProbability * 100.0f);
        ImGui::TextColored(Col::TextSecondary, "Predicted Risk:");
        ImGui::TextWrapped("%s", whatIfPred.primaryRiskFactor.c_str());

        ImGui::Spacing();
        if (ImGui::Button("⚡ Apply Perturbation to Live Physics", ImVec2(-1, 26))) {
            if (perturbIdx >= 0 && perturbIdx < (int)physics.getBodies().size()) {
                auto& mut = physics.getBodies()[perturbIdx];
                mut.eccentricity = (double)whatIfEcc;
                mut.semiMajorAxisAU = (double)whatIfSmaAU;
                mut.semiMajorAxisM = (double)whatIfSmaAU * UnitConverter::AU_TO_METERS;
                mut.massKg = (double)whatIfMassM * UnitConverter::EARTH_MASS_KG;
                aiManager.forceRecompute(physics);
            }
        }

        if (ImGui::Button("🔄 Reset Sliders to Live Values", ImVec2(-1, 24))) {
            whatIfEcc = (float)bodies[perturbIdx].eccentricity;
            whatIfSmaAU = (float)((bodies[perturbIdx].semiMajorAxisAU > 0.0) ? bodies[perturbIdx].semiMajorAxisAU : bodies[perturbIdx].distanceAU);
            whatIfMassM = (float)(bodies[perturbIdx].massKg / UnitConverter::EARTH_MASS_KG);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (ImGui::Button("🚀 Return to UNIVERSE Simulation", ImVec2(-1, 30))) {
        m_activeTopTab = 0;
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace AstroGenesis

