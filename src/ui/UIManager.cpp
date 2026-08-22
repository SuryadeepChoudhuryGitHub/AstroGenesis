#include "ui/UIManager.hpp"
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
                         float windowWidth, float windowHeight, float fps) {
    float topBarH    = 48.0f;
    float statusBarH = 28.0f;
    float leftPanelW = 210.0f;
    float rightPanelW = 310.0f;
    float bottomH    = 180.0f;

    m_viewportX = leftPanelW;
    m_viewportY = topBarH;
    m_viewportW = windowWidth - leftPanelW - rightPanelW;
    m_viewportH = windowHeight - topBarH - bottomH - statusBarH;

    ImVec2 mousePos = ImGui::GetMousePos();
    ImGuiIO& io = ImGui::GetIO();
    m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                         mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH) && !io.WantCaptureMouse;

    // 1. Top Bar (can switch systems or open modals)
    drawTopBar(windowWidth, physics, camera, objRepo);

    // 2. Left Hierarchy & Navigation Panel (Selection Place 1, can switch systems or select bodies)
    drawLeftPanel(physics, camera, objRepo, topBarH, statusBarH, windowHeight);

    // 3. 3D Viewport HUD Direct Hover & Click (Selection Place 2)
    drawViewportHUD(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

    // 4. Bottom Row Cards (including 2D Orbit Vis Schematic: Selection Place 3)
    float bottomY = windowHeight - statusBarH - bottomH;
    float bpW = m_viewportW / 3.0f;
    drawTimeControls(physics, camera, objRepo, leftPanelW, bottomY, bpW, bottomH);
    drawSimMetrics  (physics, fps, leftPanelW + bpW,      bottomY, bpW, bottomH);
    drawOrbitVis    (physics, camera, leftPanelW + bpW * 2, bottomY, bpW, bottomH);

    // Dynamic fetch of current selected body AFTER all interactions and system loads
    const CelestialBody& currentBody = physics.getSelectedBody();

    // 5. Floating Info Overlay
    drawInfoOverlay(currentBody, m_viewportX, m_viewportY);

    // 6. Right Scientific Data Panel
    drawRightPanel(physics, currentBody, dataManager, topBarH, windowWidth, windowHeight, statusBarH);

    // 7. Status Bar
    drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);

    // 8. Modals / Overlay Windows
    if (m_showAsteroidBeltDiagnostics) {
        drawAsteroidBeltDiagnostics(physics, objRepo, windowWidth, windowHeight);
    }
    if (m_showMatterLab) {
        drawMatterLab(physics, windowWidth, windowHeight);
    }
    if (m_showDataManager) {
        m_dataManagerUI.render(m_showDataManager, dataManager, objRepo, physics, windowWidth, windowHeight);
    }
    if (m_showValidationDashboard) {
        m_validationUI.render(m_showValidationDashboard, valEngine, objRepo, physics, windowWidth, windowHeight);
    }
}

void UIManager::drawTopBar(float width, PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo) {
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
    ImGui::SameLine(0, 32);

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

    // Top Bar Action Buttons: Reset Workspace, Data Manager, Validation, Asteroids, Matter Lab
    float rightOffset = width - 830.0f;
    ImGui::SameLine(rightOffset);

    // 0. RESET WORKSPACE (Clean Start)
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

    // 1. DATA MANAGER
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

    // 2. VALIDATION
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

    // 3. ASTEROID BELT
    ImGui::SameLine(0, 5);
    if (m_showAsteroidBeltDiagnostics) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.75f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.16f, 0.26f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button("☄ ASTEROID BELT (N(a))", ImVec2(185, 28))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    ImGui::PopStyleColor(2);

    // 4. DEFORMABLE MATTER LAB
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

void UIManager::drawRightPanel(PhysicsEngine& physics, const CelestialBody& body, DataManager& dataManager, float topBarH, float winW, float winH, float statusBarH) {
    float panelW = 310.0f;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(winW - panelW, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##RightPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

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
void UIManager::drawViewportHUD(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH) {
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
                ImVec2 pt1(center.x + body.dynamicOrbitCurve[s].x * scale,
                           center.y + body.dynamicOrbitCurve[s].z * scale);
                ImVec2 pt2(center.x + body.dynamicOrbitCurve[s + 1].x * scale,
                           center.y + body.dynamicOrbitCurve[s + 1].z * scale);
                dl->AddLine(pt1, pt2, orbitLineCol, 1.2f);
            }
        } else {
            double orbitRadiusAU = (body.realOrbitRadiusAU > 0.0) ? body.realOrbitRadiusAU : (body.semiMajorAxisAU > 0.0 ? body.semiMajorAxisAU : (double)glm::length(body.position));
            if (orbitRadiusAU > 0.00001) {
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

void UIManager::drawMatterLab(PhysicsEngine& physics, float winW, float winH) {
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
            addEventLog("Black Hole Tidal Disruption spawned");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Spawns a 150 km asteroid on an extreme periapsis trajectory near a massive gravitational attractor. Watch differential gravity stretch, yield, and fragment the object into a tidal debris stream!");
        }

        ImGui::SameLine();
        if (ImGui::Button("\xE2\x98\x84 Hypervelocity Impact & Crater Fracture", ImVec2(340, 28))) {
            matter.spawnHypervelocityCollision();
            addEventLog("Hypervelocity Collision spawned");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Collides a high-speed Iron impactor with a Basalt rock target, producing realistic contact stress, plastic deformation, impact heating, and fragmentation!");
        }

        if (ImGui::Button("\xE2\x9A\xA1 Tensile Stress & Necking / Ductile Failure", ImVec2(340, 28))) {
            matter.spawnTensileTest();
            addEventLog("Tensile Test specimen spawned");
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Anchors a specimen on one end while applying tensile velocity to the other. Demonstrates linear elasticity, von Mises yielding, necking, and ductile fracture!");
        }

        ImGui::SameLine();
        if (ImGui::Button("\xF0\x9F\x94\xA5 Thermal Heating & Melting Phase Change", ImVec2(340, 28))) {
            matter.spawnThermalMeltingLab();
            addEventLog("Thermal Melting specimen spawned");
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

} // namespace AstroGenesis
