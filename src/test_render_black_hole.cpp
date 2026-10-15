#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <algorithm>
#include <cstring>

#include <glad/gl.h>
#include <GLFW/glfw3.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "glfw/deps/stb_image_write.h"

#include "renderer/Renderer.hpp"
#include "renderer/Camera.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "simulation/RelativisticPhysics.hpp"

using namespace AstroGenesis;

static void saveFrameToPNG(int width, int height, const std::string& filename) {
    std::vector<unsigned char> pixels(width * height * 4);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, pixels.data());

    // OpenGL reads from bottom-left; flip vertically for PNG
    std::vector<unsigned char> flipped(width * height * 4);
    for (int y = 0; y < height; ++y) {
        std::memcpy(&flipped[y * width * 4],
                    &pixels[(height - 1 - y) * width * 4],
                    width * 4);
    }

    stbi_write_png(filename.c_str(), width, height, 4, flipped.data(), width * 4);
    std::cout << "[VisualValidation] Saved screenshot: " << filename << std::endl;
}

int main(int argc, char** argv) {
    std::cout << "==========================================================" << std::endl;
    std::cout << " AstroGenesis Black Hole Visual Validation Capture Tool" << std::endl;
    std::cout << "==========================================================" << std::endl;

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    const int width = 1280;
    const int height = 720;
    GLFWwindow* window = glfwCreateWindow(width, height, "Offscreen Black Hole Renderer", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create headless GLFW window" << std::endl;
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGL(glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Renderer renderer;
    if (!renderer.initialize()) {
        std::cerr << "Failed to initialize Renderer" << std::endl;
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    // Set cinematic parameters (HDR tone mapping, bloom, exposure)
    renderer.setCinematicParameters(
        true,       // enabled
        1.10f,      // exposure
        0.45f,      // bloomIntensity
        1,          // ACES tone mapping
        false,      // DoF
        3.0f,
        0.05f,
        false,      // Vignette
        false,      // CA
        0.001f,
        500.0f
    );

    float aspect = (float)width / (float)height;

    // Configure a realistic Cygnus X-1 spinning black hole with accretion disk
    VisualBodyState cygX1;
    cygX1.isBlackHole = true;
    cygX1.isStar = false;
    cygX1.name = "Cygnus X-1";
    cygX1.positionAU = glm::vec3(0.0f);
    cygX1.renderRadius = 0.5f;

    double massKg = 14.8 * RelativisticPhysics::SOLAR_MASS_KG;
    double rSchwM = RelativisticPhysics::calculateSchwarzschildRadius(massKg);
    double auM = RelativisticPhysics::AU_TO_METERS;

    cygX1.schwarzschildRadiusAU = (float)(rSchwM / auM);
    cygX1.photonSphereRadiusAU = (float)(RelativisticPhysics::calculatePhotonSphereRadius(massKg) / auM);
    cygX1.shadowRadiusAU = (float)(RelativisticPhysics::calculateCriticalImpactParameter(massKg) / auM);
    cygX1.spinParameter = 0.95f;
    cygX1.spinAxis = glm::vec3(0.0f, 1.0f, 0.0f); // Spin along Y axis
    cygX1.hasAccretionDisk = true;
    cygX1.accretionRateSolarMassPerYear = 1.2e-5f;

    // Novikov-Thorne disk inner edge at prograde ISCO
    double rIscoM = RelativisticPhysics::calculateISCORadius(massKg, cygX1.spinParameter, true);
    cygX1.diskInnerRadiusAU = (float)(rIscoM / auM);
    cygX1.diskOuterRadiusAU = cygX1.schwarzschildRadiusAU * 14.0f;
    cygX1.diskPeakTempK = 1.2e7f;

    // Define test camera positions
    struct TestView {
        std::string name;
        float pitch;
        float yaw;
        float distance;
        bool hasDisk;
        std::string filename;
    };

    std::vector<TestView> views = {
        { "Face-On Disk", glm::radians(85.0f), glm::radians(0.0f), 6.2f, true, "output/visual_validation/01_face_on.png" },
        { "Inclined Disk (35 deg)", glm::radians(35.0f), glm::radians(0.0f), 6.2f, true, "output/visual_validation/02_inclined_35deg.png" },
        { "Near Edge-On Disk (10 deg)", glm::radians(10.0f), glm::radians(0.0f), 6.2f, true, "output/visual_validation/03_near_edge_on.png" },
        { "Edge-On Disk (2 deg)", glm::radians(2.0f), glm::radians(0.0f), 6.2f, true, "output/visual_validation/04_edge_on.png" },
        { "Isolated Black Hole (Pure Lensing)", glm::radians(15.0f), glm::radians(30.0f), 2.4f, false, "output/visual_validation/05_pure_lensing.png" }
    };

    for (const auto& view : views) {
        Camera camera;
        camera.resetOverview(glm::vec3(0.0f), view.distance);
        camera.setOrbit(view.yaw, view.pitch);
        camera.update(0.016f);

        cygX1.hasAccretionDisk = view.hasDisk;

        renderer.beginViewport(0, 0, width, height, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.renderSkybox(camera, aspect);
        renderer.renderCelestialBody(camera, aspect, cygX1, {}, glm::vec3(0.0f), "", VisualMode::Cinematic, DebugVisualOverlay::None, 0.0f);
        renderer.endViewport(width, height);

        saveFrameToPNG(width, height, view.filename);
    }

    std::cout << "All visual validation frames rendered successfully." << std::endl;

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
