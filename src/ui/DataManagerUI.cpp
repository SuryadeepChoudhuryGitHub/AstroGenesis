#include "ui/DataManagerUI.hpp"
#include "ui/IconSystem.hpp"
#include "data/SeedData.hpp"
#include "data/UnitConverter.hpp"
#include <cstdio>
#include <algorithm>

namespace AstroGenesis {

namespace DMCol {
    static ImVec4 BgDark       {0.024f, 0.035f, 0.065f, 1.00f};
    static ImVec4 BgPanel      {0.035f, 0.050f, 0.090f, 0.98f};
    static ImVec4 BgChild      {0.045f, 0.065f, 0.115f, 0.90f};
    static ImVec4 Border       {0.120f, 0.180f, 0.280f, 0.60f};
    static ImVec4 Accent       {0.000f, 0.850f, 1.000f, 1.00f};
    static ImVec4 AccentDim    {0.000f, 0.500f, 0.700f, 0.70f};
    static ImVec4 TextPrimary  {0.900f, 0.930f, 0.970f, 1.00f};
    static ImVec4 TextSecondary{0.460f, 0.540f, 0.680f, 1.00f};
    static ImVec4 TabActive    {0.000f, 0.500f, 0.750f, 0.35f};
    static ImVec4 Green        {0.150f, 0.880f, 0.450f, 1.00f};
    static ImVec4 Yellow       {0.980f, 0.780f, 0.120f, 1.00f};
    static ImVec4 Red          {0.950f, 0.250f, 0.200f, 1.00f};
}

DataManagerUI::DataManagerUI() {}

void DataManagerUI::render(bool& showWindow, 
                           DataManager& dataManager, 
                           ObjectRepository& objRepo,
                           PhysicsEngine& physics,
                           float winW, float winH) {
    if (!showWindow) return;

    float modalW = std::min(980.0f, winW - 60.0f);
    float modalH = std::min(680.0f, winH - 60.0f);

    ImGui::SetNextWindowSize(ImVec2(modalW, modalH), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImVec2((winW - modalW) * 0.5f, (winH - modalH) * 0.5f), ImGuiCond_Appearing);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(16, 14));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, DMCol::BgPanel);
    ImGui::PushStyleColor(ImGuiCol_Border, DMCol::Border);

    if (ImGui::Begin("DATA MANAGER  —  Astronomical Database & External Providers", &showWindow, ImGuiWindowFlags_NoCollapse)) {
        // Top Header & Status Banner
        UIIcon::Icon(IconId::Database, 15.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Accent), 6.0f);
        ImGui::SameLine();
        ImGui::TextColored(DMCol::Accent, "DATA-DRIVEN ASTRONOMY ENGINE");
        if (modalW >= 950.0f) {
            ImGui::SameLine();
            ImGui::TextColored(DMCol::TextSecondary, "| SQLite DB: data/astrogenesis.db (%d objects)", objRepo.getObjectCount());
        }
        
        float rightBadgeX = modalW - 150.0f;
        if (rightBadgeX > ImGui::GetCursorPosX() + 16.0f) {
            ImGui::SameLine(rightBadgeX);
        } else {
            ImGui::SameLine(0, 16.0f);
        }
        if (dataManager.isOfflineMode()) {
            UIIcon::Icon(IconId::Warning, 13.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Yellow), 4.0f);
            ImGui::SameLine();
            ImGui::TextColored(DMCol::Yellow, "OFFLINE MODE");
        } else {
            UIIcon::Icon(IconId::CheckCircle, 13.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Green), 4.0f);
            ImGui::SameLine();
            ImGui::TextColored(DMCol::Green, "API ONLINE");
        }

        ImGui::Separator();
        ImGui::Spacing();

        // 4 Main Tabs with Vector Icons
        struct TabDef { IconId icon; const char* label; };
        TabDef tabDefs[] = { 
            { IconId::Search,   "SEARCH & IMPORT (LIVE API)" }, 
            { IconId::Database, "DATABASE EXPLORER" }, 
            { IconId::Time,     "IMPORT HISTORY" }, 
            { IconId::Settings, "SOURCE CONFIGURATION" } 
        };

        for (int i = 0; i < 4; ++i) {
            if (i > 0) ImGui::SameLine(0, 6);
            bool isActive = (i == m_activeTab);
            char tid[32];
            snprintf(tid, sizeof(tid), "##DMTab%d", i);
            if (UIIcon::Button(tid, tabDefs[i].icon, tabDefs[i].label, ImVec2(0, 28), isActive)) {
                m_activeTab = i;
            }
        }

        ImGui::Spacing();
        ImGui::Separator();
        ImGui::Spacing();

        if (m_activeTab == 0) {
            drawSearchAndImportTab(dataManager, objRepo, physics);
        } else if (m_activeTab == 1) {
            drawDatabaseExplorerTab(dataManager, objRepo, physics);
        } else if (m_activeTab == 2) {
            drawImportHistoryTab(dataManager);
        } else if (m_activeTab == 3) {
            drawSourceConfigTab(dataManager, objRepo);
        }
    }
    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

