#include "ui/SystemWorkspaceUI.hpp"
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
    static ImVec4 BgPopup      {0.050f, 0.075f, 0.130f, 0.98f};
    static ImVec4 Border       {0.120f, 0.180f, 0.280f, 0.60f};
    static ImVec4 BorderLight  {0.180f, 0.280f, 0.420f, 0.75f};
    static ImVec4 Accent       {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 AccentDim    {0.000f, 0.500f, 0.700f, 0.70f};
    static ImVec4 AccentHover  {0.200f, 0.920f, 1.000f, 1.00f};
    static ImVec4 TextPrimary  {0.900f, 0.930f, 0.970f, 1.00f};
    static ImVec4 TextSecondary{0.460f, 0.540f, 0.680f, 1.00f};
    static ImVec4 SelectedBg   {0.000f, 0.600f, 0.850f, 0.22f};
    static ImVec4 Green        {0.150f, 0.880f, 0.450f, 1.00f};
    static ImVec4 Yellow       {0.980f, 0.780f, 0.120f, 1.00f};
    static ImVec4 Orange       {0.980f, 0.550f, 0.150f, 1.00f};
    static ImVec4 Red          {0.950f, 0.250f, 0.200f, 1.00f};
    static ImVec4 Purple       {0.750f, 0.350f, 0.950f, 1.00f};
}

SystemWorkspaceUI::SystemWorkspaceUI() {
    m_builderSystem.name = "My Custom Star System";
    m_builderSystem.type = "Custom";
    m_builderSystem.source = "User";
    m_builderSystem.description = "Custom multi-body celestial simulation";
    openCustomBuilderNew();
}

void SystemWorkspaceUI::openCustomBuilderNew() {
    m_currentMode = 1;
    m_builderBodies.clear();
    m_selectedNodeIndex = 0;
    snprintf(m_systemNameBuf, sizeof(m_systemNameBuf), "My Custom Star System");
    snprintf(m_systemDescBuf, sizeof(m_systemDescBuf), "Custom multi-body celestial system");

    // Add a default primary star
    CelestialBody star;
    star.dbId = 1;
    star.id = "primary_star";
    star.name = "Sol-Like Star";
    star.type = "G2V Main Sequence Star";
    star.category = m_systemNameBuf;
    star.color = glm::vec3(1.0f, 0.82f, 0.35f);
    star.massKg = UnitConverter::SOLAR_MASS_KG;
    star.radiusM = UnitConverter::SOLAR_RADIUS_M;
    star.surfaceTempK = 5778.0;
    star.luminosityW = UnitConverter::SOLAR_LUMINOSITY_W;
    star.positionM = glm::dvec3(0.0);
    star.velocityMps = glm::dvec3(0.0);
    recomputeDerivedProperties(star, nullptr);
    m_builderBodies.push_back(star);

    // Add a default habitable zone planet
    CelestialBody planet;
    planet.dbId = 2;
    planet.id = "planet_a";
    planet.name = "Terra Nova";
    planet.type = "Terrestrial Planet";
    planet.category = m_systemNameBuf;
    planet.parentObjectId = 1;
    planet.color = glm::vec3(0.15f, 0.70f, 0.95f);
    planet.massKg = UnitConverter::EARTH_MASS_KG;
    planet.radiusM = UnitConverter::EARTH_RADIUS_M;
    planet.semiMajorAxisAU = 1.0;
    planet.eccentricity = 0.0167;
    applyOrbitInitializer(planet, star, 1.0, 0.0167, 0.0);
    m_builderBodies.push_back(planet);
}

void SystemWorkspaceUI::openCustomBuilderWithSystem(const std::string& systemName, ObjectRepository& objRepo) {
    loadSystemIntoBuilder(systemName, objRepo);
    m_currentMode = 1;
}

void SystemWorkspaceUI::loadSystemIntoBuilder(const std::string& systemName, ObjectRepository& objRepo) {
    auto sysOpt = objRepo.getSystemByName(systemName);
    if (sysOpt.has_value()) {
        m_builderSystem = sysOpt.value();
    } else {
        m_builderSystem.name = systemName;
        m_builderSystem.type = "Custom";
        m_builderSystem.source = "User";
        m_builderSystem.description = "Imported system instance";
    }

    snprintf(m_systemNameBuf, sizeof(m_systemNameBuf), "%s", m_builderSystem.name.c_str());
    snprintf(m_systemDescBuf, sizeof(m_systemDescBuf), "%s", m_builderSystem.description.c_str());

    m_builderBodies = objRepo.getSystemBodies(systemName);
    m_selectedNodeIndex = 0;
    for (auto& b : m_builderBodies) {
        CelestialBody* parentPtr = nullptr;
        if (b.parentObjectId.has_value()) {
            for (auto& p : m_builderBodies) {
                if (p.dbId == b.parentObjectId.value()) { parentPtr = &p; break; }
            }
        }
        recomputeDerivedProperties(b, parentPtr);
    }
}

void SystemWorkspaceUI::recomputeDerivedProperties(CelestialBody& body, const CelestialBody* parent) {
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

    // 3. Radius & Mass Presentation Strings
    char radBuf[64];
    snprintf(radBuf, sizeof(radBuf), "%'.1f km", body.radiusM / 1000.0);
    body.radiusStr = radBuf;
    body.massStr = UnitConverter::formatMass(body.massKg);

    // 4. Orbital Parameters derivation if parent exists
    if (parent && body.semiMajorAxisAU > 0.0) {
        double aM = body.semiMajorAxisAU * UnitConverter::AU_TO_METERS;
        body.semiMajorAxisM = aM;
        double mu = UnitConverter::G_CONST * (parent->massKg + body.massKg);
        if (mu > 0.0) {
            double periodSec = 2.0 * UnitConverter::PI * std::sqrt(std::pow(aM, 3.0) / mu);
            body.orbitalPeriodDays = periodSec / UnitConverter::SEC_PER_DAY;
            body.orbitalPeriodStr = UnitConverter::formatPeriod(periodSec);

            double meanVelMps = std::sqrt(mu / aM);
            body.orbitalSpeedKmpS = meanVelMps / 1000.0;
            char spdBuf[64];
            snprintf(spdBuf, sizeof(spdBuf), "%.2f km/s", body.orbitalSpeedKmpS);
            body.orbitalSpeedStr = spdBuf;
        }

        char smaBuf[64], eccBuf[32];
        snprintf(smaBuf, sizeof(smaBuf), "%.3f AU (%.1fM km)", body.semiMajorAxisAU, body.semiMajorAxisAU * UnitConverter::AU_TO_KM / 1e6);
        body.semiMajorAxisStr = smaBuf;
        snprintf(eccBuf, sizeof(eccBuf), "%.4f", body.eccentricity);
        body.eccentricityStr = eccBuf;

        double periAU = body.semiMajorAxisAU * (1.0 - body.eccentricity);
        double apoAU  = body.semiMajorAxisAU * (1.0 + body.eccentricity);
        char pBuf[64], aBuf[64];
        snprintf(pBuf, sizeof(pBuf), "%.3f AU", periAU);
        snprintf(aBuf, sizeof(aBuf), "%.3f AU", apoAU);
        body.periapsisStr = pBuf;
        body.apoapsisStr = aBuf;
    }
}

