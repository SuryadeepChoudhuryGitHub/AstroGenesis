#include "core/Application.hpp"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <cstdio>
#include <algorithm>

namespace AstroGenesis {

Application::Application() {}

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

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;

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

    // Mouse drag for Orbit around the center of the celestial object
    if (isViewportHovered && (glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS ||
                              glfwGetMouseButton(m_window, GLFW_MOUSE_BUTTON_RIGHT) == GLFW_PRESS)) {
        m_camera.processMouseOrbit(deltaX, deltaY);
    }

    // Scroll wheel for Zoom in and out around the celestial object center
    if (isViewportHovered && io.MouseWheel != 0.0f) {
        m_camera.processMouseZoom(io.MouseWheel);
    }

    // Keyboard zoom shortcuts (+ / -)
    if (isViewportHovered) {
        if (glfwGetKey(m_window, GLFW_KEY_EQUAL) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_KP_ADD) == GLFW_PRESS) {
            m_camera.processMouseZoom(1.0f * deltaTime * 5.0f);
        }
        if (glfwGetKey(m_window, GLFW_KEY_MINUS) == GLFW_PRESS || glfwGetKey(m_window, GLFW_KEY_KP_SUBTRACT) == GLFW_PRESS) {
            m_camera.processMouseZoom(-1.0f * deltaTime * 5.0f);
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

        // ImGui frame start (processes mouse wheel & input events for current frame)
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // Render UI to establish viewport bounds and hover status
        m_uiManager.renderUI(m_physics, m_camera, (float)m_windowWidth, (float)m_windowHeight, fps);

        // Process scroll wheel zoom & orbit inputs
        processInput(deltaTime);

        // Update physics and lock camera fixed around the center of the celestial object
        m_physics.update(deltaTime);
        m_camera.setTargetPosition(m_physics.getSelectedBody().position);
        m_camera.update(deltaTime);

        // Get viewport bounds from UI
        float vpX, vpY, vpW, vpH;
        m_uiManager.getViewportBounds(vpX, vpY, vpW, vpH);

        glm::vec4 bgDark{0.039f, 0.055f, 0.102f, 1.00f};
        m_renderer.beginViewport(0, 0, fbW, fbH, bgDark);

        // Render celestial bodies in 3D viewport region
        glViewport((int)vpX, (int)(fbH - vpY - vpH), (int)vpW, (int)vpH);
        float aspect = vpW / std::max(vpH, 1.0f);

        // Render skybox (stars background) first, before planets
        m_renderer.renderSkybox(m_camera, aspect);

        glm::vec3 solPos{0.0f};
        for (const auto& body : m_physics.getBodies()) {
            if (body.id == "sol") {
                solPos = body.position;
                break;
            }
        }

        // Camera target is the world-space position of the focused body.
        // Use actual body position directly (not interpolated) so the camera
        // stays perfectly locked to the planet even at high time multipliers.
        glm::vec3 camTarget = m_physics.getSelectedBody().position;
        for (const auto& body : m_physics.getBodies()) {
            m_renderer.renderSphere(m_camera, aspect, body, solPos, camTarget);
        }

        m_renderer.endViewport(fbW, fbH);

        // Render ImGui UI over the viewport
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

    if (m_window) {
        glfwDestroyWindow(m_window);
        m_window = nullptr;
    }
    glfwTerminate();
}

} // namespace AstroGenesis
