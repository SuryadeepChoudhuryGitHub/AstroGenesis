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

void DataManagerUI::openDatabaseExplorer() {
    m_activeTab = 0;
}

void DataManagerUI::selectObjectById(int64_t id, const std::string& category) {
    m_activeTab = 0;
    m_selectedObjectId = id;
    if (!category.empty()) {
        m_pendingCategorySelection = category;
    }
}

void DataManagerUI::render(bool& showWindow, 
                           DataManager& dataManager, 
                           ObjectRepository& objRepo,
                           PhysicsEngine& physics,
                           float winW, float winH,
                           Camera* camera,
                           int* activeTopTab) {
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
            { IconId::Database, "DATABASE EXPLORER" }, 
            { IconId::Search,   "SEARCH & IMPORT (LIVE API)" }, 
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
            drawDatabaseExplorerTab(dataManager, objRepo, physics, camera, activeTopTab, showWindow);
        } else if (m_activeTab == 1) {
            drawSearchAndImportTab(dataManager, objRepo, physics);
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
    ImGui::TextColored(DMCol::TextSecondary, "Search CDS/SIMBAD Stellar Catalogues, NASA Exoplanet Archive, JPL Horizons, or JPL SBDB to import real celestial objects.");

    ImGui::Spacing();

    // Provider Selector Combo (0: Auto_Resolve, 1: Stellar_Catalog, 2: NASA_Exoplanet, 3: JPL_Horizons, 4: JPL_SBDB)
    const char* providers[] = {
        "Auto-Resolve (All Astronomical Catalogues)",
        "Stellar Catalogue (CDS/SIMBAD & Bright Stars)",
        "NASA Exoplanet Archive (Confirmed Exoplanets & Host Stars)",
        "NASA JPL Horizons (Planets, Moons, Major Bodies)",
        "NASA JPL Small-Body Database (Asteroids & Comets)"
    };
    ImGui::Text("Data Source:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(450.0f);
    ImGui::Combo("##ProviderCombo", &m_selectedProviderIdx, providers, 5);

    // Search Query Bar
    ImGui::Text("Target Query:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(300.0f);
    bool enterPressed = ImGui::InputTextWithHint("##SearchInput", "e.g. Sirius, Rigel, Ceres, TRAPPIST-1, Earth...", m_searchBuffer, sizeof(m_searchBuffer), ImGuiInputTextFlags_EnterReturnsTrue);
    
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
    if (ImGui::BeginTable("##SearchResultsTable", 6, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 320))) {
        ImGui::TableSetupColumn("Name / Target", ImGuiTableColumnFlags_WidthStretch, 0.22f);
        ImGui::TableSetupColumn("Type / Classification", ImGuiTableColumnFlags_WidthStretch, 0.20f);
        ImGui::TableSetupColumn("Catalogue / ID", ImGuiTableColumnFlags_WidthStretch, 0.18f);
        ImGui::TableSetupColumn("Details & Aliases", ImGuiTableColumnFlags_WidthStretch, 0.22f);
        ImGui::TableSetupColumn("Quality", ImGuiTableColumnFlags_WidthFixed, 55.0f);
        ImGui::TableSetupColumn("Action", ImGuiTableColumnFlags_WidthFixed, 180.0f);
        ImGui::TableHeadersRow();

        for (size_t i = 0; i < results.size(); ++i) {
            const auto& item = results[i];
            ImGui::TableNextRow();
            
            // Col 0: Name / Target
            ImGui::TableSetColumnIndex(0);
            if (item.type.find("Star") != std::string::npos || item.type.find("Binary") != std::string::npos) {
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

            // Col 1: Type / Classification
            ImGui::TableSetColumnIndex(1);
            if (item.type.find("Star") != std::string::npos || item.type.find("Binary") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.98f, 0.78f, 0.20f, 1.0f), "%s", item.type.c_str());
            } else if (item.type.find("Exoplanet") != std::string::npos) {
                ImGui::TextColored(ImVec4(0.40f, 0.75f, 0.95f, 1.0f), "%s", item.type.c_str());
            } else {
                ImGui::TextColored(DMCol::TextSecondary, "%s", item.type.c_str());
            }

            // Col 2: Catalogue / ID
            ImGui::TableSetColumnIndex(2);
            ImGui::TextColored(DMCol::TextSecondary, "%s", item.sourceName.c_str());
            ImGui::Text("%s", item.sourceId.c_str());

            // Col 3: Details & Aliases
            ImGui::TableSetColumnIndex(3);
            ImGui::TextWrapped("%s", item.details.c_str());
            if (!item.aliases.empty()) {
                ImGui::TextColored(DMCol::TextSecondary, "Aliases: %s", item.aliases.c_str());
            }

            // Col 4: Quality / Completeness
            ImGui::TableSetColumnIndex(4);
            int compPct = (int)std::round(item.dataCompleteness * 100.0f);
            if (compPct <= 0) compPct = 85;
            ImVec4 compCol = (compPct >= 90) ? DMCol::Green : (compPct >= 60) ? DMCol::Yellow : DMCol::TextSecondary;
            ImGui::TextColored(compCol, "%d%%", compPct);

            // Col 5: Action
            ImGui::TableSetColumnIndex(5);
            ImGui::PushID((int)i);
            
            // Check if item is a stellar system (e.g. Sirius System, Alpha Centauri System)
            if (item.sourceId.rfind("system_", 0) == 0 || item.type.find("Binary") != std::string::npos || item.type.find("Multiple Star") != std::string::npos) {
                ImVec4 sysCol(0.18f, 0.42f, 0.28f, 0.90f);
                if (UIIcon::Button("##ImpStellarSysBtn", IconId::Play, "Import System", ImVec2(150, 22), false, &sysCol)) {
                    dataManager.importStellarSystemAsync(item.sourceId);
                }
            } else if (item.sourceName.find("Exoplanet") != std::string::npos || m_selectedProviderIdx == 2) {
                std::string host = item.name;
                if (host.rfind(" (Host Star)") != std::string::npos) {
                    host = host.substr(0, host.length() - 12);
                } else if (item.details.find("Host: ") != std::string::npos) {
                    size_t hPos = item.details.find("Host: ");
                    size_t pipe = item.details.find(" |", hPos);
                    if (pipe != std::string::npos) host = item.details.substr(hPos + 6, pipe - (hPos + 6));
                }
                ImVec4 sysCol(0.18f, 0.42f, 0.28f, 0.90f);
                if (UIIcon::Button("##ImpSysBtn", IconId::Play, "Import Sys", ImVec2(95, 22), false, &sysCol)) {
                    dataManager.importExoplanetSystemAsync(host);
                }
                ImGui::SameLine(0, 4);
                if (UIIcon::Button("##ImpObjBtn", IconId::Import, "Import", ImVec2(65, 22))) {
                    dataManager.importObjectAsync(ProviderType::NASA_Exoplanet, item.sourceId, host);
                }
            } else if (item.type.find("Star") != std::string::npos) {
                std::string starCat = item.name;
                ImVec4 impCol(0.45f, 0.35f, 0.08f, 0.9f);
                if (item.alreadyInDatabase) {
                    ImVec4 inDbCol(0.12f, 0.28f, 0.18f, 0.85f);
                    if (UIIcon::Button("##InDbBtn", IconId::Check, "In DB (Update)", ImVec2(150, 22), false, &inDbCol)) {
                        dataManager.importObjectAsync(ProviderType::Auto_Resolve, item.sourceId, starCat);
                    }
                } else {
                    if (UIIcon::Button("##ImportStarBtn", IconId::Import, "Import Star", ImVec2(150, 22), false, &impCol)) {
                        dataManager.importObjectAsync(ProviderType::Auto_Resolve, item.sourceId, starCat);
                    }
                }
            } else {
                std::string targetCat = (m_selectedProviderIdx == 4 || item.type.find("Asteroid") != std::string::npos || item.type.find("Comet") != std::string::npos) ? "Asteroid Belt" : "Solar System";
                if (item.alreadyInDatabase) {
                    ImVec4 inDbCol(0.12f, 0.28f, 0.18f, 0.85f);
                    if (UIIcon::Button("##InDbBtn", IconId::Check, "In DB (Update)", ImVec2(150, 22), false, &inDbCol)) {
                        ProviderType pType = (ProviderType)m_selectedProviderIdx;
                        dataManager.importObjectAsync(pType, item.sourceId, targetCat);
                    }
                } else {
                    ImVec4 impCol = (item.type.find("Star") != std::string::npos) ? ImVec4(0.45f, 0.35f, 0.08f, 0.9f) : ImVec4(0.08f, 0.35f, 0.55f, 0.85f);
                    if (UIIcon::Button("##ImportBtn", IconId::Import, "Import to DB", ImVec2(150, 22), false, &impCol)) {
                        ProviderType pType = (ProviderType)m_selectedProviderIdx;
                        dataManager.importObjectAsync(pType, item.sourceId, targetCat);
                    }
                }
            }
            ImGui::PopID();
        }

        ImGui::EndTable();
    }
}