void SystemWorkspaceUI::applyOrbitInitializer(CelestialBody& body, const CelestialBody& parent, double distAU, double ecc, double incDeg) {
    body.semiMajorAxisAU = distAU;
    body.eccentricity = ecc;
    double aM = distAU * UnitConverter::AU_TO_METERS;
    body.semiMajorAxisM = aM;

    double posX, posY, posZ, velX, velY, velZ;
    UnitConverter::keplerianToCartesian(aM, ecc, incDeg, 0.0, 0.0, 0.0, parent.massKg, body.massKg,
                                       posX, posY, posZ, velX, velY, velZ);

    body.positionM = parent.positionM + glm::dvec3(posX, posY, posZ);
    body.velocityMps = parent.velocityMps + glm::dvec3(velX, velY, velZ);
    body.position = glm::vec3((float)(body.positionM.x / UnitConverter::AU_TO_METERS),
                              (float)(body.positionM.y / UnitConverter::AU_TO_METERS),
                              (float)(body.positionM.z / UnitConverter::AU_TO_METERS));
    body.velocity = glm::vec3((float)(body.velocityMps.x / UnitConverter::AU_TO_METERS),
                              (float)(body.velocityMps.y / UnitConverter::AU_TO_METERS),
                              (float)(body.velocityMps.z / UnitConverter::AU_TO_METERS));

    recomputeDerivedProperties(body, &parent);
}

void SystemWorkspaceUI::createNewDefaultObject(const std::string& type, std::optional<int64_t> parentId) {
    int64_t nextId = 1;
    for (const auto& b : m_builderBodies) {
        if (b.dbId >= nextId) nextId = b.dbId + 1;
    }

    CelestialBody b;
    b.dbId = nextId;
    b.category = m_systemNameBuf;
    b.parentObjectId = parentId;

    if (type == "Star") {
        b.id = "star_" + std::to_string(nextId);
        b.name = "Companion Star " + std::to_string(nextId);
        b.type = "K-Type Orange Dwarf Star";
        b.color = glm::vec3(1.0f, 0.65f, 0.2f);
        b.massKg = 0.8 * UnitConverter::SOLAR_MASS_KG;
        b.radiusM = 0.85 * UnitConverter::SOLAR_RADIUS_M;
        b.surfaceTempK = 4800.0;
        b.positionM = glm::dvec3(5.0 * UnitConverter::AU_TO_METERS, 0.0, 0.0);
    } else if (type == "Planet") {
        b.id = "planet_" + std::to_string(nextId);
        b.name = "Planet " + std::to_string(nextId);
        b.type = "Terrestrial Planet";
        b.color = glm::vec3(0.3f, 0.75f, 0.6f);
        b.massKg = 1.5 * UnitConverter::EARTH_MASS_KG;
        b.radiusM = 1.15 * UnitConverter::EARTH_RADIUS_M;
        b.surfaceTempK = 260.0;
        b.semiMajorAxisAU = 1.5;
        b.eccentricity = 0.02;
    } else if (type == "Moon") {
        b.id = "moon_" + std::to_string(nextId);
        b.name = "Moon " + std::to_string(nextId);
        b.type = "Planetary Moon";
        b.color = glm::vec3(0.75f, 0.75f, 0.78f);
        b.massKg = UnitConverter::LUNAR_MASS_KG;
        b.radiusM = UnitConverter::LUNAR_RADIUS_M;
        b.surfaceTempK = 220.0;
        b.semiMajorAxisAU = 0.00257; // ~384,000 km
    } else if (type == "Asteroid") {
        b.id = "asteroid_" + std::to_string(nextId);
        b.name = "Asteroid " + std::to_string(nextId);
        b.type = "C-Type Asteroid";
        b.color = glm::vec3(0.6f, 0.55f, 0.5f);
        b.massKg = 5.0e17;
        b.radiusM = 45000.0;
        b.semiMajorAxisAU = 2.7;
    } else if (type == "Black Hole") {
        b.id = "black_hole_" + std::to_string(nextId);
        b.name = "Singularity " + std::to_string(nextId);
        b.type = "Stellar Mass Black Hole";
        b.color = glm::vec3(0.65f, 0.15f, 0.9f);
        b.massKg = 10.0 * UnitConverter::SOLAR_MASS_KG;
        b.radiusM = 29530.0; // Rs
        b.surfaceTempK = 0.0;
    }

    const CelestialBody* pPtr = nullptr;
    if (parentId.has_value()) {
        for (const auto& p : m_builderBodies) {
            if (p.dbId == parentId.value()) { pPtr = &p; break; }
        }
    } else if (!m_builderBodies.empty()) {
        pPtr = &m_builderBodies[0];
    }

    if (pPtr && b.semiMajorAxisAU > 0.0) {
        applyOrbitInitializer(b, *pPtr, b.semiMajorAxisAU, b.eccentricity, 0.0);
    } else {
        recomputeDerivedProperties(b, pPtr);
    }

    m_builderBodies.push_back(b);
    m_selectedNodeIndex = (int)m_builderBodies.size() - 1;
}

void SystemWorkspaceUI::render(DataManager& dataManager, 
                              ObjectRepository& objRepo, 
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

    ImGui::Begin("##SystemWorkspaceRoot", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);

    drawHeaderAndModes(contentW);

    float bodyH = contentH - 64.0f;
    if (m_currentMode == 0) {
        drawImportMode(dataManager, objRepo, physics, camera, activeTopTab, contentW, bodyH);
    } else if (m_currentMode == 1) {
        drawCustomBuilderMode(objRepo, physics, camera, activeTopTab, contentW, bodyH);
    } else if (m_currentMode == 2) {
        drawSavedSystemsMode(objRepo, physics, camera, activeTopTab, contentW, bodyH);
    } else if (m_currentMode == 3) {
        drawPresetsMode(objRepo, physics, camera, activeTopTab, contentW, bodyH);
    }

    if (m_showValidationWarningPopup) {
        drawValidationModal(objRepo, physics, camera, activeTopTab);
    }

    ImGui::End();

    ImGui::PopStyleColor();
    ImGui::PopStyleVar(2);
}

