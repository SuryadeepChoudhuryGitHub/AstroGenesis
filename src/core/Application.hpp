#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#include "renderer/Camera.hpp"
#include "renderer/Renderer.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "simulation/ValidationEngine.hpp"
#include "data/DatabaseManager.hpp"
#include "data/DataManager.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/repositories/EphemerisRepository.hpp"
#include "data/repositories/ValidationRepository.hpp"
#include "ui/UIManager.hpp"

namespace AstroGenesis {

class Application {
public:
    Application();
    ~Application();

    bool initialize(int width, int height, const char* title);
    void run();
    void shutdown();

private:
    void processInput(float deltaTime);

    GLFWwindow* m_window = nullptr;
    int m_windowWidth = 1600;
    int m_windowHeight = 900;

    Camera m_camera;
    Renderer m_renderer;

    // Database and Data Access Layer
    DatabaseManager& m_db;
    ObjectRepository m_objRepo;
    EphemerisRepository m_ephemRepo;
    ValidationRepository m_valRepo;
    DataManager m_dataManager;

    // Simulation and Validation Engines
    PhysicsEngine m_physics;
    ValidationEngine m_valEngine;
    UIManager m_uiManager;

    double m_lastMouseX = 0.0;
    double m_lastMouseY = 0.0;
    bool m_isDraggingViewport = false;
};

} // namespace AstroGenesis
