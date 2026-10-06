#pragma once

#include "imgui.h"
#include "renderer/Camera.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "simulation/ValidationEngine.hpp"
#include "simulation/UndoRedoManager.hpp"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "ui/DataManagerUI.hpp"
#include "ui/ValidationUI.hpp"
#include "ui/SystemWorkspaceUI.hpp"
#include "ui/ObjectWorkspaceUI.hpp"

namespace AstroGenesis {

namespace ai {
class AIManager;
}

struct EventLogEntry {
    std::string timeStr;
    std::string message;
};

enum class SandboxTool {
    Select,
    Move,
    Place
};

enum class DragState {
    Idle,
    DragPending,
    Dragging,
    Released
};

enum class GizmoHandle {
    None,
    AxisX,
    AxisY,
    AxisZ,
    PlaneXZ,    // Orbital horizontal plane
    PlaneXY,    // Front vertical plane
    PlaneYZ,    // Side vertical plane
    CenterFree, // Free move along camera view plane
    CenterPlane = PlaneXZ
};

class UIManager {
public:
    UIManager();

    void initialize();
    void renderUI(PhysicsEngine& physics, 
                  Camera& camera, 
                  ObjectRepository& objRepo,
                  DataManager& dataManager,
                  ValidationEngine& valEngine,
                  VisualStateAdapter& visualAdapter,
                  ai::AIManager& aiManager,
                  float windowWidth, float windowHeight, float fps);

    bool isViewportHovered() const { return m_viewportHovered; }
    int getHoveredBodyIndex() const { return m_hoveredBodyIndex; }
    void getViewportBounds(float& outX, float& outY, float& outW, float& outH) const;

    void addEventLog(const std::string& message);

    void openDataManager() { m_showDataManager = true; }
    void openValidationDashboard() { m_showValidationDashboard = true; }
    bool shouldOpenProfiler() const { return m_requestOpenProfiler; }
    void clearProfilerRequest() { m_requestOpenProfiler = false; }
    void requestOpenProfiler() { m_requestOpenProfiler = true; }
    void setActiveTopTab(int tab) { m_activeTopTab = tab; }
    int getActiveTopTab() const { return m_activeTopTab; }

    // Interactive Simulation Sandbox Controls
    bool isManipulatingObject() const { 
        return (m_dragState == DragState::DragPending || m_dragState == DragState::Dragging || m_isDraggingGizmo) || m_placementActive; 
    }
    bool isMoveToolActive() const { return m_activeTool == SandboxTool::Move; }
    bool isPlaceToolActive() const { return m_placementActive || m_activeTool == SandboxTool::Place; }
    bool isGizmoHovered() const { 
        return (m_activeGizmoHandle != GizmoHandle::None) || (m_dragState != DragState::Idle); 
    }
    SandboxTool getActiveTool() const { return m_activeTool; }
    void setActiveTool(SandboxTool tool) { m_activeTool = tool; }
    void toggleMoveTool() {
        m_activeTool = (m_activeTool == SandboxTool::Move) ? SandboxTool::Select : SandboxTool::Move;
    }

    void startPlacement(const std::string& templateClass, int mode = 0) {
        m_placementTemplate = templateClass;
        m_placementMode = mode;
        m_placementActive = true;
        m_activeTool = SandboxTool::Place;
    }
    void cancelPlacement() {
        m_placementActive = false;
        if (m_activeTool == SandboxTool::Place) {
            m_activeTool = SandboxTool::Select;
        }
    }

    void deleteSelectedObject(PhysicsEngine& physics);
    void duplicateSelectedObject(PhysicsEngine& physics);
    void undo(PhysicsEngine& physics);
    void redo(PhysicsEngine& physics);
    bool canUndo() const { return m_undoRedo.canUndo(); }
    bool canRedo() const { return m_undoRedo.canRedo(); }

    void toggleHierarchy() { m_showHierarchy = !m_showHierarchy; }
    void toggleDetails() { m_showDetailsModal = !m_showDetailsModal; }
    void toggleUniverseLeft() { m_universeLeftCollapsed = !m_universeLeftCollapsed; }
    void toggleUniverseRight() { m_universeRightCollapsed = !m_universeRightCollapsed; }
    void toggleUniverseBottom() { m_universeBottomCollapsed = !m_universeBottomCollapsed; }

    // Core Global & Workspace Layout Panels
    void drawTopBar(float width, PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter);
    void drawPhotoModeToolbar(PhysicsEngine& physics, Camera& camera, VisualStateAdapter& visualAdapter, float winW, float winH);
    void drawHiddenUIOverlay(VisualStateAdapter& visualAdapter, float winW, float winH);

    // UNIVERSE Workspace Panels (Observatory)
    void drawLeftPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH);
    void drawInfoOverlay(const CelestialBody& body, float x, float y);
    void drawRightPanel(PhysicsEngine& physics, Camera& camera, CelestialBody& body, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, float topBarH, float winW, float winH, float statusBarH);
    void drawTimeControls(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float x, float y, float w, float h);
    void drawSimMetrics(PhysicsEngine& physics, float fps, float x, float y, float w, float h);
    void drawOrbitVis(PhysicsEngine& physics, Camera& camera, float x, float y, float w, float h);