void SystemWorkspaceUI::drawHeaderAndModes(float winW) {
    ImGui::BeginGroup();
    ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
    ImGui::Text("🌌 SYSTEM WORKSPACE");
    ImGui::PopStyleColor();
    ImGui::SameLine();
    ImGui::TextColored(Col::TextSecondary, "| Create, Import & Manage Multi-Body Celestial Architectures");
    ImGui::EndGroup();

    ImGui::SameLine(winW - 680.0f);

    // Mode Selector Buttons
    const char* modeLabels[] = { "📥 IMPORT EXISTING", "🛠 CREATE CUSTOM", "📂 SAVED SYSTEMS", "⚡ PRESETS" };
    for (int i = 0; i < 4; ++i) {
        if (i > 0) ImGui::SameLine(0, 6);
        bool isActive = (m_currentMode == i);
        if (isActive) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.80f, 0.90f));
            ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.07f, 0.11f, 0.18f, 0.85f));
            ImGui::PushStyleColor(ImGuiCol_Text, Col::TextSecondary);
        }
        if (ImGui::Button(modeLabels[i], ImVec2(150, 26))) {
            m_currentMode = i;
        }
        ImGui::PopStyleColor(2);
    }

    ImGui::Separator();
    ImGui::Spacing();
}

void SystemWorkspaceUI::drawImportMode(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH) {
    float leftW = 340.0f;
    float rightW = contentW - leftW - 32.0f;

    // Left Column: Source Selection & Search
    ImGui::BeginChild("##ImportLeft", ImVec2(leftW, contentH), true);
    ImGui::TextColored(Col::Accent, "DATA SOURCE");
    ImGui::Spacing();

    const char* providers[] = { "JPL Horizons Ephemeris", "JPL Small-Body DB (SBDB)", "NASA Exoplanet Archive", "Bundled Seed Catalog" };
    for (int i = 0; i < 4; ++i) {
        bool selected = (m_selectedProviderIdx == i);
        if (ImGui::RadioButton(providers[i], selected)) {
            m_selectedProviderIdx = i;
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(Col::Accent, "SEARCH ASTRONOMICAL SYSTEM");
    ImGui::PushItemWidth(-1);
    bool enterPressed = ImGui::InputTextWithHint("##importSearch", "e.g. Kepler-90, TRAPPIST-1, Solar System...", m_searchBuffer, sizeof(m_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    ImGui::PopItemWidth();

    ImGui::Spacing();
    if (ImGui::Button("🔍 Search External Data", ImVec2(-1, 28)) || enterPressed) {
        m_importPreviewBodies.clear();
        m_importSelectionFlags.clear();
        m_selectedImportSystemName = m_searchBuffer;

        if (m_selectedProviderIdx == 3) {
            // Local seed catalog lookup
            auto bodies = objRepo.getSystemBodies(m_searchBuffer);
            if (bodies.empty()) bodies = objRepo.getSystemBodies("Solar System");
            for (const auto& b : bodies) {
                CelestialBodyRecord rec;
                rec.object.slug = b.id;
                rec.object.name = b.name;
                rec.object.type = b.type;
                rec.object.category = b.category;
                rec.object.parentObjectId = b.parentObjectId;
                rec.physical.massKg = b.massKg;
                rec.physical.radiusM = b.radiusM;
                rec.physical.surfaceTempK = b.surfaceTempK;
                rec.orbital.semiMajorAxisAU = b.semiMajorAxisAU;
                rec.orbital.eccentricity = b.eccentricity;
                rec.sourceName = b.sourceName;
                m_importPreviewBodies.push_back(rec);
                m_importSelectionFlags.push_back(true);
            }
        } else {
            // Trigger DataManager search
            ProviderType pType = (m_selectedProviderIdx == 0) ? ProviderType::JPL_Horizons :
                                 (m_selectedProviderIdx == 1) ? ProviderType::JPL_SBDB : ProviderType::NASA_Exoplanet;
            dataManager.searchAsync(pType, m_searchBuffer);
            m_importLoading = true;
        }
    }

    if (m_importLoading) {
        if (!dataManager.isSearching()) {
            m_importLoading = false;
            auto results = dataManager.getSearchResults();
            if (!results.empty()) {
                m_selectedImportSystemName = results[0].name;
            }
        } else {
            ImGui::TextColored(Col::Yellow, "Querying astronomical authority...");
        }
    }

    ImGui::Spacing();
    ImGui::TextColored(Col::TextSecondary, "POPULAR REAL SYSTEMS:");
    const char* quickSearches[] = { "Kepler-90", "TRAPPIST-1 System", "Solar System", "Kepler-186 System", "Proxima Centauri", "Asteroid Belt" };
    for (const char* qs : quickSearches) {
        if (ImGui::SmallButton(qs)) {
            snprintf(m_searchBuffer, sizeof(m_searchBuffer), "%s", qs);
            m_selectedProviderIdx = 3; // Seed catalog
            auto bodies = objRepo.getSystemBodies(qs);
            m_importPreviewBodies.clear();
            m_importSelectionFlags.clear();
            m_selectedImportSystemName = qs;
            for (const auto& b : bodies) {
                CelestialBodyRecord rec;
                rec.object.slug = b.id;
                rec.object.name = b.name;
                rec.object.type = b.type;
                rec.object.category = b.category;
                rec.object.parentObjectId = b.parentObjectId;
                rec.physical.massKg = b.massKg;
                rec.physical.radiusM = b.radiusM;
                rec.physical.surfaceTempK = b.surfaceTempK;
                rec.orbital.semiMajorAxisAU = b.semiMajorAxisAU;
                rec.orbital.eccentricity = b.eccentricity;
                rec.sourceName = b.sourceName;
                m_importPreviewBodies.push_back(rec);
                m_importSelectionFlags.push_back(true);
            }
        }
        ImGui::SameLine();
    }
    ImGui::NewLine();

    ImGui::EndChild();

    ImGui::SameLine();

    // Right Column: System Preview & Ingestion
    ImGui::BeginChild("##ImportRight", ImVec2(rightW, contentH), true);
    ImGui::TextColored(Col::Accent, "SYSTEM PREVIEW & OBJECT SELECTION");
    ImGui::TextColored(Col::TextSecondary, "Select objects to include in the local simulation instance:");
    ImGui::Separator();
    ImGui::Spacing();

    if (m_importPreviewBodies.empty()) {
        ImGui::TextColored(Col::TextSecondary, "No system currently loaded for preview. Select a system on the left or enter a search query.");
    } else {
        ImGui::TextColored(Col::TextPrimary, "Target: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::Yellow, "%s", m_selectedImportSystemName.c_str());
        ImGui::SameLine(300);
        ImGui::TextColored(Col::TextSecondary, "Objects Detected: %zu", m_importPreviewBodies.size());

        ImGui::Spacing();
        if (ImGui::Button("☑ Select All")) {
            for (size_t k = 0; k < m_importSelectionFlags.size(); ++k) m_importSelectionFlags[k] = true;
        }
        ImGui::SameLine();
        if (ImGui::Button("☐ Deselect All")) {
            for (size_t k = 0; k < m_importSelectionFlags.size(); ++k) m_importSelectionFlags[k] = false;
        }

        ImGui::Spacing();
        ImGui::BeginChild("##ImportTableChild", ImVec2(0, contentH - 160), true);
        if (ImGui::BeginTable("##ImportObjTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
            ImGui::TableSetupColumn("Import", ImGuiTableColumnFlags_WidthFixed, 50);
            ImGui::TableSetupColumn("Object Name", ImGuiTableColumnFlags_WidthFixed, 180);
            ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 160);
            ImGui::TableSetupColumn("Mass", ImGuiTableColumnFlags_WidthFixed, 140);
            ImGui::TableSetupColumn("Semi-Major Axis", ImGuiTableColumnFlags_WidthFixed, 140);
            ImGui::TableSetupColumn("Source Provenance", ImGuiTableColumnFlags_WidthStretch);
            ImGui::TableHeadersRow();

            for (size_t i = 0; i < m_importPreviewBodies.size(); ++i) {
                const auto& rec = m_importPreviewBodies[i];
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0);
                bool sel = m_importSelectionFlags[i];
                char chkId[32];
                snprintf(chkId, sizeof(chkId), "##chk_%zu", i);
                if (ImGui::Checkbox(chkId, &sel)) {
                    m_importSelectionFlags[i] = sel;
                }

                ImGui::TableSetColumnIndex(1);
                bool isStar = rec.object.type.find("Star") != std::string::npos;
                ImGui::TextColored(isStar ? Col::Yellow : Col::Accent, "%s %s", isStar ? "★" : "●", rec.object.name.c_str());

                ImGui::TableSetColumnIndex(2);
                ImGui::TextUnformatted(rec.object.type.c_str());

                ImGui::TableSetColumnIndex(3);
                if (rec.physical.massKg.has_value()) {
                    ImGui::TextUnformatted(UnitConverter::formatMass(rec.physical.massKg.value()).c_str());
                } else {
                    ImGui::TextColored(Col::TextSecondary, "N/A");
                }

                ImGui::TableSetColumnIndex(4);
                if (rec.orbital.semiMajorAxisAU.has_value()) {
                    ImGui::Text("%.3f AU", rec.orbital.semiMajorAxisAU.value());
                } else {
                    ImGui::TextColored(Col::TextSecondary, "Central");
                }

                ImGui::TableSetColumnIndex(5);
                ImGui::TextColored(Col::TextSecondary, "%s", rec.sourceName.c_str());
            }
            ImGui::EndTable();
        }
        ImGui::EndChild();

        // Import Actions Footer
        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.45f, 0.25f, 0.9f));
        if (ImGui::Button("📥 IMPORT AS LOCAL SYSTEM", ImVec2(220, 32))) {
            // Save selected objects as a local system instance
            SystemRecord sysRec;
            sysRec.name = m_selectedImportSystemName;
            sysRec.type = "Imported";
            sysRec.source = "Imported Astronomical Data";
            sysRec.description = "Imported multi-body instance of " + m_selectedImportSystemName;

            std::vector<CelestialBody> toSave;
            for (size_t i = 0; i < m_importPreviewBodies.size(); ++i) {
                if (m_importSelectionFlags[i]) {
                    auto bOpt = objRepo.getHydratedBodyBySlug(m_importPreviewBodies[i].object.slug);
                    if (bOpt.has_value()) toSave.push_back(bOpt.value());
                }
            }

            if (!toSave.empty()) {
                objRepo.saveCustomSystem(sysRec, toSave);
                m_actionFeedbackMsg = "System '" + sysRec.name + "' successfully imported to local database (" + std::to_string(toSave.size()) + " bodies).";
            }
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.65f, 0.85f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
        if (ImGui::Button("🚀 RUN SIMULATION NOW", ImVec2(200, 32))) {
            physics.loadFromDatabase(objRepo, m_selectedImportSystemName);
            camera.resetOverview(glm::vec3(0.0f), 6.0f);
            activeTopTab = 0; // Switch to UNIVERSE live simulation
        }
        ImGui::PopStyleColor(2);

        if (!m_actionFeedbackMsg.empty()) {
            ImGui::SameLine(0, 16);
            ImGui::TextColored(Col::Green, "%s", m_actionFeedbackMsg.c_str());
        }
    }

    ImGui::EndChild();
}

void SystemWorkspaceUI::drawCustomBuilderMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH) {
    float treeW = 280.0f;
    float editorW = 380.0f;
    float canvasW = contentW - treeW - editorW - 36.0f;
    float actionH = 42.0f;
    float panelH = contentH - actionH;

    // 1. System Tree (Left)
    drawBuilderSystemTree(objRepo, treeW, panelH);

    ImGui::SameLine();

    // 2. Orbital Canvas & Validation (Center)
    drawBuilderSchematicCanvas(canvasW, panelH);

    ImGui::SameLine();

    // 3. Object Property Editor (Right)
    drawBuilderObjectEditor(objRepo, editorW, panelH);

    // 4. Action Footer
    drawBuilderActionFooter(objRepo, physics, camera, activeTopTab);
}

void SystemWorkspaceUI::drawBuilderSystemTree(ObjectRepository& objRepo, float panelW, float panelH) {
    ImGui::BeginChild("##BuilderTreePanel", ImVec2(panelW, panelH), true);

    ImGui::TextColored(Col::Accent, "SYSTEM HIERARCHY");
    ImGui::Spacing();

    // System Name Input
    ImGui::TextColored(Col::TextSecondary, "System Name:");
    ImGui::PushItemWidth(-1);
    if (ImGui::InputText("##SysNameInput", m_systemNameBuf, sizeof(m_systemNameBuf))) {
        m_builderSystem.name = m_systemNameBuf;
        for (auto& b : m_builderBodies) b.category = m_systemNameBuf;
    }
    ImGui::PopItemWidth();

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Add Object Quick Buttons
    ImGui::TextColored(Col::TextSecondary, "+ Add Celestial Body:");
    if (ImGui::SmallButton("+ Star")) createNewDefaultObject("Star");
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Planet")) {
        int64_t pId = 1;
        if (!m_builderBodies.empty()) pId = m_builderBodies[0].dbId;
        createNewDefaultObject("Planet", pId);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Moon")) {
        int64_t pId = 1;
        if (m_selectedNodeIndex >= 0 && m_selectedNodeIndex < (int)m_builderBodies.size()) {
            pId = m_builderBodies[m_selectedNodeIndex].dbId;
        }
        createNewDefaultObject("Moon", pId);
    }
    ImGui::SameLine();
    if (ImGui::SmallButton("+ Black Hole")) createNewDefaultObject("Black Hole");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Hierarchical Tree View of Bodies
    ImGui::TextColored(Col::TextSecondary, "Bodies in System (%zu):", m_builderBodies.size());

    ImGui::BeginChild("##TreeNodesChild", ImVec2(0, panelH - 160), false);
    for (int i = 0; i < (int)m_builderBodies.size(); ++i) {
        const auto& b = m_builderBodies[i];
        bool isSelected = (i == m_selectedNodeIndex);

        ImGui::PushID(i);
        std::string icon = "●";
        ImVec4 iconCol = Col::Accent;
        if (b.type.find("Star") != std::string::npos) { icon = "★"; iconCol = Col::Yellow; }
        else if (b.type.find("Moon") != std::string::npos) { icon = "◐"; iconCol = Col::TextSecondary; }
        else if (b.type.find("Black Hole") != std::string::npos) { icon = "🕳"; iconCol = Col::Purple; }
        else if (b.type.find("Asteroid") != std::string::npos) { icon = "☄"; iconCol = Col::Orange; }

        if (isSelected) {
            ImGui::PushStyleColor(ImGuiCol_Header, Col::SelectedBg);
            ImGui::PushStyleColor(ImGuiCol_Text, Col::Accent);
        }

        std::string label = icon + " " + b.name + " (" + b.type + ")";
        if (b.parentObjectId.has_value()) {
            label = "  └─ " + label;
        }

        if (ImGui::Selectable(label.c_str(), isSelected)) {
            m_selectedNodeIndex = i;
        }

        if (isSelected) {
            ImGui::PopStyleColor(2);
        }

        ImGui::PopID();
    }
    ImGui::EndChild();

    ImGui::EndChild();
}

void SystemWorkspaceUI::drawBuilderSchematicCanvas(float panelW, float panelH) {
    ImGui::BeginChild("##BuilderSchematicPanel", ImVec2(panelW, panelH), true);

    ImGui::TextColored(Col::Accent, "ORBITAL SCHEMATIC & PREVIEW");
    ImGui::SameLine(panelW - 140.0f);
    ImGui::TextColored(Col::TextSecondary, "Zoom: %.1fx", m_schematicZoom);

    ImGui::Separator();

    ImVec2 canvasPos = ImGui::GetCursorScreenPos();
    ImVec2 canvasSize = ImVec2(panelW - 16, panelH - 120);
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Canvas Background
    drawList->AddRectFilled(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(8, 12, 22, 255));
    drawList->AddRect(canvasPos, ImVec2(canvasPos.x + canvasSize.x, canvasPos.y + canvasSize.y), IM_COL32(30, 45, 70, 180));

    ImVec2 center = ImVec2(canvasPos.x + canvasSize.x * 0.5f + m_schematicPan.x,
                           canvasPos.y + canvasSize.y * 0.5f + m_schematicPan.y);

    // Coordinate grid / Distance rings (0.5 AU, 1 AU, 2 AU, 5 AU)
    float baseScale = (canvasSize.y * 0.40f) * m_schematicZoom; // 1 AU = baseScale pixels
    double ringDistances[] = { 0.387, 0.723, 1.0, 1.524, 2.77, 5.2 };
    for (double rAU : ringDistances) {
        float rPx = (float)(rAU * baseScale);
        if (rPx > 5.0f && rPx < canvasSize.x) {
            drawList->AddCircle(center, rPx, IM_COL32(25, 40, 65, 120), 64, 1.0f);
            char lbl[32];
            snprintf(lbl, sizeof(lbl), "%.2f AU", rAU);
            drawList->AddText(ImVec2(center.x + rPx + 4, center.y - 6), IM_COL32(70, 95, 130, 180), lbl);
        }
    }

    // Draw Orbits and Bodies
    for (int i = 0; i < (int)m_builderBodies.size(); ++i) {
        const auto& b = m_builderBodies[i];
        bool isSelected = (i == m_selectedNodeIndex);

        ImVec2 parentCenter = center;
        if (b.parentObjectId.has_value()) {
            for (const auto& p : m_builderBodies) {
                if (p.dbId == b.parentObjectId.value()) {
                    float px = (float)(p.position.x * baseScale);
                    float py = (float)(p.position.z * baseScale);
                    parentCenter = ImVec2(center.x + px, center.y + py);
                    break;
                }
            }
        }

        // Draw Elliptical Orbit Track
        if (b.semiMajorAxisAU > 0.0) {
            float aPx = (float)(b.semiMajorAxisAU * baseScale);
            float bPx = aPx * (float)std::sqrt(std::max(0.01, 1.0 - b.eccentricity * b.eccentricity));
            ImU32 orbitCol = isSelected ? IM_COL32(0, 215, 255, 200) : IM_COL32(40, 80, 130, 120);
            drawList->AddEllipse(parentCenter, ImVec2(aPx, bPx), orbitCol, 0.0f, 64, isSelected ? 1.5f : 1.0f);
        }

        // Draw Body Marker
        float bx = (float)(b.position.x * baseScale);
        float by = (float)(b.position.z * baseScale);
        ImVec2 bPos = ImVec2(center.x + bx, center.y + by);

        float markerR = 4.0f;
        if (b.type.find("Star") != std::string::npos) markerR = 8.0f;
        else if (b.type.find("Black Hole") != std::string::npos) markerR = 7.0f;

        ImU32 fillCol = IM_COL32((int)(b.color.r * 255), (int)(b.color.g * 255), (int)(b.color.b * 255), 255);
        drawList->AddCircleFilled(bPos, markerR, fillCol);
        if (isSelected) {
            drawList->AddCircle(bPos, markerR + 3.0f, IM_COL32(0, 255, 255, 255), 24, 2.0f);
        }

        drawList->AddText(ImVec2(bPos.x + markerR + 4, bPos.y - 7), isSelected ? IM_COL32(0, 255, 255, 255) : IM_COL32(180, 200, 225, 220), b.name.c_str());
    }

    // Pre-Flight Validation Banner
    ImGui::SetCursorScreenPos(ImVec2(canvasPos.x, canvasPos.y + canvasSize.y + 6));
    m_currentValidationWarnings = ObjectRepository(DatabaseManager::getInstance()).validateSystem(m_builderBodies);

    if (m_currentValidationWarnings.empty()) {
        ImGui::TextColored(Col::Green, "✔ Pre-flight Validation: All orbital parameters physically consistent & simulation-ready.");
    } else {
        ImGui::TextColored(Col::Orange, "⚠ Validation Alerts (%zu):", m_currentValidationWarnings.size());
        for (size_t k = 0; k < std::min((size_t)2, m_currentValidationWarnings.size()); ++k) {
            ImGui::TextColored(Col::Yellow, "  • %s: %s", m_currentValidationWarnings[k].title.c_str(), m_currentValidationWarnings[k].message.c_str());
        }
    }

    ImGui::EndChild();
}

void SystemWorkspaceUI::drawBuilderObjectEditor(ObjectRepository& objRepo, float panelW, float panelH) {
    ImGui::BeginChild("##BuilderEditorPanel", ImVec2(panelW, panelH), true);

    if (m_selectedNodeIndex < 0 || m_selectedNodeIndex >= (int)m_builderBodies.size()) {
        ImGui::TextColored(Col::TextSecondary, "Select a body from the system hierarchy to edit properties.");
        ImGui::EndChild();
        return;
    }

    CelestialBody& body = m_builderBodies[m_selectedNodeIndex];
    const CelestialBody* parentPtr = nullptr;
    if (body.parentObjectId.has_value()) {
        for (const auto& p : m_builderBodies) {
            if (p.dbId == body.parentObjectId.value()) { parentPtr = &p; break; }
        }
    }


    ImGui::TextColored(Col::Accent, "OBJECT PROPERTY INSPECTOR");
    ImGui::TextColored(Col::TextSecondary, "%s", body.name.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // 1. Identity Section
    if (ImGui::CollapsingHeader("IDENTITY & CLASSIFICATION", ImGuiTreeNodeFlags_DefaultOpen)) {
        char nameBuf[64];
        snprintf(nameBuf, sizeof(nameBuf), "%s", body.name.c_str());
        ImGui::TextColored(Col::TextSecondary, "Display Name:");
        if (ImGui::InputText("##ObjName", nameBuf, sizeof(nameBuf))) {
            body.name = nameBuf;
        }

        char typeBuf[64];
        snprintf(typeBuf, sizeof(typeBuf), "%s", body.type.c_str());
        ImGui::TextColored(Col::TextSecondary, "Object Classification:");
        if (ImGui::InputText("##ObjType", typeBuf, sizeof(typeBuf))) {
            body.type = typeBuf;
        }

        ImGui::TextColored(Col::TextSecondary, "Color Accent:");
        float col[3] = { body.color.r, body.color.g, body.color.b };
        if (ImGui::ColorEdit3("##ObjColor", col)) {
            body.color = glm::vec3(col[0], col[1], col[2]);
        }
    }

    // 2. Physical Parameters (Mass, Radius, Derived Density & Gravity)
    if (ImGui::CollapsingHeader("PHYSICAL PROPERTIES", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Mass:");
        
        double massVal = body.massKg;
        if (body.type.find("Star") != std::string::npos || body.type.find("Black Hole") != std::string::npos) {
            double massSun = body.massKg / UnitConverter::SOLAR_MASS_KG;
            float mF = (float)massSun;
            if (ImGui::DragFloat("Mass (M☉)##mSun", &mF, 0.05f, 0.01f, 1000.0f, "%.3f M☉")) {
                body.massKg = (double)mF * UnitConverter::SOLAR_MASS_KG;
                recomputeDerivedProperties(body, parentPtr);
            }
        } else {
            double massEarth = body.massKg / UnitConverter::EARTH_MASS_KG;
            float mF = (float)massEarth;
            if (ImGui::DragFloat("Mass (M⊕)##mEarth", &mF, 0.05f, 0.001f, 10000.0f, "%.3f M⊕")) {
                body.massKg = (double)mF * UnitConverter::EARTH_MASS_KG;
                recomputeDerivedProperties(body, parentPtr);
            }
        }

        ImGui::TextColored(Col::Green, "[Editable]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Physical Radius:");
        if (body.type.find("Star") != std::string::npos) {
            double rSun = body.radiusM / UnitConverter::SOLAR_RADIUS_M;
            float rF = (float)rSun;
            if (ImGui::DragFloat("Radius (R☉)##rSun", &rF, 0.02f, 0.01f, 500.0f, "%.3f R☉")) {
                body.radiusM = (double)rF * UnitConverter::SOLAR_RADIUS_M;
                recomputeDerivedProperties(body, parentPtr);
            }
        } else {
            double rKm = body.radiusM / 1000.0;
            float rF = (float)rKm;
            if (ImGui::DragFloat("Radius (km)##rKm", &rF, 10.0f, 1.0f, 200000.0f, "%.1f km")) {
                body.radiusM = (double)rF * 1000.0;
                recomputeDerivedProperties(body, parentPtr);
            }
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

        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Escape Velocity: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextPrimary, "%s", body.escapeVelocityStr.c_str());
    }

    // 3. Orbit Initializer & State Parameters
    if (ImGui::CollapsingHeader("ORBITAL MECHANICS & INITIALIZER", ImGuiTreeNodeFlags_DefaultOpen)) {
        ImGui::TextColored(Col::Accent, "⚙ ORBIT INITIALIZER TOOL");
        ImGui::TextColored(Col::TextSecondary, "Calculate initial state vectors (r, v) around parent:");

        // Parent body selector
        const char* currentParentName = parentPtr ? parentPtr->name.c_str() : "Barycenter / None";
        if (ImGui::BeginCombo("Parent Body##Combo", currentParentName)) {
            for (const auto& other : m_builderBodies) {
                if (other.dbId != body.dbId) {
                    bool isSel = (body.parentObjectId.has_value() && body.parentObjectId.value() == other.dbId);
                    if (ImGui::Selectable(other.name.c_str(), isSel)) {
                        body.parentObjectId = other.dbId;
                        parentPtr = &other;
                        recomputeDerivedProperties(body, parentPtr);
                    }
                }
            }
            ImGui::EndCombo();
        }

        float smaF = (float)body.semiMajorAxisAU;
        if (ImGui::DragFloat("Semi-Major Axis (AU)##Sma", &smaF, 0.05f, 0.001f, 100.0f, "%.3f AU")) {
            body.semiMajorAxisAU = smaF;
            if (parentPtr) applyOrbitInitializer(body, *parentPtr, body.semiMajorAxisAU, body.eccentricity, 0.0);
        }

        float eccF = (float)body.eccentricity;
        if (ImGui::SliderFloat("Eccentricity (e)##Ecc", &eccF, 0.0f, 0.95f, "%.4f")) {
            body.eccentricity = eccF;
            if (parentPtr) applyOrbitInitializer(body, *parentPtr, body.semiMajorAxisAU, body.eccentricity, 0.0);
        }

        if (parentPtr && ImGui::Button("⚡ Initialize Perfect Circular Orbit (e=0)", ImVec2(-1, 24))) {
            body.eccentricity = 0.0;
            applyOrbitInitializer(body, *parentPtr, body.semiMajorAxisAU, 0.0, 0.0);
        }

        ImGui::Spacing();
        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Orbital Period: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextPrimary, "%s", body.orbitalPeriodStr.c_str());

        ImGui::TextColored(Col::Accent, "[Derived]");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextSecondary, "Mean Velocity: ");
        ImGui::SameLine();
        ImGui::TextColored(Col::TextPrimary, "%s", body.orbitalSpeedStr.c_str());
    }

    // 4. Remove Body
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.5f, 0.15f, 0.15f, 0.85f));
    if (ImGui::Button("🗑 Remove Celestial Body", ImVec2(-1, 26))) {
        if (m_builderBodies.size() > 1) {
            m_builderBodies.erase(m_builderBodies.begin() + m_selectedNodeIndex);
            m_selectedNodeIndex = std::max(0, m_selectedNodeIndex - 1);
        }
    }
    ImGui::PopStyleColor();

    ImGui::EndChild();
}

