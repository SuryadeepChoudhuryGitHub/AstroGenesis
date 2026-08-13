#include "ui/UIManager.hpp"
#include <cstdio>
#include <algorithm>

namespace AstroGenesis {

namespace Col {
    static ImVec4 BgDark       {0.039f, 0.055f, 0.102f, 1.00f};
    static ImVec4 BgPanel      {0.059f, 0.086f, 0.161f, 0.94f};
    static ImVec4 BgChild      {0.071f, 0.098f, 0.173f, 0.90f};
    static ImVec4 BgPopup      {0.078f, 0.110f, 0.200f, 0.98f};
    static ImVec4 Border       {0.180f, 0.220f, 0.350f, 0.45f};
    static ImVec4 Accent       {0.000f, 0.831f, 1.000f, 1.00f};
    static ImVec4 AccentDim    {0.000f, 0.500f, 0.700f, 0.70f};
    static ImVec4 AccentHover  {0.000f, 0.900f, 1.000f, 1.00f};
    static ImVec4 TextPrimary  {0.850f, 0.890f, 0.950f, 1.00f};
    static ImVec4 TextSecondary{0.500f, 0.560f, 0.680f, 1.00f};
    static ImVec4 SelectedBg   {0.000f, 0.831f, 1.000f, 0.12f};
    static ImVec4 TabActive    {0.000f, 0.831f, 1.000f, 0.25f};
    static ImVec4 Green        {0.200f, 0.850f, 0.400f, 1.00f};
    static ImVec4 Yellow       {0.950f, 0.750f, 0.100f, 1.00f};
}

static bool SectionHeader(const char* label, bool defaultOpen = true) {
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(Col::Accent.x, Col::Accent.y, Col::Accent.z, 0.1f));
    bool open = ImGui::CollapsingHeader(label, defaultOpen ? ImGuiTreeNodeFlags_DefaultOpen : 0);
    ImGui::PopStyleColor(2);
    return open;
}

static void StatItem(const char* icon, const char* label, const char* value) {
    ImGui::BeginGroup();
    ImGui::TextColored(Col::Accent, "%s", icon);
    ImGui::SameLine();
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "%s", label);
    ImGui::TextColored(Col::TextPrimary, "%s", value);
    ImGui::EndGroup();
    ImGui::EndGroup();
}

static void ColorDot(ImVec4 color) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddCircleFilled(ImVec2(p.x + 5, p.y + 7), 4.0f, ImGui::ColorConvertFloat4ToU32(color));
    ImGui::Dummy(ImVec2(12, 14));
    ImGui::SameLine();
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
    c[ImGuiCol_FrameBg]              = ImVec4(0.10f, 0.13f, 0.22f, 0.70f);
    c[ImGuiCol_FrameBgHovered]       = ImVec4(0.15f, 0.18f, 0.28f, 0.80f);
    c[ImGuiCol_FrameBgActive]        = ImVec4(0.00f, 0.50f, 0.70f, 0.40f);
    c[ImGuiCol_TitleBg]              = Col::BgDark;
    c[ImGuiCol_TitleBgActive]        = ImVec4(0.06f, 0.09f, 0.16f, 1.00f);
    c[ImGuiCol_CheckMark]            = Col::Accent;
    c[ImGuiCol_SliderGrab]           = Col::Accent;
    c[ImGuiCol_SliderGrabActive]     = Col::AccentHover;
    c[ImGuiCol_Button]               = ImVec4(0.10f, 0.14f, 0.24f, 0.80f);
    c[ImGuiCol_ButtonHovered]        = ImVec4(0.00f, 0.50f, 0.70f, 0.50f);
    c[ImGuiCol_ButtonActive]         = ImVec4(0.00f, 0.60f, 0.80f, 0.70f);
    c[ImGuiCol_Header]               = Col::SelectedBg;
    c[ImGuiCol_HeaderHovered]        = ImVec4(0.00f, 0.60f, 0.80f, 0.25f);
    c[ImGuiCol_HeaderActive]         = ImVec4(0.00f, 0.70f, 0.90f, 0.35f);
    c[ImGuiCol_Text]                 = Col::TextPrimary;
    c[ImGuiCol_TextDisabled]         = Col::TextSecondary;
}

