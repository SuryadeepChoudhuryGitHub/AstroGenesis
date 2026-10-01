#include "core/Application.hpp"
#include "data/SeedData.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <algorithm>
#include <iostream>
#include <filesystem>
#include "renderer/ShaderLoader.hpp"

namespace AstroGenesis {

Application::Application() 
    : m_db(DatabaseManager::getInstance()),
      m_objRepo(m_db),
      m_ephemRepo(m_db),
      m_valRepo(m_db),
      m_dataManager(m_db, m_objRepo, m_ephemRepo, m_valRepo),
      m_valEngine(m_objRepo, m_ephemRepo, m_valRepo) {}

Application::~Application() {
    shutdown();
}

bool Application::initialize(int width, int height, const char* title) {
    m_windowWidth = width;
    m_windowHeight = height;

    // Normalize working directory so relative paths (assets/, data/) resolve reliably
    try {
        if (!std::filesystem::exists("assets") || !std::filesystem::exists("data")) {
            std::string exeDir = getExecutableDir();
            std::vector<std::filesystem::path> rootCandidates = {
                std::filesystem::current_path() / "..",
                std::filesystem::current_path() / "../..",
            };
            if (!exeDir.empty()) {
                rootCandidates.push_back(exeDir);
                rootCandidates.push_back(std::filesystem::path(exeDir) / "..");
                rootCandidates.push_back(std::filesystem::path(exeDir) / "../..");
                rootCandidates.push_back(std::filesystem::path(exeDir) / "../../..");
            }
            for (const auto& cand : rootCandidates) {
                if (std::filesystem::exists(cand / "assets") && std::filesystem::exists(cand / "data")) {
                    std::filesystem::current_path(cand);
                    break;
                }
            }
        }
    } catch (...) {}

    if (!glfwInit()) {
        fprintf(stderr, "Failed to initialize GLFW\n");
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif
    glfwWindowHint(GLFW_SAMPLES, 4);

    m_window = glfwCreateWindow(width, height, title, nullptr, nullptr);
    if (!m_window) {
        fprintf(stderr, "Failed to create GLFW Window\n");
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(m_window);
    glfwSwapInterval(1); // Enable VSync

    int version = gladLoadGL(glfwGetProcAddress);
    if (!version) {
        fprintf(stderr, "Failed to initialize GLAD\n");
        return false;
    }

    if (!m_renderer.initialize()) {
        fprintf(stderr, "Failed to initialize Renderer\n");
        return false;
    }

    // 1. Initialize SQLite Database & Run Migrations
    if (!m_db.initialize("data/astrogenesis.db")) {
        std::cerr << "[Application] Warning: Database initialization error: " << m_db.getLastError() << std::endl;
    }

    // 2. Ensure Database has latest NASA/JPL high-precision baseline datasets
    SeedData::seedDefaultDatabase(m_objRepo);

    // 3. Initialize External Data Providers
    m_dataManager.initialize();

    // 4. Load Solar System from SQLite Database into Physics Engine
    if (!m_physics.loadFromDatabase(m_objRepo, "Solar System")) {
        std::cerr << "[Application] Failed to load Solar System from database." << std::endl;
    }

    // 5. Initialize Local Machine Learning Orbital Stability Predictor Subsystem
    m_aiManager.initialize("assets/models/orbital_stability_model.json");

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

    const char* fontCandidates[] = {
        "C:/Windows/Fonts/segoeui.ttf",
        "C:/Windows/Fonts/arial.ttf",
        "C:/Windows/Fonts/calibri.ttf",
        "C:/Windows/Fonts/tahoma.ttf",
        "assets/fonts/arial.ttf",
        "assets/fonts/segoeui.ttf"
    };

    static const ImWchar glyphRanges[] = {
        0x0020, 0x00FF, // Basic Latin + Latin Supplement (degree, sup2, sup3, plusminus, micro, etc.)
        0x0100, 0x017F, // Latin Extended-A
        0x0370, 0x03FF, // Greek (alpha, beta, delta, tau, etc.)
        0x2000, 0x206F, // General Punctuation (dash, ellipsis, etc.)
        0x2070, 0x209F, // Superscripts and Subscripts (sup2, sup3, sup4, sup-, etc.)
        0x2100, 0x214F, // Letterlike Symbols (deg C, etc.)
        0x2190, 0x21FF, // Arrows (left, up, right, down, refresh, etc.)
        0x2200, 0x22FF, // Mathematical Operators (sum, delta, nabla, sqrt, infty, etc.)
        0x2300, 0x23FF, // Miscellaneous Technical
        0x25A0, 0x25FF, // Geometric Shapes
        0x2600, 0x26FF, // Miscellaneous Symbols
        0x2700, 0x27BF, // Dingbats
        0x2B00, 0x2BFF, // Miscellaneous Symbols and Arrows
        0
    };

    ImFontConfig fontConfig;
    fontConfig.OversampleH = 3;
    fontConfig.OversampleV = 3;
    fontConfig.PixelSnapH = false;

    bool fontLoaded = false;
    for (const char* path : fontCandidates) {
        FILE* f = fopen(path, "rb");
        if (f) {
            fclose(f);
            ImFont* font = io.Fonts->AddFontFromFileTTF(path, 15.0f, &fontConfig, glyphRanges);
            if (font) {
                fontLoaded = true;
                break;
            }
        }
    }

    if (fontLoaded) {
        // Merge Segoe UI Symbol for complete technical, mathematical, and astronomical Unicode coverage
        const char* symbolFont = "C:/Windows/Fonts/seguisym.ttf";
        FILE* sf = fopen(symbolFont, "rb");
        if (sf) {
            fclose(sf);
            ImFontConfig mergeConfig;
            mergeConfig.MergeMode = true;
            mergeConfig.OversampleH = 2;
            mergeConfig.OversampleV = 2;
            mergeConfig.PixelSnapH = false;
            io.Fonts->AddFontFromFileTTF(symbolFont, 15.0f, &mergeConfig, glyphRanges);
        }
    } else {
        io.Fonts->AddFontDefault();
    }

    m_uiManager.initialize();

    ImGui_ImplGlfw_InitForOpenGL(m_window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Camera initial focus
    m_camera.setTargetPosition(m_physics.getSelectedBody().position, true);
    m_camera.setTargetBodyRadius(m_physics.getSelectedBody().radius3D);

    return true;
}

void Application::processInput(float deltaTime) {
    ImGuiIO& io = ImGui::GetIO();
    double mouseX, mouseY;
    glfwGetCursorPos(m_window, &mouseX, &mouseY);

    float deltaX = (float)(mouseX - m_lastMouseX);
    float deltaY = (float)(mouseY - m_lastMouseY);

    m_lastMouseX = mouseX;
    m_lastMouseY = mouseY;

    bool isViewportHovered = m_uiManager.isViewportHovered();
    bool isManipulatingGizmo = m_uiManager.isManipulatingObject();
    bool canInteract3D = isViewportHovered && !io.WantCaptureMouse && !isManipulatingGizmo;

    bool leftDown   = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT)   == GLFW_PRESS);
    bool rightDown  = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT)  == GLFW_PRESS);
    bool middleDown = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_MIDDLE) == GLFW_PRESS);

    int activeTab = m_uiManager.getActiveTopTab();

    if (activeTab == 1) { // ── EDIT WORKSPACE ──
        // Right-drag: Dedicated camera orbit in EDIT mode so left-drag can manipulate objects
        if (rightDown && !middleDown) {
            if (!m_isDraggingViewport && canInteract3D) {
                m_isDraggingViewport = true;
            }
        } else {
            m_isDraggingViewport = false;
        }
        if (m_isDraggingViewport) {
            m_camera.processMouseOrbit(deltaX, deltaY);
        }

        // Middle-drag: Camera pan in EDIT mode
        if (middleDown) {
            if (!m_isPanningViewport && canInteract3D) {
                m_isPanningViewport = true;
            }
        } else {
            m_isPanningViewport = false;
        }
        if (m_isPanningViewport) {
            m_camera.processMousePan(deltaX, deltaY);
        }

        // Left-drag: Dedicated to object manipulation / gizmo.
        // Only orbit if Select tool is active on empty space (no gizmo, no hovered body)
        if (leftDown && !rightDown && !middleDown) {
            bool toolBusy = m_uiManager.isMoveToolActive() || m_uiManager.isPlaceToolActive() ||
                            m_uiManager.isManipulatingObject() || (m_uiManager.getHoveredBodyIndex() >= 0) ||
                            m_uiManager.isGizmoHovered();
            if (!toolBusy && canInteract3D) {
                m_camera.processMouseOrbit(deltaX, deltaY);
            }
        }
    } else { // ── UNIVERSE & OTHER WORKSPACES ──
        // Right-drag OR Middle-drag: Pan camera (scaled by target distance)
        if (rightDown || middleDown) {
            if (!m_isPanningViewport && canInteract3D) {
                m_isPanningViewport = true;
            }
        } else {
            m_isPanningViewport = false;
        }
        if (m_isPanningViewport) {
            m_camera.processMousePan(deltaX, deltaY);
        }

        // Left-drag on empty viewport: Orbit camera (when not clicking body or UI)
        if (leftDown && !rightDown && !middleDown) {
            if (!m_isDraggingViewport && canInteract3D && m_uiManager.getHoveredBodyIndex() < 0) {
                m_isDraggingViewport = true;
            }
        } else {
            m_isDraggingViewport = false;
        }
        if (m_isDraggingViewport) {
            m_camera.processMouseOrbit(deltaX, deltaY);
        }
    }

    if (canInteract3D && io.MouseWheel != 0.0f) {
        m_camera.processMouseZoom(io.MouseWheel);
    }

    if (isViewportHovered && !io.WantTextInput && !io.WantCaptureKeyboard) {
        if (glfwGetKey(m_window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            m_camera.processMouseZoom(1.0f * deltaTime * 5.0f);
        }
        if (glfwGetKey(m_window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            m_camera.processMouseZoom(-1.0f * deltaTime * 5.0f);
        }
        // Q and E: Smooth camera roll in Photo Mode or Viewport Hover
        if (glfwGetKey(m_window, GLFW_KEY_Q) == GLFW_PRESS) {
            m_camera.roll(-1.2f * deltaTime);
        }
        if (glfwGetKey(m_window, GLFW_KEY_E) == GLFW_PRESS) {
            m_camera.roll(1.2f * deltaTime);
        }
    }

    if (!io.WantTextInput && !io.WantCaptureKeyboard) {
        // Space: Pause/Resume simulation
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            m_physics.togglePause();
        }
        // R: Reset simulation workspace
        if (ImGui::IsKeyPressed(ImGuiKey_R, false) && !io.KeyCtrl) {
            m_physics.resetSimulation(m_objRepo);
            m_camera.resetOverview(glm::vec3(0.0f), 6.0f);
            m_uiManager.addEventLog("Simulation workspace reset to fresh start (Hotkey: R)");
        }
        // F10: Toggle Cinematic Mode
        if (ImGui::IsKeyPressed(ImGuiKey_F10, false)) {
            m_visualAdapter.toggleCinematicMode();
            m_uiManager.addEventLog(m_visualAdapter.isCinematicModeEnabled() ? "Cinematic Mode enabled (Hotkey: F10)" : "Standard Mode active (Hotkey: F10)");
        }
        // P or F11: Toggle Photo Mode
        if (ImGui::IsKeyPressed(ImGuiKey_P, false) || ImGui::IsKeyPressed(ImGuiKey_F11, false)) {
            bool next = !m_visualAdapter.isPhotoModeActive();
            m_visualAdapter.setPhotoModeActive(next);
            m_uiManager.addEventLog(next ? "Photo Mode activated (Hotkey: P / F11)" : "Exited Photo Mode");
        }
        // F12: Toggle UI Visibility for clean capture
        if (ImGui::IsKeyPressed(ImGuiKey_F12, false)) {
            m_visualAdapter.toggleUIHidden();
            m_uiManager.addEventLog(m_visualAdapter.isUIHidden() ? "UI hidden for clean capture (Hotkey: F12)" : "UI restored (Hotkey: F12)");
        }

        // ── SANDBOX SHORTCUTS & ACTIONS ─────────────────────────────────────────
        // Ctrl+Z: Undo, Ctrl+Y / Ctrl+Shift+Z: Redo
        if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Z, false)) {
            if (io.KeyShift) {
                m_uiManager.redo(m_physics);
            } else {
                m_uiManager.undo(m_physics);
            }
        } else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Y, false)) {
            m_uiManager.redo(m_physics);
        }
        // Ctrl+D: Duplicate selected object
        else if (io.KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_D, false)) {
            m_uiManager.duplicateSelectedObject(m_physics);
        }

        // Delete / Backspace: Delete selected object
        if (ImGui::IsKeyPressed(ImGuiKey_Delete, false) || ImGui::IsKeyPressed(ImGuiKey_Backspace, false)) {
            m_uiManager.deleteSelectedObject(m_physics);
        }

        // V or S: Select Tool
        if ((ImGui::IsKeyPressed(ImGuiKey_V, false) || ImGui::IsKeyPressed(ImGuiKey_S, false)) && !io.KeyCtrl) {
            m_uiManager.setActiveTool(SandboxTool::Select);
            m_uiManager.cancelPlacement();
        }

        // M or G: Toggle 3D Move Tool (Gizmo)
        if ((ImGui::IsKeyPressed(ImGuiKey_M, false) || ImGui::IsKeyPressed(ImGuiKey_G, false)) && !io.KeyCtrl) {
            m_uiManager.toggleMoveTool();
        }

        // A: Toggle Add Object palette
        if (ImGui::IsKeyPressed(ImGuiKey_A, false) && !io.KeyCtrl) {
            m_uiManager.m_showAddPalette = !m_uiManager.m_showAddPalette;
        }

        // H: Toggle hierarchy (in EDIT: drawer, in UNIVERSE: left panel)
        if (ImGui::IsKeyPressed(ImGuiKey_H, false) && !io.KeyCtrl) {
            if (m_uiManager.getActiveTopTab() == 0) {
                m_uiManager.toggleUniverseLeft();
            } else {
                m_uiManager.toggleHierarchy();
            }
        }

        // I: Toggle Scientific Details Inspector
        if (ImGui::IsKeyPressed(ImGuiKey_I, false) && !io.KeyCtrl) {
            m_uiManager.toggleDetails();
        }

        // Escape: Smart Contextual Cancel / Deselect / Unhide
        if (ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            if (m_visualAdapter.isUIHidden()) {
                m_visualAdapter.setUIHidden(false);
            } else if (m_visualAdapter.isPhotoModeActive()) {
                m_visualAdapter.setPhotoModeActive(false);
            } else if (m_uiManager.m_placementActive) {
                m_uiManager.cancelPlacement();
            } else if (m_uiManager.m_showAddPalette) {
                m_uiManager.m_showAddPalette = false;
            } else if (m_uiManager.m_showDetailsModal) {
                m_uiManager.m_showDetailsModal = false;
            } else if (m_uiManager.m_activeTool == SandboxTool::Move) {
                m_uiManager.setActiveTool(SandboxTool::Select);
            } else if (m_physics.getSelectedBodyIndex() >= 0) {
                m_physics.selectBody(-1);
            }
        }

        // Z without Ctrl: Reset camera roll
        if (ImGui::IsKeyPressed(ImGuiKey_Z, false) && !io.KeyCtrl) {
            m_camera.resetRoll();
        }

        // Number keys 1-7 switch top-level workspaces
        if (ImGui::IsKeyPressed(ImGuiKey_1, false)) m_uiManager.setActiveTopTab(0); // UNIVERSE
        if (ImGui::IsKeyPressed(ImGuiKey_2, false)) m_uiManager.setActiveTopTab(1); // EDIT
        if (ImGui::IsKeyPressed(ImGuiKey_3, false)) m_uiManager.setActiveTopTab(2); // SYSTEM
        if (ImGui::IsKeyPressed(ImGuiKey_4, false)) m_uiManager.setActiveTopTab(3); // OBJECTS
        if (ImGui::IsKeyPressed(ImGuiKey_5, false)) m_uiManager.setActiveTopTab(4); // EXPLORE
        if (ImGui::IsKeyPressed(ImGuiKey_6, false)) m_uiManager.setActiveTopTab(5); // SIMULATION
        if (ImGui::IsKeyPressed(ImGuiKey_7, false)) m_uiManager.setActiveTopTab(6); // AI ASSISTANT

        // F: Focus camera on selected object
        if (ImGui::IsKeyPressed(ImGuiKey_F, false)) {
            const auto& sel = m_physics.getSelectedBody();
            m_camera.focusOnBody(sel.position, sel.radius3D, 0.85f);
            m_uiManager.addEventLog("Camera focused on " + sel.name + " (Hotkey: F)");
        }
        // C: Clear orbital trails
        if (ImGui::IsKeyPressed(ImGuiKey_C, false)) {
            m_physics.clearTrails();
            m_uiManager.addEventLog("Cleared all orbital trails (Hotkey: C)");
        }
        // G: Toggle General Relativity
        if (ImGui::IsKeyPressed(ImGuiKey_G, false)) {
            m_physics.toggleGeneralRelativity();
            m_uiManager.addEventLog(m_physics.isGeneralRelativityEnabled() ? "Einstein 1PN GR enabled (Hotkey: G)" : "Newtonian gravity active (Hotkey: G)");
        }
        // T: Toggle True Scale
        if (ImGui::IsKeyPressed(ImGuiKey_T, false)) {
            m_physics.setTrueScaleMode(!m_physics.isTrueScaleMode());
            m_uiManager.addEventLog(m_physics.isTrueScaleMode() ? "True 1:1 Scale enabled (Hotkey: T)" : "Visibility Scaled mode (Hotkey: T)");
        }
        // O: Toggle Keplerian Orbit Lines
        if (ImGui::IsKeyPressed(ImGuiKey_O, false)) {
            m_visualAdapter.setOrbitLinesEnabled(!m_visualAdapter.areOrbitLinesEnabled());
            m_uiManager.addEventLog(m_visualAdapter.areOrbitLinesEnabled() ? "Orbit lines visible (Hotkey: O)" : "Orbit lines hidden (Hotkey: O)");
        }
        // [ and ]: Time warp speed
        if (ImGui::IsKeyPressed(ImGuiKey_LeftBracket, false)) {
            float newScale = std::max(1.0f, m_physics.getTimeScale() * 0.5f);
            m_physics.setTimeScale(newScale);
        }
        if (ImGui::IsKeyPressed(ImGuiKey_RightBracket, false)) {
            float newScale = std::min(31536000.0f * 100.0f, m_physics.getTimeScale() * 2.0f);
            m_physics.setTimeScale(newScale);
        }
    }
}