void SystemWorkspaceUI::drawBuilderActionFooter(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab) {
    ImGui::Spacing();

    // 1. SAVE SYSTEM
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.45f, 0.25f, 0.9f));
    if (ImGui::Button("💾 SAVE SYSTEM", ImVec2(140, 30))) {
        m_builderSystem.name = m_systemNameBuf;
        m_builderSystem.type = "Custom";
        m_builderSystem.source = "User";
        m_builderSystem.description = m_systemDescBuf;

        if (objRepo.saveCustomSystem(m_builderSystem, m_builderBodies)) {
            m_actionFeedbackMsg = "System '" + m_builderSystem.name + "' successfully saved (" + std::to_string(m_builderBodies.size()) + " bodies).";
        }
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // 2. DUPLICATE SYSTEM
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.20f, 0.35f, 0.55f, 0.85f));
    if (ImGui::Button("📋 DUPLICATE", ImVec2(120, 30))) {
        std::string copyName = std::string(m_systemNameBuf) + " (Copy)";
        snprintf(m_systemNameBuf, sizeof(m_systemNameBuf), "%s", copyName.c_str());
        m_builderSystem.name = copyName;
        objRepo.saveCustomSystem(m_builderSystem, m_builderBodies);
        m_actionFeedbackMsg = "Created duplicate system instance: '" + copyName + "'";
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // 3. RESET / NEW
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.40f, 0.15f, 0.15f, 0.85f));
    if (ImGui::Button("↺ RESET BUILDER", ImVec2(130, 30))) {
        openCustomBuilderNew();
        m_actionFeedbackMsg = "Workspace reset to fresh star system template.";
    }
    ImGui::PopStyleColor();

    ImGui::SameLine();

    // 4. RUN SIMULATION
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.70f, 0.90f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1, 1, 1, 1));
    if (ImGui::Button("🚀 RUN SIMULATION", ImVec2(180, 30))) {
        runBuilderSimulation(objRepo, physics, camera, activeTopTab);
    }
    ImGui::PopStyleColor(2);

    if (!m_actionFeedbackMsg.empty()) {
        ImGui::SameLine(0, 16);
        ImGui::TextColored(Col::Green, "%s", m_actionFeedbackMsg.c_str());
    }
}