void UIManager::getViewportBounds(float& outX, float& outY, float& outW, float& outH) const {
    outX = m_viewportX;
    outY = m_viewportY;
    outW = m_viewportW;
    outH = m_viewportH;
}

void UIManager::renderUI(PhysicsEngine& physics, Camera& camera, float windowWidth, float windowHeight, float fps) {
    float topBarH     = 48.0f;
    float statusBarH  = 28.0f;
    float leftPanelW  = 210.0f;
    float rightPanelW = 310.0f;
    float bottomH     = 210.0f;

    m_viewportX = leftPanelW;
    m_viewportY = topBarH;
    m_viewportW = windowWidth - leftPanelW - rightPanelW;
    m_viewportH = windowHeight - topBarH - statusBarH - bottomH;

    // Determine mouse hovering viewport
    ImVec2 mousePos = ImGui::GetMousePos();
    m_viewportHovered = (mousePos.x >= m_viewportX && mousePos.x <= m_viewportX + m_viewportW &&
                         mousePos.y >= m_viewportY && mousePos.y <= m_viewportY + m_viewportH);

    const CelestialBody& currentBody = physics.getSelectedBody();

    drawTopBar(windowWidth);
    drawLeftPanel(physics, camera, topBarH, statusBarH, windowHeight);
    drawInfoOverlay(currentBody, m_viewportX, m_viewportY);
    drawRightPanel(currentBody, topBarH, windowWidth, windowHeight, statusBarH);

    float bottomY = windowHeight - statusBarH - bottomH;
    float bpW = m_viewportW / 4.0f;
    drawTimeControls(physics, leftPanelW,           bottomY, bpW, bottomH);
    drawSimMetrics  (physics, fps, leftPanelW + bpW, bottomY, bpW, bottomH);
    drawOrbitVis    (leftPanelW + bpW * 2,          bottomY, bpW, bottomH);
    drawAIAssistant (leftPanelW + bpW * 3,          bottomY, bpW, bottomH);

    drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);
}

