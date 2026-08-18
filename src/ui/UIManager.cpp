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
    drawRightPanel(physics, currentBody, topBarH, windowWidth, windowHeight, statusBarH);
    drawViewportHUD(physics, camera, m_viewportX, m_viewportY, m_viewportW, m_viewportH);

    float bottomY = windowHeight - statusBarH - bottomH;
    float bpW = m_viewportW / 4.0f;
    drawTimeControls(physics, leftPanelW,           bottomY, bpW, bottomH);
    drawSimMetrics  (physics, fps, leftPanelW + bpW, bottomY, bpW, bottomH);
    drawOrbitVis    (physics, camera, leftPanelW + bpW * 2, bottomY, bpW, bottomH);
    drawStatusBar(physics, camera, windowWidth, windowHeight, statusBarH);

    if (m_showAsteroidBeltDiagnostics) {
        drawAsteroidBeltDiagnostics(physics, windowWidth, windowHeight);
    }
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

    // Asteroid Belt Diagnostics & Resonance Analysis Window Toggle
    ImGui::SameLine(width - 240.0f);
    if (m_showAsteroidBeltDiagnostics) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.45f, 0.75f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.10f, 0.16f, 0.26f, 0.85f));
        ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    }
    if (ImGui::Button("☄ ASTEROID BELT (N(a))", ImVec2(220, 28))) {
        m_showAsteroidBeltDiagnostics = !m_showAsteroidBeltDiagnostics;
    }
    ImGui::PopStyleColor(2);

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
                camera.setTargetPosition(bodies[i].position, true);
                camera.setTargetBodyRadius(bodies[i].radius3D);
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

    const char* sections[] = { "EXOPLANET SYSTEMS", "STAR CLUSTERS", "GALAXIES", "FAVORITES" };
    for (auto& sec : sections) {
        SectionHeader(sec, false);
    }

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::drawInfoOverlay(const CelestialBody& body, float x, float y) {
    ImGui::SetNextWindowPos(ImVec2(x + 12, y + 12));
    ImGui::SetNextWindowSize(ImVec2(280, 0));
    ImGui::SetNextWindowBgAlpha(0.78f);
    ImGui::Begin("##EarthInfo", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_AlwaysAutoResize);

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

    InfoRow("Distance (Sol)",   body.distanceStr);
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

    ImGui::End();
}