void SystemWorkspaceUI::runBuilderSimulation(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab) {
    // 1. Pre-flight validation check
    auto warnings = objRepo.validateSystem(m_builderBodies);
    bool hasCriticalErrors = false;
    for (const auto& w : warnings) {
        if (w.severity == SystemValidationWarning::Severity::Error) hasCriticalErrors = true;
    }

    if (hasCriticalErrors) {
        m_showValidationWarningPopup = true;
        return;
    }

    // 2. Save current state to database so it can be reloaded cleanly
    m_builderSystem.name = m_systemNameBuf;
    m_builderSystem.type = "Custom";
    m_builderSystem.source = "User";
    m_builderSystem.description = m_systemDescBuf;
    objRepo.saveCustomSystem(m_builderSystem, m_builderBodies);

    // 3. Pass data to existing physics engine without any alterations to gravity/equations
    physics.loadFromDatabase(objRepo, m_builderSystem.name);

    // 4. Reset camera & switch to UNIVERSE live simulation
    camera.resetOverview(glm::vec3(0.0f), 6.0f);
    activeTopTab = 0; // UNIVERSE workspace
}

void SystemWorkspaceUI::drawValidationModal(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab) {
    ImGui::OpenPopup("Pre-Flight Validation Warning");
    ImVec2 center = ImGui::GetMainViewport()->GetCenter();
    ImGui::SetNextWindowPos(center, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(520, 280));

    if (ImGui::BeginPopupModal("Pre-Flight Validation Warning", &m_showValidationWarningPopup, ImGuiWindowFlags_NoResize)) {
        ImGui::TextColored(Col::Orange, "⚠ PRE-FLIGHT SYSTEM VALIDATION ALERTS");
        ImGui::Separator();
        ImGui::Spacing();

        for (const auto& w : m_currentValidationWarnings) {
            ImGui::TextColored(Col::Yellow, "• %s", w.title.c_str());
            ImGui::TextWrapped("%s", w.message.c_str());
            ImGui::Spacing();
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (ImGui::Button("Review & Edit in Builder", ImVec2(180, 28))) {
            m_showValidationWarningPopup = false;
        }

        ImGui::SameLine();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.70f, 0.25f, 0.15f, 0.90f));
        if (ImGui::Button("Run Anyway in Simulation", ImVec2(200, 28))) {
            m_showValidationWarningPopup = false;
            m_builderSystem.name = m_systemNameBuf;
            objRepo.saveCustomSystem(m_builderSystem, m_builderBodies);
            physics.loadFromDatabase(objRepo, m_builderSystem.name);
            camera.resetOverview(glm::vec3(0.0f), 6.0f);
            activeTopTab = 0;
        }
        ImGui::PopStyleColor();

        ImGui::EndPopup();
    }
}