void UIManager::drawTopBar(float width) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(width, 48));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 10));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.035f, 0.05f, 0.09f, 0.97f));

    ImGui::Begin("##TopBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("\xE2\x97\x86");
    ImGui::SameLine();
    ImGui::Text("ASTROGENESIS");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "SPACE SIMULATION ENGINE");
    ImGui::SameLine(0, 40);

    const char* tabs[] = { "UNIVERSE", "SYSTEM", "OBJECTS", "EXPLORE", "SIMULATION", "AI ASSISTANT" };
    static int activeTab = 0;
    for (int i = 0; i < 6; ++i) {
        if (i > 0) ImGui::SameLine(0, 4);
        bool isActive = (i == activeTab);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, Col::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::Button(tabs[i], ImVec2(0, 28))) activeTab = i;
        ImGui::PopStyleColor(2);
    }

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void UIManager::drawLeftPanel(PhysicsEngine& physics, Camera& camera, float topBarH, float statusBarH, float winH) {
    float panelW = 210;
    float panelH = winH - topBarH - statusBarH;
    ImGui::SetNextWindowPos(ImVec2(0, topBarH));
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);

    ImGui::Begin("##LeftPanel", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    static char search[128] = "";
    ImGui::PushItemWidth(-30);
    ImGui::InputTextWithHint("##search", "Search Anything...", search, sizeof(search));
    ImGui::PopItemWidth();
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "\xE2\x96\xBC");
    ImGui::Separator();

    if (SectionHeader("SOLAR SYSTEM")) {
        const auto& bodies = physics.getBodies();
        int selectedIndex = physics.getSelectedBodyIndex();

        for (int i = 0; i < (int)bodies.size(); ++i) {
            bool isSelected = (i == selectedIndex);
            if (isSelected) {
                ImGui::PushStyleColor(ImGuiCol_Header, Col::SelectedBg);
                ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
            }

            if (ImGui::Selectable(("##body" + std::to_string(i)).c_str(), isSelected, 0, ImVec2(0, 36))) {
                physics.selectBody(i);
                camera.setTargetPosition(bodies[i].position);
            }

            ImVec2 p = ImGui::GetItemRectMin();
            ImGui::SetCursorScreenPos(ImVec2(p.x + 28, p.y + 2));
            ImGui::Text("%s", bodies[i].name.c_str());
            ImGui::SetCursorScreenPos(ImVec2(p.x + 28, p.y + 18));
            ImGui::TextColored(Col::TextSecondary, "%s", bodies[i].distanceStr.c_str());

            ImGui::GetWindowDrawList()->AddCircleFilled(
                ImVec2(p.x + 14, p.y + 18), 8.0f,
                isSelected ? ImGui::ColorConvertFloat4ToU32(Col::Accent) : ImGui::ColorConvertFloat4ToU32(Col::TextSecondary));

            if (isSelected) ImGui::PopStyleColor(2);
        }
    }

    ImGui::Separator();

    const char* sections[] = { "EXOPLANET SYSTEMS", "STAR CLUSTERS", "GALAXIES", "FAVORITES" };
    for (auto& sec : sections) {
        SectionHeader(sec, false);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::drawInfoOverlay(const CelestialBody& body, float x, float y) {
    ImGui::SetNextWindowPos(ImVec2(x + 12, y + 12));
    ImGui::SetNextWindowSize(ImVec2(260, 0));
    ImGui::SetNextWindowBgAlpha(0.75f);
    ImGui::Begin("##EarthInfo", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

    ImGui::TextColored(Col::Accent, "%s", body.name.c_str());
    ImGui::SameLine();
    ImGui::TextColored(Col::Yellow, "\xE2\x98\x85");
    ImGui::TextColored(Col::TextSecondary, "%s", body.type.c_str());
    ImGui::Separator();

    auto InfoRow = [](const char* label, const std::string& value) {
        ImGui::TextColored(Col::TextSecondary, "%-18s", label);
        ImGui::SameLine(120);
        ImGui::TextColored(Col::TextPrimary, "%s", value.c_str());
    };

    InfoRow("Distance",        body.distanceStr);
    InfoRow("Radius",          body.radiusStr);
    InfoRow("Mass",            body.massStr);
    InfoRow("Gravity",         body.gravityStr);
    InfoRow("Temperature",     body.tempStr);
    InfoRow("Orbital Period",  body.orbitalPeriodStr);
    InfoRow("Rotation Period", body.rotationPeriodStr);
    InfoRow("Axial Tilt",      body.axialTiltStr);
    InfoRow("Atmosphere",      body.atmosphereStr);
    InfoRow("Moons",           std::to_string(body.moons));

    ImGui::End();
}

void UIManager::drawRightPanel(const CelestialBody& body, float topBarH, float winW, float winH, float statusBarH) {
    float panelW = 310;
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

    if (SectionHeader("RADIATION & ENVIRONMENT")) {
        float hw = (panelW - 40) / 2.0f;
        ImGui::BeginGroup();
        StatItem("\xE2\x98\x80", "Solar Radiation", body.solarRadiationStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9A\xA0", "Radiation Level", body.radLevelStr.c_str());
        ImGui::EndGroup();

        ImGui::BeginGroup();
        StatItem("\xE2\x97\x86", "Magnetic Field", body.magneticFieldStr.c_str());
        ImGui::SameLine(hw);
        StatItem("\xE2\x9C\xA8", "Aurora Activity", body.auroraActivityStr.c_str());
        ImGui::EndGroup();
    }

    ImGui::Separator();

    if (SectionHeader("COMPOSITION")) {
        for (const auto& item : body.composition) {
            ColorDot(ImVec4(item.color.r, item.color.g, item.color.b, item.color.a));
            ImGui::Text("%.2f%%  %s", item.percentage, item.name.c_str());
        }
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::drawTimeControls(PhysicsEngine& physics, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##TimeControls", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "TIME CONTROLS");
    ImGui::Separator();

    bool isPaused = physics.isPaused();
    if (ImGui::Button("|<", ImVec2(32, 26))) { physics.stepFrameBackward(); }
    ImGui::SameLine();
    if (ImGui::Button(isPaused ? " > " : " || ", ImVec2(32, 26))) { physics.togglePause(); }
    ImGui::SameLine();
    if (ImGui::Button(">|", ImVec2(32, 26))) { physics.stepFrameForward(); }

    float scale = physics.getTimeScale();
    ImGui::PushItemWidth(w - 30);
    if (ImGui::SliderFloat("##speed", &scale, 0.0f, 10.0f, "Speed: %.1fx")) {
        physics.setTimeScale(scale);
    }
    ImGui::PopItemWidth();

    ImGui::TextColored(Col::TextSecondary, "%s", physics.getSimulationTimeStr().c_str());

    ImGui::End();
}

void UIManager::drawSimMetrics(const PhysicsEngine& physics, float fps, float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##SimMetrics", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "SIMULATION METRICS");
    ImGui::Separator();

    float colW = (w - 30) / 4.0f;
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "FPS");
    ImGui::TextColored(Col::Green, "%.0f", fps);
    ImGui::EndGroup();

    ImGui::SameLine(colW);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Objects");
    ImGui::TextColored(Col::TextPrimary, "%d", physics.getObjectCount());
    ImGui::EndGroup();

    ImGui::SameLine(colW * 2);
    ImGui::BeginGroup();
    ImGui::TextColored(Col::TextSecondary, "Physics Step");
    ImGui::TextColored(Col::TextPrimary, "%.2f ms", physics.getPhysicsStepTimeMs());
    ImGui::EndGroup();

    ImGui::End();
}

void UIManager::drawOrbitVis(float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##OrbitVis", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "ORBIT VISUALIZATION");
    ImGui::Separator();
    ImGui::TextColored(Col::TextSecondary, "Interactive solar orbit map");

    ImGui::End();
}

void UIManager::drawAIAssistant(float x, float y, float w, float h) {
    ImGui::SetNextWindowPos(ImVec2(x, y));
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::Begin("##AIAssistant", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

    ImGui::TextColored(Col::Accent, "AI ASSISTANT");
    ImGui::SameLine(w - 50);
    ImGui::TextColored(Col::Green, "BETA");
    ImGui::Separator();

    static char aiInput[128] = "";
    ImGui::PushItemWidth(w - 20);
    ImGui::InputTextWithHint("##aiinput", "Ask ASTROGENESIS AI...", aiInput, sizeof(aiInput));
    ImGui::PopItemWidth();

    ImGui::End();
}

void UIManager::drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH) {
    ImGui::SetNextWindowPos(ImVec2(0, winH - barH));
    ImGui::SetNextWindowSize(ImVec2(winW, barH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 4));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.03f, 0.04f, 0.08f, 0.97f));

    ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    glm::vec3 eye = camera.getEyePosition();
    const CelestialBody& target = physics.getSelectedBody();

    ImGui::TextColored(Col::TextSecondary, "Camera: Orbit");
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "Pos:");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextPrimary, "X %.2f  Y %.2f  Z %.2f", eye.x, eye.y, eye.z);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "Focus:");
    ImGui::SameLine();
    ImGui::TextColored(Col::Accent, "%s", target.name.c_str());

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

} // namespace AstroGenesis
