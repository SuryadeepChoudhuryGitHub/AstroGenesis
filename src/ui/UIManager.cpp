#include "ui/UIManager.hpp"
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

static void InfoRow(const char* label, const std::string& value, float labelWidth = 135.0f) {
    ImGui::TextColored(Col::TextSecondary, "%s", label);
    ImGui::SameLine(labelWidth);
    ImGui::TextColored(Col::TextPrimary, "%s", value.c_str());
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

    // Seed default astrophysical events in the event log
    m_eventLogs.clear();
    m_eventLogs.push_back({ "00:15:30", "Simulation started" });
    m_eventLogs.push_back({ "00:15:31", "Loaded Solar System" });
    m_eventLogs.push_back({ "00:15:32", "Einstein GR (1PN) engine active" });
    m_eventLogs.push_back({ "00:15:34", "Earth selected" });
    m_eventLogs.push_back({ "00:15:35", "Time acceleration set to 1 day/s" });
}

void UIManager::addEventLog(const std::string& message) {
    char buf[16];
    static int logSec = 36;
    snprintf(buf, sizeof(buf), "00:15:%02d", logSec++);
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

void UIManager::renderUI(PhysicsEngine& physics, Camera& camera, float windowWidth, float windowHeight, float fps) {
    float topBarH     = 48.0f;
    float leftPanelW  = 230.0f;
    float rightPanelW = 340.0f;
    float bottomH     = 220.0f;

    m_viewportX = leftPanelW;
    m_viewportY = topBarH;
    m_viewportW = windowWidth - leftPanelW - rightPanelW;
    m_viewportH = windowHeight - topBarH - bottomH;

    // Determine mouse hovering viewport (must be within 3D rect AND not captured by any ImGui window/widget)
    ImVec2 mousePos = ImGui::GetMousePos();
    ImGuiIO& io = ImGui::GetIO();
    m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                         mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH) && !io.WantCaptureMouse;

    const CelestialBody& currentBody = physics.getSelectedBody();

    // 1. Top Bar
    drawTopBar(windowWidth);

    // 2. Left Hierarchy & Navigation Panel
    drawLeftPanel(physics, camera, topBarH, windowHeight);

    // 3. Center Viewport Header & Floating Info Cards
    drawCenterViewportHeader(currentBody, camera, m_viewportX, m_viewportY, m_viewportW);
    drawFloatingInfoCards(currentBody, m_viewportX, m_viewportY);

    // 4. Right Scientific Data Panel
    drawRightPanel(physics, currentBody, topBarH, windowWidth, windowHeight);

    // 5. 3D Viewport HUD (Interactive decluttered hover reticle)
    drawViewportHUD(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

    // 6. Bottom Row (3 Beautiful Cards: Time Controls, Physics Engine, Orbit Visualizer)
    float bottomY = windowHeight - bottomH;
    float cardW = m_viewportW / 3.0f;
    drawTimeControls(physics, leftPanelW,                 bottomY, cardW, bottomH);
    drawSimMetrics  (physics, fps, leftPanelW + cardW,    bottomY, cardW, bottomH);
    drawOrbitVis    (physics, camera, leftPanelW + cardW * 2.0f, bottomY, cardW, bottomH);

    // 7. Modals
    if (m_showAsteroidBeltDiagnostics) {
        drawAsteroidBeltDiagnostics(physics, windowWidth, windowHeight);
    }
    if (m_showMatterLab) {
        drawMatterLab(physics, windowWidth, windowHeight);
    }
}

// =========================================================================
// 1. TOP BAR
// =========================================================================

void UIManager::drawTopBar(float width) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, 48));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.024f, 0.035f, 0.065f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, Col::Border);

    ImGui::Begin("##TopBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    // Logo & Engine Title
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("\xE2\x97\x86"); // Diamond logo icon
    ImGui::SameLine(0, 6);
    ImGui::Text("ASTROGENESIS");
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 8);
    ImGui::TextColored(Col::TextSecondary, "\xE2\x9C\xA6 SPACE SIMULATION ENGINE");
    ImGui::SameLine(0, 32);

    // Pill Navigation Tabs
    const char* tabs[] = { "UNIVERSE", "SYSTEM", "OBJECTS", "EXPLORE", "SIMULATION", "AI ASSISTANT" };
    static int activeTab = 0;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool isActive = (i == activeTab);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Col::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
            ImGui::PushStyleColor(ImGuiCol_Border, Col::AccentDim);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }
        if (ImGui::Button(tabs[i], ImVec2(0, 28))) activeTab = i;
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    // Top Right Action Buttons: Asteroid Belt, Deformable Matter Lab, Settings & Help
    ImGui::SameLine(width - 540.0f);
    if (m_showAsteroidBeltDiagnostics) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.40f, 0.70f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.14f, 0.24f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button("☄ ASTEROID BELT (N-BODY)", ImVec2(200, 28))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine(width - 330.0f);
    if (m_showMatterLab) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.60f, 0.30f, 0.12f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.14f, 0.11f, 0.20f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.70f, 0.40f, 1.0f));
    }
    if (ImGui::Button("⬡ DEFORMABLE MATTER LAB", ImVec2(215, 28))) {
        m_showMatterLab = !m_showMatterLab;
    }
    ImGui::PopStyleColor(2);

    // Settings & Help icon buttons
    ImGui::SameLine(width - 100.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.20f, 0.75f));
    ImGui::Button("⚙##Settings", ImVec2(28, 28));
    ImGui::SameLine(0, 6);
    ImGui::Button("?##Help", ImVec2(28, 28));
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 2. LEFT PANEL (HIERARCHY & VIEW CONTROLS)
// =========================================================================