void SystemWorkspaceUI::drawSavedSystemsMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH) {
    ImGui::BeginChild("##SavedSystemsListChild", ImVec2(contentW - 32.0f, contentH), true);

    ImGui::TextColored(Col::Accent, "SAVED & RECENT STAR SYSTEMS");
    ImGui::TextColored(Col::TextSecondary, "All astronomical and user-created systems stored in the SQLite database:");
    ImGui::Separator();
    ImGui::Spacing();

    auto systems = objRepo.getAllSystems();

    if (ImGui::BeginTable("##SavedSysTable", 7, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY)) {
        ImGui::TableSetupColumn("System Name", ImGuiTableColumnFlags_WidthFixed, 220);
        ImGui::TableSetupColumn("Type", ImGuiTableColumnFlags_WidthFixed, 100);
        ImGui::TableSetupColumn("Source", ImGuiTableColumnFlags_WidthFixed, 180);
        ImGui::TableSetupColumn("Objects", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("Last Modified", ImGuiTableColumnFlags_WidthFixed, 150);
        ImGui::TableSetupColumn("Description", ImGuiTableColumnFlags_WidthStretch);
        ImGui::TableSetupColumn("Actions", ImGuiTableColumnFlags_WidthFixed, 240);
        ImGui::TableHeadersRow();

        for (const auto& sys : systems) {
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(Col::Accent, "★ %s", sys.name.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextUnformatted(sys.type.c_str());

            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(Col::TextSecondary, "%s", sys.source.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", sys.objectCount);

            ImGui::TableSetColumnIndex(4);
            ImGui::TextColored(Col::TextSecondary, "%s", sys.updatedAt.c_str());

            ImGui::TableSetColumnIndex(5);
            ImGui::TextUnformatted(sys.description.c_str());

            ImGui::TableSetColumnIndex(6);
            ImGui::PushID((int)sys.id);

            // RUN button
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.60f, 0.80f, 0.85f));
            if (ImGui::SmallButton("▶ Run")) {
                physics.loadFromDatabase(objRepo, sys.name);
                camera.resetOverview(glm::vec3(0.0f), 6.0f);
                activeTopTab = 0; // UNIVERSE
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            if (ImGui::SmallButton("✏ Edit")) {
                loadSystemIntoBuilder(sys.name, objRepo);
                m_currentMode = 1; // Switch to Custom Builder
            }

            ImGui::SameLine();
            if (ImGui::SmallButton("📋 Copy")) {
                std::string newName = sys.name + " (Copy)";
                objRepo.duplicateSystem(sys.id, newName);
                m_actionFeedbackMsg = "Duplicated '" + sys.name + "' -> '" + newName + "'";
            }

            if (sys.type != "Preset") {
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.50f, 0.15f, 0.15f, 0.75f));
                if (ImGui::SmallButton("🗑 Del")) {
                    objRepo.deleteSystem(sys.id);
                }
                ImGui::PopStyleColor();
            }

            ImGui::PopID();
        }

        ImGui::EndTable();
    }

    ImGui::EndChild();
}