void UIManager::drawRightPanel(PhysicsEngine& physics, const CelestialBody& body, float topBarH, float winW, float winH, float statusBarH) {
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
            
            // Keplerian Differential Velocity (Inner moves faster than outer)
            ImGui::BeginGroup();
            StatItem("\xE2\x9C\xA8", "Inner Speed (74.5k km)", "23.1 km/s (5.6h)");
            ImGui::SameLine(hw);
            StatItem("\xE2\x9C\xA8", "Outer Speed (140.2k km)", "16.8 km/s (14.9h)");
            ImGui::EndGroup();

            // Local Gravity & Escape Velocity Gradient
            ImGui::BeginGroup();
            StatItem("\xE2\x86\x93", "Local Gravity (g)", "6.84 → 1.93 m/s²");
            ImGui::SameLine(hw);
            StatItem("\xE2\x86\x97", "Escape Velocity", "32.7 → 23.8 km/s");
            ImGui::EndGroup();

            // Thermodynamics & Relativistic Dilation
            ImGui::BeginGroup();
            StatItem("\xE2\x97\x8B", "Ring Temp. (Ice)", "85 K (-188 °C)");
            ImGui::SameLine(hw);
            StatItem("\xE2\x8F\xB1", "Relativistic Drift", "-1.35 × 10⁻⁸");
            ImGui::EndGroup();

            // Total Mass & Fluid Perturbation State
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

    if (SectionHeader("COMPOSITION")) {
        // Donut chart
        float totalPct = 0.0f;
        for (const auto& item : body.composition) totalPct += item.percentage;
        if (totalPct > 0.0f && !body.composition.empty()) {
            float chartRadius = 40.0f;
            float innerRadius = 24.0f;
            ImVec2 chartCenter = ImVec2(ImGui::GetCursorScreenPos().x + chartRadius + 10.0f,
                                        ImGui::GetCursorScreenPos().y + chartRadius + 5.0f);
            ImDrawList* dl = ImGui::GetWindowDrawList();
            float startAngle = -3.14159f / 2.0f; // Start from top
            for (const auto& item : body.composition) {
                float sweep = (item.percentage / totalPct) * 2.0f * 3.14159f;
                if (sweep < 0.01f) continue;
                ImU32 col = ImGui::ColorConvertFloat4ToU32(ImVec4(item.color.r, item.color.g, item.color.b, 1.0f));
                // Draw arc segments
                int segments = std::max(4, (int)(sweep * 20.0f));
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
            // Inner circle to create donut hole
            dl->AddCircleFilled(chartCenter, innerRadius, ImGui::ColorConvertFloat4ToU32(Col::BgChild), 32);

            // Legend on the right side
            float legendX = chartCenter.x + chartRadius + 16.0f;
            float legendY = chartCenter.y - chartRadius + 4.0f;
            for (const auto& item : body.composition) {
                ImVec2 dotPos = ImVec2(legendX, legendY + 4.0f);
                dl->AddCircleFilled(dotPos, 4.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(item.color.r, item.color.g, item.color.b, 1.0f)));
                ImGui::SetCursorScreenPos(ImVec2(legendX + 10.0f, legendY - 2.0f));
                ImGui::TextColored(Col::Accent, "%.2f%%", item.percentage);
                ImGui::SameLine();
                ImGui::TextColored(Col::TextSecondary, " %s", item.name.c_str());
                legendY += 18.0f;
            }
            ImGui::SetCursorScreenPos(ImVec2(chartCenter.x - chartRadius, chartCenter.y + chartRadius + 10.0f));
            ImGui::Dummy(ImVec2(0, 5));
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
    if (ImGui::Button("|<", ImVec2(28, 24))) { physics.stepFrameBackward(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Step Backward (Frame)"); }
    ImGui::SameLine();
    if (ImGui::Button(isPaused ? " > " : " || ", ImVec2(28, 24))) { physics.togglePause(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip(isPaused ? "Play (Space)" : "Pause (Space)"); }
    ImGui::SameLine();
    if (ImGui::Button(">|", ImVec2(28, 24))) { physics.stepFrameForward(); }
    if (ImGui::IsItemHovered()) { ImGui::SetTooltip("Step Forward (Frame)"); }

    ImGui::SameLine(0, 8);
    if (ImGui::Button("1s/s", ImVec2(36, 24))) { physics.setTimeScale(1.0f); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("1d/s", ImVec2(36, 24))) { physics.setTimeScale(86400.0f); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("1m/s", ImVec2(36, 24))) { physics.setTimeScale(2592000.0f); }
    ImGui::SameLine(0, 4);
    if (ImGui::Button("1y/s", ImVec2(36, 24))) { physics.setTimeScale(31536000.0f); }

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

    // Energy & Angular Momentum Conservation Row
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
    // Simulation Time vs Real Time Tracking
    ImGui::TextColored(Col::TextSecondary, "Time Flow: ");
    ImGui::SameLine();
    ImGui::TextColored(Col::Accent, "%s", physics.getSimVsRealTimeStr().c_str());

    ImGui::Spacing();
    ImGui::Separator();

    // General Relativity Toggle Button
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

    // Get available drawing area below the header
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

    // Scroll zoom within the orbit vis panel
    if (panelHovered && io.MouseWheel != 0.0f) {
        m_orbitVisZoom *= (io.MouseWheel > 0) ? 1.15f : 0.87f;
        m_orbitVisZoom = std::clamp(m_orbitVisZoom, 0.15f, 20.0f);
    }

    // Max orbit radius for scaling (Neptune ~30 AU)
    float maxAU = 32.0f / m_orbitVisZoom;
    float scale = halfSize / maxAU;

    const auto& bodies = physics.getBodies();
    int selectedIdx = physics.getSelectedBodyIndex();
    float hitRadius = 12.0f; // generous click area

    // Draw orbit rings
    for (const auto& body : bodies) {
        if (body.realOrbitRadiusAU <= 0.0) continue;
        float ringRadius = (float)body.realOrbitRadiusAU * scale;
        if (ringRadius < 2.0f || ringRadius > halfSize * 2.5f) continue;
        dl->AddCircle(center, ringRadius,
            ImGui::ColorConvertFloat4ToU32(ImVec4(0.2f, 0.3f, 0.5f, 0.35f)), 64, 1.0f);
    }

    // --- Compute screen positions for all bodies ---
    struct BodyScreenInfo { int index; float px, py, dotR; bool visible; };
    std::vector<BodyScreenInfo> screenBodies;

    for (int i = 0; i < (int)bodies.size(); ++i) {
        float px, py;
        if (i == 0) { // Sun
            px = center.x;
            py = center.y;
        } else {
            px = center.x + bodies[i].position.x * scale;
            py = center.y + bodies[i].position.z * scale;
        }
        bool visible = (px >= x - 20 && px <= x + w + 20 && py >= y - 20 && py <= y + h + 20);
        bool isSelected = (i == selectedIdx);
        float dotR = (i == 0) ? std::min(8.0f, std::max(3.0f, (float)bodies[0].realRadiusAU * scale * 50.0f))
                              : (isSelected ? 5.0f : 3.0f);
        screenBodies.push_back({i, px, py, dotR, visible});
    }

    // --- Detect hover and click ---
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

    // Handle click to select
    if (hoveredIdx >= 0 && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(hoveredIdx);
        camera.setTargetPosition(bodies[hoveredIdx].position, true);
        camera.setTargetBodyRadius(bodies[hoveredIdx].radius3D);
    }

    // --- Draw Sun ---
    {
        auto& sb = screenBodies[0];
        bool isSunHovered = (hoveredIdx == 0);
        bool isSunSelected = (selectedIdx == 0);
        float sunR = sb.dotR;
        if (isSunHovered) sunR += 2.0f;

        dl->AddCircleFilled(ImVec2(sb.px, sb.py), sunR,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.85f, 0.3f, 1.0f)), 16);
        // Sun glow
        dl->AddCircle(ImVec2(sb.px, sb.py), sunR + 3.0f,
            ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.75f, 0.2f, isSunHovered ? 0.5f : 0.25f)), 16, 2.0f);
        if (isSunSelected) {
            dl->AddCircle(ImVec2(sb.px, sb.py), sunR + 5.0f,
                ImGui::ColorConvertFloat4ToU32(Col::Accent), 16, 1.5f);
        }
    }

    // --- Draw planet dots ---
    for (int i = 1; i < (int)screenBodies.size(); ++i) {
        auto& sb = screenBodies[i];
        if (!sb.visible) continue;
        const auto& body = bodies[sb.index];
        bool isSelected = (sb.index == selectedIdx);
        bool isHovered = (sb.index == hoveredIdx);

        float dotR = sb.dotR;
        if (isHovered && !isSelected) dotR += 2.0f;

        ImU32 dotCol = ImGui::ColorConvertFloat4ToU32(
            ImVec4(body.color.r, body.color.g, body.color.b, 1.0f));

        // Hover glow
        if (isHovered) {
            dl->AddCircleFilled(ImVec2(sb.px, sb.py), dotR + 4.0f,
                ImGui::ColorConvertFloat4ToU32(ImVec4(body.color.r, body.color.g, body.color.b, 0.15f)), 12);
        }

        dl->AddCircleFilled(ImVec2(sb.px, sb.py), dotR, dotCol, 12);

        if (isSelected) {
            // Selection ring
            dl->AddCircle(ImVec2(sb.px, sb.py), dotR + 3.0f,
                ImGui::ColorConvertFloat4ToU32(Col::Accent), 12, 1.5f);
        }

        // Show label for selected or hovered
        if (isSelected || isHovered) {
            const char* label = body.name.c_str();
            ImVec2 textSize = ImGui::CalcTextSize(label);
            ImU32 labelCol = ImGui::ColorConvertFloat4ToU32(isSelected ? Col::Accent : Col::TextPrimary);
            dl->AddText(ImVec2(sb.px - textSize.x * 0.5f, sb.py + dotR + 4.0f), labelCol, label);
        }
    }

    // Tooltip for hovered body
    if (hoveredIdx >= 0) {
        ImGui::SetCursorScreenPos(mousePos);
        ImGui::BeginTooltip();
        ImGui::TextColored(Col::Accent, "%s", bodies[hoveredIdx].name.c_str());
        ImGui::TextColored(Col::TextSecondary, "%s", bodies[hoveredIdx].type.c_str());
        if (hoveredIdx > 0) {
            ImGui::TextColored(Col::TextPrimary, "Distance: %s", bodies[hoveredIdx].distanceStr.c_str());
        }
        ImGui::TextColored(Col::TextSecondary, "Click to select");
        ImGui::EndTooltip();
    }

    // Zoom level indicator
    ImGui::SetCursorScreenPos(ImVec2(contentMin.x + 2, contentMax.y - 14));
    ImGui::TextColored(Col::TextSecondary, "%.1fx", m_orbitVisZoom);

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
    ImGui::TextColored(Col::TextPrimary, "X %.2f  Y %.2f  Z %.2f AU", eye.x, eye.y, eye.z);
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "Dist:");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextPrimary, "%.4f AU", camera.getDistance());
    ImGui::SameLine(0, 20);
    ImGui::TextColored(Col::TextSecondary, "Focus:");
    ImGui::SameLine();
    ImGui::TextColored(Col::Accent, "%s", target.name.c_str());
    ImGui::SameLine(winW - 130);
    ImGui::TextColored(Col::Green, "Realistic");

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

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

        if (m_viewportHovered && inViewport && i != selectedIdx) {
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

    // Handle left click inside the 3D viewport on a hovered body (only when different from current selection)
    if (m_viewportHovered && m_hoveredBodyIndex >= 0 && m_hoveredBodyIndex != selectedIdx && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        physics.selectBody(m_hoveredBodyIndex);
        camera.focusOnBody(bodies[m_hoveredBodyIndex].position, bodies[m_hoveredBodyIndex].radius3D, 0.85f);
    }

    // Draw HUD hover targeting reticle clipped to the 3D viewport area
    ImDrawList* fg = ImGui::GetForegroundDrawList();
    fg->PushClipRect(ImVec2(vpX, vpY), ImVec2(vpX + vpW, vpY + vpH), true);

    // Draw compact Hover Reticle & Info Tag for hovered body (only when hovering and not currently selected)
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

            // Keep pill inside viewport bounds
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

void UIManager::drawAsteroidBeltDiagnostics(PhysicsEngine& physics, float winW, float winH) {
    auto& belt = physics.getAsteroidBelt();
    const auto& diag = belt.getDiagnostics();
    const auto& hist = belt.getHistogram();

    float w = 680.0f;
    float h = 540.0f;
    ImGui::SetNextWindowPos(ImVec2((winW - w) * 0.5f, (winH - h) * 0.5f), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(w, h));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.06f, 0.11f, 0.96f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.2f, 0.4f, 0.7f, 0.6f));

    if (ImGui::Begin("ASTEROID BELT DYNAMICS & KIRKWOOD RESONANCES##Diag", &m_showAsteroidBeltDiagnostics, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(Col::Accent, "HYBRID ASTRODYNAMICS & RESONANCE DEPLETION ENGINE");
        ImGui::Separator();

        // 1. Population & Conservation Grid (4 Columns)
        float colW = (w - 50.0f) / 4.0f;
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Physical Asteroids");
        ImGui::TextColored(Col::Green, "%d Active", diag.activePhysical);
        ImGui::TextColored(Col::TextSecondary, "Escaped: %d | Sun: %d", diag.escapedPhysical, diag.sunCollidedPhysical);
        ImGui::EndGroup();

        ImGui::SameLine(colW);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Visual Population");
        ImGui::TextColored(Col::Accent, "%d GPU", diag.totalVisual);
        ImGui::TextColored(Col::TextSecondary, "Instanced (1 Draw)");
        ImGui::EndGroup();

        ImGui::SameLine(colW * 2);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Mean Semi-Major Axis");
        ImGui::TextColored(Col::TextPrimary, "%.3f AU", diag.meanSemiMajorAxisAU);
        ImGui::TextColored(Col::TextSecondary, "Mean Inc: %.1f\xC2\xB0", diag.meanInclinationDeg);
        ImGui::EndGroup();

        ImGui::SameLine(colW * 3);
        ImGui::BeginGroup();
        ImGui::TextColored(Col::TextSecondary, "Eccentricity (Mean/Max)");
        ImGui::TextColored(Col::TextPrimary, "%.3f / %.3f", diag.meanEccentricity, diag.maxEccentricity);
        ImGui::TextColored(Col::TextSecondary, "Excited (e>0.25): %d", diag.highlyExcitedCount);
        ImGui::EndGroup();

        ImGui::Spacing();
        ImGui::Separator();

        // 2. N(a) Histogram with Jupiter Mean-Motion Resonance Lines
        ImGui::TextColored(Col::Accent, "N(a) ASTEROID DISTRIBUTION & KIRKWOOD GAPS (2.0 - 3.6 AU)");
        ImGui::TextColored(Col::TextSecondary, "Vertical lines mark theoretical Jupiter orbital resonance locations:");

        ImDrawList* dl = ImGui::GetWindowDrawList();
        ImVec2 plotPos = ImGui::GetCursorScreenPos();
        float plotW = w - 30.0f;
        float plotH = 150.0f;

        // Background box for histogram
        dl->AddRectFilled(plotPos, ImVec2(plotPos.x + plotW, plotPos.y + plotH),
                          ImGui::ColorConvertFloat4ToU32(ImVec4(0.025f, 0.035f, 0.07f, 0.95f)), 4.0f);
        dl->AddRect(plotPos, ImVec2(plotPos.x + plotW, plotPos.y + plotH),
                    ImGui::ColorConvertFloat4ToU32(ImVec4(0.15f, 0.25f, 0.45f, 0.5f)), 4.0f);

        // Draw histogram bars
        int numBins = (int)hist.counts.size();
        float barWidth = plotW / (float)numBins;
        int maxCount = std::max(1, hist.maxBinCount);

        for (int b = 0; b < numBins; ++b) {
            float count = (float)hist.counts[b];
            float barHeight = (count / (float)maxCount) * (plotH - 24.0f);
            float bx0 = plotPos.x + (float)b * barWidth;
            float bx1 = bx0 + barWidth - 1.0f;
            float by1 = plotPos.y + plotH - 18.0f;
            float by0 = by1 - barHeight;

            ImU32 barCol = ImGui::ColorConvertFloat4ToU32(ImVec4(0.35f, 0.65f, 0.95f, 0.85f));
            dl->AddRectFilled(ImVec2(bx0, by0), ImVec2(bx1, by1), barCol);
        }

        // Helper to draw vertical resonance line
        auto drawResonance = [&](float aRes, const char* label, ImVec4 col) {
            if (aRes >= hist.minAU && aRes <= hist.maxAU) {
                float normX = (aRes - hist.minAU) / (hist.maxAU - hist.minAU);
                float rx = plotPos.x + normX * plotW;
                ImU32 lineCol = ImGui::ColorConvertFloat4ToU32(col);
                dl->AddLine(ImVec2(rx, plotPos.y + 4.0f), ImVec2(rx, plotPos.y + plotH - 18.0f), lineCol, 1.5f);
                dl->AddText(ImVec2(rx - 8.0f, plotPos.y + 4.0f), lineCol, label);
            }
        };

        drawResonance(ParticleHistogram::RES_4_1, "4:1", ImVec4(0.8f, 0.8f, 0.4f, 0.8f));
        drawResonance(ParticleHistogram::RES_3_1, "3:1", ImVec4(1.0f, 0.35f, 0.35f, 0.9f));
        drawResonance(ParticleHistogram::RES_5_2, "5:2", ImVec4(1.0f, 0.55f, 0.25f, 0.9f));
        drawResonance(ParticleHistogram::RES_7_3, "7:3", ImVec4(1.0f, 0.75f, 0.25f, 0.9f));
        drawResonance(ParticleHistogram::RES_2_1, "2:1", ImVec4(1.0f, 0.35f, 0.35f, 0.9f));

        // Axis labels
        dl->AddText(ImVec2(plotPos.x + 6.0f, plotPos.y + plotH - 16.0f),
                    ImGui::ColorConvertFloat4ToU32(Col::TextSecondary), "2.0 AU");
        dl->AddText(ImVec2(plotPos.x + plotW * 0.5f - 16.0f, plotPos.y + plotH - 16.0f),
                    ImGui::ColorConvertFloat4ToU32(Col::TextSecondary), "2.8 AU");
        dl->AddText(ImVec2(plotPos.x + plotW - 46.0f, plotPos.y + plotH - 16.0f),
                    ImGui::ColorConvertFloat4ToU32(Col::TextSecondary), "3.6 AU");

        ImGui::Dummy(ImVec2(plotW, plotH + 4.0f));

        ImGui::Spacing();
        ImGui::Separator();

        // 3. Performance & Population Configuration Controls
        ImGui::TextColored(Col::Accent, "POPULATION & PERFORMANCE CONTROLS");

        static int configPhysCount = belt.getPhysicalCount();
        static int configVisCount = belt.getVisualCount();
        float sizeMult = belt.getVisualSizeMultiplier();

        ImGui::PushItemWidth(240.0f);
        ImGui::SliderInt("Physical Asteroids (N-body)", &configPhysCount, 200, 5000, "%d Bodies");
        ImGui::SameLine(360.0f);
        ImGui::SliderInt("Visual Asteroids (GPU)", &configVisCount, 10000, 500000, "%d GPU");

        if (ImGui::SliderFloat("Visual Size Scale", &sizeMult, 0.2f, 5.0f, "%.1fx")) {
            belt.setVisualSizeMultiplier(sizeMult);
        }
        ImGui::PopItemWidth();

        ImGui::Spacing();

        if (ImGui::Button("\xE2\x9F\xB3 Re-seed & Reinitialize Belt", ImVec2(220, 26))) {
            physics.reseedAsteroidBelt(configPhysCount, configVisCount);
        }
        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.25f, 0.15f, 0.9f));
        if (ImGui::Button("\xE2\x9A\xA1 Jupiter Flyby Perturbation Impulse Test", ImVec2(280, 26))) {
            belt.triggerResonanceImpulseTest();
        }
        if (ImGui::IsItemHovered()) {
            ImGui::SetTooltip("Simulates strong gravitational kicks at Jupiter resonances (3:1 and 2:1) to demonstrate orbital excitation and gap depletion!");
        }
        ImGui::PopStyleColor();

        ImGui::SameLine(w - 120.0f);
        if (ImGui::Button("Close", ImVec2(90, 26))) {
            m_showAsteroidBeltDiagnostics = false;
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();
}

} // namespace AstroGenesis