void DataManagerUI::drawSearchAndImportTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics) {
    ImGui::TextColored(DMCol::Accent, "QUERY EXTERNAL ASTRONOMICAL DATA PROVIDER");
    ImGui::TextColored(DMCol::TextSecondary, "Search NASA JPL Horizons, Small-Body Database (SBDB), or NASA Exoplanet Archive to import real celestial objects.");

    ImGui::Spacing();

    // Provider Selector Combo
    const char* providers[] = {
        "NASA JPL Horizons (Planets, Moons, Major Bodies)",
        "NASA JPL Small-Body Database (Asteroids & Comets)",
        "NASA Exoplanet Archive (Confirmed Exoplanets & Host Stars)"
    };
    ImGui::Text("Data Source:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(450.0f);
    ImGui::Combo("##ProviderCombo", &m_selectedProviderIdx, providers, 3);

    // Search Query Bar
    ImGui::Text("Target Query:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300.0f);
    bool enterPressed = ImGui::InputTextWithHint("##SearchInput", "e.g. Ceres, Apophis, TRAPPIST-1, Earth...", m_searchBuffer, sizeof(m_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    
    ImGui::SameLine();
    bool isSearching = dataManager.isSearching();
    if (isSearching) {
        ImGui::Button("Searching...", ImVec2(100, 24));
    } else {
        if (UIIcon::Button("##DMSearchBtn", IconId::Search, "SEARCH", ImVec2(100, 24)) || enterPressed) {
            ProviderType pType = (ProviderType)m_selectedProviderIdx;
            dataManager.searchAsync(pType, m_searchBuffer);
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Asynchronous Import Status Banner
    auto jobState = dataManager.getImportJobState();
    if (jobState.isRunning) {
        UIIcon::Icon(IconId::Time, 14.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Yellow), 6.0f);
        ImGui::SameLine();
        ImGui::TextColored(DMCol::Yellow, "%s", jobState.currentTask.c_str());
        ImGui::ProgressBar(jobState.progress, ImVec2(ImGui::GetContentRegionAvail().x, 6.0f));
    } else if (!jobState.lastResult.empty()) {
        if (jobState.lastSuccess) {
            UIIcon::Icon(IconId::Check, 14.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Green), 6.0f);
            ImGui::SameLine();
            ImGui::TextColored(DMCol::Green, "%s", jobState.lastResult.c_str());
        } else {
            UIIcon::Icon(IconId::Error, 14.0f, ImGui::ColorConvertFloat4ToU32(DMCol::Red), 6.0f);
            ImGui::SameLine();
            ImGui::TextColored(DMCol::Red, "%s", jobState.lastResult.c_str());
        }
    }

    ImGui::Spacing();
    ImGui::TextColored(DMCol::Accent, "SEARCH RESULTS");

    // Search Results Table
    auto results = dataManager.getSearchResults();
    if (ImGui::BeginTable("##SearchResultsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 320))) {
        ImGui::TableSetupColumn("Name / Target", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Type / Classification", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Source ID / Code", ImGuiTableColumnFlags_WidthStretch, 0.15f);
        ImGui::TableSetupColumn("Details", ImGuiTableColumnFlags_WidthStretch, 0.20f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < results.size(); ++i) {
            const auto& item = results[i];
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            if (item.type.find("Star") != std::string::npos) {
                UIIcon::Icon(IconId::Star, 13.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.98f, 0.82f, 0.25f, 1.0f)), 5.0f);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.98f, 0.82f, 0.25f, 1.0f), "%s", item.name.c_str());
            } else if (item.type.find("Exoplanet") != std::string::npos || item.type.find("Planet") != std::string::npos) {
                UIIcon::Icon(IconId::Planet, 13.0f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.25f, 0.85f, 0.95f, 1.0f)), 5.0f);
                ImGui::SameLine();
                ImGui::TextColored(ImVec4(0.25f, 0.85f, 0.95f, 1.0f), "%s", item.name.c_str());
            } else {
                UIIcon::Icon(IconId::Asteroid, 13.0f, ImGui::ColorConvertFloat4ToU32(DMCol::TextSecondary), 5.0f);
                ImGui::SameLine();
                ImGui::TextColored(DMCol::TextPrimary, "%s", item.name.c_str());
            }

            ImGui::TableSetColumnIndex(1);
            if (item.type.find("Star") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.98f, 0.78f, 0.20f, 1.0f), "%s", item.type.c_str());
            } else if (item.type.find("Exoplanet") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.40f, 0.75f, 0.95f, 1.0f), "%s", item.type.c_str());
            } else {
                ImGui::TextColored(DMCol::TextSecondary, "%s", item.type.c_str());
            }

            ImGui::TableSetColumnIndex(2);
            ImGui::Text("%s", item.sourceId.c_str());

            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("%s", item.details.c_str());

            ImGui::TableSetColumnIndex(4);
            ImGui::PushID((int)i);
            std::string targetCat = (m_selectedProviderIdx == 1) ? "Asteroid Belt" : (m_selectedProviderIdx == 2 ? ((item.type.find("Star") != std::string::npos) ? "Host Star" : "Exoplanet System") : "Solar System");
            if (item.alreadyInDatabase) {
                ImVec4 inDbCol(0.12f, 0.28f, 0.18f, 0.85f);
                if (UIIcon::Button("##InDbBtn", IconId::Check, "In DB (Update)", ImVec2(130, 22), false, &inDbCol)) {
                    ProviderType pType = (ProviderType)m_selectedProviderIdx;
                    dataManager.importObjectAsync(pType, item.sourceId, targetCat);
                }
            } else {
                ImVec4 impCol = (item.type.find("Star") != std::string::npos) ? ImVec4(0.45f, 0.35f, 0.08f, 0.9f) : ImVec4(0.08f, 0.35f, 0.55f, 0.85f);
                if (UIIcon::Button("##ImportBtn", IconId::Import, "Import to DB", ImVec2(130, 22), false, &impCol)) {
                    ProviderType pType = (ProviderType)m_selectedProviderIdx;
                    dataManager.importObjectAsync(pType, item.sourceId, targetCat);
                }
            }
            ImGui::PopID();

        }

        ImGui::EndTable();
    }
}

