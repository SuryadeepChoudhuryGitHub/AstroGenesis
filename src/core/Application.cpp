#include "core/Application.hpp"
#include "data/SeedData.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <algorithm>
#include <iostream>

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
        0x0020, 0x00FF, // Basic Latin + Latin Supplement
        0x0100, 0x017F, // Latin Extended-A
        0x0370, 0x03FF, // Greek (alpha, beta, etc.)
        0x2000, 0x206F, // General Punctuation
        0x2070, 0x209F, // Superscripts and Subscripts (², ³, ⁴, ⁻, etc.)
        0x2100, 0x214F, // Letterlike Symbols (℃, etc.)
        0x2190, 0x21FF, // Arrows (←, ↑, →, ↓)
        0x2200, 0x22FF, // Mathematical Operators (∑, ∆, ∇, √, ∞, etc.)
        0x25A0, 0x25FF, // Geometric Shapes (■, ▲, ▼, ◆, ⬡, ⌖, etc.)
        0x2600, 0x26FF, // Miscellaneous Symbols (★, ☉, ☄, ⚡, ⚙, etc.)
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
    if (!fontLoaded) {
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
    bool canInteract3D = isViewportHovered && !io.WantCaptureMouse;

    bool leftDown  = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT)  == GLFW_PRESS);
    bool rightDown = (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS);

    if (leftDown || rightDown) {
        if (!m_isDraggingViewport && canInteract3D) {
            m_isDraggingViewport = true;
        }
    } else {
        m_isDraggingViewport = false;
    }

    if (m_isDraggingViewport) {
        m_camera.processMouseOrbit(deltaX, deltaY);
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
    }

    if (!io.WantTextInput && !io.WantCaptureKeyboard) {
        if (ImGui::IsKeyPressed(ImGuiKey_Space, false)) {
            m_physics.togglePause();
        }
        if (ImGui::IsKeyPressed(ImGuiKey_R, false)) {
            m_physics.resetSimulation(m_objRepo);
            m_camera.resetOverview(glm::vec3(0.0f), 6.0f);
            m_uiManager.addEventLog("Simulation workspace reset to fresh start (Hotkey: R)");
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

        // Render UI with dynamic database, data manager and validation engine access
        m_uiManager.renderUI(m_physics, m_camera, m_objRepo, m_dataManager, m_valEngine, (float)m_windowWidth, (float)m_windowHeight, fps);

        // Process mouse & keyboard interactions
        processInput(deltaTime);

        // Advance simulation dynamics
        m_physics.update(deltaTime);
        m_camera.setTargetPosition(m_physics.getSelectedBody().position);
        m_camera.update(deltaTime);

        // Get 3D viewport bounds
        float vpX, vpY, vpW, vpH;
        m_uiManager.getViewportBounds(vpX, vpY, vpW, vpH);

        glm::vec4 bgDark{0.039f, 0.055f, 0.102f, 1.00f};
        m_renderer.beginViewport(0, 0, fbW, fbH, bgDark);

        glViewport((int)vpX, (int)(fbH - vpY - vpH), (int)vpW, (int)vpH);
        float aspect = vpW / std::max(vpH, 1.0f);

        // Skybox
        m_renderer.renderSkybox(m_camera, aspect);

        glm::vec3 solPos{0.0f};
        for (const auto& body : m_physics.getBodies()) {
            if (body.id == "sol" || body.type.find("Star") != std::string::npos) {
                solPos = body.position;
                break;
            }
        }

        glm::vec3 camTarget = m_camera.getTargetPosition();

        // Dynamic 3D motion trails
        m_renderer.renderTrails(m_camera, aspect, m_physics.getBodies(), camTarget, m_physics.getSelectedBodyIndex());

        // Asteroid belt / particle field
        m_renderer.renderParticleField(m_camera, aspect, m_physics.getAsteroidBelt(), solPos, camTarget, m_physics.getSimulatedTimeSeconds());

        // Celestial body spheres
        for (const auto& body : m_physics.getBodies()) {
            m_renderer.renderSphere(m_camera, aspect, body, solPos, camTarget);
        }

        // Planetary rings
        m_renderer.renderRings(m_camera, aspect, m_physics.getBodies(), solPos, camTarget);

        // Deformable matter bodies
        m_renderer.renderDeformableBodies(m_camera, aspect, m_physics.getMatterSystem(), solPos, camTarget);

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