void UIManager::drawLeftPanel(PhysicsEngine& physics, Camera& camera, float topBarH, float winH) {
    float panelW = 230.0f;
    float panelH = winH - topBarH;
    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(10, 10));

    ImGui::Begin("##LeftPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    // Search bar with filter icon
    ImGui::PushItemWidth(panelW - 55);
    ImGui::InputTextWithHint("##search", "Search objects...", m_searchQuery, sizeof(m_searchQuery));
    ImGui::PopItemWidth();
    ImGui::SameLine(0, 4);
    ImGui::Button("∇##Filter", ImVec2(28, 24));
    ImGui::Spacing();
    ImGui::Separator();

    // SOLAR SYSTEM Section
    if (SectionHeader("SOLAR SYSTEM", true)) {
        const auto& bodies = physics.getBodies();
        int selectedIndex = physics.getSelectedBodyIndex();

        for (int i = 0; i < (int)bodies.size(); ++i) {
            // Apply search filter if active
            if (m_searchQuery[0] != '\0') {
                std::string bName = bodies[i].name;
                std::string q = m_searchQuery;
                std::transform(bName.begin(), bName.end(), bName.begin(), ::tolower);
                std::transform(q.begin(), q.end(), q.begin(), ::tolower);
                if (bName.find(q) == std::string::npos) continue;
            }

            bool isSelected = (i == selectedIndex);
            ImDrawList* dl = ImGui::GetWindowDrawList();

            ImGui::PushID(i);
            ImVec2 pMin = ImGui::GetCursorScreenPos();
            ImVec2 pMax = ImVec2(pMin.x + panelW - 20, pMin.y + 32);

            // Background pill for selected / hover
            if (isSelected) {
                dl->AddRectFilled(pMin, pMax, ImGui::ColorConvertFloat4ToU32(Col::SelectedBg), 6.0f);
                dl->AddRect(pMin, pMax, ImGui::ColorConvertFloat4ToU32(Col::SelectedBorder), 6.0f, 0, 1.2f);
            }

            if (ImGui::InvisibleButton("##item", ImVec2(panelW - 20, 32))) {
                physics.selectBody(i);
                camera.focusOnBody(bodies[i].position, bodies[i].radius3D, 0.85f);
                addEventLog(bodies[i].name + " selected");
            }

            bool isHovered = ImGui::IsItemHovered();
            if (isHovered && !isSelected) {
                dl->AddRectFilled(pMin, pMax, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 0.05f)), 6.0f);
            }

            // Planet colored circle dot
            ImVec2 dotCenter = ImVec2(pMin.x + 16, pMin.y + 16);
            ImU32 dotCol = ImGui::ColorConvertFloat4ToU32(ImVec4(bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 1.0f));
            dl->AddCircleFilled(dotCenter, isSelected ? 5.5f : 4.5f, dotCol);
            if (isSelected) {
                dl->AddCircle(dotCenter, 8.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent), 16, 1.2f);
            }

            // Planet Name
            dl->AddText(ImVec2(pMin.x + 30, pMin.y + 8),
                        isSelected ? ImGui::ColorConvertFloat4ToU32(Col::TextPrimary) : ImGui::ColorConvertFloat4ToU32(Col::TextSecondary),
                        bodies[i].name.c_str());

            // Right-aligned Distance
            std::string distStr = (bodies[i].id == "sol") ? "0.00 AU" : bodies[i].distanceStr;
            ImVec2 distSize = ImGui::CalcTextSize(distStr.c_str());
            dl->AddText(ImVec2(pMax.x - distSize.x - 8, pMin.y + 8),
                        isSelected ? ImGui::ColorConvertFloat4ToU32(Col::Accent) : ImGui::ColorConvertFloat4ToU32(Col::TextSecondary),
                        distStr.c_str());

            ImGui::PopID();
        }
    }

    ImGui::Separator();
    const char* collapsedSections[] = {
        "EXOPLANET SYSTEMS",
        "STAR CLUSTERS",
        "GALAXIES",
        "DEEP SPACE OBJECTS",
        "FAVORITES"
    };
    for (auto& sec : collapsedSections) {
        SectionHeader(sec, false);
    }

    // Bottom "VIEW CONTROLS" Area
    float viewCtrlH = 175.0f;
    float viewCtrlY = panelH - viewCtrlH - 10.0f;
    ImGui::SetCursorPos(ImVec2(10, viewCtrlY));
    ImGui::Separator();
    ImGui::TextColored(Col::Accent, "VIEW CONTROLS");

    // Mouse Navigation Graphic Schematic
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 mouseDiagPos = ImGui::GetCursorScreenPos();
    ImVec2 mouseCenter = ImVec2(mouseDiagPos.x + 85, mouseDiagPos.y + 30);

    // Draw mouse silhouette & crosshairs
    dl->AddRect(ImVec2(mouseCenter.x - 14, mouseCenter.y - 20), ImVec2(mouseCenter.x + 14, mouseCenter.y + 20),
                ImGui::ColorConvertFloat4ToU32(Col::BorderLight), 10.0f, 0, 1.2f);
    dl->AddLine(ImVec2(mouseCenter.x, mouseCenter.y - 20), ImVec2(mouseCenter.x, mouseCenter.y - 6),
                ImGui::ColorConvertFloat4ToU32(Col::BorderLight), 1.2f);
    dl->AddCircleFilled(ImVec2(mouseCenter.x, mouseCenter.y - 12), 3.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent));

    // Axis indicators
    dl->AddLine(ImVec2(mouseCenter.x - 40, mouseCenter.y), ImVec2(mouseCenter.x + 40, mouseCenter.y),
                ImGui::ColorConvertFloat4ToU32(ImVec4(Col::AccentDim.x, Col::AccentDim.y, Col::AccentDim.z, 0.35f)), 1.0f);
    dl->AddCircle(ImVec2(mouseCenter.x - 30, mouseCenter.y), 4.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent), 12, 1.0f);

    ImGui::Dummy(ImVec2(0, 52));

    // Quick Tool Icons Row
    if (ImGui::Button("⤓##ZoomIn", ImVec2(29, 24))) { camera.processMouseZoom(1.0f); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Zoom In (+)"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("⤒##ZoomOut", ImVec2(29, 24))) { camera.processMouseZoom(-1.0f); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Zoom Out (-)"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("⛶##ResetV", ImVec2(29, 24))) { camera.resetCenter(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reset Center"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("🏷##Labels", ImVec2(29, 24))) { /* Toggle labels */ }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Toggle Labels"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("🧭##North", ImVec2(29, 24))) { camera.resetCenter(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Align Ecliptic"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("📈##Stats", ImVec2(29, 24))) { m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics; }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Asteroid Belt Analysis"); }

    ImGui::Spacing();
    if (ImGui::Button("RESET VIEW", ImVec2(panelW - 20, 26))) {
        camera.resetCenter();
    }

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 3. CENTER VIEWPORT HEADER & SUB-NAVIGATION TABS
// =========================================================================

void UIManager::drawCenterViewportHeader(const CelestialBody& body, Camera& camera, float x, float y, float w) {
    ImGui::SetNextWindowPos(ImVec2(x + 16, y + 12));
    ImGui::SetNextWindowSize(ImVec2(w - 32, 40));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 6);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 6));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.02f, 0.035f, 0.07f, 0.0f)); // Transparent
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));

    ImGui::Begin("##CenterHeader", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    // Large Body Title (e.g. EARTH) with cyan initial letter
    std::string uppercaseName = body.name;
    std::transform(uppercaseName.begin(), uppercaseName.end(), uppercaseName.begin(), ::toupper);

    ImGui::SetWindowFontScale(1.15f);
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("%c", uppercaseName[0]);
    ImGui::PopStyleColor();
    ImGui::SameLine(0, 0);
    ImGui::Text("%s", uppercaseName.substr(1).c_str());
    ImGui::SetWindowFontScale(1.0f);
    ImGui::SameLine(0, 6);
    ImGui::TextColored(Col::Yellow, "\xE2\x98\x86"); // Star icon
    ImGui::SameLine(0, 4);
    ImGui::TextColored(Col::TextSecondary, "-");
    ImGui::SameLine(0, 20);

    // Sub-navigation pill tabs: OVERVIEW, INFO, PHYSICAL, ORBIT, ATMOSPHERE, COMPOSITION, HISTORY
    const char* subTabs[] = { "OVERVIEW", "INFO", "PHYSICAL", "ORBIT", "ATMOSPHERE", "COMPOSITION", "HISTORY" };
    for (int i = 0; i < 7; ++i) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool isActive = (i == m_centerSubTab);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Col::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
            ImGui::PushStyleColor(ImGuiCol_Border, Col::Accent);
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 1.0f);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.05f, 0.08f, 0.14f, 0.65f));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
            ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 0.0f);
        }
        if (ImGui::Button(subTabs[i], ImVec2(0, 24))) {
            m_centerSubTab = i;
        }
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(3);
    }

    // Viewport Top-Right Floating Tool Icons
    ImGui::SameLine(w - 140.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.06f, 0.10f, 0.18f, 0.85f));
    if (ImGui::Button("⚙##VPSet", ImVec2(26, 24))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Simulation Settings"); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("⌖##VPTarget", ImVec2(26, 24))) {
        camera.focusOnBody(body.position, body.radius3D, 0.85f);
        addEventLog("Focused camera on " + body.name);
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Center on %s", body.name.c_str()); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("⛶##VPFull", ImVec2(26, 24))) {
        camera.resetCenter();
        addEventLog("Reset Camera View");
    }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reset Camera Orbit"); }
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 4. CENTER FLOATING INFORMATION CARDS
// =========================================================================