void DataManagerUI::drawDatabaseExplorerTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics) {
    auto categories = objRepo.getAvailableCategories();
    if (categories.empty()) categories.push_back("Solar System");

    ImGui::TextColored(DMCol::Accent, "SYSTEM CATEGORY:");
    ImGui::SameLine();

    for (size_t i = 0; i < categories.size(); ++i) {
        if (i > 0) ImGui::SameLine(0, 6);
        bool isSel = ((int)i == m_selectedCategoryIdx);
        if (isSel) {
            ImGui::PushStyleColor(ImGuiCol_Button, DMCol::TabActive);
            ImGui::PushStyleColor(ImGuiCol_Text, DMCol::Accent);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.12f, 0.20f, 0.75f));
            ImGui::PushStyleColor(ImGuiCol_Text, DMCol::TextSecondary);
        }
        if (ImGui::Button(categories[i].c_str())) {
            m_selectedCategoryIdx = (int)i;
        }
        ImGui::PopStyleColor(2);
    }

    std::string currentCat = (m_selectedCategoryIdx < (int)categories.size()) ? categories[m_selectedCategoryIdx] : "Solar System";

    // Dedicated Action Toolbar Row for the Explorer (No Overlap / Collision)
    ImGui::Spacing();
    ImVec4 loadCol(0.12f, 0.45f, 0.25f, 0.90f);
    if (UIIcon::Button("##LoadSystemBtn", IconId::Play, "LOAD SYSTEM INTO SIMULATION", ImVec2(250, 26), false, &loadCol)) {
        physics.loadFromDatabase(objRepo, currentCat);
    }
    ImGui::SameLine(0, 10);
    ImVec4 refreshCol(0.18f, 0.25f, 0.40f, 0.85f);
    if (UIIcon::Button("##RefreshBaselineBtn", IconId::Refresh, "Refresh Database with NASA/JPL Baseline", ImVec2(310, 26), false, &refreshCol)) {
        SeedData::seedDefaultDatabase(objRepo);
        physics.loadFromDatabase(objRepo, currentCat);
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Two Columns: Left list of objects, Right detail view
    float listW = 280.0f;
    float detailW = ImGui::GetContentRegionAvail().x - listW - 16.0f;

    // Left List
    ImGui::BeginChild("##ExplorerList", ImVec2(listW, 400), true);
    auto objects = objRepo.getAllObjects(currentCat, true, m_explorerFilter);
    ImGui::InputTextWithHint("##filter", "Filter list...", m_explorerFilter, sizeof(m_explorerFilter));
    ImGui::Separator();

    for (const auto& obj : objects) {
        bool isSelected = (obj.id == m_selectedObjectId);
        if (isSelected) ImGui::PushStyleColor(ImGuiCol_Header, DMCol::TabActive);
        
        char label[128];
        snprintf(label, sizeof(label), "%s (%s)", obj.name.c_str(), obj.type.c_str());
        if (ImGui::Selectable(label, isSelected)) {
            m_selectedObjectId = obj.id;
        }
        if (isSelected) ImGui::PopStyleColor();
    }
    ImGui::EndChild();

    ImGui::SameLine();

    // Right Details
    ImGui::BeginChild("##ExplorerDetails", ImVec2(detailW, 400), true);
    if (m_selectedObjectId > 0) {
        auto hydrated = objRepo.getHydratedBody(m_selectedObjectId);
        if (hydrated.has_value()) {
            const auto& b = hydrated.value();
            ImGui::TextColored(DMCol::Accent, "OBJECT: %s", b.name.c_str());
            ImGui::TextColored(DMCol::TextSecondary, "Slug: %s | Type: %s | Category: %s", b.id.c_str(), b.type.c_str(), b.category.c_str());
            ImGui::Separator();

            ImGui::Columns(2, "detailCols", false);
            ImGui::TextColored(DMCol::TextSecondary, "Mass:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.massStr.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Radius:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.radiusStr.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Semi-Major Axis:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.semiMajorAxisStr.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Eccentricity:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.eccentricityStr.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Orbital Period:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.orbitalPeriodStr.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Data Source:");
            ImGui::NextColumn();
            ImGui::TextColored(DMCol::Green, "%s", b.sourceName.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Source Record ID:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.sourceObjectId.c_str());
            ImGui::NextColumn();

            ImGui::TextColored(DMCol::TextSecondary, "Import Timestamp:");
            ImGui::NextColumn();
            ImGui::Text("%s", b.importTimestamp.c_str());
            ImGui::NextColumn();

            ImGui::Columns(1);
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            // Actions
            if (UIIcon::Button(IconId::Target, "Focus in Viewport", ImVec2(160, 26))) {
                physics.selectBodyById(b.id);
            }
            ImGui::SameLine();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 0.85f));
            if (UIIcon::Button(IconId::Delete, "Delete Object", ImVec2(130, 26))) {
                objRepo.deleteObject(b.dbId);
                m_selectedObjectId = 0;
            }
            ImGui::PopStyleColor();
        }
    } else {
        ImGui::TextColored(DMCol::TextSecondary, "Select an object from the left panel to inspect properties.");
    }
    ImGui::EndChild();
}