void SystemWorkspaceUI::drawPresetsMode(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, int& activeTopTab, float contentW, float contentH) {
    ImGui::BeginChild("##PresetsGridChild", ImVec2(contentW - 32.0f, contentH), true);

    ImGui::TextColored(Col::Accent, "ASTRONOMICAL PRESET TEMPLATES");
    ImGui::TextColored(Col::TextSecondary, "Instant starting points backed by astronomical database baseline records:");
    ImGui::Separator();
    ImGui::Spacing();

    struct PresetInfo {
        const char* name;
        const char* tag;
        const char* desc;
        const char* icon;
        ImVec4 col;
    };

    PresetInfo presets[] = {
        { "Solar System", "NASA/JPL", "Sun, 8 major planets, dwarf planets, and major moons.", "☉", Col::Yellow },
        { "Earth-Moon System", "Barycentric", "Isolated high-precision Earth-Moon two-body orbital dynamics.", "🌍", Col::Accent },
        { "TRAPPIST-1 System", "NASA Exoplanets", "Ultracool red dwarf host star with 7 Earth-sized temperate planets.", "🔴", Col::Orange },
        { "Kepler-90 System", "NASA Exoplanets", "G-type yellow star hosting 8 confirmed transiting exoplanets.", "★", Col::Yellow },
        { "Binary Star System", "Astrophysics", "Alpha Centauri AB style co-orbiting pair with circumbinary planet.", "♊", Col::Accent },
        { "Extreme Tidal Test", "General Relativity", "10 Solar Mass Black Hole with Blue Supergiant and orbiting planet.", "🕳", Col::Purple },
        { "Proxima Centauri", "NASA Exoplanets", "Closest stellar neighbour hosting habitable zone exoplanet Proxima b.", "☄", Col::Red },
        { "Asteroid Belt", "JPL SBDB", "Inner Solar System asteroid belt with Ceres, Vesta, Pallas, and Hygiea.", "☄", Col::Orange }
    };

    float cardW = (contentW - 80.0f) / 3.0f;
    float cardH = 150.0f;

    for (int i = 0; i < 8; ++i) {
        if (i % 3 != 0) ImGui::SameLine(0, 16);

        ImGui::PushID(i);
        ImGui::BeginChild(presets[i].name, ImVec2(cardW, cardH), true);

        ImGui::TextColored(presets[i].col, "%s %s", presets[i].icon, presets[i].name);
        ImGui::SameLine(cardW - 120.0f);
        ImGui::TextColored(Col::TextSecondary, "[%s]", presets[i].tag);

        ImGui::Spacing();
        ImGui::TextWrapped("%s", presets[i].desc);

        ImGui::Spacing();
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.80f, 0.85f));
        if (ImGui::Button("🚀 Run in Universe", ImVec2(130, 24))) {
            physics.loadFromDatabase(objRepo, presets[i].name);
            camera.resetOverview(glm::vec3(0.0f), 6.0f);
            activeTopTab = 0; // Switch to UNIVERSE
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("✏ Edit in Builder", ImVec2(120, 24))) {
            loadSystemIntoBuilder(presets[i].name, objRepo);
            m_currentMode = 1; // Custom Builder
        }

        ImGui::EndChild();
        ImGui::PopID();
    }

    ImGui::EndChild();
}

} // namespace AstroGenesis
