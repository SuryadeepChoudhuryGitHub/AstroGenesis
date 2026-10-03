#pragma once

#include "imgui.h"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "renderer/Camera.hpp"

namespace AstroGenesis {

class DataManagerUI {
public:
    DataManagerUI();

    void render(bool& showWindow, 
                DataManager& dataManager, 
                ObjectRepository& objRepo,
                PhysicsEngine& physics,
                float winW, float winH,
                Camera* camera = nullptr,
                int* activeTopTab = nullptr);

    void openDatabaseExplorer();
    void selectObjectById(int64_t id, const std::string& category = "");

private:
    void drawSearchAndImportTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics);
    void drawDatabaseExplorerTab(DataManager& dataManager, ObjectRepository& objRepo, PhysicsEngine& physics, Camera* camera, int* activeTopTab, bool& showWindow);
    void drawImportHistoryTab(DataManager& dataManager);
    void drawSourceConfigTab(DataManager& dataManager, ObjectRepository& objRepo);

    int m_activeTab = 0; // Tab 0 is DATABASE EXPLORER
    int m_selectedProviderIdx = 0; // 0: JPL Horizons, 1: JPL SBDB, 2: NASA Exoplanet
    char m_searchBuffer[128] = "Ceres";
    char m_categoryBuffer[64] = "Asteroid Belt";

    // Explorer State
    int m_selectedCategoryIdx = -1;
    std::string m_pendingCategorySelection;
    char m_explorerFilter[64] = "";
    int64_t m_selectedObjectId = 0;
    std::string m_statusMessage;
    float m_statusTimer = 0.0f;
};

} // namespace AstroGenesis
