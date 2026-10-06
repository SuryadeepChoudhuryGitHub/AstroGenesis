#include "ui/UIManager.hpp"
#include "ui/IconSystem.hpp"
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
    static ImVec4 AccentCyan   {0.000f, 0.850f, 1.000f, 1.00f};
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

static void StatItem(IconId icon, const char* label, const char* value) {
    ImGui::BeginGroup();
    UIIcon::Icon(icon, IconSize::Small, UICol::Accent);
    ImGui::SameLine(0, 5);
    ImGui::TextColored(Col::TextSecondary, "%s", label);
    ImGui::Indent(19.0f);
    ImGui::TextColored(Col::TextPrimary, "%s", (value && *value) ? value : "-");
    ImGui::Unindent(19.0f);
    ImGui::EndGroup();
}

static void StatItem(const char* icon, const char* label, const char* value) {
    ImGui::BeginGroup();
    if (icon && *icon) {
        ImGui::TextColored(Col::Accent, "%s", icon);
        ImGui::SameLine(0, 5);
    }
    ImGui::TextColored(Col::TextSecondary, "%s", label);
    ImGui::Indent(19.0f);
    ImGui::TextColored(Col::TextPrimary, "%s", (value && *value) ? value : "-");
    ImGui::Unindent(19.0f);
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
    float topBarH    = 42.0f;
    float statusBarH = 26.0f;

    // 1. Top Bar (Global Navigation & Workspace Switcher)
    drawTopBar(windowWidth, physics, camera, objRepo, visualAdapter);

    // 2. Route Top-Level Workspaces
    if (m_activeTopTab == 0) {
        // ── UNIVERSE WORKSPACE (Primary Live Simulation & Scientific Observatory) ────
        float leftW   = m_universeLeftCollapsed ? 0.0f : 210.0f;
        int selIdx    = physics.getSelectedBodyIndex();
        float rightW  = (m_universeRightCollapsed || selIdx < 0 || selIdx >= (int)physics.getBodies().size()) ? 0.0f : 310.0f;
        float bottomH = m_universeBottomCollapsed ? 0.0f : 180.0f;

        m_viewportX = leftW;
        m_viewportY = topBarH;
        m_viewportW = windowWidth - leftW - rightW;
        m_viewportH = windowHeight - topBarH - bottomH - statusBarH;

        m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                             mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH) && !io.WantCaptureMouse;

        // 1. Left Hierarchy & System List Panel
        drawLeftPanel(physics, camera, objRepo, topBarH, statusBarH, windowHeight);

        // 2. Central 3D Viewport HUD
        drawViewportHUD(physics, camera, visualAdapter, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // 3. Bottom Panels (Time Controls, Physics Metrics, 2D Orbit Radar)
        if (!m_universeBottomCollapsed) {
            float bottomY = windowHeight - statusBarH - bottomH;
            float bpW = m_viewportW / 3.0f;
            drawTimeControls(physics, camera, objRepo, leftW, bottomY, bpW, bottomH);
            drawSimMetrics  (physics, fps, leftW + bpW,      bottomY, bpW, bottomH);
            drawOrbitVis    (physics, camera, leftW + bpW * 2, bottomY, bpW, bottomH);
        } else {
            float btnW = 190.0f;
            ImGui::SetNextWindowPos(ImVec2(m_viewportX + (m_viewportW - btnW) * 0.5f, windowHeight - statusBarH - 34.0f));
            ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.050f, 0.090f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Border, Col::BorderLight);
            ImGui::Begin("##ExpandBottomPanelWin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
            if (ImGui::Button("▲ SIMULATION CONTROLS", ImVec2(btnW - 16.0f, 22))) {
                m_universeBottomCollapsed = false;
            }
            if (ImGui::IsItemHovered()) {
                ImGui::SetTooltip("Expand Time Controls, Physics Metrics & Orbit Radar");
            }
            ImGui::End();
            ImGui::PopStyleColor(2);
        }

        // 4. Right Scientific Information Panel & Info Overlay
        if (selIdx >= 0 && selIdx < (int)physics.getBodies().size()) {
            CelestialBody& currentBody = physics.getBodies()[selIdx];
            drawInfoOverlay(currentBody, m_viewportX, m_viewportY);
            drawRightPanel(physics, camera, currentBody, dataManager, objRepo, visualAdapter, aiManager, topBarH, windowWidth, windowHeight, statusBarH);
        }

        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 1) {
        // ── EDIT WORKSPACE (Interactive Simulation-First Sandbox & Studio) ───────
        float hierW = m_showHierarchy ? 260.0f : 0.0f;

        m_viewportX = hierW;
        m_viewportY = topBarH;
        m_viewportW = windowWidth - hierW;
        m_viewportH = windowHeight - topBarH - statusBarH;

        m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                             mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH) && !io.WantCaptureMouse;

        // Interactive 3D Move Gizmo (when Move tool is active or dragging body - updates hover and drag state first)
        drawInteractiveGizmo(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // Viewport HUD (selection, hover targeting, reticle)
        drawViewportHUD(physics, camera, visualAdapter, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // Placement Guide (when adding a new celestial body)
        drawPlacementGuide(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // Collapsible System Hierarchy Drawer (Left)
        drawCollapsibleHierarchy(physics, camera, objRepo, topBarH, statusBarH, windowHeight);

        // Floating Simulation Controls Bar (Bottom Center)
        drawFloatingSimBar(physics, camera, objRepo, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

        // Interactive Celestial Object Property Studio (Right side of EDIT workspace)
        int selIdx = physics.getSelectedBodyIndex();
        if (selIdx >= 0 && selIdx < (int)physics.getBodies().size()) {
            if (m_editPropCollapsed) {
                drawCompactContextCard(physics, camera, objRepo, m_viewportX, m_viewportY, m_viewportW, m_viewportH);
            } else {
                drawEditPropertiesPanel(physics, camera, objRepo, visualAdapter, aiManager, dataManager, m_viewportX, m_viewportY, m_viewportW, m_viewportH);
            }
        }

        // Detailed Object Deep-Dive Inspector (Slide-out panel on demand)
        if (m_showDetailsModal) {
            drawDetailsInspector(physics, dataManager, objRepo, visualAdapter, aiManager, topBarH, windowWidth, windowHeight, statusBarH);
        }

        // Add Object Palette Modal
        if (m_showAddPalette) {
            drawAddObjectPalette(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);
        }

        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    }
    else if (m_activeTopTab == 2) {
        // ── SYSTEM WORKSPACE (Import Existing & Custom System Builder) ─────────
        m_systemWorkspaceUI.render(dataManager, objRepo, physics, camera, m_activeTopTab, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 3) {
        // ── OBJECTS WORKSPACE (Celestial Object Library & Editor) ──────────────
        m_objectWorkspaceUI.render(objRepo, physics, camera, m_activeTopTab, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 4) {
        // ── EXPLORE WORKSPACE (Discovery & Catalog Browser) ────────────────────
        drawExploreWorkspace(objRepo, physics, camera, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 5) {
        // ── SIMULATION WORKSPACE (Physics Config, Integrator, Validation) ──────
        drawSimulationWorkspace(physics, camera, valEngine, objRepo, visualAdapter, windowWidth, windowHeight);
        drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
    } 
    else if (m_activeTopTab == 6) {
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
        m_dataManagerUI.render(m_showDataManager, dataManager, objRepo, physics, windowWidth, windowHeight, &camera, &m_activeTopTab);
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
    UIIcon::Icon(IconId::LogoCompact, IconSize::Medium, UICol::Accent);
    ImGui::SameLine(0, 8);
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("ASTROGENESIS");
    ImGui::PopStyleColor();
    if (width >= 1200.0f) {
        ImGui::SameLine(0, 8);
        ImGui::TextColored(Col::TextSecondary, "SPACE SIMULATION ENGINE");
    }
    ImGui::SameLine(0, 18);

    // Top Navigation Tabs
    const char* tabs[] = { "UNIVERSE", "EDIT", "SYSTEM", "OBJECTS", "EXPLORE", "SIMULATION", "AI ASSISTANT" };
    for (int i = 0; i < 7; ++i) {
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

    // Top Bar Action Buttons: Responsive layout (Compact icon buttons on narrow displays, labeled on wide)
    bool isCompact = (width < 1650.0f);
    float totalBtnsW = isCompact ? (7 * 32.0f + 6 * 5.0f) : (710.0f);
    float minRightOffset = ImGui::GetCursorPosX() + 16.0f;
    float rightOffset = width - totalBtnsW - 16.0f;
    if (rightOffset < minRightOffset) {
        rightOffset = minRightOffset;
    }
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
    if (UIIcon::Button("##TopBarCine", IconId::Camera, isCompact ? nullptr : (cineOn ? "CINE: ON" : "CINE: OFF"), ImVec2(isCompact ? 32 : 110, 28))) {
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
    if (UIIcon::Button("##TopBarPhoto", IconId::Camera, isCompact ? nullptr : "PHOTO", ImVec2(isCompact ? 32 : 90, 28))) {
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
    if (UIIcon::Button("##TopBarReset", IconId::Reset, isCompact ? nullptr : "RESET", ImVec2(isCompact ? 32 : 85, 28))) {
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
    if (UIIcon::Button("##TopBarData", IconId::Database, isCompact ? nullptr : "DATA", ImVec2(isCompact ? 32 : 85, 28))) {
        m_showDataManager = !m_showDataManager;
        if (m_showDataManager) {
            m_dataManagerUI.openDatabaseExplorer();
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open Astronomical Data Manager (JPL Horizons, SBDB, Exoplanet Archive)");
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
    if (UIIcon::Button("##TopBarValidate", IconId::Validation, isCompact ? nullptr : "VALIDATE", ImVec2(isCompact ? 32 : 105, 28))) {
        m_showValidationDashboard = !m_showValidationDashboard;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open Scientific Orbital Integration Validation Dashboard (NASA Ground Truth)");
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
    if (UIIcon::Button("##TopBarAsteroids", IconId::Asteroid, isCompact ? nullptr : "ASTEROIDS", ImVec2(isCompact ? 32 : 110, 28))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open Asteroid Belt Population & Kirkwood Gaps Resonances Monitor");
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
    if (UIIcon::Button("##TopBarMatter", IconId::Physics, isCompact ? nullptr : "MATTER", ImVec2(isCompact ? 32 : 95, 28))) {
        m_showMatterLab = !m_showMatterLab;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open Continuum Mechanics Deformable Matter Laboratory");
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

    if (ImGui::Begin("PHOTO MODE STUDIO##PhotoModeWin", nullptr, ImGuiWindowFlags_NoCollapse)) {
        // Header & Exit
        UIIcon::Icon(IconId::Camera, IconSize::Small, UICol::Accent);
        ImGui::SameLine(0, 6);
        ImGui::TextColored(ImVec4(0.85f, 0.65f, 1.0f, 1.0f), "SPACE PHOTO MODE");
        ImGui::SameLine(toolbarW - 100.0f);
        if (UIIcon::Button(IconId::Close, "Exit (Esc)", ImVec2(90, 22))) {
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
        if (ImGui::SliderFloat("FOV (Lens)##PhotoFOV", &fov, 15.0f, 105.0f, "%.1f deg")) {
            camera.setFOV(fov);
        }
        // FOV quick presets
        if (ImGui::SmallButton("24mm (42 deg)##Fov24")) { camera.setFOV(42.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("50mm (28 deg)##Fov50")) { camera.setFOV(28.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("85mm (18 deg)##Fov85")) { camera.setFOV(18.0f); }
        ImGui::SameLine();
        if (ImGui::SmallButton("Wide (65 deg)##FovWide")) { camera.setFOV(65.0f); }

        // Camera Roll Slider (radians -> degrees)
        float rollDeg = glm::degrees(camera.getRoll());
        if (ImGui::SliderFloat("Roll (Tilt)##PhotoRoll", &rollDeg, -180.0f, 180.0f, "%.1f deg")) {
            camera.setRoll(glm::radians(rollDeg));
        }
        ImGui::TextDisabled("Hold [Q] / [E] to roll camera smoothly");
        if (UIIcon::Button(IconId::Reset, "Reset Roll (0 deg)##ResetRollBtn", ImVec2(140, 22))) {
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

            if (UIIcon::Button(IconId::Focus, "Auto-Focus on Target##FocusTgt", ImVec2(220, 24))) {
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
        if (UIIcon::Button(paused ? IconId::Play : IconId::Pause, paused ? "Resume Motion##PhotoPlay" : "Freeze Motion##PhotoPause", ImVec2(150, 26))) {
            physics.togglePause();
        }
        ImGui::SameLine();
        if (UIIcon::Button(IconId::Camera, "HIDE UI (F12)##HideUIBtn", ImVec2(140, 26))) {
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

// ------------------------------------------------------------------------------------------------
// UNIVERSE WORKSPACE LAYOUT METHODS (Restored Observatory)
// ------------------------------------------------------------------------------------------------

void UIManager::drawLeftPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH) {
    if (m_universeLeftCollapsed) {
        ImGui::SetNextWindowPos(ImVec2(8, topBarH + 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.050f, 0.090f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Border, Col::BorderLight);
        ImGui::Begin("##ExpandLeftPanelWin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
        if (UIIcon::Button(IconId::ChevronRight, "OBJECTS", ImVec2(95, 24))) {
            m_universeLeftCollapsed = false;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Expand Celestial Hierarchy Panel (Hotkey: H)");
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        return;
    }

    float panelW = 210.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##LeftPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Search Bar & Collapse Button
    ImGui::PushItemWidth(panelW - 48);
    ImGui::InputTextWithHint("##search", "Search Anything...", m_searchQuery, sizeof(m_searchQuery));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    if (UIIcon::SmallButton(IconId::ChevronLeft, "##CollapseLeft")) {
        m_universeLeftCollapsed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Collapse Hierarchy Panel (Hotkey: H)");
    }
    ImGui::Separator();

    // Active System Hierarchy (Place 1: Selection from Left List)
    const std::string curCat = physics.getCurrentCategory();
    std::string headerLabel = curCat.empty() ? "SOLAR SYSTEM" : curCat;
    std::transform(headerLabel.begin(), headerLabel.end(), headerLabel.begin(), ::toupper);

    ImGui::TextColored(Col::Accent, "%s", headerLabel.c_str());
    ImGui::SameLine(panelW - 68.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.14f, 0.14f, 0.75f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.85f, 1.0f));
    if (UIIcon::SmallButton(IconId::Reset, "Reset")) {
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
                    camera.resetOverview(glm::vec3(0.0f), 6.0f);
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
    bool isStar = (body.id == "sol" || body.type.find("Star") != std::string::npos);
    ImGui::SameLine();
    UIIcon::Icon(isStar ? IconId::Star : IconId::Orbit, IconSize::Small, isStar ? UICol::Warning : UICol::Accent);
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

void UIManager::drawRightPanel(PhysicsEngine& physics, Camera& camera, CelestialBody& body, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, float topBarH, float winW, float winH, float statusBarH) {
    if (m_universeRightCollapsed) {
        ImGui::SetNextWindowPos(ImVec2(winW - 135, topBarH + 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.050f, 0.090f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Border, Col::BorderLight);
        ImGui::Begin("##ExpandRightPanelWin", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);
        if (UIIcon::Button(IconId::ChevronLeft, "SCIENTIFIC", ImVec2(115, 24))) {
            m_universeRightCollapsed = false;
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Expand Scientific Inspector Panel");
        }
        ImGui::End();
        ImGui::PopStyleColor(2);
        return;
    }

    float panelW = 310.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(winW - panelW, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##RightPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Panel Header with Collapse Button
    ImGui::BeginGroup();
    ImGui::TextColored(Col::Accent, "SCIENTIFIC INSPECTOR");
    ImGui::SameLine(panelW - 32.0f);
    if (UIIcon::SmallButton(IconId::ChevronRight, "##CollapseRight")) {
        m_universeRightCollapsed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Collapse Scientific Panel");
    }
    ImGui::EndGroup();

    // Call to action button to jump straight into EDIT studio
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.45f, 0.65f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.00f, 0.60f, 0.85f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    if (UIIcon::Button(IconId::Edit, "Edit Body in EDIT Studio", ImVec2(panelW - 20, 26))) {
        m_activeTopTab = 1; // Switch to EDIT workspace
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Open in EDIT Studio with interactive 3D translation gizmo and sandbox tools");
    }
    ImGui::PopStyleColor(3);

    // Quick Action Bar: Focus & Reset
    if (UIIcon::Button(IconId::Focus, "Focus Camera", ImVec2((panelW - 28) / 2.0f, 22))) {
        camera.focusOnBody(body.position, body.radius3D, 0.85f);
        addEventLog("Focused camera on " + body.name);
    }
    ImGui::SameLine();
    if (UIIcon::Button(IconId::Reset, "Ephemeris", ImVec2((panelW - 28) / 2.0f, 22))) {
        int selIdx = physics.getSelectedBodyIndex();
        bool resetSuccess = false;
        if (selIdx >= 0 && selIdx < (int)physics.getBodies().size()) {
            auto& curBody = physics.getBodies()[selIdx];
            auto hydrated = objRepo.getHydratedBody(curBody.dbId);
            if (!hydrated.has_value()) {
                hydrated = objRepo.getHydratedBodyBySlug(curBody.id);
            }
            if (hydrated.has_value()) {
                curBody.positionM = hydrated->positionM;
                curBody.velocityMps = hydrated->velocityMps;
                curBody.position = hydrated->positionM / UnitConverter::AU_TO_METERS;
                curBody.velocity = hydrated->velocityMps / UnitConverter::AU_TO_METERS;
                curBody.trailHistory.clear();
                curBody.trailHistory.push_back(curBody.position);
                curBody.semiMajorAxisM = hydrated->semiMajorAxisM;
                curBody.semiMajorAxisAU = hydrated->semiMajorAxisAU;
                curBody.eccentricity = hydrated->eccentricity;
                curBody.trueAnomalyDeg = hydrated->trueAnomalyDeg;
                curBody.epochJd = hydrated->epochJd;
                curBody.orbitalPeriodDays = hydrated->orbitalPeriodDays;
                physics.updateBodyScales();
                resetSuccess = true;
                addEventLog("Reset " + curBody.name + " to official database ephemeris");
            }
        }
        if (!resetSuccess) {
            addEventLog("No official ephemeris found for " + body.name);
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset orbital parameters to official database ephemeris");
    }
    ImGui::Separator();

    // ── VISUAL & PHYSICAL STATE INSPECTOR ──────────────────────────────────────
    if (SectionHeader("VISUAL & PHYSICAL STATE")) {
        const VisualBodyState* vBody = visualAdapter.getVisualBody(body.id);
        if (!vBody) vBody = visualAdapter.getVisualBody(body.dbId);

        float halfW = (panelW - 40) / 2.0f;
        char rBuf[32];
        if (vBody) snprintf(rBuf, sizeof(rBuf), "%.4f AU", vBody->renderRadius);
        else snprintf(rBuf, sizeof(rBuf), "%.4f AU", body.radius3D);

        ImGui::BeginGroup();
        StatItem(IconId::Ruler, "Render Scale", rBuf);
        ImGui::SameLine(halfW);
        StatItem(IconId::Ruler, "Physical Radius", body.radiusStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Heat, "Blackbody Temp", body.tempStr.c_str());
        ImGui::SameLine(halfW);
        std::string phaseStr = "Solid Rock/Ice";
        if (vBody) {
            if (vBody->phase == MaterialPhase::VaporGas) phaseStr = vBody->isStar ? "Stellar Plasma" : "Vapor / Gas";
            else if (vBody->phase == MaterialPhase::LiquidMolten) phaseStr = "Molten Magma";
            else if (vBody->phase == MaterialPhase::SoftenedPlastic) phaseStr = "Softened Plastic";
            else if (vBody->phase == MaterialPhase::Solid) phaseStr = "Solid Rock/Ice";
        }
        StatItem(IconId::Physics, "Phase", phaseStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Atmosphere, "Atmosphere", (vBody && vBody->hasAtmosphere) ? "Scattering Active" : "None/Thin");
        ImGui::SameLine(halfW);
        StatItem(IconId::Atmosphere, "Cloud Cover", (vBody && vBody->hasClouds) ? "Dynamic Clouds" : "Clear");
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Orbit, "Rotation Speed", body.rotationPeriodStr.c_str());
        ImGui::SameLine(halfW);
        StatItem(IconId::Ruler, "Axial Tilt", body.axialTiltStr.c_str());
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
        StatItem(IconId::Physics, "Gravity", body.gravityStr.c_str());
        ImGui::SameLine(halfW);
        StatItem(IconId::Speed, "Escape Velocity", body.escapeVelocityStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Heat, "Surface Temp.", body.tempStr.c_str());
        ImGui::SameLine(halfW);
        StatItem(IconId::Atmosphere, "Atm. Pressure", body.pressureStr.c_str());
        ImGui::EndGroup();

        char hBuf[32], tauBuf[32];
        snprintf(hBuf, sizeof(hBuf), "%.1f km", body.scaleHeightKm);
        snprintf(tauBuf, sizeof(tauBuf), "%.2f (+%.0f K)", body.opticalDepth, body.greenhouseK);
        ImGui::BeginGroup();
        StatItem(IconId::Ruler, "Scale Height", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? hBuf : "N/A");
        ImGui::SameLine(halfW);
        StatItem(IconId::Atmosphere, "Optical Depth", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? tauBuf : "0.00 (+0 K)");
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Physics, "Mean Density", body.densityStr.c_str());
        ImGui::SameLine(halfW);
        StatItem(IconId::Time, "Day Length", body.rotationPeriodStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Orbit, "Year Length", body.yearLengthStr.c_str());
        ImGui::SameLine(halfW);
        StatItem(IconId::Ruler, "Surface Area", body.surfaceAreaStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("ORBITAL MECHANICS & KEPLERIAN ELEMENTS")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem(IconId::Orbit, "Semi-Major Axis", body.semiMajorAxisStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Orbit, "Eccentricity", body.eccentricityStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Target, "Perihelion", body.periapsisStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Target, "Aphelion", body.apoapsisStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Physics, "Ang. Momentum", body.angularMomentumStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Energy, "Orbital Energy", body.orbitalEnergyStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Time, "GR Precession", body.grPrecessionStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Ruler, "True Anomaly", body.trueAnomalyStr.c_str());
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
        StatItem(IconId::Orbit, "Bodies", std::to_string(pred.analyzedBodyCount).c_str());
        ImGui::SameLine(hw);
        char sepBuf[32];
        snprintf(sepBuf, sizeof(sepBuf), "%.2f R_H", pred.minMutualHillSep);
        StatItem(IconId::Ruler, "Min Sep", sepBuf);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::TextColored(Col::TextSecondary, "Primary Risk Factor:");
        ImGui::TextWrapped("%s", pred.primaryRiskFactor.c_str());

        ImGui::Spacing();
        UIIcon::Icon(IconId::Info, 13.0f, Col::TextSecondary);
        ImGui::SameLine();
        ImGui::TextDisabled("ML-based estimate of orbital stability.");

        if (UIIcon::Button(IconId::AI, "Open AI Analysis Studio", ImVec2(panelW - 20, 24))) {
            m_activeTopTab = 6;
        }
    }

    if (body.ring.hasRing) {
        ImGui::Separator();
        if (SectionHeader("PLANETARY RING ASTROPHYSICS & SHEAR")) {
            float hw = (panelW - 40) / 2.0f;
            
            ImGui::BeginGroup();
            StatItem(IconId::Speed, "Inner Speed", "23.1 km/s (5.6h)");
            ImGui::SameLine(hw);
            StatItem(IconId::Speed, "Outer Speed", "16.8 km/s (14.9h)");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Physics, "Local Gravity", "6.84 -> 1.93 m/s^2");
            ImGui::SameLine(hw);
            StatItem(IconId::Speed, "Escape Velocity", "32.7 -> 23.8 km/s");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Heat, "Ring Temp.", "85 K (-188 deg C)");
            ImGui::SameLine(hw);
            StatItem(IconId::Time, "Rel. Drift", "-1.35 x 10^-8");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Scale, "Total Ring Mass", "1.50 x 10^19 kg");
            ImGui::SameLine(hw);
            char actBuf[32];
            snprintf(actBuf, sizeof(actBuf), "%zu Active", body.ring.disturbances.size());
            StatItem(IconId::Phase, "Fluid State", body.ring.disturbances.empty() ? "Equilibrium" : actBuf);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.65f, 0.9f));
            if (UIIcon::Button(IconId::Asteroid, "Trigger Asteroid Ring Impact", ImVec2(panelW - 20, 24))) {
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
        StatItem(IconId::Sun, "Solar Radiation", body.solarRadiationStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Warning, "Radiation Level", body.radLevelStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Time, "Rel. Drift", body.timeDilationStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Speed, "Orbital Velocity", body.orbitalSpeedStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Energy, "Magnetic Field", body.magneticFieldStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Sparkles, "Aurora Activity", body.auroraActivityStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("DATA SOURCE & VERIFICATION")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem(IconId::Database, "Authority", body.sourceName.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Target, "Target ID", body.sourceObjectId.empty() ? body.id.c_str() : body.sourceObjectId.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Target, "Ref Frame", body.referenceFrame.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Time, "Epoch", body.epochUtcStr.c_str());
        ImGui::EndGroup();

        ImGui::Spacing();
        if (UIIcon::Button(IconId::Database, "Open Data Manager", ImVec2(panelW - 20, 24))) {
            m_showDataManager = true;
            m_dataManagerUI.selectObjectById(body.dbId, body.category);
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

void UIManager::drawTimeControls(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##TimeControls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "TIME CONTROLS");
    ImGui::SameLine(w - 28.0f);
    if (UIIcon::SmallButton(IconId::ChevronDown, "##CollapseBottom")) {
        m_universeBottomCollapsed = true;
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Collapse Bottom Simulation Panels");
    }
    ImGui::Separator();

    bool isPaused = physics.isPaused();
    if (UIIcon::Button(IconId::StepBackward, "##stepback", ImVec2(28, 24))) { physics.stepFrameBackward(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Step Backward (Frame)"); }
    ImGui::SameLine();
    if (UIIcon::Button(isPaused ? IconId::Play : IconId::Pause, "##playpause", ImVec2(28, 24))) { physics.togglePause(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip(isPaused ? "Play (Space)" : "Pause (Space)"); }
    ImGui::SameLine();
    if (UIIcon::Button(IconId::StepForward, "##stepfwd", ImVec2(28, 24))) { physics.stepFrameForward(); }
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
    if (UIIcon::Button(IconId::Reset, "Reset", ImVec2(66, 24))) {
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

void UIManager::drawCollapsibleHierarchy(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH) {
    if (!m_showHierarchy) return;

    float panelW = 260.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.04f, 0.075f, 0.94f));

    ImGui::Begin("##HierarchyPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Header with Collapse Button
    UIIcon::Icon(IconId::Hierarchy, 16.0f, Col::Accent);
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Col::Accent, "SYSTEM HIERARCHY");
    ImGui::SameLine(panelW - 32.0f);
    if (UIIcon::Button(IconId::Close, "##CloseHier", ImVec2(22, 20))) {
        m_showHierarchy = false;
    }
    if (ImGui::IsItemHovered()) ImGui::SetTooltip("Close Hierarchy Drawer (Hotkey: H)");

    // Search Bar
    ImGui::PushItemWidth(-1);
    ImGui::InputTextWithHint("##hierSearch", "Filter bodies...", m_searchQuery, sizeof(m_searchQuery));
    ImGui::PopItemWidth();
    ImGui::Separator();

    // Active System Hierarchy Header & Reset button
    const std::string curCat = physics.getCurrentCategory();
    std::string headerLabel = curCat.empty() ? "SOLAR SYSTEM" : curCat;
    std::transform(headerLabel.begin(), headerLabel.end(), headerLabel.begin(), ::toupper);

    ImGui::TextColored(Col::AccentCyan, "%s", headerLabel.c_str());
    ImGui::SameLine(panelW - 74.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.12f, 0.12f, 0.70f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.85f, 0.85f, 1.0f));
    if (UIIcon::Button(IconId::Reset, "Reset", ImVec2(66, 20))) {
        physics.resetSimulation(objRepo);
        camera.resetOverview(glm::vec3(0.0f), 6.0f);
        addEventLog("Simulation workspace reset to fresh start");
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Reset workspace to clean start (Hotkey: R)");
    }
    ImGui::PopStyleColor(2);

    // List celestial bodies
    ImGui::BeginChild("##HierBodyList", ImVec2(0, panelH - 165), false);
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

        std::string selectableId = "##hierBody" + std::to_string(i);
        if (ImGui::Selectable(selectableId.c_str(), isSelected, 0, ImVec2(0, 36))) {
            physics.selectBody(i);
            camera.focusOnBody(bodies[i].position, bodies[i].radius3D, 0.85f);
            addEventLog(bodies[i].name + " selected");
        }

        ImVec2 p = ImGui::GetItemRectMin();
        ImGui::SetCursorScreenPos(ImVec2(p.x + 30, p.y + 2));
        ImGui::Text("%s", bodies[i].name.c_str());
        ImGui::SetCursorScreenPos(ImVec2(p.x + 30, p.y + 18));
        ImGui::TextColored(Col::TextSecondary, "%s", bodies[i].distanceStr.c_str());

        ImGui::GetWindowDrawList()->AddCircleFilled(
            ImVec2(p.x + 14, p.y + 18), 7.0f,
            isSelected ? ImGui::ColorConvertFloat4ToU32(Col::Accent)
                       : ImGui::ColorConvertFloat4ToU32(ImVec4(bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 0.85f)));

        if (isSelected) ImGui::PopStyleColor(2);
    }
    ImGui::EndChild();

    ImGui::Separator();
    // Quick Hierarchy Actions
    if (UIIcon::Button(IconId::Add, "Add Object", ImVec2((panelW - 28) * 0.5f, 24))) {
        m_showAddPalette = true;
    }
    ImGui::SameLine();
    bool hasSel = (selectedIndex >= 0 && selectedIndex < (int)bodies.size());
    if (!hasSel) ImGui::BeginDisabled();
    if (UIIcon::Button(IconId::Delete, "Delete", ImVec2((panelW - 28) * 0.5f, 24))) {
        deleteSelectedObject(physics);
    }
    if (!hasSel) ImGui::EndDisabled();

    // Database System Quick-Switch at Bottom
    ImGui::Separator();
    auto categories = objRepo.getAvailableCategories();
    for (const auto& cat : categories) {
        if (cat == curCat) continue;
        std::string upperCat = cat;
        std::transform(upperCat.begin(), upperCat.end(), upperCat.begin(), ::toupper);
        if (ImGui::Selectable(("Switch: " + upperCat).c_str())) {
            physics.loadFromDatabase(objRepo, cat);
            camera.resetOverview(glm::vec3(0.0f), 6.0f);
            addEventLog("Switched system to " + cat);
        }
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void UIManager::deleteSelectedObject(PhysicsEngine& physics) {
    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) return;
    const CelestialBody& body = physics.getBodies()[selIdx];
    std::string name = body.name;
    m_undoRedo.recordDeleteBody(selIdx, body);
    physics.removeBody(selIdx);
    addEventLog("Deleted body: " + name);
}

void UIManager::duplicateSelectedObject(PhysicsEngine& physics) {
    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) return;
    int newIdx = physics.duplicateBody(selIdx, glm::vec3(0.12f, 0.0f, 0.08f));
    if (newIdx >= 0) {
        m_undoRedo.recordAddBody(newIdx, physics.getBodies()[newIdx]);
        addEventLog("Duplicated body: " + physics.getBodies()[newIdx].name);
    }
}

void UIManager::undo(PhysicsEngine& physics) {
    if (m_undoRedo.canUndo()) {
        std::string act = m_undoRedo.getLastActionName();
        m_undoRedo.undo(physics);
        addEventLog("Undo: " + act);
    }
}

void UIManager::redo(PhysicsEngine& physics) {
    if (m_undoRedo.canRedo()) {
        m_undoRedo.redo(physics);
        addEventLog("Redo");
    }
}

void UIManager::drawCompactContextCard(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float vpX, float vpY, float vpW, float vpH) {
    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) return;
    CelestialBody& body = physics.getBodies()[selIdx];

    float cardW = 320.0f;
    float cardX = vpX + vpW - cardW - 14.0f;
    float cardY = vpY + 14.0f;

    ImGui::SetNextWindowPos(ImVec2(cardX, cardY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(cardW, 0), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.05f, 0.09f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.65f, 0.90f, 0.45f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    std::string cardTitle = body.name + "##ContextCard";
    if (ImGui::Begin(cardTitle.c_str(), nullptr, flags)) {
        // Top row: Type badge, Full Studio button, Focus, Close
        ImGui::TextColored(Col::AccentCyan, "%s", body.type.c_str());
        float headerBtnsW = 55.0f + 65.0f + 30.0f;
        float rightBtnX = cardW - headerBtnsW - 16.0f;
        if (rightBtnX > ImGui::GetCursorPosX() + 8.0f) {
            ImGui::SameLine(rightBtnX);
        } else {
            ImGui::SameLine();
        }
        if (ImGui::SmallButton("+ Full")) {
            m_editPropCollapsed = false;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Expand to full Property Studio Inspector");

        ImGui::SameLine();
        if (UIIcon::SmallButton(IconId::Focus, "Focus")) {
            camera.focusOnBody(body.position, body.radius3D, 0.85f);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Center and track camera on this body (Hotkey: F)");

        ImGui::SameLine();
        if (UIIcon::SmallButton(IconId::Close, "##CloseCtxCard")) {
            physics.selectBody(-1);
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            return;
        }

        ImGui::Separator();

        // Key stats grid (2 columns, auto-balanced, zero collision)
        if (ImGui::BeginTable("##CtxCardGrid", 2, ImGuiTableFlags_SizingStretchSame)) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Col::TextSecondary, "Dist:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.distanceStr.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(Col::TextSecondary, "Vel:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.orbitalSpeedStr.c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Col::TextSecondary, "Mass:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.massStr.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(Col::TextSecondary, "Rad:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.radiusStr.c_str());

            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Col::TextSecondary, "Temp:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.tempStr.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(Col::TextSecondary, "Grav:");
            ImGui::SameLine(0, 5);
            ImGui::TextColored(Col::TextPrimary, "%s", body.gravityStr.c_str());

            ImGui::EndTable();
        }

        ImGui::Separator();

        // Direct sandbox tool buttons (dynamically sized to fit available width)
        float availW = ImGui::GetContentRegionAvail().x;
        float halfToolW = (availW - ImGui::GetStyle().ItemSpacing.x) * 0.5f;

        bool isMoving = (m_activeTool == SandboxTool::Move);
        if (isMoving) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.55f, 0.80f, 0.90f));
        }
        if (UIIcon::Button(IconId::Move, isMoving ? "Gizmo ON" : "Move (M)", ImVec2(halfToolW, 24))) {
            toggleMoveTool();
        }
        if (isMoving) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle 3D visual translation gizmo (Hotkey: M / G)");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Orbit, "Circularize", ImVec2(halfToolW, 24))) {
            int parentIdx = (selIdx == 0) ? -1 : 0;
            physics.calculateOrbitalVelocity(selIdx, parentIdx);
            addEventLog("Circularized orbit for " + body.name);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Recalculate velocity for stable circular Keplerian orbit");

        // Action row: Duplicate, Delete, Deep-dive (fit exact available width)
        float actBtnW1 = 76.0f;
        float actBtnW2 = 68.0f;
        float actBtnW3 = std::max(80.0f, availW - actBtnW1 - actBtnW2 - ImGui::GetStyle().ItemSpacing.x * 2.0f);

        if (UIIcon::Button(IconId::Copy, "Clone", ImVec2(actBtnW1, 22))) {
            duplicateSelectedObject(physics);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate body (Ctrl+D)");

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.15f, 0.15f, 0.8f));
        if (UIIcon::Button(IconId::Delete, "Del", ImVec2(actBtnW2, 22))) {
            deleteSelectedObject(physics);
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete body (Delete/Backspace)");

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.40f, 0.65f, 0.90f));
        if (UIIcon::Button(IconId::Edit, "Studio", ImVec2(actBtnW3, 22))) {
            m_editPropCollapsed = false;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Expand full Property Studio Inspector");
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::drawEditPropertiesPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, DataManager& dataManager, float vpX, float vpY, float vpW, float vpH) {
    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) return;
    CelestialBody& mutBody = physics.getBodies()[selIdx];
    const CelestialBody& body = mutBody;

    float panelW = 380.0f;
    float panelH = std::min(720.0f, vpH - 24.0f);
    float panelX = vpX + vpW - panelW - 14.0f;
    float panelY = vpY + 12.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH), ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.04f, 0.08f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.65f, 0.90f, 0.50f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;
    std::string winTitle = body.name + " — Property Studio##EditStudioPanel";
    if (ImGui::Begin(winTitle.c_str(), nullptr, flags)) {
        // ── TOP HEADER: Type badge, Compact toggle, Focus, Close ────────────
        std::string classStr = body.classificationStr.empty() ? body.getClassName() : body.classificationStr;
        ImVec4 cBadge = Col::AccentCyan;
        if (body.isStar()) cBadge = ImVec4(0.95f, 0.75f, 0.20f, 1.0f);
        else if (body.isGasOrIceGiant()) cBadge = ImVec4(0.35f, 0.65f, 0.95f, 1.0f);
        else if (body.isBlackHole()) cBadge = ImVec4(0.80f, 0.35f, 0.95f, 1.0f);
        else if (body.isMoon()) cBadge = ImVec4(0.65f, 0.75f, 0.85f, 1.0f);
        ImGui::TextColored(cBadge, "[%s]", classStr.c_str());
        ImGui::SameLine();
        ImGui::TextColored(Col::AccentCyan, "%s", body.type.c_str());
        ImGui::SameLine(panelW - 142.0f);
        if (UIIcon::SmallButton(IconId::ChevronDown, "Compact")) {
            m_editPropCollapsed = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Switch to minimal compact card");

        ImGui::SameLine();
        if (UIIcon::SmallButton(IconId::Focus, "Focus")) {
            camera.focusOnBody(body.position, body.radius3D, 0.85f);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Focus and track camera (Hotkey: F)");

        ImGui::SameLine();
        if (UIIcon::SmallButton(IconId::Close, "##CloseEditPanel")) {
            physics.selectBody(-1);
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            return;
        }

        // ── QUICK SANDBOX ACTION TOOLBAR ─────────────────────────────────────
        bool isMoving = (m_activeTool == SandboxTool::Move);
        if (isMoving) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.55f, 0.80f, 0.90f));
        if (UIIcon::Button(IconId::Move, isMoving ? "Gizmo ON" : "Move (M)", ImVec2(104, 24))) {
            toggleMoveTool();
        }
        if (isMoving) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle 3D visual translation gizmo (Hotkey: M / G)");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Orbit, "Circularize", ImVec2(104, 24))) {
            physics.circularizeOrbit(selIdx);
            addEventLog("Circularized orbit for " + body.name);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Recalculate velocity for stable circular Keplerian orbit");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Copy, "Clone", ImVec2(66, 24))) {
            duplicateSelectedObject(physics);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Duplicate body (Ctrl+D)");

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.45f, 0.15f, 0.15f, 0.85f));
        if (UIIcon::Button(IconId::Delete, "Del", ImVec2(56, 24))) {
            deleteSelectedObject(physics);
            ImGui::PopStyleColor();
            ImGui::End();
            ImGui::PopStyleColor(2);
            ImGui::PopStyleVar(2);
            return;
        }
        ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Delete body (Delete/Backspace)");

        ImGui::Separator();

        bool isPlanetOrMinor = (mutBody.type.find("Planet") != std::string::npos || 
                               mutBody.type.find("Moon") != std::string::npos || 
                               mutBody.type.find("Asteroid") != std::string::npos || 
                               mutBody.type.find("Comet") != std::string::npos);
        bool isDwarfStar = (mutBody.type.find("Dwarf") != std::string::npos && !isPlanetOrMinor);
        bool isStar = !isPlanetOrMinor && (mutBody.id == "sol" || mutBody.type.find("Star") != std::string::npos || isDwarfStar);
        bool isBlackHole = (mutBody.type.find("Black Hole") != std::string::npos);

        // ── SECTION 1: PHYSICAL PROPERTIES & DIMENSIONS ──────────────────────
        if (SectionHeader("PHYSICAL PROPERTIES & DIMENSIONS")) {
            ImGui::TextColored(physics.isPaused() ? Col::Yellow : Col::Green, 
                               physics.isPaused() ? "Paused (Live Real-Time Tuning Active)" : "Running (Live Real-Time Tuning Active)");

            // 1. Radius Sliders
            if (isStar) {
                float rSun = (float)(mutBody.radiusM / UnitConverter::SOLAR_RADIUS_M);
                if (ImGui::DragFloat("Radius (R_Sun)##StudioEditR", &rSun, 0.02f, 0.01f, 1500.0f, "%.3f R_Sun")) {
                    mutBody.radiusM = std::max(1000.0, (double)rSun * UnitConverter::SOLAR_RADIUS_M);
                    mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                    char rBuf[64];
                    snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            } else {
                float rEarth = (float)(mutBody.radiusM / UnitConverter::EARTH_RADIUS_M);
                if (ImGui::DragFloat("Radius (R_Earth)##StudioEditR", &rEarth, 0.02f, 0.005f, 250.0f, "%.3f R_Earth")) {
                    mutBody.radiusM = std::max(100.0, (double)rEarth * UnitConverter::EARTH_RADIUS_M);
                    mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                    char rBuf[64];
                    snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            }

            float rKm = (float)(mutBody.radiusM / 1000.0);
            if (ImGui::DragFloat("Radius (km)##StudioEditRkm", &rKm, 10.0f, 10.0f, 5000000.0f, "%.1f km")) {
                mutBody.radiusM = std::max(100.0, (double)rKm * 1000.0);
                mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                char rBuf[64];
                snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
                mutBody.radiusStr = rBuf;
                physics.updateBodyScales();
            }

            // 2. Mass Sliders
            double curM = mutBody.massKg;
            if (isStar || isBlackHole) {
                float mSun = (float)(curM / UnitConverter::SOLAR_MASS_KG);
                if (ImGui::DragFloat("Mass (M_Sun)##StudioEditM", &mSun, 0.05f, 0.001f, 50000.0f, "%.3f M_Sun")) {
                    mutBody.massKg = std::max(1e15, (double)mSun * UnitConverter::SOLAR_MASS_KG);
                    mutBody.massStr = UnitConverter::formatMass(mutBody.massKg);
                }
            } else {
                float mEarth = (float)(curM / UnitConverter::EARTH_MASS_KG);
                if (ImGui::DragFloat("Mass (M_Earth)##StudioEditM", &mEarth, 0.05f, 0.0001f, 10000.0f, "%.3f M_Earth")) {
                    mutBody.massKg = std::max(1e12, (double)mEarth * UnitConverter::EARTH_MASS_KG);
                    mutBody.massStr = UnitConverter::formatMass(mutBody.massKg);
                }
            }

            // 3. Surface Temperature Slider
            float tempK = (float)mutBody.surfaceTempK;
            if (ImGui::DragFloat("Surface Temp (K)##StudioEditT", &tempK, 15.0f, 2.7f, 50000.0f, "%.0f K")) {
                physics.setBodyCustomTemperature(selIdx, (double)tempK);
            }
            if (mutBody.hasCustomTemp) {
                ImGui::SameLine();
                if (ImGui::SmallButton("Reset Eq##StudioResetEqT")) {
                    physics.resetBodyToThermalEquilibrium(selIdx);
                }
                if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset temperature to radiative blackbody equilibrium");
            }

            // 4. Derived physical overview
            if (mutBody.radiusM > 0.0 && mutBody.massKg > 0.0) {
                mutBody.surfaceGravityMps2 = (UnitConverter::G_CONST * mutBody.massKg) / (mutBody.radiusM * mutBody.radiusM);
                char gravBuf[64];
                snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s^2 (%.2f g)", mutBody.surfaceGravityMps2, mutBody.surfaceGravityMps2 / 9.80665);
                mutBody.gravityStr = gravBuf;

                double vol = (4.0 / 3.0) * UnitConverter::PI * std::pow(mutBody.radiusM, 3.0);
                mutBody.meanDensityKgM3 = mutBody.massKg / vol;
                char densBuf[64];
                snprintf(densBuf, sizeof(densBuf), "%.1f kg/m^3", mutBody.meanDensityKgM3);
                mutBody.densityStr = densBuf;

                mutBody.escapeVelocityKmpS = std::sqrt(2.0 * UnitConverter::G_CONST * mutBody.massKg / mutBody.radiusM) / 1000.0;
                char escBuf[64];
                snprintf(escBuf, sizeof(escBuf), "%.2f km/s", mutBody.escapeVelocityKmpS);
                mutBody.escapeVelocityStr = escBuf;
            }

            float halfCol = (panelW - 36.0f) / 2.0f;
            ImGui::BeginGroup();
            StatItem(IconId::Phase, "Phase", isStar ? "Plasma" : (mutBody.surfaceTempK > 1500.0 ? "Molten Magma" : "Solid Rock/Ice"));
            ImGui::SameLine(halfCol);
            StatItem(IconId::Physics, "Gravity", mutBody.gravityStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Speed, "Escape Vel", mutBody.escapeVelocityStr.c_str());
            ImGui::SameLine(halfCol);
            StatItem(IconId::Scale, "Density", mutBody.densityStr.c_str());
            ImGui::EndGroup();
        }

        ImGui::Separator();

        // ── SECTION 2: ATMOSPHERE & ROTATION ─────────────────────────────────
        if (SectionHeader("ATMOSPHERE & ROTATION")) {
            if (!isStar && !isBlackHole) {
                bool atmo = mutBody.hasAtmosphere;
                if (ImGui::Checkbox("Atmosphere Present##StudioAtmo", &atmo)) {
                    physics.setBodyAtmosphere(selIdx, atmo);
                }
                if (atmo) {
                    float pressKpa = (float)mutBody.surfacePressureKpa;
                    if (ImGui::DragFloat("Pressure (kPa)##StudioP", &pressKpa, 0.5f, 0.0f, 15000.0f, "%.1f kPa")) {
                        physics.setBodySurfacePressureKpa(selIdx, (double)pressKpa);
                    }

                    float co2Pct = 0.0f, ch4Pct = 0.0f, h2oPct = 0.0f, n2Pct = 0.0f;
                    for (const auto& ab : mutBody.chemicalInventory) {
                        if (ab.speciesId == "CO2") co2Pct = ab.percentage;
                        else if (ab.speciesId == "CH4") ch4Pct = ab.percentage;
                        else if (ab.speciesId == "H2O") h2oPct = ab.percentage;
                        else if (ab.speciesId == "N2") n2Pct = ab.percentage;
                    }
                    if (ImGui::SliderFloat("CO2 (%)##StudioCO2", &co2Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "CO2", co2Pct);
                    }
                    if (ImGui::SliderFloat("CH4 (%)##StudioCH4", &ch4Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "CH4", ch4Pct);
                    }
                    if (ImGui::SliderFloat("H2O Vapor (%)##StudioH2O", &h2oPct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "H2O", h2oPct);
                    }
                    if (ImGui::SliderFloat("N2 (%)##StudioN2", &n2Pct, 0.0f, 100.0f, "%.1f%%")) {
                        physics.setBodyGasPercentage(selIdx, "N2", n2Pct);
                    }

                    ImGui::TextDisabled("Greenhouse: +%.1f K (tau = %.2f)", mutBody.greenhouseK, mutBody.opticalDepth);

                    if (ImGui::SmallButton("Reset Atmosphere to Baseline##StudioResetAtmo")) {
                        physics.resetBodyAtmosphereToBaseline(selIdx);
                    }
                }
            } else if (isStar) {
                float lum = (float)(mutBody.luminosityW / 3.828e26);
                if (ImGui::DragFloat("Luminosity (L_Sun)##StudioLum", &lum, 0.02f, 0.0001f, 100000.0f, "%.3f L_Sun")) {
                    physics.setStarLuminositySolar(selIdx, (double)lum);
                }
            }

            float col[3] = { mutBody.color.r, mutBody.color.g, mutBody.color.b };
            if (ImGui::ColorEdit3("Albedo Tint##StudioCol", col)) {
                mutBody.color = glm::vec3(col[0], col[1], col[2]);
            }

            float tilt = mutBody.axialTiltDeg;
            if (ImGui::SliderFloat("Axial Tilt (deg)##StudioTilt", &tilt, 0.0f, 180.0f, "%.1f deg")) {
                mutBody.axialTiltDeg = tilt;
                char tiltBuf[32];
                snprintf(tiltBuf, sizeof(tiltBuf), "%.2f deg", mutBody.axialTiltDeg);
                mutBody.axialTiltStr = tiltBuf;
            }
        }

        ImGui::Separator();

        // ── SECTION 3: ORBITAL DYNAMICS & VELOCITY TUNING ────────────────────
        if (SectionHeader("ORBITAL DYNAMICS & VELOCITY TUNING")) {
            if (isStar) {
                ImGui::TextDisabled("Central gravitational attractor");
            } else {
                double vMag = glm::length(mutBody.velocityMps) / 1000.0;
                ImGui::Text("Orbital Speed: %.2f km/s (%s)", vMag, mutBody.orbitalSpeedStr.c_str());

                ImGui::TextColored(Col::TextSecondary, "Scale Orbital Speed:");
                if (ImGui::Button("0.5x##StudioV05", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 0.5); }
                ImGui::SameLine();
                if (ImGui::Button("0.9x##StudioV09", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 0.9); }
                ImGui::SameLine();
                if (ImGui::Button("1.1x##StudioV11", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 1.1); }
                ImGui::SameLine();
                if (ImGui::Button("1.5x##StudioV15", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, 1.5); }
                ImGui::SameLine();
                if (ImGui::Button("Rev##StudioVRev", ImVec2(52, 22))) { physics.scaleBodyVelocity(selIdx, -1.0); }

                ImGui::Spacing();
                ImGui::TextColored(Col::TextSecondary, "Impulse Maneuver (Delta-v):");
                if (ImGui::Button("-5 km/s##StudioRet5", ImVec2(68, 22))) { physics.applyProgradeDeltaV(selIdx, -5.0); }
                ImGui::SameLine();
                if (ImGui::Button("-1 km/s##StudioRet1", ImVec2(68, 22))) { physics.applyProgradeDeltaV(selIdx, -1.0); }
                ImGui::SameLine();
                if (ImGui::Button("+1 km/s##StudioPro1", ImVec2(68, 22))) { physics.applyProgradeDeltaV(selIdx, +1.0); }
                ImGui::SameLine();
                if (ImGui::Button("+5 km/s##StudioPro5", ImVec2(68, 22))) { physics.applyProgradeDeltaV(selIdx, +5.0); }

                static float studioCustomDv = 0.0f;
                ImGui::PushItemWidth(panelW - 128.0f);
                ImGui::DragFloat("##StudioCustomDv", &studioCustomDv, 0.1f, -100.0f, 100.0f, "Delta-v: %+.2f km/s");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Apply Dv##StudioApplyDv", ImVec2(76, 22))) {
                    physics.applyProgradeDeltaV(selIdx, (double)studioCustomDv);
                    studioCustomDv = 0.0f;
                }

                static float studioNormDv = 0.0f;
                ImGui::PushItemWidth(panelW - 128.0f);
                ImGui::DragFloat("##StudioNormDv", &studioNormDv, 0.1f, -50.0f, 50.0f, "Norm: %+.2f km/s");
                ImGui::PopItemWidth();
                ImGui::SameLine();
                if (ImGui::Button("Apply Norm##StudioApplyNorm", ImVec2(76, 22))) {
                    physics.applyNormalDeltaV(selIdx, (double)studioNormDv);
                    studioNormDv = 0.0f;
                }

                ImGui::Spacing();
                float curA = (float)mutBody.semiMajorAxisAU;
                if (curA <= 0.0f) curA = (float)glm::length(mutBody.position);
                if (ImGui::DragFloat("Semi-Major Axis (AU)##StudioSMA", &curA, 0.02f, 0.05f, 150.0f, "%.3f AU")) {
                    physics.setBodyOrbitRadiusAU(selIdx, (double)curA);
                }

                float curEcc = (float)mutBody.eccentricity;
                if (ImGui::SliderFloat("Eccentricity (e)##StudioEcc", &curEcc, 0.0f, 0.95f, "%.3f")) {
                    physics.setBodyEccentricity(selIdx, (double)curEcc);
                }
            }
        }

        ImGui::Separator();

        // ── SECTION 4: AI STABILITY & ORBITAL ELEMENTS ───────────────────────
        if (SectionHeader("AI STABILITY & ORBITAL ELEMENTS")) {
            const auto& pred = aiManager.getCurrentPrediction();
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

            ImGui::TextColored(Col::TextSecondary, "Orbital Stability:");
            ImGui::SameLine();
            ImGui::TextColored(statusCol, "%s", statusPrefix);
            if (!pred.confidence.empty()) {
                ImGui::SameLine();
                ImGui::TextDisabled("(%s conf)", pred.confidence.c_str());
            }

            float hw = (panelW - 36.0f) / 2.0f;
            ImGui::BeginGroup();
            StatItem(IconId::Orbit, "Semi-Major Axis", body.semiMajorAxisStr.c_str());
            ImGui::SameLine(hw);
            StatItem(IconId::Orbit, "Eccentricity", body.eccentricityStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Target, "Perihelion", body.periapsisStr.c_str());
            ImGui::SameLine(hw);
            StatItem(IconId::Target, "Aphelion", body.apoapsisStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Energy, "Orbital Energy", body.orbitalEnergyStr.c_str());
            ImGui::SameLine(hw);
            StatItem(IconId::Ruler, "True Anomaly", body.trueAnomalyStr.c_str());
            ImGui::EndGroup();
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::drawInteractiveGizmo(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH) {
    if (m_activeTool != SandboxTool::Move) {
        if (m_dragState != DragState::Idle) {
            physics.setManipulatedBodyIndex(-1);
            m_dragState = DragState::Idle;
        }
        m_activeGizmoHandle = GizmoHandle::None;
        m_lockedGizmoHandle = GizmoHandle::None;
        m_isDraggingGizmo = false;
        return;
    }

    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) {
        if (m_dragState != DragState::Idle) {
            physics.setManipulatedBodyIndex(-1);
            m_dragState = DragState::Idle;
        }
        m_activeGizmoHandle = GizmoHandle::None;
        m_lockedGizmoHandle = GizmoHandle::None;
        m_isDraggingGizmo = false;
        return;
    }

    CelestialBody& body = physics.getBodies()[selIdx];

    glm::vec2 screenCenter;
    float screenRadius = 0.0f;
    bool inFrustum = camera.projectToScreen(body.position, camera.getTargetPosition(),
                                            vpX, vpY, vpW, vpH, screenCenter, screenRadius, body.radius3D);

    if (!inFrustum && m_dragState == DragState::Idle) {
        m_activeGizmoHandle = GizmoHandle::None;
        return;
    }

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;

    // World camera eye calculation (Target + EyeOffset)
    glm::vec3 camEyeWorld = camera.getTargetPosition() + camera.getEyePosition();
    float camDistToBody = glm::length(camEyeWorld - body.position);
    if (camDistToBody < 1e-6f) camDistToBody = camera.getDistance();

    // Adaptive Gizmo sizing in AU: Target constant screen arm length ~92px
    float tanHalfFov = std::tan(glm::radians(camera.getFOV() * 0.5f));
    float targetArmPixels = 92.0f;
    float gizmoArmAU = (targetArmPixels * camDistToBody * (2.0f * tanHalfFov)) / std::max(vpH, 100.0f);

    // Keep arm at least 1.45x body visual radius so it never clips inside giant bodies
    gizmoArmAU = std::max(gizmoArmAU, body.radius3D * 1.45f);
    gizmoArmAU = std::max(gizmoArmAU, 0.00001f);

    float planeArmAU = gizmoArmAU * 0.38f;
    float handlePixelRadius = 11.0f;
    float centerDiscRadius = std::max(14.0f, std::min(screenRadius + 4.0f, 26.0f));

    // Project Axis Endpoints
    glm::vec3 xTipAU = body.position + glm::vec3(gizmoArmAU, 0.0f, 0.0f);
    glm::vec3 yTipAU = body.position + glm::vec3(0.0f, gizmoArmAU, 0.0f);
    glm::vec3 zTipAU = body.position + glm::vec3(0.0f, 0.0f, gizmoArmAU);

    glm::vec2 sTipX(0.0f), sTipY(0.0f), sTipZ(0.0f);
    float rX = 0.0f, rY = 0.0f, rZ = 0.0f;
    bool xOk = camera.projectToScreen(xTipAU, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sTipX, rX, 0.01f);
    bool yOk = camera.projectToScreen(yTipAU, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sTipY, rY, 0.01f);
    bool zOk = camera.projectToScreen(zTipAU, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sTipZ, rZ, 0.01f);

    // Subtle negative axis projections for 3D depth cue
    glm::vec2 sNegX(0.0f), sNegY(0.0f), sNegZ(0.0f);
    bool negXOk = camera.projectToScreen(body.position - glm::vec3(gizmoArmAU * 0.30f, 0, 0), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sNegX, rX, 0.01f);
    bool negYOk = camera.projectToScreen(body.position - glm::vec3(0, gizmoArmAU * 0.30f, 0), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sNegY, rY, 0.01f);
    bool negZOk = camera.projectToScreen(body.position - glm::vec3(0, 0, gizmoArmAU * 0.30f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sNegZ, rZ, 0.01f);

    // Project Planar Quads (XZ, XY, YZ)
    // Quad XZ (Orbital Plane)
    glm::vec2 sQ_XZ[4];
    bool qXZOk = camera.projectToScreen(body.position, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XZ[0], rX) &&
                 camera.projectToScreen(body.position + glm::vec3(planeArmAU, 0.0f, 0.0f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XZ[1], rX) &&
                 camera.projectToScreen(body.position + glm::vec3(planeArmAU, 0.0f, planeArmAU), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XZ[2], rX) &&
                 camera.projectToScreen(body.position + glm::vec3(0.0f, 0.0f, planeArmAU), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XZ[3], rX);

    // Quad XY (Front Plane)
    glm::vec2 sQ_XY[4];
    bool qXYOk = camera.projectToScreen(body.position, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XY[0], rY) &&
                 camera.projectToScreen(body.position + glm::vec3(planeArmAU, 0.0f, 0.0f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XY[1], rY) &&
                 camera.projectToScreen(body.position + glm::vec3(planeArmAU, planeArmAU, 0.0f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XY[2], rY) &&
                 camera.projectToScreen(body.position + glm::vec3(0.0f, planeArmAU, 0.0f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_XY[3], rY);

    // Quad YZ (Side Plane)
    glm::vec2 sQ_YZ[4];
    bool qYZOk = camera.projectToScreen(body.position, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_YZ[0], rZ) &&
                 camera.projectToScreen(body.position + glm::vec3(0.0f, planeArmAU, 0.0f), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_YZ[1], rZ) &&
                 camera.projectToScreen(body.position + glm::vec3(0.0f, planeArmAU, planeArmAU), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_YZ[2], rZ) &&
                 camera.projectToScreen(body.position + glm::vec3(0.0f, 0.0f, planeArmAU), camera.getTargetPosition(), vpX, vpY, vpW, vpH, sQ_YZ[3], rZ);

    // Distance to segment lambda
    auto distToSegment = [](const ImVec2& p, const glm::vec2& a, const glm::vec2& b) -> float {
        float l2 = (b.x - a.x) * (b.x - a.x) + (b.y - a.y) * (b.y - a.y);
        if (l2 < 1e-4f) return std::sqrt((p.x - a.x) * (p.x - a.x) + (p.y - a.y) * (p.y - a.y));
        float t = std::clamp(((p.x - a.x) * (b.x - a.x) + (p.y - a.y) * (b.y - a.y)) / l2, 0.0f, 1.0f);
        float px = a.x + t * (b.x - a.x);
        float py = a.y + t * (b.y - a.y);
        return std::sqrt((p.x - px) * (p.x - px) + (p.y - py) * (p.y - py));
    };

    // Point in convex quad lambda
    auto pointInQuad = [](const ImVec2& pt, const glm::vec2* q) -> bool {
        auto cross2D = [](const ImVec2& p, const glm::vec2& a, const glm::vec2& b) -> float {
            return (p.x - a.x) * (b.y - a.y) - (p.y - a.y) * (b.x - a.x);
        };
        float d0 = cross2D(pt, q[0], q[1]);
        float d1 = cross2D(pt, q[1], q[2]);
        float d2 = cross2D(pt, q[2], q[3]);
        float d3 = cross2D(pt, q[3], q[0]);
        bool hasNeg = (d0 < 0.0f) || (d1 < 0.0f) || (d2 < 0.0f) || (d3 < 0.0f);
        bool hasPos = (d0 > 0.0f) || (d1 > 0.0f) || (d2 > 0.0f) || (d3 > 0.0f);
        return !(hasNeg && hasPos);
    };

    // ── HIT TESTING (When not dragging) ──
    if (m_dragState == DragState::Idle) {
        float distToCenter = glm::length(glm::vec2(mousePos.x, mousePos.y) - screenCenter);
        bool hoveredCenter = distToCenter <= centerDiscRadius;

        bool hoveredTipX = xOk && (glm::length(glm::vec2(mousePos.x, mousePos.y) - sTipX) <= handlePixelRadius);
        bool hoveredTipY = yOk && (glm::length(glm::vec2(mousePos.x, mousePos.y) - sTipY) <= handlePixelRadius);
        bool hoveredTipZ = zOk && (glm::length(glm::vec2(mousePos.x, mousePos.y) - sTipZ) <= handlePixelRadius);

        bool hoveredStemX = xOk && (distToSegment(mousePos, screenCenter, sTipX) <= 6.5f);
        bool hoveredStemY = yOk && (distToSegment(mousePos, screenCenter, sTipY) <= 6.5f);
        bool hoveredStemZ = zOk && (distToSegment(mousePos, screenCenter, sTipZ) <= 6.5f);

        bool hoveredQuadXZ = qXZOk && pointInQuad(mousePos, sQ_XZ);
        bool hoveredQuadXY = qXYOk && pointInQuad(mousePos, sQ_XY);
        bool hoveredQuadYZ = qYZOk && pointInQuad(mousePos, sQ_YZ);

        // Priority order: Center -> Tips -> Quads -> Stems
        if (hoveredCenter) m_activeGizmoHandle = GizmoHandle::CenterFree;
        else if (hoveredTipX) m_activeGizmoHandle = GizmoHandle::AxisX;
        else if (hoveredTipY) m_activeGizmoHandle = GizmoHandle::AxisY;
        else if (hoveredTipZ) m_activeGizmoHandle = GizmoHandle::AxisZ;
        else if (hoveredQuadXZ) m_activeGizmoHandle = GizmoHandle::PlaneXZ;
        else if (hoveredQuadXY) m_activeGizmoHandle = GizmoHandle::PlaneXY;
        else if (hoveredQuadYZ) m_activeGizmoHandle = GizmoHandle::PlaneYZ;
        else if (hoveredStemX) m_activeGizmoHandle = GizmoHandle::AxisX;
        else if (hoveredStemY) m_activeGizmoHandle = GizmoHandle::AxisY;
        else if (hoveredStemZ) m_activeGizmoHandle = GizmoHandle::AxisZ;
        else m_activeGizmoHandle = GizmoHandle::None;
    }

    // Interactive mouse cursor feedback
    if (m_activeGizmoHandle != GizmoHandle::None || m_dragState != DragState::Idle) {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
    }

    // ── STATE MACHINE: DRAG LIFECYCLE ──
    // 1. Idle -> DragPending
    if (m_dragState == DragState::Idle) {
        if (io.MouseClicked[0] && m_viewportHovered && m_activeGizmoHandle != GizmoHandle::None) {
            m_dragState = DragState::DragPending;
            m_lockedGizmoHandle = m_activeGizmoHandle;
            m_dragStartMousePos = mousePos;
            m_dragStartBodyPosAU = body.position;
            m_dragStartBodyVelMps = body.velocityMps;
            m_dragHasMoved = false;

            // Lock body integration in physics engine so background steps don't fight user manipulation
            physics.setManipulatedBodyIndex(selIdx);

            glm::vec3 rayOrig, rayDir;
            camera.screenToWorldRay(mousePos.x, mousePos.y, vpX, vpY, vpW, vpH, rayOrig, rayDir);

            if (m_lockedGizmoHandle == GizmoHandle::AxisX ||
                m_lockedGizmoHandle == GizmoHandle::AxisY ||
                m_lockedGizmoHandle == GizmoHandle::AxisZ) {

                if (m_lockedGizmoHandle == GizmoHandle::AxisX) m_dragAxisDir = glm::vec3(1.0f, 0.0f, 0.0f);
                else if (m_lockedGizmoHandle == GizmoHandle::AxisY) m_dragAxisDir = glm::vec3(0.0f, 1.0f, 0.0f);
                else m_dragAxisDir = glm::vec3(0.0f, 0.0f, 1.0f);

                glm::vec3 camToBody = camEyeWorld - m_dragStartBodyPosAU;
                glm::vec3 pNorm = camToBody - glm::dot(camToBody, m_dragAxisDir) * m_dragAxisDir;
                if (glm::length(pNorm) > 1e-5f) {
                    pNorm = glm::normalize(pNorm);
                } else {
                    pNorm = (std::abs(m_dragAxisDir.y) < 0.9f) ? glm::vec3(0.0f, 1.0f, 0.0f) : glm::vec3(0.0f, 0.0f, 1.0f);
                }
                m_dragConstraintPlaneNormal = pNorm;

                if (camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, m_dragStartHitAU)) {
                    m_dragStartAxisT = glm::dot(m_dragStartHitAU - m_dragStartBodyPosAU, m_dragAxisDir);
                } else {
                    m_dragStartHitAU = m_dragStartBodyPosAU;
                    m_dragStartAxisT = 0.0f;
                }
            } else if (m_lockedGizmoHandle == GizmoHandle::PlaneXZ) {
                m_dragConstraintPlaneNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                if (!camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, m_dragStartHitAU)) {
                    m_dragStartHitAU = m_dragStartBodyPosAU;
                }
            } else if (m_lockedGizmoHandle == GizmoHandle::PlaneXY) {
                m_dragConstraintPlaneNormal = glm::vec3(0.0f, 0.0f, 1.0f);
                if (!camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, m_dragStartHitAU)) {
                    m_dragStartHitAU = m_dragStartBodyPosAU;
                }
            } else if (m_lockedGizmoHandle == GizmoHandle::PlaneYZ) {
                m_dragConstraintPlaneNormal = glm::vec3(1.0f, 0.0f, 0.0f);
                if (!camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, m_dragStartHitAU)) {
                    m_dragStartHitAU = m_dragStartBodyPosAU;
                }
            } else if (m_lockedGizmoHandle == GizmoHandle::CenterFree) {
                glm::vec3 camToBody = camEyeWorld - m_dragStartBodyPosAU;
                if (glm::length(camToBody) > 1e-5f) {
                    m_dragConstraintPlaneNormal = glm::normalize(camToBody);
                } else {
                    m_dragConstraintPlaneNormal = glm::vec3(0.0f, 1.0f, 0.0f);
                }
                if (!camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, m_dragStartHitAU)) {
                    m_dragStartHitAU = m_dragStartBodyPosAU;
                }
            }
        }
    }
    // 2. DragPending -> Dragging OR Cancel/Click
    else if (m_dragState == DragState::DragPending) {
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || io.MouseClicked[1]) {
            physics.setManipulatedBodyIndex(-1);
            m_dragState = DragState::Idle;
            m_isDraggingGizmo = false;
            m_lockedGizmoHandle = GizmoHandle::None;
            return;
        }

        if (!io.MouseDown[0] || io.MouseReleased[0]) {
            // Simple click without movement - no displacement, release cleanly
            physics.setManipulatedBodyIndex(-1);
            m_dragState = DragState::Idle;
            m_isDraggingGizmo = false;
            m_lockedGizmoHandle = GizmoHandle::None;
            return;
        }

        float mouseDisp = glm::length(glm::vec2(mousePos.x - m_dragStartMousePos.x, mousePos.y - m_dragStartMousePos.y));
        if (mouseDisp >= 3.5f) {
            m_dragState = DragState::Dragging;
            m_isDraggingGizmo = true;
        }
    }
    // 3. Dragging
    else if (m_dragState == DragState::Dragging) {
        m_activeGizmoHandle = m_lockedGizmoHandle;

        // Escape or Right-click cancellation: REVERT to pre-drag state
        if (ImGui::IsKeyPressed(ImGuiKey_Escape) || io.MouseClicked[1]) {
            physics.setBodyPositionAU(selIdx, m_dragStartBodyPosAU, true);
            physics.getBodies()[selIdx].velocityMps = m_dragStartBodyVelMps;
            physics.setManipulatedBodyIndex(-1);

            m_dragState = DragState::Idle;
            m_isDraggingGizmo = false;
            m_activeGizmoHandle = GizmoHandle::None;
            m_lockedGizmoHandle = GizmoHandle::None;
            m_dragHasMoved = false;

            addEventLog("Move cancelled — reverted " + body.name);
            return;
        }

        // Mouse released: Commit transactional change
        if (!io.MouseDown[0] || io.MouseReleased[0]) {
            if (m_dragHasMoved) {
                m_undoRedo.recordReposition(selIdx, m_dragStartBodyPosAU, body.position, m_dragStartBodyVelMps, body.velocityMps);
                addEventLog("Moved " + body.name + " to " + body.distanceStr);
            }

            physics.setManipulatedBodyIndex(-1);
            m_dragState = DragState::Idle;
            m_isDraggingGizmo = false;
            m_activeGizmoHandle = GizmoHandle::None;
            m_lockedGizmoHandle = GizmoHandle::None;
            m_dragHasMoved = false;
            return;
        }

        // Only compute displacement if mouse cursor actually moved (PRESS != MOVEMENT)
        if (io.MouseDelta.x != 0.0f || io.MouseDelta.y != 0.0f) {
            glm::vec3 rayOrig, rayDir;
            camera.screenToWorldRay(mousePos.x, mousePos.y, vpX, vpY, vpW, vpH, rayOrig, rayDir);

            glm::vec3 curHitAU(0.0f);
            if (camera.intersectPlane(rayOrig, rayDir, m_dragStartBodyPosAU, m_dragConstraintPlaneNormal, curHitAU)) {
                if (m_lockedGizmoHandle == GizmoHandle::AxisX ||
                    m_lockedGizmoHandle == GizmoHandle::AxisY ||
                    m_lockedGizmoHandle == GizmoHandle::AxisZ) {

                    float curT = glm::dot(curHitAU - m_dragStartBodyPosAU, m_dragAxisDir);
                    float deltaT = curT - m_dragStartAxisT;
                    glm::vec3 newPos = m_dragStartBodyPosAU + deltaT * m_dragAxisDir;
                    physics.setBodyPositionAU(selIdx, newPos, true);
                    m_dragHasMoved = true;
                } else if (m_lockedGizmoHandle == GizmoHandle::PlaneXZ) {
                    glm::vec3 deltaP = curHitAU - m_dragStartHitAU;
                    glm::vec3 newPos = m_dragStartBodyPosAU + glm::vec3(deltaP.x, 0.0f, deltaP.z);
                    physics.setBodyPositionAU(selIdx, newPos, true);
                    m_dragHasMoved = true;
                } else if (m_lockedGizmoHandle == GizmoHandle::PlaneXY) {
                    glm::vec3 deltaP = curHitAU - m_dragStartHitAU;
                    glm::vec3 newPos = m_dragStartBodyPosAU + glm::vec3(deltaP.x, deltaP.y, 0.0f);
                    physics.setBodyPositionAU(selIdx, newPos, true);
                    m_dragHasMoved = true;
                } else if (m_lockedGizmoHandle == GizmoHandle::PlaneYZ) {
                    glm::vec3 deltaP = curHitAU - m_dragStartHitAU;
                    glm::vec3 newPos = m_dragStartBodyPosAU + glm::vec3(0.0f, deltaP.y, deltaP.z);
                    physics.setBodyPositionAU(selIdx, newPos, true);
                    m_dragHasMoved = true;
                } else if (m_lockedGizmoHandle == GizmoHandle::CenterFree) {
                    glm::vec3 deltaP = curHitAU - m_dragStartHitAU;
                    glm::vec3 newPos = m_dragStartBodyPosAU + deltaP;
                    physics.setBodyPositionAU(selIdx, newPos, true);
                    m_dragHasMoved = true;
                }
            }
        }
    }

    // ── GIZMO VISUAL RENDERING ──
    GizmoHandle curHandle = (m_dragState != DragState::Idle) ? m_lockedGizmoHandle : m_activeGizmoHandle;

    // Displacement guide line connecting start position to current position while dragging
    if (m_dragState == DragState::Dragging && m_dragHasMoved) {
        glm::vec2 sStartPos;
        float rStart = 0.0f;
        if (camera.projectToScreen(m_dragStartBodyPosAU, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sStartPos, rStart)) {
            dl->AddLine(ImVec2(sStartPos.x, sStartPos.y), ImVec2(screenCenter.x, screenCenter.y),
                        ImColor(255, 230, 80, 200), 1.5f);
            dl->AddCircleFilled(ImVec2(sStartPos.x, sStartPos.y), 4.0f, ImColor(255, 230, 80, 240));
        }
    }

    // Infinite axis guideline while dragging an axis
    if (m_dragState == DragState::Dragging &&
        (m_lockedGizmoHandle == GizmoHandle::AxisX || m_lockedGizmoHandle == GizmoHandle::AxisY || m_lockedGizmoHandle == GizmoHandle::AxisZ)) {
        glm::vec3 pFarNeg = m_dragStartBodyPosAU - m_dragAxisDir * (gizmoArmAU * 8.0f);
        glm::vec3 pFarPos = m_dragStartBodyPosAU + m_dragAxisDir * (gizmoArmAU * 8.0f);
        glm::vec2 sFarNeg, sFarPos;
        float rN = 0.0f, rP = 0.0f;
        if (camera.projectToScreen(pFarNeg, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sFarNeg, rN) &&
            camera.projectToScreen(pFarPos, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sFarPos, rP)) {
            ImU32 colGuide = (m_lockedGizmoHandle == GizmoHandle::AxisX) ? ImColor(255, 90, 90, 100) :
                             (m_lockedGizmoHandle == GizmoHandle::AxisY) ? ImColor(90, 255, 110, 100) :
                                                                          ImColor(90, 170, 255, 100);
            dl->AddLine(ImVec2(sFarNeg.x, sFarNeg.y), ImVec2(sFarPos.x, sFarPos.y), colGuide, 1.0f);
        }
    }

    // Negative subtle axis arms
    if (negXOk) dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sNegX.x, sNegX.y), ImColor(220, 60, 60, 50), 1.0f);
    if (negYOk) dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sNegY.x, sNegY.y), ImColor(50, 220, 60, 50), 1.0f);
    if (negZOk) dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sNegZ.x, sNegZ.y), ImColor(50, 120, 240, 50), 1.0f);

    // Planar Quads: Draw behind axis lines
    if (qXZOk) {
        bool act = (curHandle == GizmoHandle::PlaneXZ);
        ImU32 fill = act ? ImColor(255, 230, 80, 140) : ImColor(100, 220, 255, 45);
        ImU32 line = act ? ImColor(255, 255, 120, 255) : ImColor(100, 220, 255, 150);
        dl->AddQuadFilled(ImVec2(sQ_XZ[0].x, sQ_XZ[0].y), ImVec2(sQ_XZ[1].x, sQ_XZ[1].y),
                          ImVec2(sQ_XZ[2].x, sQ_XZ[2].y), ImVec2(sQ_XZ[3].x, sQ_XZ[3].y), fill);
        dl->AddQuad(ImVec2(sQ_XZ[0].x, sQ_XZ[0].y), ImVec2(sQ_XZ[1].x, sQ_XZ[1].y),
                    ImVec2(sQ_XZ[2].x, sQ_XZ[2].y), ImVec2(sQ_XZ[3].x, sQ_XZ[3].y), line, act ? 2.0f : 1.2f);
    }
    if (qXYOk) {
        bool act = (curHandle == GizmoHandle::PlaneXY);
        ImU32 fill = act ? ImColor(255, 190, 50, 140) : ImColor(255, 180, 60, 40);
        ImU32 line = act ? ImColor(255, 220, 90, 255) : ImColor(255, 180, 60, 140);
        dl->AddQuadFilled(ImVec2(sQ_XY[0].x, sQ_XY[0].y), ImVec2(sQ_XY[1].x, sQ_XY[1].y),
                          ImVec2(sQ_XY[2].x, sQ_XY[2].y), ImVec2(sQ_XY[3].x, sQ_XY[3].y), fill);
        dl->AddQuad(ImVec2(sQ_XY[0].x, sQ_XY[0].y), ImVec2(sQ_XY[1].x, sQ_XY[1].y),
                    ImVec2(sQ_XY[2].x, sQ_XY[2].y), ImVec2(sQ_XY[3].x, sQ_XY[3].y), line, act ? 2.0f : 1.2f);
    }
    if (qYZOk) {
        bool act = (curHandle == GizmoHandle::PlaneYZ);
        ImU32 fill = act ? ImColor(50, 220, 255, 140) : ImColor(50, 200, 255, 40);
        ImU32 line = act ? ImColor(100, 245, 255, 255) : ImColor(50, 200, 255, 140);
        dl->AddQuadFilled(ImVec2(sQ_YZ[0].x, sQ_YZ[0].y), ImVec2(sQ_YZ[1].x, sQ_YZ[1].y),
                          ImVec2(sQ_YZ[2].x, sQ_YZ[2].y), ImVec2(sQ_YZ[3].x, sQ_YZ[3].y), fill);
        dl->AddQuad(ImVec2(sQ_YZ[0].x, sQ_YZ[0].y), ImVec2(sQ_YZ[1].x, sQ_YZ[1].y),
                    ImVec2(sQ_YZ[2].x, sQ_YZ[2].y), ImVec2(sQ_YZ[3].x, sQ_YZ[3].y), line, act ? 2.0f : 1.2f);
    }

    // Center Free View-Plane Disc
    bool centerAct = (curHandle == GizmoHandle::CenterFree);
    ImU32 colCenterRing = centerAct ? ImColor(255, 255, 255, 255) : ImColor(180, 220, 255, 140);
    ImU32 colCenterFill = centerAct ? ImColor(255, 230, 80, 110) : ImColor(100, 190, 255, 30);
    dl->AddCircle(ImVec2(screenCenter.x, screenCenter.y), centerDiscRadius, colCenterRing, 32, centerAct ? 2.2f : 1.5f);
    dl->AddCircleFilled(ImVec2(screenCenter.x, screenCenter.y), centerDiscRadius * 0.55f, colCenterFill, 32);

    // Primary Axis Lines and Tip Handles
    if (xOk) {
        bool act = (curHandle == GizmoHandle::AxisX);
        ImU32 col = act ? ImColor(255, 80, 80, 255) : ImColor(220, 50, 50, 200);
        dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sTipX.x, sTipX.y), col, act ? 3.5f : 2.2f);
        dl->AddCircleFilled(ImVec2(sTipX.x, sTipX.y), act ? handlePixelRadius + 1.5f : handlePixelRadius, col);
        if (act) dl->AddCircle(ImVec2(sTipX.x, sTipX.y), handlePixelRadius + 3.0f, ImColor(255, 255, 255, 240), 16, 1.5f);
        dl->AddText(ImVec2(sTipX.x + 7, sTipX.y - 7), col, "X");
    }
    if (yOk) {
        bool act = (curHandle == GizmoHandle::AxisY);
        ImU32 col = act ? ImColor(80, 255, 80, 255) : ImColor(50, 220, 50, 200);
        dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sTipY.x, sTipY.y), col, act ? 3.5f : 2.2f);
        dl->AddCircleFilled(ImVec2(sTipY.x, sTipY.y), act ? handlePixelRadius + 1.5f : handlePixelRadius, col);
        if (act) dl->AddCircle(ImVec2(sTipY.x, sTipY.y), handlePixelRadius + 3.0f, ImColor(255, 255, 255, 240), 16, 1.5f);
        dl->AddText(ImVec2(sTipY.x + 7, sTipY.y - 7), col, "Y");
    }
    if (zOk) {
        bool act = (curHandle == GizmoHandle::AxisZ);
        ImU32 col = act ? ImColor(100, 170, 255, 255) : ImColor(50, 110, 240, 200);
        dl->AddLine(ImVec2(screenCenter.x, screenCenter.y), ImVec2(sTipZ.x, sTipZ.y), col, act ? 3.5f : 2.2f);
        dl->AddCircleFilled(ImVec2(sTipZ.x, sTipZ.y), act ? handlePixelRadius + 1.5f : handlePixelRadius, col);
        if (act) dl->AddCircle(ImVec2(sTipZ.x, sTipZ.y), handlePixelRadius + 3.0f, ImColor(255, 255, 255, 240), 16, 1.5f);
        dl->AddText(ImVec2(sTipZ.x + 7, sTipZ.y - 7), col, "Z");
    }

    // ── LIVE HUD COORDINATE BADGE WHILE DRAGGING ──
    if (m_dragState == DragState::Dragging) {
        const char* handleName = "FREE MOVE";
        if (m_lockedGizmoHandle == GizmoHandle::AxisX) handleName = "X AXIS";
        else if (m_lockedGizmoHandle == GizmoHandle::AxisY) handleName = "Y AXIS (VERTICAL)";
        else if (m_lockedGizmoHandle == GizmoHandle::AxisZ) handleName = "Z AXIS";
        else if (m_lockedGizmoHandle == GizmoHandle::PlaneXZ) handleName = "XZ ORBITAL PLANE";
        else if (m_lockedGizmoHandle == GizmoHandle::PlaneXY) handleName = "XY FRONT PLANE";
        else if (m_lockedGizmoHandle == GizmoHandle::PlaneYZ) handleName = "YZ SIDE PLANE";

        glm::vec3 deltaPos = body.position - m_dragStartBodyPosAU;
        char coordBuf[256];
        snprintf(coordBuf, sizeof(coordBuf),
                 "TRANSLATE: %s [%s]\n"
                 "Pos: X: %+.3f  Y: %+.3f  Z: %+.3f AU\n"
                 "Off: dX: %+.3f  dY: %+.3f  dZ: %+.3f AU\n"
                 "Dist: %s  |  [Esc] Cancel  [Release] Confirm",
                 body.name.c_str(), handleName,
                 body.position.x, body.position.y, body.position.z,
                 deltaPos.x, deltaPos.y, deltaPos.z,
                 body.distanceStr.c_str());

        ImVec2 tSize = ImGui::CalcTextSize(coordBuf);
        ImVec2 pBox(mousePos.x + 18, mousePos.y + 18);

        // Keep inside viewport bounds
        if (pBox.x + tSize.x + 20 > vpX + vpW) pBox.x = mousePos.x - tSize.x - 24;
        if (pBox.y + tSize.y + 20 > vpY + vpH) pBox.y = mousePos.y - tSize.y - 20;

        dl->AddRectFilled(pBox, ImVec2(pBox.x + tSize.x + 16, pBox.y + tSize.y + 14),
                          ImColor(8, 14, 24, 235), 6.0f);
        dl->AddRect(pBox, ImVec2(pBox.x + tSize.x + 16, pBox.y + tSize.y + 14),
                    ImColor(255, 220, 80, 220), 6.0f, 0, 1.5f);
        dl->AddText(ImVec2(pBox.x + 8, pBox.y + 7), ImColor(255, 255, 255, 255), coordBuf);
    }
}

void UIManager::drawPlacementGuide(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH) {
    if (!m_placementActive) return;

    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = io.MousePos;

    glm::vec3 rayOrig, rayDir;
    camera.screenToWorldRay(mousePos.x, mousePos.y, vpX, vpY, vpW, vpH, rayOrig, rayDir);

    glm::vec3 planePoint(0.0f);
    if (!physics.getBodies().empty() && m_placementParentIdx >= 0 && m_placementParentIdx < (int)physics.getBodies().size()) {
        planePoint = physics.getBodies()[m_placementParentIdx].position;
    }

    glm::vec3 spawnPosAU(0.0f);
    bool hit = camera.intersectPlane(rayOrig, rayDir, planePoint, glm::vec3(0, 1, 0), spawnPosAU);
    if (!hit) return;

    glm::vec2 sSpawn;
    float sRad;
    if (camera.projectToScreen(spawnPosAU, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sSpawn, sRad, 0.02f)) {
        ImDrawList* dl = ImGui::GetForegroundDrawList();
        dl->AddCircle(ImVec2(sSpawn.x, sSpawn.y), 18.0f, ImColor(0, 240, 255, 230), 32, 2.0f);
        dl->AddCircleFilled(ImVec2(sSpawn.x, sSpawn.y), 5.0f, ImColor(255, 255, 100, 200));

        if (!physics.getBodies().empty() && m_placementParentIdx >= 0 && m_placementParentIdx < (int)physics.getBodies().size()) {
            const auto& parent = physics.getBodies()[m_placementParentIdx];
            float rAU = glm::length(glm::vec2(spawnPosAU.x - parent.position.x, spawnPosAU.z - parent.position.z));
            const int segs = 64;
            std::vector<ImVec2> pts;
            for (int s = 0; s <= segs; ++s) {
                float theta = (float)s / (float)segs * 6.2831853f;
                glm::vec3 pOrbit = parent.position + glm::vec3(cosf(theta) * rAU, 0.0f, sinf(theta) * rAU);
                glm::vec2 sOrb;
                float rO;
                if (camera.projectToScreen(pOrbit, camera.getTargetPosition(), vpX, vpY, vpW, vpH, sOrb, rO, 0.01f)) {
                    pts.push_back(ImVec2(sOrb.x, sOrb.y));
                }
            }
            if (pts.size() > 1) {
                for (size_t s = 0; s < pts.size() - 1; ++s) {
                    dl->AddLine(pts[s], pts[s + 1], ImColor(0, 220, 255, 110), 1.5f);
                }
            }
        }

        char labelBuf[128];
        snprintf(labelBuf, sizeof(labelBuf), "Place %s [%.2f AU]\nLeft-Click: Spawn | Esc/Right-Click: Cancel",
                 m_placementTemplate.c_str(), glm::length(spawnPosAU));
        dl->AddText(ImVec2(sSpawn.x + 22, sSpawn.y - 12), ImColor(255, 255, 255, 230), labelBuf);
    }

    if (io.MouseClicked[0] && m_viewportHovered) {
        int parentIdx = (!physics.getBodies().empty()) ? m_placementParentIdx : -1;
        bool autoOrbit = (m_placementMode == 0);
        int newIdx = physics.spawnCelestialBody(m_placementTemplate, spawnPosAU, parentIdx, autoOrbit);
        if (newIdx >= 0) {
            m_undoRedo.recordAddBody(newIdx, physics.getBodies()[newIdx]);
            physics.selectBody(newIdx);
            addEventLog("Placed new " + m_placementTemplate + " at " + physics.getBodies()[newIdx].distanceStr);
        }
        cancelPlacement();
    } else if (io.MouseClicked[1] || ImGui::IsKeyPressed(ImGuiKey_Escape)) {
        cancelPlacement();
    }
}

void UIManager::drawFloatingSimBar(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float vpX, float vpY, float vpW, float vpH) {
    float barW = 620.0f;
    float barH = 46.0f;
    float barX = vpX + (vpW - barW) * 0.5f;
    float barY = vpY + vpH - barH - 12.0f;

    ImGui::SetNextWindowPos(ImVec2(barX, barY));
    ImGui::SetNextWindowSize(ImVec2(barW, barH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 23.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 8));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.05f, 0.09f, 0.92f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.70f, 0.90f, 0.45f));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoSavedSettings;
    if (ImGui::Begin("##FloatingSimBar", nullptr, flags)) {
        if (m_showHierarchy) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.55f, 0.80f, 0.85f));
        if (UIIcon::Button(IconId::Hierarchy, "##barHier", ImVec2(32, 28))) {
            m_showHierarchy = !m_showHierarchy;
        }
        if (m_showHierarchy) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Toggle System Hierarchy Outliner (Hotkey: H)");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        bool isSelect = (m_activeTool == SandboxTool::Select);
        if (isSelect) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.55f, 0.80f, 0.85f));
        if (UIIcon::Button(IconId::Select, "Select", ImVec2(76, 28))) {
            m_activeTool = SandboxTool::Select;
            m_placementActive = false;
        }
        if (isSelect) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Select tool: Click body to select (Hotkey: V / S)");

        ImGui::SameLine();
        bool isMove = (m_activeTool == SandboxTool::Move);
        if (isMove) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.55f, 0.80f, 0.85f));
        if (UIIcon::Button(IconId::Move, "Move", ImVec2(72, 28))) {
            toggleMoveTool();
        }
        if (isMove) ImGui::PopStyleColor();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("3D Translation Gizmo: Drag planes & axes (Hotkey: M / G)");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Add, "Add", ImVec2(66, 28))) {
            m_showAddPalette = true;
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Add celestial objects to simulation (Hotkey: A)");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        bool isPaused = physics.isPaused();
        if (UIIcon::Button(isPaused ? IconId::Play : IconId::Pause, "##barPlay", ImVec2(32, 28))) {
            physics.togglePause();
            addEventLog(physics.isPaused() ? "Simulation paused" : "Simulation running");
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Play / Pause simulation (Hotkey: Space)");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::StepForward, "##barStep", ImVec2(30, 28))) {
            physics.stepSingleFrame(1.0f / 60.0f);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Step forward single frame");

        ImGui::SameLine();
        float warp = physics.getTimeScale();
        char warpStr[32];
        if (warp >= 86400.0f * 365.25f) snprintf(warpStr, sizeof(warpStr), "%.1f yr/s", warp / (86400.0f * 365.25f));
        else if (warp >= 86400.0f * 30.0f) snprintf(warpStr, sizeof(warpStr), "%.1f mo/s", warp / (86400.0f * 30.0f));
        else if (warp >= 86400.0f) snprintf(warpStr, sizeof(warpStr), "%.1f d/s", warp / 86400.0f);
        else snprintf(warpStr, sizeof(warpStr), "%.1fx", warp);

        ImGui::PushItemWidth(80);
        if (ImGui::BeginCombo("##SpeedCombo", warpStr)) {
            const float presets[] = { 0.1f, 1.0f, 10.0f, 60.0f, 3600.0f, 86400.0f, 86400.0f * 30.0f, 86400.0f * 365.25f };
            const char* names[]   = { "0.1x Realtime", "1x Realtime", "10x", "1 min/s", "1 hour/s", "1 day/s", "1 month/s", "1 year/s" };
            for (int p = 0; p < 8; ++p) {
                if (ImGui::Selectable(names[p], warp == presets[p])) {
                    physics.setTimeScale(presets[p]);
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Simulation time acceleration rate");

        ImGui::SameLine();
        ImGui::TextDisabled("|");
        ImGui::SameLine();

        if (!canUndo()) ImGui::BeginDisabled();
        if (UIIcon::Button(IconId::Undo, "##barUndo", ImVec2(28, 28))) {
            undo(physics);
        }
        if (!canUndo()) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Undo last edit (Ctrl+Z)");

        ImGui::SameLine();
        if (!canRedo()) ImGui::BeginDisabled();
        if (UIIcon::Button(IconId::Redo, "##barRedo", ImVec2(28, 28))) {
            redo(physics);
        }
        if (!canRedo()) ImGui::EndDisabled();
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Redo last edit (Ctrl+Y)");

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Target, "##barOverview", ImVec2(28, 28))) {
            camera.resetOverview(glm::vec3(0.0f), 6.0f);
        }
        if (ImGui::IsItemHovered()) ImGui::SetTooltip("Reset camera overview (Hotkey: R)");
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::drawAddObjectPalette(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH) {
    float palW = 440.0f;
    float palH = 340.0f;
    float palX = vpX + (vpW - palW) * 0.5f;
    float palY = vpY + (vpH - palH) * 0.5f;

    ImGui::SetNextWindowPos(ImVec2(palX, palY), ImGuiCond_Appearing);
    ImGui::SetNextWindowSize(ImVec2(palW, palH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.05f, 0.09f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.70f, 0.90f, 0.50f));

    if (ImGui::Begin("Add Celestial Object##AddPalette", &m_showAddPalette, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(Col::AccentCyan, "Select Object Template:");
        ImGui::Spacing();

        struct TemplateInfo {
            const char* id;
            IconId icon;
            const char* label;
            const char* desc;
        };

        const TemplateInfo templates[] = {
            { "Planet",    IconId::Orbit,     "Planet",      "Rocky terrestrial planet" },
            { "GasGiant",  IconId::Orbit,     "Gas Giant",   "Massive Jovian gas giant" },
            { "Star",      IconId::Star,      "Star",        "Luminous main sequence star" },
            { "Moon",      IconId::Moon,      "Moon",        "Natural planetary satellite" },
            { "Asteroid",  IconId::Asteroid,  "Asteroid",    "Small irregular planetesimal" },
            { "Comet",     IconId::Asteroid,  "Comet",       "Volatile icy body" },
            { "BlackHole", IconId::BlackHole, "Black Hole",  "Relativistic gravitational singularity" }
        };

        for (int t = 0; t < 7; ++t) {
            bool isSel = (m_placementTemplate == templates[t].id);
            if (isSel) ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0f, 0.50f, 0.75f, 0.80f));
            if (UIIcon::Button(templates[t].icon, templates[t].label, ImVec2(195, 28))) {
                m_placementTemplate = templates[t].id;
            }
            if (isSel) ImGui::PopStyleColor();
            if (ImGui::IsItemHovered()) ImGui::SetTooltip("%s", templates[t].desc);

            if (t % 2 == 0 && t < 6) ImGui::SameLine();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::TextColored(Col::AccentCyan, "Placement Mode:");
        ImGui::RadioButton("Smart Keplerian Orbit (Auto v_circ)", &m_placementMode, 0);
        ImGui::SameLine();
        ImGui::RadioButton("Free Placement (Zero Velocity)", &m_placementMode, 1);

        ImGui::Spacing();
        if (m_placementMode == 0 && !physics.getBodies().empty()) {
            ImGui::TextColored(Col::TextSecondary, "Primary Attractor:");
            ImGui::SameLine();
            const auto& bodies = physics.getBodies();
            if (m_placementParentIdx >= (int)bodies.size()) m_placementParentIdx = 0;
            if (ImGui::BeginCombo("##ParentCombo", bodies[m_placementParentIdx].name.c_str())) {
                for (int i = 0; i < (int)bodies.size(); ++i) {
                    if (ImGui::Selectable(bodies[i].name.c_str(), i == m_placementParentIdx)) {
                        m_placementParentIdx = i;
                    }
                }
                ImGui::EndCombo();
            }
        }

        ImGui::Spacing();
        ImGui::Separator();

        if (ImGui::Button("Spawn in Viewport", ImVec2(240, 30))) {
            startPlacement(m_placementTemplate, m_placementMode);
            m_showAddPalette = false;
        }
        ImGui::SameLine();
        if (ImGui::Button("Cancel", ImVec2(100, 30))) {
            m_showAddPalette = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void UIManager::drawDetailsInspector(PhysicsEngine& physics, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, float topBarH, float winW, float winH, float statusBarH) {
    int selIdx = physics.getSelectedBodyIndex();
    if (selIdx < 0 || selIdx >= (int)physics.getBodies().size()) {
        m_showDetailsModal = false;
        return;
    }
    CelestialBody& body = physics.getBodies()[selIdx];

    float inspW = 460.0f;
    float inspH = std::min(680.0f, winH - topBarH - statusBarH - 20.0f);
    ImGui::SetNextWindowPos(ImVec2(winW - inspW - 16.0f, topBarH + 10.0f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(inspW, inspH), ImGuiCond_FirstUseEver);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.025f, 0.04f, 0.08f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.0f, 0.70f, 0.90f, 0.45f));

    if (ImGui::Begin((body.name + " — Scientific Inspector & Property Studio").c_str(), &m_showDetailsModal)) {
        float panelW = ImGui::GetContentRegionAvail().x + 16.0f;

        // ── VISUAL & PHYSICAL STATE INSPECTOR ──────────────────────────────────────
        if (SectionHeader("VISUAL & PHYSICAL STATE")) {
            const VisualBodyState* vBody = visualAdapter.getVisualBody(body.id);
            if (!vBody) vBody = visualAdapter.getVisualBody(body.dbId);

            float halfW = (panelW - 40) / 2.0f;
            char rBuf[32];
            if (vBody) snprintf(rBuf, sizeof(rBuf), "%.4f AU", vBody->renderRadius);
            else snprintf(rBuf, sizeof(rBuf), "%.4f AU", body.radius3D);

            ImGui::BeginGroup();
            StatItem(IconId::Ruler, "Render Scale", rBuf);
            ImGui::SameLine(halfW);
            StatItem(IconId::Ruler, "Physical Radius", body.radiusStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Heat, "Blackbody Temp", body.tempStr.c_str());
            ImGui::SameLine(halfW);
            std::string phaseStr = "Solid Rock/Ice";
            if (vBody) {
                if (vBody->phase == MaterialPhase::VaporGas) phaseStr = vBody->isStar ? "Stellar Plasma" : "Vapor / Gas";
                else if (vBody->phase == MaterialPhase::LiquidMolten) phaseStr = "Molten Magma";
                else if (vBody->phase == MaterialPhase::SoftenedPlastic) phaseStr = "Softened Plastic";
                else if (vBody->phase == MaterialPhase::Solid) phaseStr = "Solid Rock/Ice";
            }
            StatItem(IconId::Phase, "Material Phase", phaseStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Atmosphere, "Atmosphere", (vBody && vBody->hasAtmosphere) ? "Scattering Active" : "None/Thin");
            ImGui::SameLine(halfW);
            StatItem(IconId::Atmosphere, "Cloud Cover", (vBody && vBody->hasClouds) ? "Dynamic Clouds" : "Clear");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Speed, "Rotation Speed", body.rotationPeriodStr.c_str());
            ImGui::SameLine(halfW);
            StatItem(IconId::Ruler, "Axial Tilt", body.axialTiltStr.c_str());
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
            StatItem(IconId::Physics, "Gravity", body.gravityStr.c_str());
            ImGui::SameLine(halfW);
            StatItem(IconId::Speed, "Escape Velocity", body.escapeVelocityStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Heat, "Surface Temp.", body.tempStr.c_str());
            ImGui::SameLine(halfW);
            StatItem(IconId::Atmosphere, "Atmospheric Pressure", body.pressureStr.c_str());
            ImGui::EndGroup();

            char hBuf[32], tauBuf[32];
            snprintf(hBuf, sizeof(hBuf), "%.1f km", body.scaleHeightKm);
            snprintf(tauBuf, sizeof(tauBuf), "%.2f (+%.0f K)", body.opticalDepth, body.greenhouseK);
            ImGui::BeginGroup();
            StatItem(IconId::Ruler, "Scale Height", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? hBuf : "N/A");
            ImGui::SameLine(halfW);
            StatItem(IconId::Atmosphere, "Optical Depth (τ)", (body.hasAtmosphere && body.surfacePressurePa > 1.0) ? tauBuf : "0.00 (+0 K)");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Physics, "Mean Density", body.densityStr.c_str());
            ImGui::SameLine(halfW);
            StatItem(IconId::Time, "Day Length", body.rotationPeriodStr.c_str());
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Orbit, "Year Length", body.yearLengthStr.c_str());
            ImGui::SameLine(halfW);
            StatItem(IconId::Ruler, "Surface Area", body.surfaceAreaStr.c_str());
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
                    snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            } else {
                float rEarth = (float)(mutBody.radiusM / UnitConverter::EARTH_RADIUS_M);
                if (ImGui::DragFloat("Radius (R⊕)##LiveEditR", &rEarth, 0.02f, 0.005f, 250.0f, "%.3f R⊕")) {
                    mutBody.radiusM = (double)rEarth * UnitConverter::EARTH_RADIUS_M;
                    mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                    char rBuf[64];
                    snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
                    mutBody.radiusStr = rBuf;
                    physics.updateBodyScales();
                }
            }

            float rKm = (float)(mutBody.radiusM / 1000.0);
            if (ImGui::DragFloat("Radius (km)##LiveEditRkm", &rKm, 10.0f, 10.0f, 5000000.0f, "%.1f km")) {
                mutBody.radiusM = (double)rKm * 1000.0;
                mutBody.realRadiusAU = mutBody.radiusM / UnitConverter::AU_TO_METERS;
                char rBuf[64];
                snprintf(rBuf, sizeof(rBuf), "%.1f km", mutBody.radiusM / 1000.0);
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
            if (ImGui::SliderFloat("Axial Tilt (deg)##LiveEditTilt", &tilt, 0.0f, 180.0f, "%.1f deg")) {
                mutBody.axialTiltDeg = tilt;
                char tiltBuf[32];
                snprintf(tiltBuf, sizeof(tiltBuf), "%.2f deg", mutBody.axialTiltDeg);
                mutBody.axialTiltStr = tiltBuf;
            }

            // Update derived physical quantities
            if (mutBody.radiusM > 0.0 && mutBody.massKg > 0.0) {
                mutBody.surfaceGravityMps2 = (UnitConverter::G_CONST * mutBody.massKg) / (mutBody.radiusM * mutBody.radiusM);
                char gravBuf[64];
                snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s^2 (%.2f g)", mutBody.surfaceGravityMps2, mutBody.surfaceGravityMps2 / 9.80665);
                mutBody.gravityStr = gravBuf;

                double vol = (4.0 / 3.0) * UnitConverter::PI * std::pow(mutBody.radiusM, 3.0);
                mutBody.meanDensityKgM3 = mutBody.massKg / vol;
                char densBuf[64];
                snprintf(densBuf, sizeof(densBuf), "%.1f kg/m^3", mutBody.meanDensityKgM3);
                mutBody.densityStr = densBuf;

                mutBody.escapeVelocityKmpS = std::sqrt(2.0 * UnitConverter::G_CONST * mutBody.massKg / mutBody.radiusM) / 1000.0;
                char escBuf[64];
                snprintf(escBuf, sizeof(escBuf), "%.2f km/s", mutBody.escapeVelocityKmpS);
                mutBody.escapeVelocityStr = escBuf;
            }

            ImGui::Spacing();
            if (UIIcon::Button(IconId::Edit, "Open in Object Editor Workspace", ImVec2(panelW - 20, 24))) {
                m_objectWorkspaceUI.setSelectedObjectBySlug(mutBody.id, objRepo);
                m_activeTopTab = 3; // OBJECTS workspace
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
                if (UIIcon::Button(IconId::Orbit, "Circularize Orbit at Current Distance", ImVec2(panelW - 20, 24))) {
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
        StatItem(IconId::Orbit, "Semi-Major Axis", body.semiMajorAxisStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Orbit, "Eccentricity", body.eccentricityStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Target, "Perihelion (Closest)", body.periapsisStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Target, "Aphelion (Farthest)", body.apoapsisStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Physics, "Angular Momentum", body.angularMomentumStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Energy, "Orbital Energy", body.orbitalEnergyStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Time, "GR Precession", body.grPrecessionStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Ruler, "True Anomaly", body.trueAnomalyStr.c_str());
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
        StatItem(IconId::Orbit, "Bodies", std::to_string(pred.analyzedBodyCount).c_str());
        ImGui::SameLine(hw);
        char sepBuf[32];
        snprintf(sepBuf, sizeof(sepBuf), "%.2f R_H", pred.minMutualHillSep);
        StatItem(IconId::Ruler, "Min Sep", sepBuf);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::TextColored(Col::TextSecondary, "Primary Risk Factor:");
        ImGui::TextWrapped("%s", pred.primaryRiskFactor.c_str());

        ImGui::Spacing();
        UIIcon::Icon(IconId::Info, 13.0f, Col::TextSecondary);
        ImGui::SameLine();
        ImGui::TextDisabled("ML-based estimate of orbital stability.");

        if (UIIcon::Button(IconId::AI, "Open AI Analysis Studio", ImVec2(panelW - 20, 24))) {
            m_activeTopTab = 6;
        }
    }


    if (body.ring.hasRing) {
        ImGui::Separator();
        if (SectionHeader("PLANETARY RING ASTROPHYSICS & SHEAR")) {
            float hw = (panelW - 40) / 2.0f;
            
            ImGui::BeginGroup();
            StatItem(IconId::Speed, "Inner Speed (74.5k km)", "23.1 km/s (5.6h)");
            ImGui::SameLine(hw);
            StatItem(IconId::Speed, "Outer Speed (140.2k km)", "16.8 km/s (14.9h)");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Physics, "Local Gravity (g)", "6.84 -> 1.93 m/s^2");
            ImGui::SameLine(hw);
            StatItem(IconId::Speed, "Escape Velocity", "32.7 -> 23.8 km/s");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Heat, "Ring Temp. (Ice)", "85 K (-188 deg C)");
            ImGui::SameLine(hw);
            StatItem(IconId::Time, "Relativistic Drift", "-1.35 x 10^-8");
            ImGui::EndGroup();

            ImGui::BeginGroup();
            StatItem(IconId::Scale, "Total Ring Mass", "1.50 x 10^19 kg");
            ImGui::SameLine(hw);
            char actBuf[32];
            snprintf(actBuf, sizeof(actBuf), "%zu Active", body.ring.disturbances.size());
            StatItem(IconId::Phase, "Fluid State", body.ring.disturbances.empty() ? "Equilibrium" : actBuf);
            ImGui::EndGroup();

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.65f, 0.9f));
            if (UIIcon::Button(IconId::Asteroid, "Trigger Asteroid Ring Impact", ImVec2(panelW - 20, 24))) {
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
        StatItem(IconId::Sun, "Solar Radiation", body.solarRadiationStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Warning, "Radiation Level", body.radLevelStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Time, "Relativistic Drift", body.timeDilationStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Speed, "Orbital Velocity", body.orbitalSpeedStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Energy, "Magnetic Field", body.magneticFieldStr.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Sparkles, "Aurora Activity", body.auroraActivityStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("DATA SOURCE & VERIFICATION")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem(IconId::Database, "Authority", body.sourceName.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Target, "Target ID", body.sourceObjectId.empty() ? body.id.c_str() : body.sourceObjectId.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem(IconId::Target, "Ref Frame", body.referenceFrame.c_str());
        ImGui::SameLine(hw);
        StatItem(IconId::Time, "Epoch", body.epochUtcStr.c_str());
        ImGui::EndGroup();

        ImGui::Spacing();
        if (UIIcon::Button(IconId::Database, "Open Data Manager", ImVec2(panelW - 20, 24))) {
            m_showDataManager = true;
            m_dataManagerUI.selectObjectById(body.dbId, body.category);
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

    }
    ImGui::End();
    ImGui::PopStyleColor(2);
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

    // 3D Ray-Sphere Picking: Prioritize physical ray intersection with depth sorting
    // This reliably selects smaller moons in front of giant planets without ambiguity
    if (m_viewportHovered && !bodies.empty()) {
        glm::vec3 rayOrig, rayDir;
        camera.screenToWorldRay(mousePos.x, mousePos.y, vpX, vpY, vpW, vpH, rayOrig, rayDir);
        std::vector<glm::vec3> posAU;
        std::vector<float> rad3D;
        posAU.reserve(bodies.size());
        rad3D.reserve(bodies.size());
        for (const auto& b : bodies) {
            posAU.push_back(b.position);
            rad3D.push_back(b.radius3D);
        }
        int rayPickIdx = camera.pickBody(rayOrig, rayDir, posAU, rad3D, vpH, 22.0f);
        if (rayPickIdx >= 0) {
            bestHoverIdx = rayPickIdx;
        }
    }

    m_hoveredBodyIndex = bestHoverIdx;

    // Direct Left Click in 3D Viewport on body selects it
    // In EDIT mode, do not change selection if user is hovering/interacting with gizmo handles, manipulating an object, or placing
    bool canSelectBody = m_viewportHovered && !isGizmoHovered() && !isManipulatingObject() && !m_placementActive;
    if (canSelectBody && m_hoveredBodyIndex >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(m_hoveredBodyIndex);
        // Only focus camera automatically in UNIVERSE mode.
        // In EDIT mode, keep camera steady so the user can select and move without camera snapping around.
        if (m_activeTopTab == 0) {
            camera.focusOnBody(bodies[m_hoveredBodyIndex].position, bodies[m_hoveredBodyIndex].radius3D, 0.85f);
        }
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
    float visBtnX = (physics.getSelectedBodyIndex() >= 0) ? (vpX + vpW - 340.0f - 180.0f) : (vpX + vpW - 180.0f);
    ImVec2 visBtnPos(visBtnX, vpY + 10.0f);
    ImGui::SetCursorScreenPos(visBtnPos);
    static bool showVisPopup = false;
    if (UIIcon::Button(IconId::Settings, "VISUALIZATION", ImVec2(170, 26))) {
        showVisPopup = !showVisPopup;
    }

    if (showVisPopup) {
        ImGui::SetNextWindowPos(ImVec2(visBtnPos.x - 70.0f, visBtnPos.y + 32.0f), ImGuiCond_Always);
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








void UIManager::drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH) {
    ImGui::SetNextWindowPos(ImVec2(0, winH - barH));
    ImGui::SetNextWindowSize(ImVec2(winW, barH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.03f, 0.06f, 0.98f));

    ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    const CelestialBody& sel = physics.getSelectedBody();
    UIIcon::Icon(IconId::Target, 13.0f, Col::Accent);
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Col::Accent, "TARGET: %s (%s)", sel.name.c_str(), sel.type.c_str());

    ImGui::SameLine(0, 16);
    UIIcon::Icon(IconId::Ruler, 13.0f, Col::TextSecondary);
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Col::TextSecondary, "Dist: %s", sel.distanceStr.c_str());

    ImGui::SameLine(0, 16);
    UIIcon::Icon(IconId::Camera, 13.0f, Col::TextSecondary);
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Col::TextSecondary, "Cam: %.2f AU (fov %.0f deg)", camera.getDistance(), camera.getFOV());

    ImGui::SameLine(0, 16);
    UIIcon::Icon(IconId::Physics, 13.0f, Col::TextSecondary);
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Col::TextSecondary, "Engine: %s", physics.isGeneralRelativityEnabled() ? "Einstein 1PN GR" : "Newtonian");

    ImGui::SameLine(0, 16);
    UIIcon::Icon(IconId::Database, 13.0f, Col::Green);
    ImGui::SameLine(0, 4);
    if (ImGui::Selectable("Database: Active", false, 0, ImVec2(105, 14))) {
        m_showDataManager = true;
        m_dataManagerUI.openDatabaseExplorer();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("SQLite 3.46 database connected. Click to open Data Manager.");
    }

    if (winW >= 1200.0f) {
        ImGui::SameLine(winW - 220.0f);
        UIIcon::Icon(IconId::Time, 13.0f, Col::TextSecondary);
        ImGui::SameLine(0, 4);
        ImGui::TextColored(Col::TextSecondary, "Epoch: %s", sel.epochUtcStr.c_str());
    }

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
        ImGui::Text("Melting Point: %.1f K (%.0f deg C)", selMat.meltingPointK, selMat.meltingPointK - 273.15);
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

        if (UIIcon::Button(IconId::BlackHole, "Black Hole Tidal Disruption Laboratory", ImVec2(340, 28))) {
            matter.spawnBlackHoleTidalDisruptionLab();
            camera.resetOverview(glm::vec3(0.0468f, 0.0f, 0.0f), 0.12f);
            addEventLog("Black Hole Tidal Disruption spawned (Camera centered at 0.047 AU)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Spawns a 150 km asteroid on an extreme periapsis trajectory near a massive gravitational attractor. Watch differential gravity stretch, yield, and fragment the object into a tidal debris stream!");
        }

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Asteroid, "Hypervelocity Impact & Crater Fracture", ImVec2(340, 28))) {
            matter.spawnHypervelocityCollision();
            camera.resetOverview(glm::vec3(0.0f), 0.00025f);
            addEventLog("Hypervelocity Collision spawned (Camera focused on impact origin)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Collides a high-speed Iron impactor with a Basalt rock target, producing realistic contact stress, plastic deformation, impact heating, and fragmentation!");
        }

        if (UIIcon::Button(IconId::Physics, "Tensile Stress & Ductile Failure", ImVec2(340, 28))) {
            matter.spawnTensileTest();
            camera.resetOverview(glm::vec3(0.0f), 0.00010f);
            addEventLog("Tensile Test specimen spawned (Camera focused on test specimen)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Anchors a specimen on one end while applying tensile velocity to the other. Demonstrates linear elasticity, von Mises yielding, necking, and ductile fracture!");
        }

        ImGui::SameLine();
        if (UIIcon::Button(IconId::Heat, "Thermal Heating & Melting Phase Change", ImVec2(340, 28))) {
            matter.spawnThermalMeltingLab();
            camera.resetOverview(glm::vec3(0.0f), 0.00012f);
            addEventLog("Thermal Melting specimen spawned (Camera focused on melting ice block)");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Applies intense heat flux to an ice/metal block, demonstrating thermal conduction, thermal softening, melting, and fluid drop relaxation!");
        }

        ImGui::Spacing();
        if (UIIcon::Button(IconId::Delete, "Clear All Deformable Bodies", ImVec2(240, 26))) {
            matter.clearAllBodies();
            addEventLog("Cleared deformable bodies");
        }
        ImGui::SameLine(w - 120.0f);
        if (UIIcon::Button(IconId::Close, "Close", ImVec2(90, 26))) {
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

    UIIcon::Icon(IconId::Orbit, 18.0f, Col::Accent);
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Col::Accent, "ASTRONOMICAL EXPLORER & CATALOG DISCOVERY");
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
            UIIcon::Icon(isStar ? IconId::Star : IconId::Orbit, 13.0f, isStar ? Col::Yellow : Col::Accent);
            ImGui::SameLine(0, 4);
            ImGui::TextUnformatted(obj.name.c_str());

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
                ImGui::Text("%.1f km", phys->radiusM.value() / 1000.0);
            } else {
                ImGui::TextColored(Col::TextSecondary, "N/A");
            }

            ImGui::TableSetColumnIndex(5);
            ImGui::PushID((int)obj.id);
            if (UIIcon::SmallButton(IconId::Search, "Inspect")) {
                m_objectWorkspaceUI.setSelectedObjectBySlug(obj.slug, objRepo);
                m_activeTopTab = 3; // OBJECTS workspace
            }
            ImGui::SameLine();
            if (UIIcon::SmallButton(IconId::Play, "Test in Universe")) {
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

    UIIcon::Icon(IconId::Physics, 18.0f, Col::Accent);
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Col::Accent, "PHYSICS ENGINE, TIME SYSTEM & DIAGNOSTICS");
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
    if (UIIcon::Button(IconId::Scale, "Open Real-Data Validation Dashboard", ImVec2(-1, 28))) {
        m_showValidationDashboard = true;
    }
    if (UIIcon::Button(IconId::Asteroid, "Open Asteroid Belt Statistical Tool (N(a))", ImVec2(-1, 28))) {
        m_showAsteroidBeltDiagnostics = true;
    }
    if (UIIcon::Button(IconId::Heat, "Open Deformable Matter Impact Lab", ImVec2(-1, 28))) {
        m_showMatterLab = true;
    }
    if (UIIcon::Button(IconId::Speed, "Open Engine Performance Profiler", ImVec2(-1, 28))) {
        m_requestOpenProfiler = true;
        addEventLog("Performance Profiler window opened");
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

    if (UIIcon::Button(IconId::Orbit, "Return to UNIVERSE View", ImVec2(-1, 32))) {
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
    UIIcon::Icon(IconId::AI, 18.0f, Col::Accent);
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Col::Accent, "ASTROGENESIS AI & MACHINE LEARNING STUDIO");
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
    ImGui::Text("  - Primary Host Star Mass: %.3f M_Sun", feat.starMassKg / 1.9885e30);
    ImGui::Text("  - Number of Orbiting Bodies: %d", feat.bodyCount);
    ImGui::Text("  - Min Mutual Hill Separation: %.2f R_Hill", feat.minMutualHillSep);
    ImGui::Text("  - Max Planetary Eccentricity: %.4f", feat.maxEccentricity);
    ImGui::Text("  - Angular Momentum Deficit (AMD): %.5f", feat.angularMomentumDeficit);
    ImGui::Text("  - Planetary Orbit Crossing: %s", feat.hasOrbitCrossing ? "YES (CRITICAL RISK)" : "NO (CLEAR)");
    ImGui::Text("  - Inference Latency: %.1f us", pred.inferenceTimeUs);

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Yellow, "Risk Factor Diagnostics:");
    if (pred.riskFactors.empty()) {
        UIIcon::Icon(IconId::Check, 13.0f, Col::Green);
        ImGui::SameLine(0, 4);
        ImGui::TextColored(Col::Green, "All orbital separation criteria satisfied.");
    } else {
        for (const auto& risk : pred.riskFactors) {
            UIIcon::Icon(IconId::Warning, 13.0f, Col::Orange);
            ImGui::SameLine(0, 4);
            ImGui::TextColored(Col::Orange, "%s", risk.c_str());
        }
    }

    ImGui::Spacing();
    if (UIIcon::Button(IconId::Reset, "Force Re-Evaluate Simulation State", ImVec2(-1, 26))) {
        aiManager.forceRecompute(physics);
    }

    ImGui::Spacing();
    UIIcon::Icon(IconId::Info, 13.0f, Col::TextSecondary);
    ImGui::SameLine(0, 4);
    ImGui::TextDisabled("Scientific Honesty: ML-based estimate of stability from trained dynamical patterns. Does not replace symplectic physics integrator.");

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
        if (UIIcon::Button(IconId::Energy, "Apply Perturbation to Live Physics", ImVec2(-1, 26))) {
            if (perturbIdx >= 0 && perturbIdx < (int)physics.getBodies().size()) {
                auto& mut = physics.getBodies()[perturbIdx];
                mut.eccentricity = (double)whatIfEcc;
                mut.semiMajorAxisAU = (double)whatIfSmaAU;
                mut.semiMajorAxisM = (double)whatIfSmaAU * UnitConverter::AU_TO_METERS;
                mut.massKg = (double)whatIfMassM * UnitConverter::EARTH_MASS_KG;
                aiManager.forceRecompute(physics);
            }
        }

        if (UIIcon::Button(IconId::Reset, "Reset Sliders to Live Values", ImVec2(-1, 24))) {
            whatIfEcc = (float)bodies[perturbIdx].eccentricity;
            whatIfSmaAU = (float)((bodies[perturbIdx].semiMajorAxisAU > 0.0) ? bodies[perturbIdx].semiMajorAxisAU : bodies[perturbIdx].distanceAU);
            whatIfMassM = (float)(bodies[perturbIdx].massKg / UnitConverter::EARTH_MASS_KG);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    if (UIIcon::Button(IconId::Orbit, "Return to UNIVERSE Simulation", ImVec2(-1, 30))) {
        m_activeTopTab = 0;
    }

    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace AstroGenesis

