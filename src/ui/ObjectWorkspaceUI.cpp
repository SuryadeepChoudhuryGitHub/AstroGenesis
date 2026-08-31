#include "ui/ObjectWorkspaceUI.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "data/UnitConverter.hpp"
#include <cstdio>
#include <cmath>
#include <algorithm>
#include <iostream>

namespace AstroGenesis {

namespace Col {
    static ImVec4 BgDark       {0.024f, 0.035f, 0.065f, 1.00f};
    static ImVec4 BgPanel      {0.035f, 0.050f, 0.090f, 0.96f};
    static ImVec4 BgChild      {0.045f, 0.065f, 0.115f, 0.90f};
    static ImVec4 Border       {0.120f, 0.180f, 0.280f, 0.60f};
    static ImVec4 Accent       {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 TextPrimary  {0.900f, 0.930f, 0.970f, 1.00f};
    static ImVec4 TextSecondary{0.460f, 0.540f, 0.680f, 1.00f};
    static ImVec4 SelectedBg   {0.000f, 0.600f, 0.850f, 0.22f};
    static ImVec4 Green        {0.150f, 0.880f, 0.450f, 1.00f};
    static ImVec4 Yellow       {0.980f, 0.780f, 0.120f, 1.00f};
    static ImVec4 Orange       {0.980f, 0.550f, 0.150f, 1.00f};
    static ImVec4 Red          {0.950f, 0.250f, 0.200f, 1.00f};
    static ImVec4 Purple       {0.750f, 0.350f, 0.950f, 1.00f};
}

ObjectWorkspaceUI::ObjectWorkspaceUI() {
    createNewObjectTemplate("Planet");
}

void ObjectWorkspaceUI::setSelectedObjectBySlug(const std::string& slug, ObjectRepository& objRepo) {
    auto bOpt = objRepo.getHydratedBodyBySlug(slug);
    if (bOpt.has_value()) {
        m_editingBody = bOpt.value();
        m_selectedObjectId = m_editingBody.dbId;
        m_selectedObjectSlug = slug;
        recomputeDerived(m_editingBody);
    }
}

void ObjectWorkspaceUI::createNewObjectTemplate(const std::string& objectType) {
    m_editingBody = CelestialBody();
    m_editingBody.dbId = 0;
    m_editingBody.category = "Custom";
    m_editingBody.sourceName = "User Created";

    if (objectType == "Star") {
        m_editingBody.id = "custom_star";
        m_editingBody.name = "Custom Star";
        m_editingBody.type = "G2V Main Sequence Star";
        m_editingBody.color = glm::vec3(1.0f, 0.82f, 0.35f);
        m_editingBody.massKg = UnitConverter::SOLAR_MASS_KG;
        m_editingBody.radiusM = UnitConverter::SOLAR_RADIUS_M;
        m_editingBody.surfaceTempK = 5778.0;
        m_editingBody.luminosityW = UnitConverter::SOLAR_LUMINOSITY_W;
        m_editingBody.rotationPeriodHours = 609.12;
    } else if (objectType == "Planet") {
        m_editingBody.id = "custom_planet";
        m_editingBody.name = "Custom Planet";
        m_editingBody.type = "Terrestrial Planet";
        m_editingBody.color = glm::vec3(0.2f, 0.75f, 0.9f);
        m_editingBody.massKg = UnitConverter::EARTH_MASS_KG;
        m_editingBody.radiusM = UnitConverter::EARTH_RADIUS_M;
        m_editingBody.semiMajorAxisAU = 1.0;
        m_editingBody.eccentricity = 0.0167;
        m_editingBody.surfaceTempK = 288.0;
        m_editingBody.rotationPeriodHours = 24.0;
        m_editingBody.axialTiltDeg = 23.44f;
    } else if (objectType == "Moon") {
        m_editingBody.id = "custom_moon";
        m_editingBody.name = "Custom Moon";
        m_editingBody.type = "Planetary Moon";
        m_editingBody.color = glm::vec3(0.75f, 0.75f, 0.80f);
        m_editingBody.massKg = UnitConverter::LUNAR_MASS_KG;
        m_editingBody.radiusM = UnitConverter::LUNAR_RADIUS_M;
        m_editingBody.semiMajorAxisAU = 0.00257; // 384,400 km
        m_editingBody.surfaceTempK = 220.0;
    } else if (objectType == "Asteroid" || objectType == "Comet") {
        m_editingBody.id = "custom_asteroid";
        m_editingBody.name = "Custom Minor Body";
        m_editingBody.type = (objectType == "Comet") ? "Comet" : "C-Type Asteroid";
        m_editingBody.color = glm::vec3(0.6f, 0.5f, 0.4f);
        m_editingBody.massKg = 1.0e18;
        m_editingBody.radiusM = 60000.0; // 60 km
        m_editingBody.semiMajorAxisAU = 2.8;
        m_editingBody.eccentricity = 0.15;
    } else if (objectType == "Black Hole") {
        m_editingBody.id = "custom_black_hole";
        m_editingBody.name = "Custom Singularity";
        m_editingBody.type = "Stellar Mass Black Hole";
        m_editingBody.color = glm::vec3(0.7f, 0.2f, 0.95f);
        m_editingBody.massKg = 10.0 * UnitConverter::SOLAR_MASS_KG;
        m_editingBody.radiusM = 29530.0; // Rs
        m_editingBody.surfaceTempK = 6.17e-9;
    }

    recomputeDerived(m_editingBody);
    m_selectedObjectId = 0;
    m_selectedObjectSlug = m_editingBody.id;
    m_isDirty = true;
}

void ObjectWorkspaceUI::recomputeDerived(CelestialBody& body) {
    body.realRadiusAU = (body.radiusM > 0.0) ? (body.radiusM / UnitConverter::AU_TO_METERS) : 0.0;
    body.radius3D = VisualStateAdapter::calculateRenderRadius(body.radiusM, body.realRadiusAU, false, 1.0f, 1.0f);

    // 1. Mean Density
    if (body.radiusM > 0.0 && body.massKg > 0.0) {
        double vol = (4.0 / 3.0) * UnitConverter::PI * std::pow(body.radiusM, 3.0);
        body.meanDensityKgM3 = body.massKg / vol;
        char densBuf[64];
        snprintf(densBuf, sizeof(densBuf), "%'.1f kg/m³", body.meanDensityKgM3);
        body.densityStr = densBuf;
    }

    // 2. Surface Gravity & Escape Velocity
    if (body.radiusM > 0.0 && body.massKg > 0.0) {
        body.surfaceGravityMps2 = (UnitConverter::G_CONST * body.massKg) / (body.radiusM * body.radiusM);
        char gravBuf[64];
        snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s² (%.2f g)", body.surfaceGravityMps2, body.surfaceGravityMps2 / 9.80665);
        body.gravityStr = gravBuf;

        body.escapeVelocityKmpS = std::sqrt(2.0 * UnitConverter::G_CONST * body.massKg / body.radiusM) / 1000.0;
        char escBuf[64];
        snprintf(escBuf, sizeof(escBuf), "%.2f km/s", body.escapeVelocityKmpS);
        body.escapeVelocityStr = escBuf;
    }

    // 3. String representations
    char radBuf[64];
    snprintf(radBuf, sizeof(radBuf), "%'.1f km", body.radiusM / 1000.0);
    body.radiusStr = radBuf;
    body.massStr = UnitConverter::formatMass(body.massKg);

    char tiltBuf[32];
    snprintf(tiltBuf, sizeof(tiltBuf), "%.2f°", body.axialTiltDeg);
    body.axialTiltStr = tiltBuf;
}


void ObjectWorkspaceUI::render(ObjectRepository& objRepo, 
                              PhysicsEngine& physics, 
                              Camera& camera,
                              int& activeTopTab,
                              float winW, float winH) {
    float topOffset = 48.0f;
    float statusH = 28.0f;
    float contentW = winW;
    float contentH = winH - topOffset - statusH;

    ImGui::SetNextWindowPos(ImVec2(0, topOffset));
    ImGui::SetNextWindowSize(ImVec2(contentW, contentH));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 12));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, Col::BgDark);

    ImGui::Begin("##ObjectWorkspaceRoot", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    // Header
    ImGui::TextColored(Col::Accent, "🪐 CELESTIAL OBJECT WORKSPACE");
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "| Individual Astronomical Object Library & Physical Property Editor");
    ImGui::Separator();
    ImGui::Spacing();

    float libW = 340.0f;
    float editorW = contentW - libW - 36.0f;
    float bodyH = contentH - 48.0f;

    // 1. Library Panel (Left)
    drawLibraryPanel(objRepo, libW, bodyH);

    ImGui::SameLine();

    // 2. Context-Sensitive Property Editor (Right)
    drawEditorPanel(objRepo, physics, camera, activeTopTab, editorW, bodyH);

    ImGui::End();
    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void ObjectWorkspaceUI::drawLibraryPanel(ObjectRepository& objRepo, float panelW, float panelH) {
    ImGui::BeginChild("##ObjLibraryPanel", ImVec2(panelW, panelH), true);

    ImGui::TextColored(Col::Accent, "OBJECT LIBRARY");
    ImGui::Spacing();

    // Create New Object Menu / Button
    if (ImGui::Button("➕ CREATE NEW OBJECT", ImVec2(-1, 28))) {
        ImGui::OpenPopup("CreateNewObjPopup");
    }

    if (ImGui::BeginPopup("CreateNewObjPopup")) {
        ImGui::TextColored(Col::Accent, "Select Celestial Object Class:");
        ImGui::Separator();
        if (ImGui::Selectable("★ Star (Main Sequence, Red Dwarf, Giant)")) createNewObjectTemplate("Star");
        if (ImGui::Selectable("● Planet (Terrestrial, Gas Giant, Super-Earth)")) createNewObjectTemplate("Planet");
        if (ImGui::Selectable("◐ Moon (Major Moon, Icy Moon)")) createNewObjectTemplate("Moon");
        if (ImGui::Selectable("☄ Asteroid (Carbonaceous, Metallic)")) createNewObjectTemplate("Asteroid");
        if (ImGui::Selectable("☄ Comet (Active Icy Nucleus)")) createNewObjectTemplate("Comet");
        if (ImGui::Selectable("🕳 Black Hole (Stellar Mass, Intermediate)")) createNewObjectTemplate("Black Hole");
        ImGui::EndPopup();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Category Tabs / Filter
    const char* catFilters[] = { "All", "Stars", "Planets", "Moons", "Asteroids", "Black Holes", "Custom" };
    ImGui::TextColored(Col::TextSecondary, "Filter Class:");
    for (int i = 0; i < 7; ++i) {
        if (i == 4) ImGui::NewLine();
        else if (i > 0) ImGui::SameLine(0, 4);

        bool isSel = (m_selectedCategoryFilter == i);
        if (isSel) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.80f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.07f, 0.11f, 0.18f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::SmallButton(catFilters[i])) {
            m_selectedCategoryFilter = i;
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Spacing();
    ImGui::PushItemWidth(-1);
    ImGui::InputTextWithHint("##ObjSearch", "Search library...", m_searchFilter, sizeof(m_searchFilter));
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // List Objects from Database
    auto allObjs = objRepo.getAllObjects("", true, m_searchFilter);

    ImGui::BeginChild("##ObjListScroll", ImVec2(0, panelH - 180), false);
    for (const auto& obj : allObjs) {
        // Filter by class
        if (m_selectedCategoryFilter == 1 && obj.type.find("Star") == std::string::npos) continue;
        if (m_selectedCategoryFilter == 2 && (obj.type.find("Planet") == std::string::npos || obj.type.find("Moon") != std::string::npos)) continue;
        if (m_selectedCategoryFilter == 3 && obj.type.find("Moon") == std::string::npos) continue;
        if (m_selectedCategoryFilter == 4 && obj.type.find("Asteroid") == std::string::npos && obj.type.find("Comet") == std::string::npos) continue;
        if (m_selectedCategoryFilter == 5 && obj.type.find("Black Hole") == std::string::npos) continue;
        if (m_selectedCategoryFilter == 6 && obj.category != "Custom") continue;

        bool isSelected = (obj.slug == m_selectedObjectSlug);
        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, Col::SelectedBg);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        }

        std::string icon = "●";
        if (obj.type.find("Star") != std::string::npos) icon = "★";
        else if (obj.type.find("Moon") != std::string::npos) icon = "◐";
        else if (obj.type.find("Black Hole") != std::string::npos) icon = "🕳";
        else if (obj.type.find("Asteroid") != std::string::npos) icon = "☄";

        std::string label = icon + " " + obj.name + " (" + obj.type + ")";
        if (ImGui::Selectable(label.c_str(), isSelected)) {
            setSelectedObjectBySlug(obj.slug, objRepo);
        }

        if (isSelected) {
            ImGui::PopStyleColor(2);
        }
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void ObjectWorkspaceUI::drawEditorPanel(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float panelW, float panelH) {
    ImGui::BeginChild("##ObjEditorMainPanel", ImVec2(panelW, panelH), true);

    ImGui::TextColored(Col::Accent, "OBJECT PROPERTY INSPECTOR & COMPOSITION");
    ImGui::SameLine(panelW - 280.0f);
    ImGui::TextColored(Col::TextSecondary, "Source: %s", m_editingBody.sourceName.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Identity
    if (ImGui::CollapsingHeader("IDENTITY & PROVENANCE", ImGuiTreeNodeFlags_DefaultOpen)) {
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", m_editingBody.name.c_str());
        ImGui::TextColored(Col::TextSecondary, "Display Name:");
        if (ImGui::InputText("##EditObjName", nameBuf, sizeof(nameBuf))) {
            m_editingBody.name = nameBuf;
            m_isDirty = true;
        }

        char slugBuf[64];
        snprintf(slugBuf, sizeof(slugBuf), "%s", m_editingBody.id.c_str());
        ImGui::TextColored(Col::TextSecondary, "Unique Slug Code:");
        if (ImGui::InputText("##EditObjSlug", slugBuf, sizeof(slugBuf))) {
            m_editingBody.id = slugBuf;
            m_isDirty = true;
        }

        char typeBuf[64];
        snprintf(typeBuf, sizeof(typeBuf), "%s", m_editingBody.type.c_str());
        ImGui::TextColored(Col::TextSecondary, "Classification:");
        if (ImGui::InputText("##EditObjType", typeBuf, sizeof(typeBuf))) {
            m_editingBody.type = typeBuf;
            m_isDirty = true;
        }

        ImGui::TextColored(Col::TextSecondary, "Color:");
        float col[3] = { m_editingBody.color.r, m_editingBody.color.g, m_editingBody.color.b };
        if (ImGui::ColorEdit3("##EditObjCol", col)) {
            m_editingBody.color = glm::vec3(col[0], col[1], col[2]);
            m_isDirty = true;
        }
    }

    // 2. Context-Sensitive Editor Body based on type
    if (m_editingBody.type.find("Star") != std::string::npos) {
        drawStarEditor(m_editingBody);
    } else if (m_editingBody.type.find("Moon") != std::string::npos) {
        drawMoonEditor(m_editingBody, objRepo);
    } else if (m_editingBody.type.find("Black Hole") != std::string::npos) {
        drawBlackHoleEditor(m_editingBody);
    } else if (m_editingBody.type.find("Asteroid") != std::string::npos || m_editingBody.type.find("Comet") != std::string::npos) {
        drawAsteroidCometEditor(m_editingBody);
    } else {
        drawPlanetEditor(m_editingBody);
    }

    // 3. Composition Editor
    drawCompositionEditor(m_editingBody);

    // 4. Action Footer
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.45f, 0.25f, 0.9f));
    if (ImGui::Button("💾 SAVE OBJECT TO LIBRARY", ImVec2(200, 30))) {
        recomputeDerived(m_editingBody);
        int64_t newId = 0;
        objRepo.saveCelestialBody(m_editingBody, &newId);
        m_editingBody.dbId = newId;
        m_selectedObjectId = newId;
        m_selectedObjectSlug = m_editingBody.id;
        m_statusFeedbackMsg = "Object '" + m_editingBody.name + "' successfully saved in SQLite library.";
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.65f, 0.85f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    if (ImGui::Button("🚀 TEST RUN IN SIMULATION", ImVec2(200, 30))) {
        recomputeDerived(m_editingBody);
        objRepo.saveCelestialBody(m_editingBody);
        physics.clearBodies();
        physics.addBody(m_editingBody);
        camera.resetOverview(glm::vec3(0.0f), 4.0f);
        activeTopTab = 0; // UNIVERSE
    }
    ImGui::PopStyleColor(2);

    if (!m_statusFeedbackMsg.empty()) {
        ImGui::SameLine(0, 16);
        ImGui::TextColored(Col::Green, "%s", m_statusFeedbackMsg.c_str());
    }

    ImGui::EndChild();
}

void ObjectWorkspaceUI::drawStarEditor(CelestialBody& body) {
    if (ImGui::CollapsingHeader("STAR ASTROPHYSICS & THERMAL DYNAMICS", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double mSun = body.massKg / UnitConverter::SOLAR_MASS_KG;
        float mF = (float)mSun;
        if (ImGui::DragFloat("Stellar Mass (M☉)##mStar", &mF, 0.05f, 0.01f, 300.0f, "%.3f M☉")) {
            body.massKg = (double)mF * UnitConverter::SOLAR_MASS_KG;
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double rSun = body.radiusM / UnitConverter::SOLAR_RADIUS_M;
        float rF = (float)rSun;
        if (ImGui::DragFloat("Stellar Radius (R☉)##rStar", &rF, 0.05f, 0.01f, 1500.0f, "%.3f R☉")) {
            body.radiusM = (double)rF * UnitConverter::SOLAR_RADIUS_M;
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        float tempF = (float)body.surfaceTempK;
        if (ImGui::DragFloat("Surface Effective Temp (K)##tStar", &tempF, 25.0f, 1000.0f, 50000.0f, "%.0f K")) {
            body.surfaceTempK = tempF;
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Mean Density: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextPrimary, "%s", body.densityStr.c_str());

        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Surface Gravity: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextPrimary, "%s", body.gravityStr.c_str());
    }
}

void ObjectWorkspaceUI::drawPlanetEditor(CelestialBody& body) {
    if (ImGui::CollapsingHeader("PLANETARY GEOPHYSICS & ORBIT", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double mEarth = body.massKg / UnitConverter::EARTH_MASS_KG;
        float mF = (float)mEarth;
        if (ImGui::DragFloat("Mass (M⊕)##mPl", &mF, 0.05f, 0.001f, 5000.0f, "%.3f M⊕")) {
            body.massKg = (double)mF * UnitConverter::EARTH_MASS_KG;
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double rKm = body.radiusM / 1000.0;
        float rF = (float)rKm;
        if (ImGui::DragFloat("Radius (km)##rPl", &rF, 10.0f, 100.0f, 150000.0f, "%.1f km")) {
            body.radiusM = (double)rF * 1000.0;
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        float smaF = (float)body.semiMajorAxisAU;
        if (ImGui::DragFloat("Semi-Major Axis (AU)##smaPl", &smaF, 0.05f, 0.01f, 100.0f, "%.3f AU")) {
            body.semiMajorAxisAU = smaF;
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        float eccF = (float)body.eccentricity;
        if (ImGui::SliderFloat("Eccentricity (e)##eccPl", &eccF, 0.0f, 0.95f, "%.4f")) {
            body.eccentricity = eccF;
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Density: %s | Surface Gravity: %s | Escape Velocity: %s", 
                           body.densityStr.c_str(), body.gravityStr.c_str(), body.escapeVelocityStr.c_str());
    }
}

void ObjectWorkspaceUI::drawMoonEditor(CelestialBody& body, ObjectRepository& objRepo) {
    if (ImGui::CollapsingHeader("MOON ORBITAL STATE & PARENT BODY", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double mMon = body.massKg / UnitConverter::LUNAR_MASS_KG;
        float mF = (float)mMon;
        if (ImGui::DragFloat("Mass (M_Moon)##mMon", &mF, 0.05f, 0.001f, 100.0f, "%.3f M_Moon")) {
            body.massKg = (double)mF * UnitConverter::LUNAR_MASS_KG;
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double rKm = body.radiusM / 1000.0;
        float rF = (float)rKm;
        if (ImGui::DragFloat("Radius (km)##rMon", &rF, 5.0f, 1.0f, 5000.0f, "%.1f km")) {
            body.radiusM = (double)rF * 1000.0;
            recomputeDerived(body);
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Density: %s | Surface Gravity: %s", body.densityStr.c_str(), body.gravityStr.c_str());
    }
}

void ObjectWorkspaceUI::drawAsteroidCometEditor(CelestialBody& body) {
    if (ImGui::CollapsingHeader("MINOR BODY & ASTEROID PARAMETERS", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        float mKgExp = (float)std::log10(std::max(1.0, body.massKg));
        if (ImGui::SliderFloat("Mass log10(kg)##mAst", &mKgExp, 10.0f, 22.0f, "10^%.1f kg")) {
            body.massKg = std::pow(10.0, (double)mKgExp);
            recomputeDerived(body);
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double rKm = body.radiusM / 1000.0;
        float rF = (float)rKm;
        if (ImGui::DragFloat("Mean Radius (km)##rAst", &rF, 1.0f, 0.1f, 500.0f, "%.1f km")) {
            body.radiusM = (double)rF * 1000.0;
            recomputeDerived(body);
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Density: %s | Gravity: %s", body.densityStr.c_str(), body.gravityStr.c_str());
    }
}

void ObjectWorkspaceUI::drawBlackHoleEditor(CelestialBody& body) {
    if (ImGui::CollapsingHeader("GENERAL RELATIVISTIC BLACK HOLE PARAMETERS", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        double mSun = body.massKg / UnitConverter::SOLAR_MASS_KG;
        float mF = (float)mSun;
        if (ImGui::DragFloat("Mass (M☉)##mBH", &mF, 0.5f, 1.0f, 1e9f, "%.1f M☉")) {
            body.massKg = (double)mF * UnitConverter::SOLAR_MASS_KG;
            // Rs = 2GM/c^2
            body.radiusM = (2.0 * UnitConverter::G_CONST * body.massKg) / (UnitConverter::SPEED_OF_LIGHT * UnitConverter::SPEED_OF_LIGHT);
            recomputeDerived(body);
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        double rsKm = body.radiusM / 1000.0;
        ImGui::TextColored(Col::TextPrimary, "Event Horizon (Schwarzschild Radius Rs): %.2f km", rsKm);

        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        double rIscoKm = rsKm * 3.0;
        ImGui::TextColored(Col::TextPrimary, "ISCO (Innermost Stable Orbit): %.2f km", rIscoKm);

        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        double hawkingK = 6.17e-8 / std::max(0.1, mSun);
        ImGui::TextColored(Col::TextPrimary, "Hawking Radiation Temperature: %.3e K", hawkingK);
    }
}

void ObjectWorkspaceUI::drawCompositionEditor(CelestialBody& body) {
    if (ImGui::CollapsingHeader("CHEMICAL COMPOSITION", ImGuiTreeNodeFlags_DefaultOpen)) {
        if (body.composition.empty()) {
            ImGui::TextColored(Col::TextSecondary, "No composition elements specified.");
        } else {
            for (size_t i = 0; i < body.composition.size(); ++i) {
                auto& c = body.composition[i];
                ImGui::PushID((int)i);
                ImGui::TextColored(Col::Accent, "• %s: %.1f%%", c.name.c_str(), c.percentage);
                ImGui::PopID();
            }
        }
    }
}

} // namespace AstroGenesis
