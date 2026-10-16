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
    cygX1.diskPeakTempK = 24000.0f; // 24,000 K peak at ISCO -> fiery amber outer rim to brilliant diamond-white inner core

    // Define test scenes
    struct TestScene {
        std::string name;
        float pitch;
        float yaw;
        float distance;
        bool hasDisk;
        float customPeakTempK;
        std::vector<VisualBodyState> sceneBodies;
        std::string filename;
    };

    // Helper to create a star
    auto makeStar = [](const std::string& name, const glm::vec3& pos, float radius, const glm::vec3& col, double tempK, double lumW) {
        VisualBodyState s;
        s.id = name;
        s.name = name;
        s.isStar = true;
        s.isBlackHole = false;
        s.positionAU = pos;
        s.renderRadius = radius;
        s.trueRadiusAU = radius;
        s.baseAlbedo = col;
        s.temperatureColor = col;
        s.surfaceTempK = tempK;
        s.luminosityWatts = lumW;
        return s;
    };

    // 1. Aligned Star (Directly behind black hole at (0,0,-15) along camera view axis -> Complete Einstein Ring)
    // Camera is at (0, 0, +6), looking at black hole at (0, 0, 0).
    // Star is at (0, 0, -15) directly behind.
    std::vector<VisualBodyState> alignedScene;
    alignedScene.push_back(makeStar("AlignedSol", glm::vec3(0.0f, 0.0f, -15.0f), 0.35f, glm::vec3(1.0f, 0.85f, 0.55f), 5778.0, 3.828e26 * 3.5));

    // 2. Off-Axis Star (Displaced near Einstein radius -> Primary curved arc + secondary counter-image)
    std::vector<VisualBodyState> offAxisScene;
    offAxisScene.push_back(makeStar("OffAxisStar", glm::vec3(2.2f, 0.8f, -15.0f), 0.35f, glm::vec3(1.0f, 0.88f, 0.65f), 6200.0, 3.828e26 * 4.0));

    // 3. Multi-Star Field (Blue supergiant, Sol-like yellow star, Red giant)
    std::vector<VisualBodyState> multiStarScene;
    multiStarScene.push_back(makeStar("RigelBlue", glm::vec3(-2.8f, 1.4f, -18.0f), 0.40f, glm::vec3(0.65f, 0.85f, 1.30f), 12000.0, 3.828e26 * 8.0));
    multiStarScene.push_back(makeStar("SolYellow", glm::vec3(1.8f, -1.2f, -14.0f), 0.35f, glm::vec3(1.0f, 0.85f, 0.55f), 5800.0, 3.828e26 * 3.5));
    multiStarScene.push_back(makeStar("BetelgeuseRed", glm::vec3(0.5f, 2.5f, -20.0f), 0.55f, glm::vec3(1.10f, 0.40f, 0.15f), 3200.0, 3.828e26 * 6.0));

    // 4. Combined: Accretion Disk + Background Lensed Star
    std::vector<VisualBodyState> diskAndStarScene;
    diskAndStarScene.push_back(makeStar("BackgroundBeacon", glm::vec3(0.0f, 0.85f, -16.0f), 0.40f, glm::vec3(1.0f, 0.90f, 0.70f), 7500.0, 3.828e26 * 5.0));

    std::vector<TestScene> scenes = {
        {
            "Test A: Aligned Star (Einstein Ring & Opaque Shadow)",
            glm::radians(0.0f), glm::radians(0.0f), 6.0f,
            false, 0.0f,
            alignedScene,
            "output/visual_validation/01_einstein_ring_aligned_star.png"
        },
        {
            "Test B: Off-Axis Star (Gravitational Arc & Counter-Image)",
            glm::radians(0.0f), glm::radians(0.0f), 6.0f,
            false, 0.0f,
            offAxisScene,
            "output/visual_validation/02_off_axis_star_arc.png"
        },
        {
            "Test C: Multi-Star Gravitational Lensing Field",
            glm::radians(0.0f), glm::radians(0.0f), 6.0f,
            false, 0.0f,
            multiStarScene,
            "output/visual_validation/03_multi_star_lensing.png"
        },
        {
            "Test D: Accretion Disk (35 deg Inclined - Doppler Beaming & Arches)",
            glm::radians(35.0f), glm::radians(0.0f), 6.2f,
            true, 0.0f,
            {},
            "output/visual_validation/04_accretion_disk_35deg.png"
        },
        {
            "Test E: Accretion Disk (Edge-On - Slicing Disk & Dual Arches)",
            glm::radians(2.0f), glm::radians(0.0f), 6.2f,
            true, 0.0f,
            {},
            "output/visual_validation/05_accretion_disk_edge_on.png"
        },
        {
            "Test F: Isolated Black Hole (Pure Milky Way Lensing)",
            glm::radians(15.0f), glm::radians(30.0f), 2.5f,
            false, 0.0f,
            {},
            "output/visual_validation/06_pure_skybox_lensing.png"
        },
        {
            "Test G: Combined Accretion Disk with Lensed Background Star",
            glm::radians(25.0f), glm::radians(0.0f), 6.2f,
            true, 0.0f,
            diskAndStarScene,
            "output/visual_validation/07_disk_and_lensed_star.png"
        },
        {
            "Test H: Accretion Disk (Golden-Amber Novikov-Thorne Temperature)",
            glm::radians(30.0f), glm::radians(0.0f), 6.2f,
            true, 7800.0f,
            {},
            "output/visual_validation/08_accretion_disk_golden_amber.png"
        }
    };

    for (const auto& scene : scenes) {
        Camera camera;
        camera.resetOverview(glm::vec3(0.0f), scene.distance);
        camera.setOrbit(scene.yaw, scene.pitch);
        camera.update(0.016f);

        cygX1.hasAccretionDisk = scene.hasDisk;
        if (scene.customPeakTempK > 0.0f) {
            cygX1.diskPeakTempK = scene.customPeakTempK;
        } else {
            cygX1.diskPeakTempK = 24000.0f;
        }

        renderer.beginViewport(0, 0, width, height, glm::vec4(0.0f, 0.0f, 0.0f, 1.0f));
        renderer.renderSkybox(camera, aspect);
        renderer.renderBlackHole(camera, aspect, cygX1, scene.sceneBodies, glm::vec3(0.0f), 0.0f);
        renderer.endViewport(width, height);

        saveFrameToPNG(width, height, scene.filename);
    }

    std::cout << "All visual validation frames rendered successfully." << std::endl;

    renderer.shutdown();
    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}