void UIManager::drawFloatingInfoCards(const CelestialBody& body, float x, float y) {
    float cardW = 270.0f;
    float startY = y + 52.0f;

    // Card 1: BASIC INFORMATION
    ImGui::SetNextWindowPos(ImVec2(x + 16, startY));
    ImGui::SetNextWindowSize(ImVec2(cardW, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.024f, 0.038f, 0.075f, 0.85f)); // Translucent glassmorphism
    ImGui::PushStyleColor(ImGuiCol_Border, Col::BorderLight);

    if (ImGui::Begin("##BasicInfoCard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(Col::Accent, "BASIC INFORMATION");
        ImGui::Separator();

        InfoRow("Type",                 body.type);
        InfoRow("Distance from Sol",     (body.id == "sol") ? "0.00 AU" : body.distanceStr);
        if (body.id != "sol") {
            InfoRow("Orbital Velocity",  body.orbitalSpeedStr);
        }
        InfoRow("Radius",               body.radiusStr);
        InfoRow("Mass",                 body.massStr);
        InfoRow("Surface Gravity",      body.gravityStr);
        InfoRow("Escape Velocity",      body.escapeVelocityStr);
        InfoRow("Rotation Period",      body.rotationPeriodStr);
        InfoRow("Orbital Period (Year)",(body.id == "sol") ? "N/A" : body.orbitalPeriodStr);
        InfoRow("Mean Temperature",     body.tempStr);
        InfoRow("Moons",                std::to_string(body.moons));

        ImGui::End();
    }

    // Card 2: LOCATION & COORDINATES
    float card2Y = startY + 280.0f;
    ImGui::SetNextWindowPos(ImVec2(x + 16, card2Y));
    ImGui::SetNextWindowSize(ImVec2(cardW, 0));

    if (ImGui::Begin("##LocationCard", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::TextColored(Col::Accent, "LOCATION & COORDINATES");
        ImGui::Separator();

        // Calculate astronomical coordinates
        double raHours = std::fmod(std::abs(body.positionM.x * 1e-10) * 24.0, 24.0);
        int raH = (int)raHours;
        int raM = (int)((raHours - raH) * 60.0);
        int raS = (int)(((raHours - raH) * 60.0 - raM) * 60.0);
        char raBuf[32];
        snprintf(raBuf, sizeof(raBuf), "%02dh %02dm %02ds", raH, raM, raS);

        double decDeg = std::clamp((body.positionM.z * 1e-10) * 90.0, -89.9, 89.9);
        int decD = (int)decDeg;
        int decM = std::abs((int)((decDeg - decD) * 60.0));
        char decBuf[32];
        snprintf(decBuf, sizeof(decBuf), "%+03d° %02d'", decD, decM);

        InfoRow("Right Ascension",  raBuf);
        InfoRow("Declination",      decBuf);
        InfoRow("Galactic Latitude", "-60.19°");
        InfoRow("Galactic Longitude","96.63°");

        ImGui::End();
    }

    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 5. RIGHT PANEL (PHYSICAL OVERVIEW, ORBITAL MECHANICS, RELATIVITY & LOGS)
// =========================================================================

void UIManager::drawRightPanel(PhysicsEngine& physics, const CelestialBody& body, float topBarH, float winW, float winH) {
    float panelW = 340.0f;
    float panelH = winH - topBarH;
    ImGui::SetNextWindowPos(ImVec2(winW - panelW, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 10));

    ImGui::Begin("##RightPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    float halfW = (panelW - 35) * 0.5f;

    // Card 1: PHYSICAL OVERVIEW
    if (SectionHeader("PHYSICAL OVERVIEW", true)) {
        StatCard2Col("Gravity", body.gravityStr.c_str(), "Escape Velocity", body.escapeVelocityStr.c_str(), halfW);
        StatCard2Col("Surface Temp.", body.tempStr.c_str(), "Atmospheric Pressure", body.pressureStr.c_str(), halfW);
        StatCard2Col("Mean Density", body.densityStr.c_str(), "Day Length", body.rotationPeriodStr.c_str(), halfW);
        StatCard2Col("Year Length", body.yearLengthStr.c_str(), "Surface Area", body.surfaceAreaStr.c_str(), halfW);
    }

    ImGui::Separator();

    // Card 2: ORBITAL MECHANICS
    if (SectionHeader("ORBITAL MECHANICS", true)) {
        StatCard2Col("Semi-Major Axis (a)", body.semiMajorAxisStr.c_str(), "Eccentricity (e)", body.eccentricityStr.c_str(), halfW);
        StatCard2Col("Perihelion", body.periapsisStr.c_str(), "Aphelion", body.apoapsisStr.c_str(), halfW);
        StatCard2Col("Inclination (i)", "0.00005°", "Angular Momentum", body.angularMomentumStr.c_str(), halfW);
        StatCard2Col("Orbital Energy", body.orbitalEnergyStr.c_str(), "GR Precession", body.grPrecessionStr.c_str(), halfW);
    }

    ImGui::Separator();

    // Card 3: RADIATION & RELATIVITY
    if (SectionHeader("RADIATION & RELATIVITY", true)) {
        StatCard2Col("Solar Radiation", body.solarRadiationStr.c_str(), "Relativistic Drift", body.timeDilationStr.c_str(), halfW);
        StatCard2Col("Magnetic Field", body.magneticFieldStr.c_str(), "Aurora Activity", body.auroraActivityStr.c_str(), halfW);
        StatCard2Col("Radiation Level", body.radLevelStr.c_str(), "Orbital Velocity", body.orbitalSpeedStr.c_str(), halfW);
    }

    ImGui::Separator();

    // Card 4: ATMOSPHERIC COMPOSITION (Donut Chart & Legend)
    if (SectionHeader("ATMOSPHERIC COMPOSITION", true)) {
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

            // Donut hole
            dl->AddCircleFilled(chartCenter, innerRadius, ImGui::ColorConvertFloat4ToU32(Col::BgChild), 32);

            // Legend on the right side
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

    // Card 5: EVENT LOG (Simulation Live Feed)
    ImGui::TextColored(Col::Accent, "EVENT LOG");
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.02f, 0.035f, 0.065f, 0.90f));
    ImGui::BeginChild("##EventLogChild", ImVec2(panelW - 24, 90), true);

    for (const auto& log : m_eventLogs) {
        ImGui::TextColored(Col::Accent, "%s", log.timeStr.c_str());
        ImGui::SameLine(0, 8);
        ImGui::TextColored(Col::TextSecondary, "%s", log.message.c_str());
    }

    ImGui::EndChild();
    ImGui::PopStyleColor();

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 6. BOTTOM ROW: TIME CONTROLS, PHYSICS ENGINE & ORBIT VISUALIZATION
// =========================================================================

void UIManager::drawTimeControls(PhysicsEngine& physics, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

    ImGui::Begin("##TimeControls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "TIME CONTROLS");
    ImGui::Separator();

    bool isPaused = physics.isPaused();
    float btnW = 28.0f;

    // Transport buttons: |<  <<  >  ||  >>  >|
    if (ImGui::Button("|<##Rewind", ImVec2(btnW, 24))) { physics.stepFrameBackward(); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("<<##Slow", ImVec2(btnW, 24))) { physics.setTimeScale(std::max(1.0f, physics.getTimeScale() * 0.1f)); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button(isPaused ? " > ##Play" : " || ##Pause", ImVec2(btnW + 4, 24))) { physics.togglePause(); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button(">>##Fast", ImVec2(btnW, 24))) { physics.setTimeScale(std::min(31536000.0f, physics.getTimeScale() * 10.0f)); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button(">|##Step", ImVec2(btnW, 24))) { physics.stepFrameForward(); }

    // Speed Slider
    ImGui::Spacing();
    float scale = physics.getTimeScale();
    ImGui::PushItemWidth(w - 75.0f);
    if (ImGui::SliderFloat("##SpeedSlider", &scale, 1.0f, 31536000.0f, "", ImGuiSliderFlags_Logarithmic)) {
        physics.setTimeScale(scale);
    }
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::TextColored(Col::Accent, "%.2f×", scale / 86400.0f);

    // Speed Presets: 0.1x, 1x, 10x, 100x, 1000x
    ImGui::Spacing();
    float pW = (w - 48.0f) / 5.0f;
    float presets[] = { 8640.0f, 86400.0f, 864000.0f, 8640000.0f, 86400000.0f };
    const char* presetLabels[] = { "0.1×", "1×", "10×", "100×", "1000×" };

    for (int p = 0; p < 5; ++p) {
        if (p > 0) ImGui::SameLine(0, 4);
        bool isActive = (std::abs(physics.getTimeScale() - presets[p]) < 1.0f);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Col::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.20f, 0.75f));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::Button(presetLabels[p], ImVec2(pW, 22))) {
            physics.setTimeScale(presets[p]);
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    ImGui::TextColored(Col::TextSecondary, "SIMULATION TIME");
    ImGui::TextColored(Col::TextPrimary, "2024-07-04 00:15:37 UTC");

    ImGui::TextColored(Col::TextSecondary, "SIMULATION SPEED");
    ImGui::SameLine(130);
    ImGui::TextColored(Col::Accent, "%s", physics.getSimulationTimeStr().c_str());

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void UIManager::drawSimMetrics(PhysicsEngine& physics, float fps, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

    ImGui::Begin("##SimMetrics", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "PHYSICS & RELATIVITY ENGINE");
    ImGui::Separator();

    InfoRow("Engine Mode",      physics.isGeneralRelativityEnabled() ? "Einstein GR (1PN)" : "Newtonian Gravity", 125.0f);
    InfoRow("Integrator",       "Verlet (Symplectic)", 125.0f);
    InfoRow("Bodies",           std::to_string(physics.getObjectCount()), 125.0f);
    InfoRow("Step Time",        "2.45 ms", 125.0f);
    InfoRow("Total System Energy", physics.getTotalEnergyStr(), 125.0f);
    InfoRow("Angular Momentum", physics.getTotalAngularMomentumStr(), 125.0f);
    InfoRow("Time Flow",        physics.getSimVsRealTimeStr(), 125.0f);

    ImGui::Spacing();

    // Engine Active / Stable Status Pill Button
    bool grOn = physics.isGeneralRelativityEnabled();
    ImGui::PushStyleColor(ImGuiCol_Button, grOn ? ImVec4(0.08f, 0.35f, 0.18f, 0.90f) : ImVec4(0.35f, 0.18f, 0.08f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Text, grOn ? Col::Green : Col::Orange);

    if (ImGui::Button(grOn ? "ENGINE ACTIVE  🟢 STABLE" : "NEWTONIAN ONLY  🟡 BASIC", ImVec2(w - 28, 24))) {
        physics.toggleGeneralRelativity();
    }
    ImGui::PopStyleColor(2);

    ImGui::End();
    ImGui::PopStyleVar(2);
}

void UIManager::drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(14, 10));

    ImGui::Begin("##OrbitVis", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored(Col::Accent, "ORBIT VISUALIZATION");

    // Top Right Icon Buttons: [ ⬡ ], [ + ], [ - ], [ ⛶ ]
    ImGui::SameLine(w - 110.0f);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.20f, 0.75f));
    if (ImGui::Button("⬡##Vis1", ImVec2(22, 20))) { m_orbitVisZoom = 1.0f; }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Fit View"); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("+##ZoomIn", ImVec2(22, 20))) { m_orbitVisZoom = std::min(30.0f, m_orbitVisZoom * 1.25f); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Zoom In (+)"); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("-##ZoomOut", ImVec2(22, 20))) { m_orbitVisZoom = std::max(0.15f, m_orbitVisZoom * 0.8f); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Zoom Out (-)"); }
    ImGui::SameLine(0, 3);
    if (ImGui::Button("⛶##Reset", ImVec2(22, 20))) { m_orbitVisZoom = 1.0f; }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Reset Zoom"); }
    ImGui::PopStyleColor();

    ImGui::Separator();

    ImVec2 contentMin = ImGui::GetCursorScreenPos();
    ImVec2 contentMax = ImVec2(x + w - 14.0f, y + h - 36.0f);
    float areaW = contentMax.x - contentMin.x;
    float areaH = contentMax.y - contentMin.y;
    float halfSize = std::min(areaW, areaH) * 0.46f;
    ImVec2 center = ImVec2(contentMin.x + areaW * 0.5f, contentMin.y + areaH * 0.5f);

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    ImVec2 mousePos = ImGui::GetMousePos();
    bool panelHovered = (mousePos.x >= x && mousePos.x <= x + w && mousePos.y >= y && mousePos.y <= y + h);

    // Scroll wheel zoom within orbit visualization
    if (panelHovered && io.MouseWheel != 0.0f) {
        m_orbitVisZoom *= (io.MouseWheel > 0.0f) ? 1.18f : 0.85f;
        m_orbitVisZoom = std::clamp(m_orbitVisZoom, 0.15f, 30.0f);
    }

    float maxAU = 32.0f / m_orbitVisZoom;
    float scale = halfSize / maxAU;

    const auto& bodies = physics.getBodies();
    int selectedIdx = physics.getSelectedBodyIndex();

    // 1. Draw Orbit Rings
    for (const auto& body : bodies) {
        if (body.realOrbitRadiusAU <= 0.0) continue;
        float rPix = (float)body.realOrbitRadiusAU * scale;
        if (rPix < 2.0f || rPix > halfSize * 3.0f) continue;
        dl->AddCircle(center, rPix, ImGui::ColorConvertFloat4ToU32(ImVec4(0.18f, 0.28f, 0.42f, 0.40f)), 64, 1.0f);
    }

    // 2. Compute screen coordinates for celestial bodies
    struct ScreenBody {
        int index;
        float px, py;
        float dotR;
    };
    std::vector<ScreenBody> screenBodies;
    screenBodies.reserve(bodies.size());

    for (int i = 0; i < (int)bodies.size(); ++i) {
        float px = (i == 0) ? center.x : (center.x + bodies[i].position.x * scale);
        float py = (i == 0) ? center.y : (center.y + bodies[i].position.z * scale);
        bool isSelected = (i == selectedIdx);
        float dotR = (i == 0) ? 6.5f : (isSelected ? 5.5f : 3.5f);
        screenBodies.push_back({ i, px, py, dotR });
    }

    // 3. Hover Detection & Hit Testing
    int hoveredOrbitIdx = -1;
    float bestDist = 14.0f; // Click hitbox radius

    if (panelHovered) {
        for (const auto& sb : screenBodies) {
            float dx = mousePos.x - sb.px;
            float dy = mousePos.y - sb.py;
            float dist = std::sqrt(dx * dx + dy * dy);
            if (dist < bestDist) {
                bestDist = dist;
                hoveredOrbitIdx = sb.index;
            }
        }
    }

    // Click on celestial body dot to select
    if (hoveredOrbitIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(hoveredOrbitIdx);
        camera.focusOnBody(bodies[hoveredOrbitIdx].position, bodies[hoveredOrbitIdx].radius3D, 0.85f);
        addEventLog(bodies[hoveredOrbitIdx].name + " focused from Orbit Map");
    }

    // 4. Draw Sun
    bool isSunHovered = (hoveredOrbitIdx == 0);
    bool isSunSelected = (selectedIdx == 0);
    float sunR = 6.0f + (isSunHovered ? 2.0f : 0.0f);
    dl->AddCircleFilled(center, sunR, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.30f, 1.0f)), 16);
    dl->AddCircle(center, sunR + 3.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.70f, 0.20f, isSunHovered ? 0.6f : 0.30f)), 16, 1.5f);
    if (isSunSelected) {
        dl->AddCircle(center, sunR + 5.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent), 16, 1.5f);
    }

    // 5. Draw Planets
    for (int i = 1; i < (int)bodies.size(); ++i) {
        const auto& sb = screenBodies[i];
        bool isSelected = (i == selectedIdx);
        bool isHovered = (i == hoveredOrbitIdx);

        float dotR = sb.dotR + (isHovered ? 2.0f : 0.0f);
        ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 1.0f));

        if (isHovered) {
            dl->AddCircleFilled(ImVec2(sb.px, sb.py), dotR + 4.0f,
                                ImGui::ColorConvertFloat4ToU32(ImVec4(bodies[i].color.r, bodies[i].color.g, bodies[i].color.b, 0.25f)), 12);
        }

        dl->AddCircleFilled(ImVec2(sb.px, sb.py), dotR, col, 12);

        if (isSelected) {
            dl->AddCircle(ImVec2(sb.px, sb.py), dotR + 3.0f, ImGui::ColorConvertFloat4ToU32(Col::Accent), 12, 1.5f);
        }

        // Show name label for selected or hovered
        if (isSelected || isHovered) {
            const char* label = bodies[i].name.c_str();
            ImVec2 textSize = ImGui::CalcTextSize(label);
            ImU32 labelCol = ImGui::ColorConvertFloat4ToU32(isSelected ? Col::Accent : Col::TextPrimary);
            dl->AddText(ImVec2(sb.px - textSize.x * 0.5f, sb.py + dotR + 4.0f), labelCol, label);
        }
    }

    // Tooltip on Hover
    if (hoveredOrbitIdx >= 0) {
        ImGui::SetCursorScreenPos(mousePos);
        ImGui::BeginTooltip();
        ImGui::TextColored(Col::Accent, "%s", bodies[hoveredOrbitIdx].name.c_str());
        ImGui::TextColored(Col::TextSecondary, "%s", bodies[hoveredOrbitIdx].type.c_str());
        if (hoveredOrbitIdx > 0) {
            ImGui::TextColored(Col::TextPrimary, "Distance: %s", bodies[hoveredOrbitIdx].distanceStr.c_str());
        }
        ImGui::TextColored(Col::Green, "Click to focus view");
        ImGui::EndTooltip();
    }

    // 6. Clickable Legend at bottom
    ImGui::SetCursorScreenPos(ImVec2(contentMin.x, y + h - 28.0f));
    const char* legendNames[] = { "Sol", "Mercury", "Venus", "Earth", "Mars" };
    ImVec4 legendCols[] = {
        ImVec4(1.0f, 0.85f, 0.30f, 1.0f),
        ImVec4(0.85f, 0.65f, 0.40f, 1.0f),
        ImVec4(0.95f, 0.75f, 0.50f, 1.0f),
        ImVec4(0.20f, 0.65f, 1.00f, 1.0f),
        ImVec4(0.95f, 0.35f, 0.25f, 1.0f)
    };

    for (int k = 0; k < 5; ++k) {
        if (k > 0) ImGui::SameLine(0, 6);
        ImGui::PushID(k);
        ImVec2 p = ImGui::GetCursorScreenPos();
        dl->AddCircleFilled(ImVec2(p.x + 4, p.y + 8), 3.5f, ImGui::ColorConvertFloat4ToU32(legendCols[k]));
        ImGui::Dummy(ImVec2(8, 16));
        ImGui::SameLine(0, 2);

        bool isCurrent = (k == selectedIdx);
        if (isCurrent) {
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::SmallButton(legendNames[k])) {
            if (k < (int)bodies.size()) {
                physics.selectBody(k);
                camera.focusOnBody(bodies[k].position, bodies[k].radius3D, 0.85f);
                addEventLog(bodies[k].name + " selected");
            }
        }
        ImGui::PopStyleColor();
        ImGui::PopID();
    }

    // Zoom level indicator
    ImGui::SetCursorScreenPos(ImVec2(contentMin.x + 2, contentMax.y - 14));
    ImGui::TextColored(Col::TextSecondary, "%.1f×", m_orbitVisZoom);

    ImGui::End();
    ImGui::PopStyleVar(2);
}

