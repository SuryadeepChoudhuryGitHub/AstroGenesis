#pragma once

#include "imgui.h"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "simulation/PhysicsEngine.hpp"

namespace AstroGenesis {

class DataManagerUI {
public:
    DataManagerUI();

    void render(bool& showWindow, 
                DataManager& dataManager, 
                ObjectRepository& objRepo,
                PhysicsEngine& physics,
                float winW, float winH);

private:
    void drawSearchAndImportTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics);
    void drawDatabaseExplorerTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics);
    void drawImportHistoryTab(DataManager& dataManager);
    void drawSourceConfigTab(DataManager& dataManager, ObjectRepository& objRepo);

    int m_activeTab = 0;
    int m_selectedProviderIdx = 0; // 0: JPL Horizons, 1: JPL SBDB, 2: NASA Exoplanet
    char m_searchBuffer[128] = "Ceres";
    char m_categoryBuffer[64] = "Asteroid Belt";

    // Explorer State
    int m_selectedCategoryIdx = 0;
    char m_explorerFilter[64] = "";
    int64_t m_selectedObjectId = 0;
    std::string m_statusMessage;
    float m_statusTimer = 0.0f;
};

} // namespace AstroGenesis