    // Simulation Sandbox Widgets & Viewport Overlays (EDIT Studio)
    void drawFloatingSimBar(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float vpX, float vpY, float vpW, float vpH);
    void drawCompactContextCard(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float vpX, float vpY, float vpW, float vpH);
    void drawEditPropertiesPanel(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, DataManager& dataManager, float vpX, float vpY, float vpW, float vpH);
    void drawInteractiveGizmo(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH);
    void drawPlacementGuide(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH);
    void drawCollapsibleHierarchy(PhysicsEngine& physics, Camera& camera, ObjectRepository& objRepo, float topBarH, float statusBarH, float winH);
    void drawDetailsInspector(PhysicsEngine& physics, DataManager& dataManager, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, ai::AIManager& aiManager, float topBarH, float winW, float winH, float statusBarH);
    void drawAddObjectPalette(PhysicsEngine& physics, Camera& camera, float vpX, float vpY, float vpW, float vpH);

    void drawViewportHUD(PhysicsEngine& physics, Camera& camera, VisualStateAdapter& visualAdapter, float vpX, float vpY, float vpW, float vpH);
    void drawStatusBar(const PhysicsEngine& physics, const Camera& camera, float winW, float winH, float barH);
    void drawAsteroidBeltDiagnostics(PhysicsEngine& physics, ObjectRepository& objRepo, float winW, float winH);
    void drawMatterLab(PhysicsEngine& physics, Camera& camera, float winW, float winH);

    // Extra Workspaces
    void drawExploreWorkspace(ObjectRepository& objRepo, PhysicsEngine& physics, Camera& camera, float winW, float winH);
    void drawSimulationWorkspace(PhysicsEngine& physics, Camera& camera, ValidationEngine& valEngine, ObjectRepository& objRepo, VisualStateAdapter& visualAdapter, float winW, float winH);
    void drawAIAssistantWorkspace(PhysicsEngine& physics, ObjectRepository& objRepo, ai::AIManager& aiManager, float winW, float winH);

    // State Variables
    bool m_viewportHovered = false;
    int m_hoveredBodyIndex = -1;
    bool m_showAsteroidBeltDiagnostics = false;
    bool m_showMatterLab = false;
    bool m_showDataManager = false;
    bool m_showValidationDashboard = false;
    int m_activeTopTab = 0; // 0: UNIVERSE, 1: EDIT, 2: SYSTEM, 3: OBJECTS, 4: EXPLORE, 5: SIMULATION, 6: AI ASSISTANT
    char m_searchQuery[64] = "";

    // Universe Collapsible Panels
    bool m_universeLeftCollapsed = false;
    bool m_universeRightCollapsed = false;
    bool m_universeBottomCollapsed = false;
    float m_orbitVisZoom = 1.0f;

    // Viewport geometry
    float m_viewportX = 210.0f;
    float m_viewportY = 48.0f;
    float m_viewportW = 1080.0f;
    float m_viewportH = 632.0f;

    // Interactive Sandbox Editor State
    SandboxTool m_activeTool = SandboxTool::Select;
    GizmoHandle m_activeGizmoHandle = GizmoHandle::None;
    GizmoHandle m_lockedGizmoHandle = GizmoHandle::None;
    DragState m_dragState = DragState::Idle;
    bool m_isDraggingGizmo = false;
    ImVec2 m_dragStartMousePos{0.0f, 0.0f};
    glm::vec3 m_dragStartBodyPosAU{0.0f};
    glm::dvec3 m_dragStartBodyVelMps{0.0};
    glm::vec3 m_dragStartHitAU{0.0f};
    float m_dragStartAxisT = 0.0f;
    glm::vec3 m_dragAxisDir{1.0f, 0.0f, 0.0f};
    glm::vec3 m_dragConstraintPlaneNormal{0.0f, 1.0f, 0.0f};
    bool m_dragHasMoved = false;

    // Placement State
    bool m_placementActive = false;
    bool m_showAddPalette = false;
    std::string m_placementTemplate = "Planet";
    int m_placementMode = 0; // 0: Smart Orbit, 1: Free Placement, 2: Satellite/Moon
    int m_placementParentIdx = 0;

    // Collapsible Panels & Deep-Dive
    bool m_showHierarchy = false;       // Collapsed by default for clean simulation visibility
    bool m_showDetailsModal = false;    // Deep-dive scientific inspector
    int m_detailsActiveTab = 0;         // 0: Overview, 1: Atmosphere, 2: Orbits, 3: AI Stability, 4: Rings/Matter, 5: Composition
    bool m_contextCardCollapsed = false;
    bool m_editPropCollapsed = false;
    bool m_requestOpenProfiler = false;

    // History
    UndoRedoManager m_undoRedo;
    std::vector<EventLogEntry> m_eventLogs;

    DataManagerUI m_dataManagerUI;
    ValidationUI m_validationUI;
    SystemWorkspaceUI m_systemWorkspaceUI;
    ObjectWorkspaceUI m_objectWorkspaceUI;
};

} // namespace AstroGenesis