// =========================================================================
// 7. 3D VIEWPORT HUD RETICLE (DECLUTTERED & NON-OVERLAPPING)
// =========================================================================

void UIManager::drawViewportHUD(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH) {
    if (vpW <= 0.0f || vpH <= 0.0f) return;

    ImVec2 mousePos = ImGui::GetMousePos();
    const auto& bodies = physics.getBodies();
    int selectedIdx = physics.getSelectedBodyIndex();

    int bestHoverIdx = -1;
    float bestDist = 1e9f;
    float baseHitbox = std::max(24.0f, vpH * 0.035f);
    glm::vec2 targetScreenPos(0.0f);
    float targetScreenRadius = 0.0f;

    // Find ONLY the single closest celestial body to the mouse
    for (int i = 0; i < (int)bodies.size(); ++i) {
        glm::vec2 sPos(0.0f);
        float sRadius = 0.0f;
        bool inFrustum = camera.projectToScreen(bodies[i].position, camera.getTargetPosition(),
                                                vpX, vpY, vpW, vpH, sPos, sRadius, bodies[i].radius3D);

        bool inViewport = (inFrustum && sPos.x >= vpX && sPos.x <= vpX + vpW && sPos.y >= vpY && sPos.y <= vpY + vpH);
        if (m_viewportHovered && inViewport && i != selectedIdx) {
            float dx = mousePos.x - sPos.x;
            float dy = mousePos.y - sPos.y;
            float dist = std::sqrt(dx * dx + dy * dy);

            if (dist <= std::max(baseHitbox, sRadius + 14.0f) && dist < bestDist) {
                bestDist = dist;
                bestHoverIdx = i;
                targetScreenPos = sPos;
                targetScreenRadius = sRadius;
            }
        }
    }

    m_hoveredBodyIndex = bestHoverIdx;

    // Click to select
    if (m_viewportHovered && m_hoveredBodyIndex >= 0 && m_hoveredBodyIndex != selectedIdx && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(m_hoveredBodyIndex);
        camera.focusOnBody(bodies[m_hoveredBodyIndex].position, bodies[m_hoveredBodyIndex].radius3D, 0.85f);
        addEventLog(bodies[m_hoveredBodyIndex].name + " focused");
    }

    // Render single clean hover reticle without clutter
    if (m_hoveredBodyIndex >= 0 && m_hoveredBodyIndex != selectedIdx) {
        ImDrawList* fg = ImGui::GetForegroundDrawList();
        fg->PushClipRect(ImVec2(vpX, vpY), ImVec2(vpX + vpW, vpY + vpH), true);

        const auto& body = bodies[m_hoveredBodyIndex];
        float ringR = std::max(13.0f, targetScreenRadius + 4.0f);
        ImU32 bodyCol = ImGui::ColorConvertFloat4ToU32(ImVec4(body.color.r, body.color.g, body.color.b, 1.0f));

        // Targeting ring
        fg->AddCircle(ImVec2(targetScreenPos.x, targetScreenPos.y), ringR, bodyCol, 32, 1.5f);

        // Hover Info Pill Badge
        std::string label = body.name + "  •  " + (body.id == "sol" ? "0.00 AU" : body.distanceStr);
        ImVec2 textSize = ImGui::CalcTextSize(label.c_str());
        float pillW = textSize.x + 14.0f;
        float pillH = textSize.y + 6.0f;
        float pillX = targetScreenPos.x + ringR + 8.0f;
        float pillY = targetScreenPos.y - pillH * 0.5f;

        if (pillX + pillW > vpX + vpW - 10.0f) {
            pillX = targetScreenPos.x - ringR - 8.0f - pillW;
        }

        fg->AddRectFilled(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH),
                          ImGui::ColorConvertFloat4ToU32(ImVec4(0.024f, 0.038f, 0.075f, 0.94f)), 4.0f);
        fg->AddRect(ImVec2(pillX, pillY), ImVec2(pillX + pillW, pillY + pillH), bodyCol, 4.0f, 0, 1.0f);
        fg->AddText(ImVec2(pillX + 7.0f, pillY + 3.0f), bodyCol, label.c_str());

        fg->PopClipRect();
    }
}