void DataManagerUI::drawImportHistoryTab(DataManager& dataManager) {
    ImGui::TextColored(DMCol::Accent, "EXTERNAL API IMPORT AUDIT LOG");
    ImGui::TextColored(DMCol::TextSecondary, "Chronological record of all external ephemeris and catalog synchronization jobs.");

    ImGui::Spacing();

    auto logs = dataManager.getImportHistory(50);
    if (ImGui::BeginTable("##ImportLogsTable", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 420))) {
        ImGui::TableSetupColumn("Timestamp", ImGuiTableColumnFlags_WidthFixed, 140.0f);
        ImGui::TableSetupColumn("Target Object", ImGuiTableColumnFlags_WidthStretch, 0.25f);
        ImGui::TableSetupColumn("Status", ImGuiTableColumnFlags_WidthFixed, 80.0f);
        ImGui::TableSetupColumn("Count", ImGuiTableColumnFlags_WidthFixed, 50.0f);
        ImGui::TableSetupColumn("Details / Error Message", ImGuiTableColumnFlags_WidthStretch, 0.45f);
        ImGui::TableHeadersRow();

        for (const auto& log : logs) {
            ImGui::TableNextRow();
            
            ImGui::TableSetColumnIndex(0);
            ImGui::TextColored(DMCol::TextSecondary, "%s", log.timestamp.c_str());

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(DMCol::TextPrimary, "%s", log.targetObject.c_str());

            ImGui::TableSetColumnIndex(2);
            if (log.status == "SUCCESS") {
                ImGui::TextColored(DMCol::Green, "SUCCESS");
            } else {
                ImGui::TextColored(DMCol::Red, "FAILED");
            }

            ImGui::TableSetColumnIndex(3);
            ImGui::Text("%d", log.recordsCount);

            ImGui::TableSetColumnIndex(4);
            ImGui::TextWrapped("%s", log.details.c_str());
        }

        ImGui::EndTable();
    }
}

