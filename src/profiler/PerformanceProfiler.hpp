#pragma once

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>

#include <vector>
#include <deque>
#include <string>
#include <chrono>
#include <memory>
#include <algorithm>

namespace AstroGenesis {

class PhysicsEngine;
class VisualStateAdapter;
class Renderer;

enum class PerformanceStatus {
    Stable,   // Optimal throughput (FPS >= 55, low frame times, stable hardware)
    Warning,  // Moderate contention or frame drops (30-55 FPS, elevated timings)
    Critical  // Severe lag or bottleneck (< 30 FPS, high frame times)
};

struct PerformanceSnapshot {
    double timestampSec = 0.0;
    float fps = 60.0f;
    float frameTimeMs = 16.67f;
    float physicsTimeMs = 0.0f;
    float renderCpuTimeMs = 0.0f;
    float gpuTimeMs = 0.0f;
    float cpuUsagePercent = 0.0f;
    float gpuUsagePercent = 0.0f;
    float ramWorkingSetMB = 0.0f;
    float ramPrivateMB = 0.0f;
    float vramDedicatedMB = 0.0f;
    float vramUsedMB = 0.0f;
    int celestialBodyCount = 0;
    int physicalParticleCount = 0;
    int visualParticleCount = 0;
    float timeScale = 1.0f;
    bool isPaused = false;
    bool isGeneralRelativity = false;
    PerformanceStatus status = PerformanceStatus::Stable;
    std::string statusReason;
};

class PerformanceProfiler {
public:
    PerformanceProfiler();
    ~PerformanceProfiler();

    void initialize(GLFWwindow* mainWindow);
    void shutdown();

    // Frame Lifecycle Hooks
    void beginFrame();
    void markPhysicsTime(float physicsMs);
    void markRenderCpuTime(float renderCpuMs);
    void beginGpuQuery();
    void endGpuQuery();
    void recordMetrics(const PhysicsEngine& physics, 
                       const VisualStateAdapter& visualAdapter, 
                       const Renderer& renderer, 
                       float deltaTime, 
                       float fps);

    // Render profiler window in its dedicated OS window
    void render(const VisualStateAdapter& visualAdapter, const Renderer& renderer);

    // Visibility Controls
    bool isOpen() const { return m_isOpen; }
    void setOpen(bool open);
    void toggleOpen();

    // Profiler Controls
    void resetGraphs();
    void exportReport();
    bool isCollectionPaused() const { return m_paused; }
    void setCollectionPaused(bool paused) { m_paused = paused; }

    int getSamplingIntervalMs() const { return m_samplingIntervalMs; }
    void setSamplingIntervalMs(int ms) { m_samplingIntervalMs = ms; }

private:
    void sampleSystemMetrics();
    void evaluatePerformanceStatus();
    void renderHeaderOverview();
    void renderControlsBar();
    void renderGraphs();
    void renderDetailedMetrics(const VisualStateAdapter& visualAdapter, const Renderer& renderer);
    void renderSystemInfo();
    void applyTheme();

    void drawLineGraph(const char* id,
                       const float* values1, const char* label1, ImU32 color1, const char* unit1,
                       const float* values2, const char* label2, ImU32 color2, const char* unit2,
                       int count, float minVal, float maxVal, const ImVec2& size,
                       float refLine1 = -1.0f, float refLine2 = -1.0f);

    GLFWwindow* m_mainWindow = nullptr;
    GLFWwindow* m_profilerWindow = nullptr;
    ImGuiContext* m_profilerContext = nullptr;

    bool m_isOpen = false;
    bool m_paused = false;
    bool m_firstPositioning = true;
    int m_samplingIntervalMs = 100; // Default 100ms
    float m_sampleAccumulator = 0.0f;

    // OpenGL GPU Queries
    GLuint m_gpuQueries[2] = {0, 0};
    int m_gpuQueryFront = 0;
    int m_gpuQueryBack = 1;
    bool m_gpuQueryActive = false;
    bool m_gpuQueryInFlight = false;
    float m_lastGpuTimeMs = 0.0f;

    // High resolution CPU timings
    float m_lastPhysicsTimeMs = 0.0f;
    float m_lastRenderCpuTimeMs = 0.0f;
    std::chrono::high_resolution_clock::time_point m_frameStartTime;

    // Hardware & Process Resource Monitor
    float m_cpuUsagePercent = 0.0f;
    float m_ramWorkingSetMB = 0.0f;
    float m_ramPrivateMB = 0.0f;
    float m_totalPhysRamGB = 0.0f;
    float m_availPhysRamGB = 0.0f;
    float m_vramDedicatedMB = 0.0f;
    float m_vramUsedMB = 0.0f;
    float m_vramAvailMB = 0.0f;

    // Windows CPU calculation state
    uint64_t m_lastSysKernel = 0;
    uint64_t m_lastSysUser = 0;
    uint64_t m_lastProcKernel = 0;
    uint64_t m_lastProcUser = 0;

    // Hardware string cache
    std::string m_gpuVendor;
    std::string m_gpuRenderer;
    std::string m_glVersion;
    std::string m_cpuInfo;
    int m_logicalCpuCores = 1;

    // Current State & Metric Buffers
    PerformanceSnapshot m_currentSnapshot;
    static constexpr size_t MAX_HISTORY_SAMPLES = 300;

    std::vector<float> m_historyFps;
    std::vector<float> m_historyFrameTime;
    std::vector<float> m_historyCpuUsage;
    std::vector<float> m_historyGpuTime;
    std::vector<float> m_historyRamMB;
    std::vector<float> m_historyVramMB;
    std::deque<PerformanceSnapshot> m_snapshotHistory;

    // Export Feedback Notification
    std::string m_lastExportPath;
    float m_exportNotificationTimer = 0.0f;
    bool m_exportSuccess = false;

    // Active Tab Navigation inside Profiler
    int m_activeTab = 0; // 0: Overview & Graphs, 1: Detailed Breakdown, 2: Hardware & Report
};

} // namespace AstroGenesis