// =========================================================================
// 8. ASTEROID BELT & DEFORMABLE MATTER LAB MODALS
// =========================================================================

void UIManager::drawAsteroidBeltDiagnostics(PhysicsEngine& physics, float winW, float winH) {
    auto& belt = physics.getAsteroidBelt();
    const auto& diag = belt.getDiagnostics();
    const auto& hist = belt.getHistogram();

    float w = 720.0f;
    float h = 540.0f;
    ImGui::SetNextWindowPos(ImVec2((winW - w) * 0.5f, (winH - h) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.05f, 0.09f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, Col::BorderLight);

    if (ImGui::Begin("ASTEROID BELT (N-BODY) & RESONANCE ANALYSIS##BeltModal", &m_showAsteroidBeltDiagnostics, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(Col::Accent, "REAL-TIME ASTROPHYSICAL N(a) POPULATION HISTOGRAM & KIRKWOOD GAPS");
        ImGui::Separator();

        // Draw Histogram
        ImVec2 plotMin = ImGui::GetCursorScreenPos();
        float plotW = w - 40.0f;
        float plotH = 180.0f;
        ImVec2 plotMax = ImVec2(plotMin.x + plotW, plotMin.y + plotH);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        dl->AddRectFilled(plotMin, plotMax, ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.06f, 0.95f)), 4.0f);
        dl->AddRect(plotMin, plotMax, ImGui::ColorConvertFloat4ToU32(Col::Border), 4.0f);

        int maxBinCount = 1;
        for (int c : hist.counts) maxBinCount = std::max(maxBinCount, c);

        int numBins = (int)hist.counts.size();
        float binW = plotW / (float)std::max(1, numBins);
        for (int b = 0; b < numBins; ++b) {
            float barH = ((float)hist.counts[b] / (float)maxBinCount) * (plotH - 24.0f);
            ImVec2 b0(plotMin.x + b * binW, plotMax.y - barH - 12.0f);
            ImVec2 b1(plotMin.x + (b + 1) * binW - 1.0f, plotMax.y - 12.0f);
            dl->AddRectFilled(b0, b1, ImGui::ColorConvertFloat4ToU32(ImVec4(0.20f, 0.60f, 0.90f, 0.85f)));
        }

        // Kirkwood Resonance Lines (3:1, 5:2, 7:3, 2:1)
        struct ResLine { double a; const char* label; };
        ResLine resList[] = { { 2.50, "3:1" }, { 2.82, "5:2" }, { 2.95, "7:3" }, { 3.28, "2:1" } };
        for (const auto& res : resList) {
            float normA = (float)((res.a - hist.minAU) / (hist.maxAU - hist.minAU));
            if (normA >= 0.0f && normA <= 1.0f) {
                float lx = plotMin.x + normA * plotW;
                dl->AddLine(ImVec2(lx, plotMin.y + 4), ImVec2(lx, plotMax.y - 12),
                            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.35f, 0.25f, 0.75f)), 1.2f);
                dl->AddText(ImVec2(lx - 8, plotMin.y + 6), ImGui::ColorConvertFloat4ToU32(Col::Yellow), res.label);
            }
        }

        ImGui::SetCursorScreenPos(ImVec2(plotMin.x, plotMax.y + 10.0f));
        ImGui::Separator();

        // Belt Population Metrics
        float halfW = (w - 40.0f) * 0.5f;
        StatCard2Col("Active Physical Asteroids", std::to_string(diag.activePhysical).c_str(),
                     "Visual Asteroids (GPU)", std::to_string(diag.totalVisual).c_str(), halfW);
        StatCard2Col("Mean Semi-Major Axis", (std::to_string(diag.meanSemiMajorAxisAU).substr(0, 5) + " AU").c_str(),
                     "Mean Eccentricity (e)", std::to_string(diag.meanEccentricity).substr(0, 5).c_str(), halfW);
        StatCard2Col("Resonance Excitation Count", std::to_string(diag.highlyExcitedCount).c_str(),
                     "Energy Conservation Drift", (std::to_string(diag.energyDriftPct).substr(0, 6) + " %").c_str(), halfW);

        ImGui::Spacing();
        if (ImGui::Button("⚡ Jupiter Flyby Perturbation Impulse Test", ImVec2(300, 26))) {
            belt.triggerResonanceImpulseTest();
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
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.05f, 0.09f, 0.98f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.65f, 0.35f, 0.15f, 0.70f));

    if (ImGui::Begin("DEFORMABLE MATTER & MATERIALS PHYSICS LABORATORY##MatterLab", &m_showMatterLab, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ImVec4(0.95f, 0.70f, 0.35f, 1.0f), "COUPLED CONTINUUM MECHANICS, XPBD, PLASTICITY & FRACTURE ENGINE");
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

        // 2. Material Property Inspector
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

        if (ImGui::Button("🌌 Black Hole Tidal Disruption Laboratory", ImVec2(340, 28))) {
            matter.spawnBlackHoleTidalDisruptionLab();
            addEventLog("Black Hole Tidal Disruption spawned");
        }
        ImGui::SameLine();
        if (ImGui::Button("☄ Hypervelocity Impact & Crater Fracture", ImVec2(340, 28))) {
            matter.spawnHypervelocityCollision();
            addEventLog("Hypervelocity Collision spawned");
        }

        if (ImGui::Button("⚡ Tensile Stress & Necking / Ductile Failure", ImVec2(340, 28))) {
            matter.spawnTensileTest();
            addEventLog("Tensile Test specimen spawned");
        }
        ImGui::SameLine();
        if (ImGui::Button("🔥 Thermal Heating & Melting Phase Change", ImVec2(340, 28))) {
            matter.spawnThermalMeltingLab();
            addEventLog("Thermal Melting specimen spawned");
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