void DataManagerUI::drawSourceConfigTab(DataManager& dataManager, ObjectRepository& objRepo) {
    ImGui::TextColored(DMCol::Accent, "API ENDPOINTS & CACHE POLICIES");
    ImGui::Spacing();

    bool offline = dataManager.isOfflineMode();
    if (ImGui::Checkbox("Enable Strict Offline Mode (Disable Network Calls)", &offline)) {
        dataManager.setOfflineMode(offline);
    }
    ImGui::TextColored(DMCol::TextSecondary, "When offline mode is enabled, AstroGenesis operates strictly from data/astrogenesis.db.");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(DMCol::Accent, "ACTIVE PROVIDERS:");
    ImGui::BulletText("NASA JPL Horizons API: https://ssd.jpl.nasa.gov/api/horizons.api");
    ImGui::BulletText("NASA JPL SBDB API: https://ssd-api.jpl.nasa.gov/sbdb.api");
    ImGui::BulletText("NASA Exoplanet Archive TAP: https://exoplanetarchive.ipac.caltech.edu/TAP/sync");

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::TextColored(DMCol::Accent, "DATABASE MAINTENANCE:");
    if (UIIcon::Button(IconId::Reset, "Reset & Reload Bundled Baseline Seed Data", ImVec2(320, 28))) {
        SeedData::seedDefaultDatabase(objRepo);
    }
    ImGui::TextColored(DMCol::TextSecondary, "Restores baseline high-precision Solar System, Asteroid Belt, and TRAPPIST-1 datasets.");
}

} // namespace AstroGenesis