void DataManagerUI::drawDatabaseExplorerTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics, Camera* camera, int* activeTopTab, bool& showWindow) {
    auto categories = objRepo.getAvailableCategories();
    if (categories.empty()) categories.push_back("Solar System");

    if (!m_pendingCategorySelection.empty()) {
        for (size_t i = 0; i < categories.size(); ++i) {
            if (categories[i] == m_pendingCategorySelection) {
                m_selectedCategoryIdx = (int)i;
                break;
            }
        }
        m_pendingCategorySelection.clear();
    }

    if (m_selectedCategoryIdx < 0 || m_selectedCategoryIdx >= (int)categories.size()) {
        m_selectedCategoryIdx = 0;
        for (size_t i = 0; i < categories.size(); ++i) {
            if (categories[i] == "Solar System") {
                m_selectedCategoryIdx = (int)i;
                break;
            }
        }
    }

    ImGui::TextColored(DMCol::Accent, "SYSTEM CATEGORY:");
    float availWidth = ImGui::GetContentRegionAvail().x;

    for (size_t i = 0; i < categories.size(); ++i) {
        float btnW = ImGui::CalcTextSize(categories[i].c_str()).x + ImGui::GetStyle().FramePadding.x * 2.0f;
        if (i == 0) {
            ImGui::SameLine(0, 8);
        } else {
            if (ImGui::GetCursorPosX() + btnW + 12.0f < availWidth) {
                ImGui::SameLine(0, 6);
            } else {
                ImGui::Spacing();
            }
        }
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
            m_selectedObjectId = 0;
        }
        ImGui::PopStyleColor(2);
    }

    std::string currentCat = (m_selectedCategoryIdx < (int)categories.size()) ? categories[m_selectedCategoryIdx] : "Solar System";

    // Dedicated Action Toolbar Row for the Explorer
    ImGui::Spacing();
    ImVec4 loadCol(0.12f, 0.45f, 0.25f, 0.90f);
    if (UIIcon::Button("##LoadSystemBtn", IconId::Play, "LOAD SYSTEM INTO SIMULATION", ImVec2(250, 26), false, &loadCol)) {
        if (physics.loadFromDatabase(objRepo, currentCat)) {
            if (camera) camera->resetOverview(glm::vec3(0.0f), 6.0f);
            if (activeTopTab) *activeTopTab = 0; // Switch to UNIVERSE
            m_statusMessage = "Loaded system '" + currentCat + "' into simulation.";
        } else {
            m_statusMessage = "Failed to load system '" + currentCat + "' from database.";
        }
    }
    ImGui::SameLine(0, 10);
    ImVec4 refreshCol(0.18f, 0.25f, 0.40f, 0.85f);
    if (UIIcon::Button("##RefreshBaselineBtn", IconId::Refresh, "Refresh Database with NASA/JPL Baseline", ImVec2(310, 26), false, &refreshCol)) {
        SeedData::seedDefaultDatabase(objRepo);
        physics.loadFromDatabase(objRepo, currentCat);
        if (camera) camera->resetOverview(glm::vec3(0.0f), 6.0f);
        m_statusMessage = "Database refreshed with NASA/JPL baseline datasets.";
    }

    if (!m_statusMessage.empty()) {
        ImGui::SameLine(0, 14);
        ImGui::TextColored(DMCol::Green, "%s", m_statusMessage.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Filters Row
    ImGui::TextColored(DMCol::Accent, "TAXONOMY:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(170.0f);
    const char* taxonomyFilters[] = {
        "All Classes", "Stars", "Planets", "Exoplanets", "Moons", "Minor Bodies / Asteroids", "Black Holes"
    };
    ImGui::Combo("##TaxFilter", &m_taxonomyFilterIdx, taxonomyFilters, 7);

    ImGui::SameLine(0, 12);
    ImGui::TextColored(DMCol::Accent, "SOURCE:");
    ImGui::SameLine();
    ImGui::SetNextItemWidth(160.0f);
    const char* sourceFilters[] = {
        "All Sources", "NASA Exoplanet Archive", "NASA / JPL", "Bundled Baseline", "Custom / User"
    };
    ImGui::Combo("##SourceFilter", &m_sourceFilterIdx, sourceFilters, 5);

    ImGui::Spacing();

    // Two Columns: Left list of objects, Right detail view
    float listW = 280.0f;
    float detailW = ImGui::GetContentRegionAvail().x - listW - 16.0f;

    // Left List
    ImGui::BeginChild("##ExplorerList", ImVec2(listW, 420), true);
    auto allObjects = objRepo.getAllObjects(currentCat, true, m_explorerFilter);
    std::vector<ObjectRecord> objects;
    for (const auto& obj : allObjects) {
        if (m_taxonomyFilterIdx > 0) {
            std::string cls = obj.classification;
            std::string typ = obj.type;
            if (m_taxonomyFilterIdx == 1 && cls.find("Star") == std::string::npos && typ.find("Star") == std::string::npos) continue;
            if (m_taxonomyFilterIdx == 2 && cls.find("Planet") == std::string::npos && typ.find("Planet") == std::string::npos) continue;
            if (m_taxonomyFilterIdx == 3 && typ.find("Exoplanet") == std::string::npos && obj.category.find("TRAPPIST") == std::string::npos && obj.category.find("Kepler") == std::string::npos && obj.category.find("Proxima") == std::string::npos) continue;
            if (m_taxonomyFilterIdx == 4 && cls.find("Moon") == std::string::npos && typ.find("Moon") == std::string::npos) continue;
            if (m_taxonomyFilterIdx == 5 && cls.find("Asteroid") == std::string::npos && cls.find("Comet") == std::string::npos && typ.find("Asteroid") == std::string::npos && typ.find("Comet") == std::string::npos) continue;
            if (m_taxonomyFilterIdx == 6 && cls.find("Black Hole") == std::string::npos && typ.find("Black Hole") == std::string::npos) continue;
        }
        if (m_sourceFilterIdx > 0) {
            if (m_sourceFilterIdx == 1 && obj.category != "Exoplanet System" && obj.category.find("TRAPPIST") == std::string::npos && obj.category.find("Kepler") == std::string::npos && obj.type.find("Exoplanet") == std::string::npos) continue;
            if (m_sourceFilterIdx == 2 && obj.category != "Solar System" && obj.category != "Asteroid Belt") continue;
            if (m_sourceFilterIdx == 3 && obj.isSynthetic) continue;
            if (m_sourceFilterIdx == 4 && !obj.isSynthetic && obj.category != "Custom") continue;
        }
        objects.push_back(obj);
    }

    ImGui::InputTextWithHint("##filter", "Filter list...", m_explorerFilter, sizeof(m_explorerFilter));
    ImGui::Separator();

    if (!objects.empty()) {
        bool selFound = false;
        for (const auto& obj : objects) {
            if (obj.id == m_selectedObjectId) {
                selFound = true;
                break;
            }
        }
        if (!selFound) {
            m_selectedObjectId = objects[0].id;
        }
    }

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
    ImGui::BeginChild("##ExplorerDetails", ImVec2(detailW, 420), true);
    if (m_selectedObjectId > 0) {
        auto hydrated = objRepo.getHydratedBody(m_selectedObjectId);
        if (hydrated.has_value()) {
            const auto& b = hydrated.value();

            // Header & Taxonomy Badges
            ImGui::TextColored(DMCol::Accent, "%s", b.name.c_str());
            ImGui::SameLine();
            ImVec4 classBadgeColor = ImVec4(0.2f, 0.7f, 0.9f, 1.0f);
            if (b.isStar()) classBadgeColor = ImVec4(0.95f, 0.75f, 0.20f, 1.0f);
            else if (b.isGasOrIceGiant()) classBadgeColor = ImVec4(0.35f, 0.65f, 0.95f, 1.0f);
            else if (b.isBlackHole()) classBadgeColor = ImVec4(0.80f, 0.35f, 0.95f, 1.0f);
            else if (b.isMoon()) classBadgeColor = ImVec4(0.65f, 0.75f, 0.85f, 1.0f);
            else classBadgeColor = ImVec4(0.35f, 0.85f, 0.45f, 1.0f);

            ImGui::TextColored(classBadgeColor, "[%s]", b.classificationStr.empty() ? b.getClassName().c_str() : b.classificationStr.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.3f, 0.85f, 0.4f, 1.0f), "[%s]", b.provenanceStatus.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.7f, 0.7f, 0.8f, 1.0f), "[%s]", b.appearanceType.c_str());

            ImGui::TextColored(DMCol::TextSecondary, "Slug: %s | Type: %s | System/Category: %s", b.id.c_str(), b.type.c_str(), b.category.c_str());
            ImGui::Separator();

            // SECTION 1: PHYSICAL PROPERTIES
            if (ImGui::CollapsingHeader("PHYSICAL PROPERTIES", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Columns(2, "physCols", false);
                ImGui::TextColored(DMCol::TextSecondary, "Mass:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.massStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Radius:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.radiusStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Mean Density:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.densityStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Surface Gravity:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.gravityStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Escape Velocity:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.escapeVelocityStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, b.isStar() ? "Effective Temperature:" : "Surface / Eq Temp:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.tempStr.c_str());
                ImGui::NextColumn();

                if (b.isStar()) {
                    ImGui::TextColored(DMCol::TextSecondary, "Spectral Type:");
                    ImGui::NextColumn();
                    ImGui::TextColored(classBadgeColor, "%s", b.spectralType.empty() ? "N/A" : b.spectralType.c_str());
                    ImGui::NextColumn();

                    ImGui::TextColored(DMCol::TextSecondary, "Luminosity:");
                    ImGui::NextColumn();
                    if (b.luminosityW > 0.0) {
                        ImGui::Text("%.3e W (%.3f L☉)", b.luminosityW, b.luminosityW / UnitConverter::SOLAR_LUMINOSITY_W);
                    } else {
                        ImGui::Text("N/A");
                    }
                    ImGui::NextColumn();
                } else {
                    ImGui::TextColored(DMCol::TextSecondary, "Surface Pressure:");
                    ImGui::NextColumn();
                    ImGui::Text("%s", b.pressureStr.c_str());
                    ImGui::NextColumn();

                    ImGui::TextColored(DMCol::TextSecondary, "Axial Tilt:");
                    ImGui::NextColumn();
                    ImGui::Text("%s", b.axialTiltStr.c_str());
                    ImGui::NextColumn();

                    ImGui::TextColored(DMCol::TextSecondary, "Rotation Period:");
                    ImGui::NextColumn();
                    ImGui::Text("%s", b.rotationPeriodStr.c_str());
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            }

            // SECTION 2: ORBITAL MECHANICS
            if (!b.isStar() && ImGui::CollapsingHeader("ORBITAL MECHANICS", ImGuiTreeNodeFlags_DefaultOpen)) {
                ImGui::Columns(2, "orbCols", false);
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

                ImGui::TextColored(DMCol::TextSecondary, "Periapsis / Apoapsis:");
                ImGui::NextColumn();
                ImGui::Text("%s / %s", b.periapsisStr.c_str(), b.apoapsisStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Reference Frame:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.referenceFrame.empty() ? "ICRF" : b.referenceFrame.c_str());
                ImGui::NextColumn();
                ImGui::Columns(1);
            }

            // SECTION 3: ATMOSPHERE & CHEMICAL COMPOSITION
            if (ImGui::CollapsingHeader("ATMOSPHERE & CHEMICAL COMPOSITION")) {
                ImGui::Columns(2, "atmCols", false);
                ImGui::TextColored(DMCol::TextSecondary, "Atmosphere:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", b.atmosphereStr.empty() ? "None / Vacuum" : b.atmosphereStr.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Magnetic Field:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.magneticFieldStr.empty() ? "N/A" : b.magneticFieldStr.c_str());
                ImGui::NextColumn();
                ImGui::Columns(1);

                if (!b.chemicalInventory.empty()) {
                    ImGui::Spacing();
                    ImGui::TextColored(DMCol::TextSecondary, "Constituent Species:");
                    for (const auto& ch : b.chemicalInventory) {
                        ImVec4 cCol = ImVec4(ch.color.r, ch.color.g, ch.color.b, 1.0f);
                        ImGui::ColorButton("##chemDot", cCol, ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(12, 12));
                        ImGui::SameLine();
                        ImGui::Text("%s (%s): %.1f%%", ch.name.c_str(), ch.formula.c_str(), ch.percentage);
                    }
                }
            }

            // SECTION 4: VISUAL & APPEARANCE
            if (ImGui::CollapsingHeader("VISUAL & APPEARANCE MODEL")) {
                ImGui::Columns(2, "visCols", false);
                ImGui::TextColored(DMCol::TextSecondary, "Base Texture:");
                ImGui::NextColumn();
                ImGui::TextWrapped("%s", b.texturePath.empty() ? "None (Procedural PBR Surface)" : b.texturePath.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Base Albedo:");
                ImGui::NextColumn();
                ImGui::Text("%.2f", b.baseAlbedo);
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Surface Color Tint:");
                ImGui::NextColumn();
                ImGui::ColorButton("##objColor", ImVec4(b.color.r, b.color.g, b.color.b, 1.0f), ImGuiColorEditFlags_NoTooltip | ImGuiColorEditFlags_NoInputs, ImVec2(18, 18));
                ImGui::NextColumn();

                if (b.ring.hasRing) {
                    ImGui::TextColored(DMCol::TextSecondary, "Ring System:");
                    ImGui::NextColumn();
                    ImGui::Text("Inner: %.0f km | Outer: %.0f km", b.ring.innerRadiusM / 1000.0, b.ring.outerRadiusM / 1000.0);
                    ImGui::NextColumn();
                }
                ImGui::Columns(1);
            }

            // SECTION 5: DATA PROVENANCE
            if (ImGui::CollapsingHeader("DATA PROVENANCE & ARCHIVE RECORD")) {
                ImGui::Columns(2, "provCols", false);
                ImGui::TextColored(DMCol::TextSecondary, "Data Source:");
                ImGui::NextColumn();
                ImGui::TextColored(DMCol::Green, "%s", b.sourceName.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Source Record ID:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.sourceObjectId.empty() ? b.id.c_str() : b.sourceObjectId.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Import Timestamp:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.importTimestamp.c_str());
                ImGui::NextColumn();

                ImGui::TextColored(DMCol::TextSecondary, "Physical Estimation:");
                ImGui::NextColumn();
                ImGui::Text("%s", b.isEstimated ? "Estimated via empirical laws" : "Direct measurement / Astrometric observation");
                ImGui::NextColumn();
                ImGui::Columns(1);
            }

            // ACTION BUTTONS
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (UIIcon::Button(IconId::Target, "Focus in Viewport", ImVec2(150, 26))) {
                physics.selectBodyById(b.id);
                const CelestialBody* sel = nullptr;
                for (const auto& body : physics.getBodies()) {
                    if (body.id == b.id || body.dbId == b.dbId) {
                        sel = &body;
                        break;
                    }
                }
                if (sel) {
                    if (camera) camera->focusOnBody(sel->position, sel->radius3D, 0.85f);
                    if (activeTopTab) *activeTopTab = 0; // Switch to UNIVERSE
                    m_statusMessage = "Focused on '" + b.name + "' in viewport.";
                } else {
                    m_statusMessage = "Object '" + b.name + "' is not currently in the active simulation. Load its system first.";
                }
            }

            ImGui::SameLine(0, 8);
            bool inSim = false;
            for (const auto& body : physics.getBodies()) {
                if (body.id == b.id || body.dbId == b.dbId) {
                    inSim = true;
                    break;
                }
            }
            if (inSim) {
                ImVec4 inSimCol(0.2f, 0.35f, 0.45f, 0.85f);
                if (UIIcon::Button("##InSimBtn", IconId::Check, "In Active Sim", ImVec2(140, 26), false, &inSimCol)) {
                    physics.selectBodyById(b.id);
                    if (activeTopTab) *activeTopTab = 0;
                }
            } else {
                ImVec4 addSimCol(0.12f, 0.45f, 0.25f, 0.90f);
                if (UIIcon::Button("##AddSimBtn", IconId::Add, "Add to Active Sim", ImVec2(150, 26), false, &addSimCol)) {
                    physics.addBody(b);
                    m_statusMessage = "Added '" + b.name + "' to active simulation.";
                }
            }

            ImGui::SameLine(0, 8);
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.2f, 0.2f, 0.85f));
            if (UIIcon::Button(IconId::Delete, "Delete Object", ImVec2(120, 26))) {
                for (int bi = 0; bi < (int)physics.getBodies().size(); ++bi) {
                    if (physics.getBodies()[bi].id == b.id || physics.getBodies()[bi].dbId == b.dbId) {
                        physics.removeBody(bi);
                        break;
                    }
                }
                objRepo.deleteObject(b.dbId);
                m_statusMessage = "Deleted '" + b.name + "' from library.";
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