void Application::run() {
    float lastTime = (float)glfwGetTime();

    while (!glfwWindowShouldClose(m_window)) {
        glfwPollEvents();

        float currentTime = (float)glfwGetTime();
        float deltaTime = currentTime - lastTime;
        lastTime = currentTime;
        float fps = (deltaTime > 0.0f) ? (1.0f / deltaTime) : 60.0f;

        int fbW, fbH;
        glfwGetFramebufferSize(m_window, &fbW, &fbH);
        m_windowWidth = fbW;
        m_windowHeight = fbH;

        // ImGui frame start
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Advance simulation dynamics (Authoritative Physics)
        m_physics.update(deltaTime);

        // Process physical collision events
        for (const auto& colEv : m_physics.getRecentCollisions()) {
            m_visualAdapter.registerImpact(colEv.positionAU, colEv.normal, colEv.impactEnergyJoules);
            m_uiManager.addEventLog(colEv.description);
        }
        m_physics.clearRecentCollisions();

        m_aiManager.update(m_physics, deltaTime);
        if (m_uiManager.getActiveTopTab() == 0) {
            // UNIVERSE mode: camera tracks focused body
            m_camera.setTargetPosition(m_physics.getSelectedBody().position);
        } else if (m_uiManager.getActiveTopTab() == 1) {
            // EDIT mode: camera target stays locked during object manipulation
            if (!m_uiManager.isManipulatingObject()) {
                if (m_camera.isTransitioning()) {
                    m_camera.setTargetPosition(m_physics.getSelectedBody().position);
                }
            }
        } else {
            m_camera.setTargetPosition(m_physics.getSelectedBody().position);
        }
        m_camera.update(deltaTime);

        // Update Visual State Adapter (Physics State -> Visual State)
        double simDeltaTime = m_physics.isPaused() ? 0.0 : ((double)deltaTime * (double)m_physics.getTimeScale());
        m_visualAdapter.update(
            m_physics.getBodies(),
            simDeltaTime,
            m_physics.isTrueScaleMode(),
            m_physics.getSizeMultiplier(),
            m_visualAdapter.getVisualMode(),
            m_visualAdapter.getDebugOverlay()
        );
        m_visualAdapter.updateImpactEvents(deltaTime);

        // Render UI with dynamic database, data manager, validation engine, visual state adapter, and AI subsystem
        m_uiManager.renderUI(m_physics, m_camera, m_objRepo, m_dataManager, m_valEngine, m_visualAdapter, m_aiManager, (float)m_windowWidth, (float)m_windowHeight, fps);

        // Process mouse & keyboard interactions
        processInput(deltaTime);

        // Pass cinematic parameters to renderer
        m_renderer.setCinematicParameters(
            m_visualAdapter.isCinematicModeEnabled(),
            m_visualAdapter.getExposure(),
            m_visualAdapter.getBloomIntensity(),
            m_visualAdapter.getToneMappingMode(),
            m_visualAdapter.isDoFEnabled(),
            m_visualAdapter.getFocusDistance(),
            m_visualAdapter.getDoFAperture(),
            m_visualAdapter.isVignetteEnabled(),
            m_visualAdapter.isCAEnabled(),
            m_camera.getNearPlane(),
            m_camera.getFarPlane()
        );

        // Get 3D viewport bounds
        float vpX, vpY, vpW, vpH;
        m_uiManager.getViewportBounds(vpX, vpY, vpW, vpH);

        int vx = (int)vpX;
        int vy = (int)(fbH - vpY - vpH);
        int vw = std::max((int)vpW, 1);
        int vh = std::max((int)vpH, 1);
        float aspect = (float)vw / (float)vh;

        glm::vec4 bgDark{0.0f, 0.0f, 0.0f, 1.00f};
        m_renderer.beginViewport(vx, vy, vw, vh, bgDark);

        // 1. Skybox background
        m_renderer.renderSkybox(m_camera, aspect);

        glm::vec3 camTarget = m_camera.getTargetPosition();
        const auto& starLights = m_visualAdapter.getStarLightSources();
        glm::vec3 primarySunPos = starLights.empty() ? glm::vec3(0.0f) : starLights[0].positionAU;
        float simTime = (float)m_physics.getSimulatedTimeSeconds();

        // 2. Dynamic 3D motion trails & Keplerian osculating curves
        m_renderer.renderTrails(m_camera, aspect, m_physics.getBodies(), camTarget, m_physics.getSelectedBodyIndex(), m_visualAdapter.areOrbitLinesEnabled(), m_visualAdapter.areMotionTrailsEnabled());

        // 3. Asteroid belt / granular particle swarm
        m_renderer.renderParticleField(m_camera, aspect, m_physics.getAsteroidBelt(), primarySunPos, camTarget, m_physics.getSimulatedTimeSeconds());

        // 4. Physical-to-Visual Celestial Bodies (Multi-Star Lighting, Atmospheres, Clouds, Black Holes)
        const auto& visualBodies = m_visualAdapter.getVisualBodies();
        const auto& physicsBodies = m_physics.getBodies();
        for (size_t i = 0; i < visualBodies.size(); ++i) {
            const auto& vb = visualBodies[i];
            std::string texPath = (i < physicsBodies.size()) ? physicsBodies[i].texturePath : "";
            m_renderer.renderCelestialBody(m_camera, aspect, vb, starLights, camTarget, texPath, m_visualAdapter.getVisualMode(), m_visualAdapter.getDebugOverlay(), simTime);
        }

        // 5. Planetary rings with multi-shadow occlusions
        m_renderer.renderRings(m_camera, aspect, m_physics.getBodies(), starLights, camTarget);

        // 6. Collision & Impact Shockwave FX
        m_renderer.renderImpactFX(m_camera, aspect, m_visualAdapter.getActiveImpacts(), camTarget);

        // 7. Deformable matter bodies (XPBD strain, stress, fracture cracks)
        m_renderer.renderDeformableBodies(m_camera, aspect, m_physics.getMatterSystem(), starLights, camTarget, m_physics.getMatterSystem().getVisualizationMode());

        m_renderer.endViewport(fbW, fbH);

        // Render UI overlays
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(m_window);
    }
}

void Application::shutdown() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    m_renderer.shutdown();
    m_db.close();

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

} // namespace AstroGenesis
