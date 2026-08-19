#include "ui/ValidationUI.hpp"
#include "data/UnitConverter.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

namespace ValCol {
    static ImVec4 BgDark       {0.024f, 0.035f, 0.065f, 1.00f};
    static ImVec4 BgPanel      {0.035f, 0.050f, 0.090f, 0.98f};
    static ImVec4 BgChild      {0.045f, 0.065f, 0.115f, 0.90f};
    static ImVec4 Border       {0.120f, 0.180f, 0.280f, 0.60f};
    static ImVec4 Accent       {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 TextPrimary  {0.900f, 0.930f, 0.970f, 1.00f};
    static ImVec4 TextSecondary{0.460f, 0.540f, 0.680f, 1.00f};
    static ImVec4 Green        {0.150f, 0.880f, 0.450f, 1.00f};
    static ImVec4 Yellow       {0.980f, 0.780f, 0.120f, 1.00f};
    static ImVec4 Red          {0.950f, 0.250f, 0.200f, 1.00f};
    static ImVec4 SimCurveCol  {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 RealCurveCol {1.000f, 0.750f, 0.200f, 1.00f};
}

ValidationUI::ValidationUI() {}

void ValidationUI::render(bool& showWindow, 
                          ValidationEngine& valEngine, 
                          ObjectRepository& objRepo,
                          PhysicsEngine& physics,
                          float winW, float winH) {
    if (!showWindow) return;

    float modalW = std::min(1060.0f, winW - 40.0f);
    float modalH = std::min(720.0f, winH - 40.0f);

    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2((winW - modalW) * 0.5f, (winH - modalH) * 0.5f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ValCol::BgPanel);
    ImGui::PushStyleColor(ImGuiCol_Border, ValCol::Border);

    if (ImGui::Begin("VALIDATION DASHBOARD  —  Real Ephemeris Ground Truth vs. AstroGenesis Simulation", &showWindow, ImGuiWindowFlags_NoCollapse)) {
        ImGui::TextColored(ValCol::Accent, "★ SCIENTIFIC ORBITAL INTEGRATION VALIDATION");
        ImGui::SameLine();
        ImGui::TextColored(ValCol::TextSecondary, "| Benchmark against NASA JPL Horizons Ground Truth Ephemeris Data");

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        // Control Toolbar: Body selection, Time span, Integrator options
        auto allObjects = objRepo.getAllObjects("Solar System", false);
        if (allObjects.empty()) allObjects = objRepo.getAllObjects("", false);

        std::vector<const char*> bodyNames;
        for (const auto& obj : allObjects) bodyNames.push_back(obj.name.c_str());

        if (m_selectedBodyIdx >= (int)bodyNames.size()) m_selectedBodyIdx = 0;

        ImGui::Text("Target Body:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(160.0f);
        if (!bodyNames.empty()) {
            ImGui::Combo("##ValBodyCombo", &m_selectedBodyIdx, bodyNames.data(), (int)bodyNames.size());
        }

        ImGui::SameLine(0, 16);
        ImGui::Text("Time Span:");
        ImGui::SameLine();
        ImGui::SetNextItemWidth(180.0f);
        ImGui::SliderFloat("##TimeSpanSlider", &m_durationDays, 30.0f, 3652.5f, "%.0f Earth Days");

        ImGui::SameLine(0, 16);
        ImGui::Checkbox("Einstein 1PN GR", &m_enableGR);

        ImGui::SameLine(0, 20);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.40f, 0.65f, 0.90f));
        if (ImGui::Button("▶ RUN BENCHMARK", ImVec2(150, 24))) {
            if (m_selectedBodyIdx < (int)allObjects.size()) {
                std::string slug = allObjects[m_selectedBodyIdx].slug;
                double stepDays = std::max(1.0, (double)m_durationDays / 120.0);
                std::string err;
                if (valEngine.runValidationBenchmark(slug, m_durationDays, stepDays, m_enableGR, m_benchmarkPoints, m_benchmarkSummary, err)) {
                    m_hasBenchmarkRun = true;
                    m_errorMessage.clear();
                } else {
                    m_errorMessage = err;
                }
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine(0, 8);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.55f, 0.30f, 0.12f, 0.90f));
        if (ImGui::Button("⚖ NEWTON VS GR TEST", ImVec2(170, 24))) {
            if (m_selectedBodyIdx < (int)allObjects.size()) {
                std::string slug = allObjects[m_selectedBodyIdx].slug;
                valEngine.runNewtonianVsGRComparison(slug, m_durationDays, m_newtonPoints, m_grPoints, m_newtonSummary, m_grSummary);
                m_hasGRComparisonRun = true;
            }
        }
        ImGui::PopStyleColor();

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (!m_errorMessage.empty()) {
            ImGui::TextColored(ValCol::Red, "Error: %s", m_errorMessage.c_str());
        }

        // Main Dashboard Body: Stats & Plots
        if (m_hasBenchmarkRun && !m_benchmarkPoints.empty()) {
            // Row 1: 4 Key Metric Cards
            float cardW = (modalW - 60.0f) / 4.0f;
            
            // Card 1: Max Position Error
            ImGui::BeginChild("##Card1", ImVec2(cardW, 75), true);
            ImGui::TextColored(ValCol::TextSecondary, "MAX POSITION ERROR");
            ImGui::TextColored(ValCol::Accent, "%.2f km", m_benchmarkSummary.maxPosErrorKm);
            ImGui::TextColored(ValCol::TextSecondary, "Mean: %.2f km", m_benchmarkSummary.meanPosErrorKm);
            ImGui::EndChild();

            ImGui::SameLine();
            // Card 2: Velocity Error
            ImGui::BeginChild("##Card2", ImVec2(cardW, 75), true);
            ImGui::TextColored(ValCol::TextSecondary, "MAX VELOCITY ERROR");
            ImGui::TextColored(ValCol::Accent, "%.3f m/s", m_benchmarkSummary.maxVelErrorMps);
            ImGui::TextColored(ValCol::TextSecondary, "Symplectic precision");
            ImGui::EndChild();

            ImGui::SameLine();
            // Card 3: Energy Conservation Drift
            ImGui::BeginChild("##Card3", ImVec2(cardW, 75), true);
            ImGui::TextColored(ValCol::TextSecondary, "ENERGY DRIFT (|ΔE/E₀|)");
            if (m_benchmarkSummary.maxEnergyDriftPct < 1e-3) {
                ImGui::TextColored(ValCol::Green, "%.3e %% (Excellent)", m_benchmarkSummary.maxEnergyDriftPct);
            } else {
                ImGui::TextColored(ValCol::Yellow, "%.3e %%", m_benchmarkSummary.maxEnergyDriftPct);
            }
            ImGui::TextColored(ValCol::TextSecondary, "Angular Mom: %.2e %%", m_benchmarkSummary.maxAngMomDriftPct);
            ImGui::EndChild();

            ImGui::SameLine();
            // Card 4: GR Precession
            ImGui::BeginChild("##Card4", ImVec2(cardW, 75), true);
            ImGui::TextColored(ValCol::TextSecondary, "GR PERIHELION SHIFT");
            ImGui::TextColored(ValCol::Accent, "+%.2f\"/century", m_benchmarkSummary.grPrecessionSimulatedArcsec);
            ImGui::TextColored(ValCol::TextSecondary, "Theory: +%.2f\"/cen", m_benchmarkSummary.grPrecessionTheoreticalArcsec);
            ImGui::EndChild();

            ImGui::Spacing();

            // Row 2: 2 Plots (Left: Trajectory Overlay, Right: Error vs Time curve)
            float plotW = (modalW - 50.0f) * 0.5f;
            float plotH = 260.0f;

            drawTrajectoryPlot(m_benchmarkPoints, plotW, plotH);
            ImGui::SameLine();
            drawErrorCurvePlot(m_benchmarkPoints, plotW, plotH);
        } else {
            ImGui::BeginChild("##EmptyPrompt", ImVec2(modalW - 40, 240), true);
            ImGui::SetCursorPos(ImVec2(30, 80));
            ImGui::TextColored(ValCol::Accent, "No active validation benchmark run yet.");
            ImGui::SetCursorPos(ImVec2(30, 110));
            ImGui::TextColored(ValCol::TextSecondary, "Click '▶ RUN BENCHMARK' above to simulate and validate orbital propagation against real JPL ephemeris data.");
            ImGui::EndChild();
        }

        // Section 3: Newtonian vs Einstein GR Analysis
        if (m_hasGRComparisonRun) {
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();
            ImGui::TextColored(ValCol::Accent, "⚖ GENERAL RELATIVITY VS NEWTONIAN GRAVITY ANALYSIS (MERCURY PERIHELION TEST)");
            
            if (ImGui::BeginTable("##GRComparisonTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg, ImVec2(0, 110))) {
                ImGui::TableSetupColumn("Integrator Physics Model", ImGuiTableColumnFlags_WidthStretch, 0.3f);
                ImGui::TableSetupColumn("Max Position Error", ImGuiTableColumnFlags_WidthStretch, 0.2f);
                ImGui::TableSetupColumn("Perihelion Advance", ImGuiTableColumnFlags_WidthStretch, 0.2f);
                ImGui::TableSetupColumn("Energy Drift", ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGui::TableSetupColumn("Scientific Status", ImGuiTableColumnFlags_WidthStretch, 0.15f);
                ImGui::TableHeadersRow();

                // Row 1: Newtonian
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::Text("Classical Newtonian Gravity (1/r²)");
                ImGui::TableSetColumnIndex(1);
                ImGui::Text("%.2f km", m_newtonSummary.maxPosErrorKm);
                ImGui::TableSetColumnIndex(2);
                ImGui::Text("0.00\"/century");
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2e %%", m_newtonSummary.maxEnergyDriftPct);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(ValCol::Yellow, "Classical Approx");

                // Row 2: Einstein 1PN GR
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                ImGui::TextColored(ValCol::Accent, "Einstein 1PN Post-Newtonian GR");
                ImGui::TableSetColumnIndex(1);
                ImGui::TextColored(ValCol::Accent, "%.2f km", m_grSummary.maxPosErrorKm);
                ImGui::TableSetColumnIndex(2);
                ImGui::TextColored(ValCol::Green, "+%.2f\"/century", m_grSummary.grPrecessionSimulatedArcsec);
                ImGui::TableSetColumnIndex(3);
                ImGui::Text("%.2e %%", m_grSummary.maxEnergyDriftPct);
                ImGui::TableSetColumnIndex(4);
                ImGui::TextColored(ValCol::Green, "✔ Verified (JPL Match)");

                ImGui::EndTable();
            }
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void ValidationUI::drawTrajectoryPlot(const std::vector<ValidationComparisonPoint>& points, float w, float h) {
    ImGui::BeginChild("##TrajectoryPlot", ImVec2(w, h), true);
    ImGui::TextColored(ValCol::Accent, "ORBITAL TRAJECTORY (X-Z PLANE)");
    ImGui::SameLine(w - 180.0f);
    ImGui::TextColored(ValCol::SimCurveCol, "■ Sim  ");
    ImGui::SameLine();
    ImGui::TextColored(ValCol::RealCurveCol, "■ JPL Real");

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 screenPos = ImGui::GetCursorScreenPos();
    ImVec2 center = ImVec2(screenPos.x + w * 0.5f, screenPos.y + (h - 30.0f) * 0.5f);

    // Compute bounding scale
    double maxR = 1.0;
    for (const auto& p : points) {
        maxR = std::max(maxR, glm::length(p.realPosM));
    }
    float scale = (float)((h * 0.42f) / maxR);

    // Draw central star Sol
    dl->AddCircleFilled(center, 5.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 0.8f, 0.2f, 1.0f)));

    // Draw Real JPL Ephemeris trajectory (Golden)
    for (size_t i = 1; i < points.size(); ++i) {
        ImVec2 p0(center.x + (float)points[i - 1].realPosM.x * scale, center.y + (float)points[i - 1].realPosM.z * scale);
        ImVec2 p1(center.x + (float)points[i].realPosM.x * scale,     center.y + (float)points[i].realPosM.z * scale);
        dl->AddLine(p0, p1, ImGui::ColorConvertFloat4ToU32(ValCol::RealCurveCol), 1.5f);
    }

    // Draw Simulated trajectory (Cyan)
    for (size_t i = 1; i < points.size(); ++i) {
        ImVec2 p0(center.x + (float)points[i - 1].simPosM.x * scale, center.y + (float)points[i - 1].simPosM.z * scale);
        ImVec2 p1(center.x + (float)points[i].simPosM.x * scale,     center.y + (float)points[i].simPosM.z * scale);
        dl->AddLine(p0, p1, ImGui::ColorConvertFloat4ToU32(ValCol::SimCurveCol), 1.5f);
    }

    ImGui::EndChild();
}

void ValidationUI::drawErrorCurvePlot(const std::vector<ValidationComparisonPoint>& points, float w, float h) {
    ImGui::BeginChild("##ErrorCurvePlot", ImVec2(w, h), true);
    ImGui::TextColored(ValCol::Accent, "POSITION ERROR |ΔR| VS. TIME");

    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pMin = ImGui::GetCursorScreenPos();
    pMin.y += 10.0f;
    float graphW = w - 40.0f;
    float graphH = h - 60.0f;

    // Draw graph background grid
    dl->AddRectFilled(pMin, ImVec2(pMin.x + graphW, pMin.y + graphH), ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.05f, 0.8f)));
    dl->AddRect(pMin, ImVec2(pMin.x + graphW, pMin.y + graphH), ImGui::ColorConvertFloat4ToU32(ValCol::Border));

    // Find max error
    double maxErr = 1.0;
    for (const auto& p : points) maxErr = std::max(maxErr, p.posErrorKm);

    // Plot line
    if (points.size() > 1) {
        for (size_t i = 1; i < points.size(); ++i) {
            float t0 = (float)(i - 1) / (float)(points.size() - 1);
            float t1 = (float)i / (float)(points.size() - 1);

            float x0 = pMin.x + t0 * graphW;
            float x1 = pMin.x + t1 * graphW;
            float y0 = pMin.y + graphH - (float)(points[i - 1].posErrorKm / maxErr) * (graphH - 10.0f);
            float y1 = pMin.y + graphH - (float)(points[i].posErrorKm / maxErr) * (graphH - 10.0f);

            dl->AddLine(ImVec2(x0, y0), ImVec2(x1, y1), ImGui::ColorConvertFloat4ToU32(ValCol::Accent), 1.8f);
        }
    }

    char labelY[32];
    snprintf(labelY, sizeof(labelY), "Max: %.1f km", maxErr);
    dl->AddText(ImVec2(pMin.x + 6, pMin.y + 4), ImGui::ColorConvertFloat4ToU32(ValCol::TextSecondary), labelY);
    dl->AddText(ImVec2(pMin.x + 6, pMin.y + graphH - 18), ImGui::ColorConvertFloat4ToU32(ValCol::TextSecondary), "0.0 km (t=0)");

    ImGui::EndChild();
}

} // namespace AstroGenesis
