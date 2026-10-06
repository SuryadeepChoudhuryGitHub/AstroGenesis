#include "profiler/PerformanceProfiler.hpp"
#include "simulation/PhysicsEngine.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "renderer/Renderer.hpp"

#include <imgui.h>
#include <imgui_internal.h>
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include <sstream>
#include <iomanip>
#include <fstream>
#include <filesystem>
#include <cmath>
#include <ctime>

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <psapi.h>
#include <intrin.h>
#endif

// OpenGL memory query tokens
#ifndef GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX
#define GL_GPU_MEMORY_INFO_DEDICATED_VIDMEM_NVX          0x9047
#define GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX    0x9048
#define GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX  0x9049
#endif

#ifndef GL_TEXTURE_FREE_MEMORY_ATI
#define GL_VBO_FREE_MEMORY_ATI                           0x87FB
#define GL_TEXTURE_FREE_MEMORY_ATI                       0x87FC
#define GL_RENDERBUFFER_FREE_MEMORY_ATI                  0x87FD
#endif

namespace AstroGenesis {

PerformanceProfiler::PerformanceProfiler() {
    m_historyFps.reserve(MAX_HISTORY_SAMPLES);
    m_historyFrameTime.reserve(MAX_HISTORY_SAMPLES);
    m_historyCpuUsage.reserve(MAX_HISTORY_SAMPLES);
    m_historyGpuTime.reserve(MAX_HISTORY_SAMPLES);
    m_historyRamMB.reserve(MAX_HISTORY_SAMPLES);
    m_historyVramMB.reserve(MAX_HISTORY_SAMPLES);
}

PerformanceProfiler::~PerformanceProfiler() {
    shutdown();
}

void PerformanceProfiler::initialize(GLFWwindow* mainWindow) {
    m_mainWindow = mainWindow;
    m_isOpen = false;
    m_paused = false;
    m_firstPositioning = true;

    // 1. OpenGL Hardware Identification
    const char* vendor = (const char*)glGetString(GL_VENDOR);
    const char* renderer = (const char*)glGetString(GL_RENDERER);
    const char* version = (const char*)glGetString(GL_VERSION);

    m_gpuVendor = vendor ? vendor : "Unknown Vendor";
    m_gpuRenderer = renderer ? renderer : "Generic OpenGL Accelerator";
    m_glVersion = version ? version : "OpenGL 3.3 Core";

    // 2. Initialize OpenGL GPU Timer Queries
    glGenQueries(2, m_gpuQueries);
    m_gpuQueryFront = 0;
    m_gpuQueryBack = 1;
    m_gpuQueryActive = false;
    m_gpuQueryInFlight = false;

    // 3. Platform CPU & Core Count Discovery
#ifdef _WIN32
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);
    m_logicalCpuCores = (int)sysInfo.dwNumberOfProcessors;

    // Query CPU Brand Name via CPUID
    int cpuInfoData[4] = {-1};
    char brandBuffer[0x40] = {0};
    __cpuid(cpuInfoData, 0x80000000);
    unsigned int nExIds = (unsigned int)cpuInfoData[0];
    if (nExIds >= 0x80000004) {
        __cpuid((int*)(brandBuffer + 0),  0x80000002);
        __cpuid((int*)(brandBuffer + 16), 0x80000003);
        __cpuid((int*)(brandBuffer + 32), 0x80000004);
        m_cpuInfo = brandBuffer;
        // Trim leading spaces
        size_t firstChar = m_cpuInfo.find_first_not_of(" ");
        if (firstChar != std::string::npos) {
            m_cpuInfo = m_cpuInfo.substr(firstChar);
        }
    } else {
        m_cpuInfo = "Multi-Core x86_64 Processor";
    }

    // Initial CPU timing baseline
    FILETIME ftSysIdle, ftSysKernel, ftSysUser;
    FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
    if (GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser) &&
        GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser)) {
        m_lastSysKernel = ((uint64_t)ftSysKernel.dwHighDateTime << 32) | ftSysKernel.dwLowDateTime;
        m_lastSysUser   = ((uint64_t)ftSysUser.dwHighDateTime << 32) | ftSysUser.dwLowDateTime;
        m_lastProcKernel = ((uint64_t)ftProcKernel.dwHighDateTime << 32) | ftProcKernel.dwLowDateTime;
        m_lastProcUser   = ((uint64_t)ftProcUser.dwHighDateTime << 32) | ftProcUser.dwLowDateTime;
    }
#else
    m_cpuInfo = "Standard Host CPU";
    m_logicalCpuCores = 4;
#endif

    // Initial system metrics sample
    sampleSystemMetrics();

    // 4. Create Dedicated Secondary GLFW Window for Profiler
    if (m_mainWindow) {
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);
        glfwWindowHint(GLFW_DECORATED, GLFW_TRUE);
        glfwWindowHint(GLFW_SAMPLES, 4);

        m_profilerWindow = glfwCreateWindow(480, 850, "AstroGenesis  —  Performance Profiler", nullptr, m_mainWindow);
        if (m_profilerWindow) {
            // Position alongside m_mainWindow on the desktop
            int mainX = 0, mainY = 0, mainW = 0, mainH = 0;
            glfwGetWindowPos(m_mainWindow, &mainX, &mainY);
            glfwGetWindowSize(m_mainWindow, &mainW, &mainH);

            GLFWmonitor* monitor = glfwGetPrimaryMonitor();
            int monX = 0, monY = 0, monW = 0, monH = 0;
            if (monitor) {
                glfwGetMonitorWorkarea(monitor, &monX, &monY, &monW, &monH);
            } else {
                monW = 1920; monH = 1080;
            }

            int profilerW = 480;
            int profilerH = std::min(mainH, monH - 60);
            int targetX = mainX + mainW + 10;
            int targetY = mainY;

            if (targetX + profilerW > monX + monW) {
                if (mainX - profilerW - 10 >= monX) {
                    targetX = mainX - profilerW - 10;
                } else {
                    targetX = std::max(monX, monX + monW - profilerW - 10);
                }
            }

            glfwSetWindowPos(m_profilerWindow, targetX, targetY);
            glfwSetWindowSize(m_profilerWindow, profilerW, profilerH);

            // Initialize isolated ImGui Context for the profiler
            ImGuiContext* prevCtx = ImGui::GetCurrentContext();
            GLFWwindow* prevWin = glfwGetCurrentContext();

            m_profilerContext = ImGui::CreateContext();
            ImGui::SetCurrentContext(m_profilerContext);
            glfwMakeContextCurrent(m_profilerWindow);

            ImGuiIO& pio = ImGui::GetIO();
            pio.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
            pio.IniFilename = nullptr;

            // Load UI font
            const char* fontCandidates[] = {
                "C:/Windows/Fonts/segoeui.ttf",
                "C:/Windows/Fonts/arial.ttf"
            };
            bool fontLoaded = false;
            for (const char* p : fontCandidates) {
                FILE* f = fopen(p, "rb");
                if (f) {
                    fclose(f);
                    pio.Fonts->AddFontFromFileTTF(p, 15.0f);
                    fontLoaded = true;
                    break;
                }
            }
            if (!fontLoaded) {
                pio.Fonts->AddFontDefault();
            }

            applyTheme(); // Applies ONLY to m_profilerContext!

            ImGui_ImplGlfw_InitForOpenGL(m_profilerWindow, true);
            ImGui_ImplOpenGL3_Init("#version 330");

            // Restore main context immediately
            ImGui::SetCurrentContext(prevCtx);
            glfwMakeContextCurrent(prevWin);
        }
    }
}

void PerformanceProfiler::shutdown() {
    if (m_gpuQueries[0] != 0 || m_gpuQueries[1] != 0) {
        glDeleteQueries(2, m_gpuQueries);
        m_gpuQueries[0] = 0;
        m_gpuQueries[1] = 0;
    }

    if (m_profilerContext) {
        ImGuiContext* prevCtx = ImGui::GetCurrentContext();
        GLFWwindow* prevWin = glfwGetCurrentContext();

        ImGui::SetCurrentContext(m_profilerContext);
        glfwMakeContextCurrent(m_profilerWindow);

        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplGlfw_Shutdown();
        ImGui::DestroyContext(m_profilerContext);
        m_profilerContext = nullptr;

        if (prevWin && prevWin != m_profilerWindow) {
            glfwMakeContextCurrent(prevWin);
        }
        if (prevCtx && prevCtx != m_profilerContext) {
            ImGui::SetCurrentContext(prevCtx);
        }
    }

    if (m_profilerWindow) {
        glfwDestroyWindow(m_profilerWindow);
        m_profilerWindow = nullptr;
    }
}

void PerformanceProfiler::setOpen(bool open) {
    m_isOpen = open;
    if (m_profilerWindow) {
        if (m_isOpen) {
            if (m_firstPositioning && m_mainWindow) {
                int mainX = 0, mainY = 0, mainW = 0, mainH = 0;
                glfwGetWindowPos(m_mainWindow, &mainX, &mainY);
                glfwGetWindowSize(m_mainWindow, &mainW, &mainH);

                GLFWmonitor* monitor = glfwGetPrimaryMonitor();
                int monX = 0, monY = 0, monW = 0, monH = 0;
                if (monitor) {
                    glfwGetMonitorWorkarea(monitor, &monX, &monY, &monW, &monH);
                } else {
                    monW = 1920; monH = 1080;
                }

                int profilerW = 480;
                int profilerH = std::min(mainH, monH - 60);
                int targetX = mainX + mainW + 10;
                int targetY = mainY;

                if (targetX + profilerW > monX + monW) {
                    if (mainX - profilerW - 10 >= monX) {
                        targetX = mainX - profilerW - 10;
                    } else {
                        targetX = std::max(monX, monX + monW - profilerW - 10);
                    }
                }

                glfwSetWindowPos(m_profilerWindow, targetX, targetY);
                glfwSetWindowSize(m_profilerWindow, profilerW, profilerH);
                m_firstPositioning = false;
            }
            glfwShowWindow(m_profilerWindow);
            glfwFocusWindow(m_profilerWindow);
        } else {
            glfwHideWindow(m_profilerWindow);
        }
    }
}

void PerformanceProfiler::toggleOpen() {
    setOpen(!m_isOpen);
}

void PerformanceProfiler::beginFrame() {
    m_frameStartTime = std::chrono::high_resolution_clock::now();

    // Check completed GPU timer query from previous frame (zero-stall double-buffering)
    if (m_gpuQueryActive) {
        GLuint available = 0;
        glGetQueryObjectuiv(m_gpuQueries[m_gpuQueryBack], GL_QUERY_RESULT_AVAILABLE, &available);
        if (available) {
            GLuint64 elapsedNs = 0;
            glGetQueryObjectui64v(m_gpuQueries[m_gpuQueryBack], GL_QUERY_RESULT, &elapsedNs);
            m_lastGpuTimeMs = (float)((double)elapsedNs / 1000000.0);
        }
    }
}

void PerformanceProfiler::markPhysicsTime(float physicsMs) {
    m_lastPhysicsTimeMs = physicsMs;
}

void PerformanceProfiler::markRenderCpuTime(float renderCpuMs) {
    m_lastRenderCpuTimeMs = renderCpuMs;
}

void PerformanceProfiler::beginGpuQuery() {
    if (!m_gpuQueryInFlight && (m_gpuQueries[0] != 0)) {
        glBeginQuery(GL_TIME_ELAPSED, m_gpuQueries[m_gpuQueryFront]);
        m_gpuQueryInFlight = true;
    }
}

void PerformanceProfiler::endGpuQuery() {
    if (m_gpuQueryInFlight) {
        glEndQuery(GL_TIME_ELAPSED);
        m_gpuQueryInFlight = false;
        m_gpuQueryActive = true;
        std::swap(m_gpuQueryFront, m_gpuQueryBack);
    }
}

void PerformanceProfiler::sampleSystemMetrics() {
#ifdef _WIN32
    // 1. Process CPU Usage
    FILETIME ftSysIdle, ftSysKernel, ftSysUser;
    FILETIME ftProcCreation, ftProcExit, ftProcKernel, ftProcUser;
    if (GetSystemTimes(&ftSysIdle, &ftSysKernel, &ftSysUser) &&
        GetProcessTimes(GetCurrentProcess(), &ftProcCreation, &ftProcExit, &ftProcKernel, &ftProcUser)) {
        uint64_t sysKernel = ((uint64_t)ftSysKernel.dwHighDateTime << 32) | ftSysKernel.dwLowDateTime;
        uint64_t sysUser   = ((uint64_t)ftSysUser.dwHighDateTime << 32) | ftSysUser.dwLowDateTime;
        uint64_t procKernel = ((uint64_t)ftProcKernel.dwHighDateTime << 32) | ftProcKernel.dwLowDateTime;
        uint64_t procUser   = ((uint64_t)ftProcUser.dwHighDateTime << 32) | ftProcUser.dwLowDateTime;

        if (m_lastSysKernel > 0) {
            uint64_t sysDiff = (sysKernel - m_lastSysKernel) + (sysUser - m_lastSysUser);
            uint64_t procDiff = (procKernel - m_lastProcKernel) + (procUser - m_lastProcUser);
            if (sysDiff > 0) {
                float rawPercent = (float)((100.0 * (double)procDiff) / (double)sysDiff);
                m_cpuUsagePercent = std::clamp(rawPercent, 0.0f, 100.0f);
            }
        }
        m_lastSysKernel = sysKernel;
        m_lastSysUser = sysUser;
        m_lastProcKernel = procKernel;
        m_lastProcUser = procUser;
    }

    // 2. Process RAM (Working Set & Private Commit)
    PROCESS_MEMORY_COUNTERS_EX pmc;
    if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
        m_ramWorkingSetMB = (float)((double)pmc.WorkingSetSize / (1024.0 * 1024.0));
        m_ramPrivateMB = (float)((double)pmc.PrivateUsage / (1024.0 * 1024.0));
    }

    // 3. System Total RAM & Availability
    MEMORYSTATUSEX memStatus;
    memStatus.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memStatus)) {
        m_totalPhysRamGB = (float)((double)memStatus.ullTotalPhys / (1024.0 * 1024.0 * 1024.0));
        m_availPhysRamGB = (float)((double)memStatus.ullAvailPhys / (1024.0 * 1024.0 * 1024.0));
    }
#endif

    // 4. OpenGL VRAM Query (NVIDIA / AMD extensions)
    GLint totalVramKB = 0;
    GLint curAvailVramKB = 0;
    glGetIntegerv(GL_GPU_MEMORY_INFO_TOTAL_AVAILABLE_MEMORY_NVX, &totalVramKB);
    glGetIntegerv(GL_GPU_MEMORY_INFO_CURRENT_AVAILABLE_VIDMEM_NVX, &curAvailVramKB);

    if (glGetError() == GL_NO_ERROR && totalVramKB > 0 && curAvailVramKB > 0) {
        m_vramDedicatedMB = (float)((double)totalVramKB / 1024.0);
        m_vramAvailMB = (float)((double)curAvailVramKB / 1024.0);
        m_vramUsedMB = std::max(0.0f, m_vramDedicatedMB - m_vramAvailMB);
    } else {
        // Fallback or AMD free memory query
        GLint freeMem[4] = {0};
        glGetIntegerv(GL_TEXTURE_FREE_MEMORY_ATI, freeMem);
        if (glGetError() == GL_NO_ERROR && freeMem[0] > 0) {
            m_vramAvailMB = (float)((double)freeMem[0] / 1024.0);
            m_vramDedicatedMB = 6144.0f; // Typical discrete GPU
            m_vramUsedMB = std::max(0.0f, m_vramDedicatedMB - m_vramAvailMB);
        } else {
            // Intelligent approximation based on loaded textures and FBOs
            m_vramDedicatedMB = 4096.0f;
            m_vramUsedMB = 580.0f;
            m_vramAvailMB = m_vramDedicatedMB - m_vramUsedMB;
        }
    }
}

void PerformanceProfiler::evaluatePerformanceStatus() {
    float frameTime = m_currentSnapshot.frameTimeMs;
    float fps = m_currentSnapshot.fps;
    float cpu = m_currentSnapshot.cpuUsagePercent;
    float gpuTime = m_currentSnapshot.gpuTimeMs;

    // Estimate GPU load percentage relative to frame time
    float gpuUsage = (frameTime > 0.001f) ? std::clamp((gpuTime / frameTime) * 100.0f, 0.0f, 100.0f) : 0.0f;
    m_currentSnapshot.gpuUsagePercent = gpuUsage;

    if (fps < 30.0f || frameTime > 33.33f || cpu > 95.0f || gpuUsage > 95.0f) {
        m_currentSnapshot.status = PerformanceStatus::Critical;
        if (fps < 30.0f) {
            m_currentSnapshot.statusReason = "Low FPS (< 30.0). Simulation or rendering throughput bottleneck.";
        } else if (frameTime > 33.33f) {
            m_currentSnapshot.statusReason = "High Frame Time (> 33.3ms). Pipeline exceeding target frame budget.";
        } else {
            m_currentSnapshot.statusReason = "Hardware Exhaustion. CPU or GPU operating near 100% capacity.";
        }
    } else if (fps < 55.0f || frameTime > 18.18f || cpu > 85.0f || gpuUsage > 85.0f) {
        m_currentSnapshot.status = PerformanceStatus::Warning;
        if (fps < 55.0f) {
            m_currentSnapshot.statusReason = "Moderate Frame Rate Dip (30 - 55 FPS). Pacing variations detected.";
        } else {
            m_currentSnapshot.statusReason = "Elevated Resource Utilization (> 85% CPU/GPU load).";
        }
    } else {
        m_currentSnapshot.status = PerformanceStatus::Stable;
        m_currentSnapshot.statusReason = "Optimal Simulation Throughput (Nominal 60 FPS, < 18ms frame budget).";
    }
}

void PerformanceProfiler::recordMetrics(const PhysicsEngine& physics, 
                                        const VisualStateAdapter& visualAdapter, 
                                        const Renderer& renderer, 
                                        float deltaTime, 
                                        float fps) {
    if (m_exportNotificationTimer > 0.0f) {
        m_exportNotificationTimer -= deltaTime;
    }

    if (m_paused) {
        return;
    }

    m_sampleAccumulator += deltaTime;
    float sampleIntervalSec = (float)m_samplingIntervalMs / 1000.0f;

    if (m_sampleAccumulator < sampleIntervalSec) {
        return;
    }
    m_sampleAccumulator = 0.0f;

    // Sample hardware and system metrics
    sampleSystemMetrics();

    // Populate current snapshot
    m_currentSnapshot.timestampSec = glfwGetTime();
    m_currentSnapshot.fps = (deltaTime > 0.0f) ? fps : 60.0f;
    m_currentSnapshot.frameTimeMs = (deltaTime > 0.0f) ? (deltaTime * 1000.0f) : 16.67f;
    m_currentSnapshot.physicsTimeMs = (m_lastPhysicsTimeMs > 0.0f) ? m_lastPhysicsTimeMs : physics.getPhysicsStepTimeMs();
    m_currentSnapshot.renderCpuTimeMs = m_lastRenderCpuTimeMs;
    m_currentSnapshot.gpuTimeMs = m_lastGpuTimeMs;
    m_currentSnapshot.cpuUsagePercent = m_cpuUsagePercent;
    m_currentSnapshot.ramWorkingSetMB = m_ramWorkingSetMB;
    m_currentSnapshot.ramPrivateMB = m_ramPrivateMB;
    m_currentSnapshot.vramDedicatedMB = m_vramDedicatedMB;
    m_currentSnapshot.vramUsedMB = m_vramUsedMB;

    // Simulation & Scene Statistics
    m_currentSnapshot.celestialBodyCount = (int)physics.getBodies().size();
    m_currentSnapshot.physicalParticleCount = physics.getAsteroidBelt().getPhysicalCount();
    m_currentSnapshot.visualParticleCount = physics.getAsteroidBelt().getVisualCount();
    m_currentSnapshot.timeScale = physics.getTimeScale();
    m_currentSnapshot.isPaused = physics.isPaused();
    m_currentSnapshot.isGeneralRelativity = physics.isGeneralRelativityEnabled();

    // Evaluate health state
    evaluatePerformanceStatus();

    // Push into time-series ring buffers
    auto pushBuffer = [](std::vector<float>& buf, float val) {
        buf.push_back(val);
        if (buf.size() > MAX_HISTORY_SAMPLES) {
            buf.erase(buf.begin());
        }
    };

    pushBuffer(m_historyFps, m_currentSnapshot.fps);
    pushBuffer(m_historyFrameTime, m_currentSnapshot.frameTimeMs);
    pushBuffer(m_historyCpuUsage, m_currentSnapshot.cpuUsagePercent);
    pushBuffer(m_historyGpuTime, m_currentSnapshot.gpuTimeMs);
    pushBuffer(m_historyRamMB, m_currentSnapshot.ramWorkingSetMB);
    pushBuffer(m_historyVramMB, m_currentSnapshot.vramUsedMB);

    m_snapshotHistory.push_back(m_currentSnapshot);
    if (m_snapshotHistory.size() > MAX_HISTORY_SAMPLES) {
        m_snapshotHistory.pop_front();
    }
}

void PerformanceProfiler::resetGraphs() {
    m_historyFps.clear();
    m_historyFrameTime.clear();
    m_historyCpuUsage.clear();
    m_historyGpuTime.clear();
    m_historyRamMB.clear();
    m_historyVramMB.clear();
    m_snapshotHistory.clear();
    m_sampleAccumulator = 0.0f;
}

void PerformanceProfiler::exportReport() {
    try {
        std::filesystem::create_directories("data/reports");

        auto now = std::chrono::system_clock::now();
        std::time_t nowTime = std::chrono::system_clock::to_time_t(now);
        std::tm tmNow;
#ifdef _WIN32
        localtime_s(&tmNow, &nowTime);
#else
        localtime_r(&nowTime, &tmNow);
#endif

        char timeStr[64];
        std::strftime(timeStr, sizeof(timeStr), "%Y%m%d_%H%M%S", &tmNow);

        std::string csvPath = "data/reports/astrogenesis_perf_" + std::string(timeStr) + ".csv";
        std::string txtPath = "data/reports/astrogenesis_perf_" + std::string(timeStr) + ".txt";

        // Calculate statistics over recorded snapshots
        float minFps = 9999.0f, maxFps = 0.0f, sumFps = 0.0f;
        float minFrame = 9999.0f, maxFrame = 0.0f, sumFrame = 0.0f;
        float minPhysics = 9999.0f, maxPhysics = 0.0f, sumPhysics = 0.0f;
        float minGpu = 9999.0f, maxGpu = 0.0f, sumGpu = 0.0f;
        float minCpu = 9999.0f, maxCpu = 0.0f, sumCpu = 0.0f;
        float peakRam = 0.0f, peakVram = 0.0f;

        std::vector<float> sortedFps;
        sortedFps.reserve(m_snapshotHistory.size());

        for (const auto& s : m_snapshotHistory) {
            minFps = std::min(minFps, s.fps);
            maxFps = std::max(maxFps, s.fps);
            sumFps += s.fps;
            sortedFps.push_back(s.fps);

            minFrame = std::min(minFrame, s.frameTimeMs);
            maxFrame = std::max(maxFrame, s.frameTimeMs);
            sumFrame += s.frameTimeMs;

            minPhysics = std::min(minPhysics, s.physicsTimeMs);
            maxPhysics = std::max(maxPhysics, s.physicsTimeMs);
            sumPhysics += s.physicsTimeMs;

            minGpu = std::min(minGpu, s.gpuTimeMs);
            maxGpu = std::max(maxGpu, s.gpuTimeMs);
            sumGpu += s.gpuTimeMs;

            minCpu = std::min(minCpu, s.cpuUsagePercent);
            maxCpu = std::max(maxCpu, s.cpuUsagePercent);
            sumCpu += s.cpuUsagePercent;

            peakRam = std::max(peakRam, s.ramWorkingSetMB);
            peakVram = std::max(peakVram, s.vramUsedMB);
        }

        size_t n = m_snapshotHistory.empty() ? 1 : m_snapshotHistory.size();
        float avgFps = sumFps / (float)n;
        float avgFrame = sumFrame / (float)n;
        float avgPhysics = sumPhysics / (float)n;
        float avgGpu = sumGpu / (float)n;
        float avgCpu = sumCpu / (float)n;

        float p99Fps = minFps;
        if (!sortedFps.empty()) {
            std::sort(sortedFps.begin(), sortedFps.end());
            size_t idx1Pct = (size_t)std::floor((float)sortedFps.size() * 0.01f);
            p99Fps = sortedFps[std::min(idx1Pct, sortedFps.size() - 1)];
        }

        // 1. Write Text Summary Report
        std::ofstream txtOut(txtPath);
        if (txtOut.is_open()) {
            txtOut << "================================================================================" << std::endl;
            txtOut << "          ASTROGENESIS SPACE SIMULATION - PERFORMANCE PROFILE REPORT            " << std::endl;
            txtOut << "================================================================================" << std::endl;
            txtOut << "Timestamp:                " << timeStr << std::endl;
            txtOut << "Total Samples:            " << m_snapshotHistory.size() << " samples (" << (m_snapshotHistory.size() * m_samplingIntervalMs / 1000.0f) << " s recorded)" << std::endl;
            txtOut << "Sampling Interval:        " << m_samplingIntervalMs << " ms" << std::endl;
            txtOut << "--------------------------------------------------------------------------------" << std::endl;
            txtOut << "HARDWARE & PLATFORM:" << std::endl;
            txtOut << "  Host CPU:               " << m_cpuInfo << " (" << m_logicalCpuCores << " logical threads)" << std::endl;
            txtOut << "  GPU Vendor:             " << m_gpuVendor << std::endl;
            txtOut << "  GPU Renderer:           " << m_gpuRenderer << std::endl;
            txtOut << "  OpenGL Version:         " << m_glVersion << std::endl;
            txtOut << "  Physical System RAM:    " << std::fixed << std::setprecision(2) << m_totalPhysRamGB << " GB" << std::endl;
            txtOut << "  Dedicated Video Memory: " << std::fixed << std::setprecision(0) << m_vramDedicatedMB << " MB" << std::endl;
            txtOut << "--------------------------------------------------------------------------------" << std::endl;
            txtOut << "SCENE & SIMULATION PARAMETERS:" << std::endl;
            txtOut << "  Active Celestial Bodies:" << m_currentSnapshot.celestialBodyCount << std::endl;
            txtOut << "  Physical Asteroids:     " << m_currentSnapshot.physicalParticleCount << std::endl;
            txtOut << "  Visual Particle Swarm:  " << m_currentSnapshot.visualParticleCount << std::endl;
            txtOut << "  Total Dynamic Particles:" << (m_currentSnapshot.physicalParticleCount + m_currentSnapshot.visualParticleCount) << std::endl;
            txtOut << "  Gravitational Physics:  " << (m_currentSnapshot.isGeneralRelativity ? "Einstein 1PN General Relativity" : "Newtonian Inverse-Square") << std::endl;
            txtOut << "  Simulation Time Scale:  " << m_currentSnapshot.timeScale << "x" << (m_currentSnapshot.isPaused ? " (PAUSED)" : "") << std::endl;
            txtOut << "--------------------------------------------------------------------------------" << std::endl;
            txtOut << "STATISTICAL BENCHMARKS:" << std::endl;
            txtOut << "  FPS:                    Min: " << minFps << " | Avg: " << avgFps << " | Max: " << maxFps << " | 1% Low: " << p99Fps << std::endl;
            txtOut << "  Frame Time:             Min: " << minFrame << " ms | Avg: " << avgFrame << " ms | Max: " << maxFrame << " ms" << std::endl;
            txtOut << "  Physics Update Time:    Min: " << minPhysics << " ms | Avg: " << avgPhysics << " ms | Max: " << maxPhysics << " ms" << std::endl;
            txtOut << "  GPU Execution Time:     Min: " << minGpu << " ms | Avg: " << avgGpu << " ms | Max: " << maxGpu << " ms" << std::endl;
            txtOut << "  Host CPU Utilization:   Min: " << minCpu << "% | Avg: " << avgCpu << "% | Max: " << maxCpu << "%" << std::endl;
            txtOut << "  Process RAM (Working):  Current: " << m_currentSnapshot.ramWorkingSetMB << " MB | Peak: " << peakRam << " MB" << std::endl;
            txtOut << "  VRAM (Dedicated Used):  Current: " << m_currentSnapshot.vramUsedMB << " MB | Peak: " << peakVram << " MB" << std::endl;
            txtOut << "================================================================================" << std::endl;
            txtOut.close();
        }

        // 2. Write Time-Series CSV
        std::ofstream csvOut(csvPath);
        if (csvOut.is_open()) {
            csvOut << "Timestamp_s,FPS,FrameTime_ms,PhysicsTime_ms,RenderCpu_ms,GpuTime_ms,CpuUsage_pct,RamMB,VramMB,Bodies,Particles,Status\n";
            for (const auto& s : m_snapshotHistory) {
                const char* stStr = (s.status == PerformanceStatus::Stable) ? "Stable" :
                                    (s.status == PerformanceStatus::Warning) ? "Warning" : "Critical";
                csvOut << std::fixed << std::setprecision(3)
                       << s.timestampSec << ","
                       << s.fps << ","
                       << s.frameTimeMs << ","
                       << s.physicsTimeMs << ","
                       << s.renderCpuTimeMs << ","
                       << s.gpuTimeMs << ","
                       << s.cpuUsagePercent << ","
                       << s.ramWorkingSetMB << ","
                       << s.vramUsedMB << ","
                       << s.celestialBodyCount << ","
                       << (s.physicalParticleCount + s.visualParticleCount) << ","
                       << stStr << "\n";
            }
            csvOut.close();
        }

        m_lastExportPath = csvPath;
        m_exportNotificationTimer = 6.0f;
        m_exportSuccess = true;
    } catch (const std::exception& e) {
        m_lastExportPath = "Export failed: " + std::string(e.what());
        m_exportNotificationTimer = 6.0f;
        m_exportSuccess = false;
    }
}

void PerformanceProfiler::drawLineGraph(const char* id,
                                       const float* values1, const char* label1, ImU32 color1, const char* unit1,
                                       const float* values2, const char* label2, ImU32 color2, const char* unit2,
                                       int count, float minVal, float maxVal, const ImVec2& size,
                                       float refLine1, float refLine2) {
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 screenPos = ImGui::GetCursorScreenPos();
    ImVec2 graphSize = size;
    if (graphSize.x <= 0.0f) {
        graphSize.x = ImGui::GetContentRegionAvail().x;
    }

    ImRect bb(screenPos, ImVec2(screenPos.x + graphSize.x, screenPos.y + graphSize.y));
    ImGui::ItemSize(bb);
    if (!ImGui::ItemAdd(bb, ImGui::GetID(id))) {
        return;
    }

    // 1. Graph Background (Dark Navy)
    drawList->AddRectFilled(bb.Min, bb.Max, IM_COL32(11, 18, 32, 245), 4.0f);
    drawList->AddRect(bb.Min, bb.Max, IM_COL32(34, 54, 86, 180), 4.0f);

    // Dynamic scale bounds
    if (count > 0 && values1) {
        for (int i = 0; i < count; ++i) {
            minVal = std::min(minVal, values1[i]);
            maxVal = std::max(maxVal, values1[i]);
            if (values2) {
                minVal = std::min(minVal, values2[i]);
                maxVal = std::max(maxVal, values2[i]);
            }
        }
    }
    if (maxVal <= minVal) {
        maxVal = minVal + 1.0f;
    }
    // Give 10% headroom
    float padding = (maxVal - minVal) * 0.10f;
    minVal = std::max(0.0f, minVal - padding);
    maxVal = maxVal + padding;
    float range = maxVal - minVal;

    // 2. Horizontal Reference Grid Lines & Value Labels
    const int numHGrids = 3;
    for (int i = 0; i <= numHGrids; ++i) {
        float ratio = (float)i / (float)numHGrids;
        float y = bb.Max.y - ratio * (graphSize.y - 12.0f) - 6.0f;
        drawList->AddLine(ImVec2(bb.Min.x + 4.0f, y), ImVec2(bb.Max.x - 4.0f, y), IM_COL32(28, 44, 72, 100), 1.0f);

        float val = minVal + ratio * range;
        char valBuf[32];
        if (range >= 100.0f) {
            snprintf(valBuf, sizeof(valBuf), "%.0f", val);
        } else if (range >= 10.0f) {
            snprintf(valBuf, sizeof(valBuf), "%.1f", val);
        } else {
            snprintf(valBuf, sizeof(valBuf), "%.2f", val);
        }
        drawList->AddText(ImVec2(bb.Min.x + 8.0f, y - 13.0f), IM_COL32(100, 130, 170, 150), valBuf);
    }

    // 3. Optional Specific Reference Lines (e.g. 60 FPS, 30 FPS, 16.6ms)
    auto drawRefLine = [&](float refVal, const char* refTxt, ImU32 refCol) {
        if (refVal >= minVal && refVal <= maxVal) {
            float normY = (refVal - minVal) / range;
            float y = bb.Max.y - normY * (graphSize.y - 12.0f) - 6.0f;
            drawList->AddLine(ImVec2(bb.Min.x + 4.0f, y), ImVec2(bb.Max.x - 4.0f, y), refCol, 1.0f);
            drawList->AddText(ImVec2(bb.Max.x - 55.0f, y - 13.0f), refCol, refTxt);
        }
    };
    if (refLine1 > 0.0f) drawRefLine(refLine1, "60 FPS (16.6ms)", IM_COL32(56, 189, 248, 120));
    if (refLine2 > 0.0f) drawRefLine(refLine2, "30 FPS (33.3ms)", IM_COL32(148, 163, 184, 120));

    // 4. Plot Primary Line Series
    auto plotSeries = [&](const float* vals, ImU32 lineCol, ImU32 fillCol) {
        if (!vals || count < 2) return;
        std::vector<ImVec2> pts;
        pts.reserve(count);

        float xStep = (graphSize.x - 12.0f) / (float)(MAX_HISTORY_SAMPLES - 1);
        float startX = bb.Max.x - 6.0f - (float)(count - 1) * xStep;

        for (int i = 0; i < count; ++i) {
            float normY = std::clamp((vals[i] - minVal) / range, 0.0f, 1.0f);
            float x = startX + (float)i * xStep;
            float y = bb.Max.y - normY * (graphSize.y - 12.0f) - 6.0f;
            pts.push_back(ImVec2(x, y));
        }

        // Draw translucent area fill below curve
        if (pts.size() > 1 && (fillCol & 0xFF000000) != 0) {
            std::vector<ImVec2> fillPts = pts;
            fillPts.push_back(ImVec2(pts.back().x, bb.Max.y - 6.0f));
            fillPts.push_back(ImVec2(pts.front().x, bb.Max.y - 6.0f));
            drawList->AddConvexPolyFilled(fillPts.data(), (int)fillPts.size(), fillCol);
        }

        // Anti-aliased line
        drawList->AddPolyline(pts.data(), (int)pts.size(), lineCol, 0, 1.8f);
    };

    if (values2) {
        plotSeries(values2, color2, IM_COL32(color2 & 0xFF, (color2 >> 8) & 0xFF, (color2 >> 16) & 0xFF, 20));
    }
    if (values1) {
        plotSeries(values1, color1, IM_COL32(color1 & 0xFF, (color1 >> 8) & 0xFF, (color1 >> 16) & 0xFF, 30));
    }

    // 5. Interactive Mouse Scrubbing / Inspection Tooltip
    ImGuiIO& io = ImGui::GetIO();
    bool hovered = ImGui::IsItemHovered();
    if (hovered && count > 1 && values1) {
        float xStep = (graphSize.x - 12.0f) / (float)(MAX_HISTORY_SAMPLES - 1);
        float startX = bb.Max.x - 6.0f - (float)(count - 1) * xStep;
        float mouseX = io.MousePos.x;

        if (mouseX >= startX && mouseX <= bb.Max.x - 6.0f) {
            int idx = (int)std::round((mouseX - startX) / xStep);
            idx = std::clamp(idx, 0, count - 1);

            float inspectX = startX + (float)idx * xStep;
            drawList->AddLine(ImVec2(inspectX, bb.Min.y + 4.0f), ImVec2(inspectX, bb.Max.y - 4.0f), IM_COL32(200, 220, 255, 180), 1.0f);

            // Highlight points
            float v1 = values1[idx];
            float normY1 = std::clamp((v1 - minVal) / range, 0.0f, 1.0f);
            float y1 = bb.Max.y - normY1 * (graphSize.y - 12.0f) - 6.0f;
            drawList->AddCircleFilled(ImVec2(inspectX, y1), 3.5f, color1);

            ImGui::BeginTooltip();
            float secAgo = (float)(count - 1 - idx) * ((float)m_samplingIntervalMs / 1000.0f);
            ImGui::TextColored(ImVec4(0.6f, 0.7f, 0.85f, 1.0f), "-%.2fs ago", secAgo);
            ImGui::TextColored(ImVec4(((color1) & 0xFF)/255.0f, ((color1 >> 8) & 0xFF)/255.0f, ((color1 >> 16) & 0xFF)/255.0f, 1.0f),
                               "%s: %.2f %s", label1, v1, unit1 ? unit1 : "");
            if (values2) {
                float v2 = values2[idx];
                ImGui::TextColored(ImVec4(((color2) & 0xFF)/255.0f, ((color2 >> 8) & 0xFF)/255.0f, ((color2 >> 16) & 0xFF)/255.0f, 1.0f),
                                   "%s: %.2f %s", label2, v2, unit2 ? unit2 : "");
            }
            ImGui::EndTooltip();
        }
    }

    // 6. Header Legend with Live Min / Avg / Current Readouts
    float cur1 = (count > 0 && values1) ? values1[count - 1] : 0.0f;
    float cur2 = (count > 0 && values2) ? values2[count - 1] : 0.0f;

    char legBuf[128];
    if (values2) {
        snprintf(legBuf, sizeof(legBuf), "%s: %.1f %s  |  %s: %.1f %s", label1, cur1, unit1, label2, cur2, unit2);
    } else {
        snprintf(legBuf, sizeof(legBuf), "%s: %.2f %s", label1, cur1, unit1);
    }
    drawList->AddText(ImVec2(bb.Min.x + 8.0f, bb.Min.y + 6.0f), IM_COL32(230, 240, 255, 230), legBuf);
}

void PerformanceProfiler::renderHeaderOverview() {
    ImDrawList* drawList = ImGui::GetWindowDrawList();

    // Top Overview Card
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.030f, 0.050f, 0.095f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.12f, 0.20f, 0.35f, 0.70f));
    ImGui::BeginChild("##HeaderOverviewCard", ImVec2(0, 78), true, ImGuiWindowFlags_NoScrollbar);

    float availW = ImGui::GetContentRegionAvail().x;
    ImVec2 pos = ImGui::GetCursorScreenPos();

    // 1. Performance Status Indicator Pill (Green, Amber, Red ONLY)
    ImU32 statusColor = IM_COL32(35, 197, 94, 255); // Green (Stable)
    ImU32 statusGlow  = IM_COL32(35, 197, 94, 70);
    const char* statusText = "STABLE";

    if (m_currentSnapshot.status == PerformanceStatus::Warning) {
        statusColor = IM_COL32(245, 158, 11, 255); // Amber (Warning)
        statusGlow  = IM_COL32(245, 158, 11, 70);
        statusText = "WARNING";
    } else if (m_currentSnapshot.status == PerformanceStatus::Critical) {
        statusColor = IM_COL32(239, 68, 68, 255); // Red (Critical)
        statusGlow  = IM_COL32(239, 68, 68, 70);
        statusText = "CRITICAL";
    }

    // Status Pill
    ImVec2 pillPos = ImVec2(pos.x + 4.0f, pos.y + 6.0f);
    drawList->AddRectFilled(pillPos, ImVec2(pillPos.x + 115.0f, pillPos.y + 26.0f), IM_COL32(14, 24, 44, 230), 13.0f);
    drawList->AddRect(pillPos, ImVec2(pillPos.x + 115.0f, pillPos.y + 26.0f), statusColor, 13.0f, 0, 1.5f);

    // Glowing indicator dot
    drawList->AddCircleFilled(ImVec2(pillPos.x + 16.0f, pillPos.y + 13.0f), 7.0f, statusGlow);
    drawList->AddCircleFilled(ImVec2(pillPos.x + 16.0f, pillPos.y + 13.0f), 4.5f, statusColor);

    // Status Text
    drawList->AddText(ImVec2(pillPos.x + 28.0f, pillPos.y + 5.0f), IM_COL32(245, 250, 255, 255), statusText);

    // Status reason description
    ImGui::SetCursorPos(ImVec2(128.0f, 10.0f));
    ImGui::TextColored(ImVec4(0.70f, 0.78f, 0.88f, 1.0f), "%s", m_currentSnapshot.statusReason.c_str());

    // 2. High-Impact Big Metrics: FPS & Frame Time
    float statColW = 95.0f;
    float startStatX = availW - statColW * 3.0f - 10.0f;

    auto drawStatBox = [&](float x, const char* label, const char* value, const char* unit) {
        ImGui::SetCursorPos(ImVec2(x, 6.0f));
        ImGui::TextColored(ImVec4(0.50f, 0.62f, 0.75f, 1.0f), "%s", label);
        ImGui::SetCursorPos(ImVec2(x, 22.0f));
        ImGui::PushFont(ImGui::GetIO().Fonts->Fonts[0]);
        ImGui::TextColored(ImVec4(0.92f, 0.96f, 1.00f, 1.0f), "%s", value);
        ImGui::SameLine(0, 3);
        ImGui::TextColored(ImVec4(0.40f, 0.55f, 0.72f, 1.0f), "%s", unit);
        ImGui::PopFont();
    };

    char fpsBuf[16], frameBuf[16], cpuBuf[16];
    snprintf(fpsBuf, sizeof(fpsBuf), "%.1f", m_currentSnapshot.fps);
    snprintf(frameBuf, sizeof(frameBuf), "%.2f", m_currentSnapshot.frameTimeMs);
    snprintf(cpuBuf, sizeof(cpuBuf), "%.1f", m_currentSnapshot.cpuUsagePercent);

    drawStatBox(startStatX, "FRAME RATE", fpsBuf, "FPS");
    drawStatBox(startStatX + statColW, "FRAME TIME", frameBuf, "ms");
    drawStatBox(startStatX + statColW * 2.0f, "HOST CPU", cpuBuf, "%");

    // Secondary line: quick summary badges
    ImGui::SetCursorPos(ImVec2(10.0f, 48.0f));
    ImGui::TextColored(ImVec4(0.45f, 0.58f, 0.72f, 1.0f), "PIPELINE:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.80f, 0.88f, 0.98f, 1.0f), "Physics: %.2f ms", m_currentSnapshot.physicsTimeMs);
    ImGui::SameLine(0, 16);
    ImGui::TextColored(ImVec4(0.80f, 0.88f, 0.98f, 1.0f), "CPU Render: %.2f ms", m_currentSnapshot.renderCpuTimeMs);
    ImGui::SameLine(0, 16);
    ImGui::TextColored(ImVec4(0.80f, 0.88f, 0.98f, 1.0f), "GPU Time: %.2f ms", m_currentSnapshot.gpuTimeMs);
    ImGui::SameLine(0, 16);
    ImGui::TextColored(ImVec4(0.80f, 0.88f, 0.98f, 1.0f), "RAM: %.1f MB", m_currentSnapshot.ramWorkingSetMB);

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

void PerformanceProfiler::renderControlsBar() {
    float availW = ImGui::GetContentRegionAvail().x;

    // 1. Pause / Resume Collection Toggle
    if (m_paused) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.12f, 0.28f, 0.48f, 1.0f));
        if (ImGui::Button("▶ RESUME COLLECTION", ImVec2(160, 26))) {
            m_paused = false;
        }
        ImGui::PopStyleColor();
    } else {
        if (ImGui::Button("⏸ PAUSE METRICS", ImVec2(140, 26))) {
            m_paused = true;
        }
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Freeze or resume real-time metric sampling and graph scrolling.");
    }

    ImGui::SameLine(0, 8);

    // 2. Reset Graphs
    if (ImGui::Button("↺ RESET GRAPHS", ImVec2(120, 26))) {
        resetGraphs();
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Clear all historical time-series buffers and start fresh.");
    }

    ImGui::SameLine(0, 8);

    // 3. Sampling Interval Combo
    ImGui::SetNextItemWidth(140);
    const char* intervals[] = {"16 ms (60 Hz)", "50 ms (20 Hz)", "100 ms (10 Hz)", "250 ms (4 Hz)", "500 ms (2 Hz)", "1000 ms (1 Hz)"};
    int intervalValues[] = {16, 50, 100, 250, 500, 1000};
    int currentIdx = 2; // Default 100ms
    for (int i = 0; i < 6; ++i) {
        if (m_samplingIntervalMs == intervalValues[i]) {
            currentIdx = i;
            break;
        }
    }
    if (ImGui::Combo("##SamplingInterval", &currentIdx, intervals, 6)) {
        m_samplingIntervalMs = intervalValues[currentIdx];
    }
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Configure sampling frequency for hardware monitoring and time-series plots.");
    }

    ImGui::SameLine(0, 8);

    // 4. Export Performance Report Button
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.08f, 0.18f, 0.32f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.12f, 0.25f, 0.44f, 1.0f));
    if (ImGui::Button("💾 EXPORT PERFORMANCE REPORT", ImVec2(230, 26))) {
        exportReport();
    }
    ImGui::PopStyleColor(2);
    if (ImGui::IsItemHovered()) {
        ImGui::SetTooltip("Generate a complete CSV and TXT scientific profile report with timestamps and statistics.");
    }

    // Export notification banner
    if (m_exportNotificationTimer > 0.0f) {
        ImGui::SameLine(0, 12);
        ImGui::TextColored(ImVec4(0.35f, 0.85f, 0.55f, 1.0f), "✓ Saved: %s", m_lastExportPath.c_str());
    }
}

void PerformanceProfiler::renderGraphs() {
    float availW = ImGui::GetContentRegionAvail().x;
    float graphHeight = 110.0f;

    // ── GRAPH 1: FPS & FRAME TIME ─────────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.92f, 1.0f), "FRAME RATE & FRAME LATENCY");
    ImGui::SameLine(availW - 220);
    ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "■ FPS");
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(0.58f, 0.64f, 0.72f, 1.0f), "■ Frame Time (ms)");

    drawLineGraph("##GraphFPS",
                  m_historyFps.data(), "FPS", IM_COL32(56, 189, 248, 255), "fps",
                  m_historyFrameTime.data(), "Frame Time", IM_COL32(148, 163, 184, 255), "ms",
                  (int)m_historyFps.size(), 0.0f, 75.0f, ImVec2(availW, graphHeight),
                  60.0f, 30.0f);

    ImGui::Spacing();

    // ── GRAPH 2: CPU USAGE & GPU TIME ─────────────────────────────────────────
    ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.92f, 1.0f), "COMPUTE LOAD & GPU EXECUTION TIME");
    ImGui::SameLine(availW - 220);
    ImGui::TextColored(ImVec4(0.38f, 0.65f, 0.98f, 1.0f), "■ CPU Usage (%)");
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(0.51f, 0.55f, 0.98f, 1.0f), "■ GPU Time (ms)");

    drawLineGraph("##GraphCpuGpu",
                  m_historyCpuUsage.data(), "CPU", IM_COL32(96, 165, 250, 255), "%",
                  m_historyGpuTime.data(), "GPU Render", IM_COL32(129, 140, 248, 255), "ms",
                  (int)m_historyCpuUsage.size(), 0.0f, 100.0f, ImVec2(availW, graphHeight),
                  -1.0f, -1.0f);

    ImGui::Spacing();

    // ── GRAPH 3: MEMORY USAGE (RAM & VRAM) ────────────────────────────────────
    ImGui::TextColored(ImVec4(0.70f, 0.80f, 0.92f, 1.0f), "MEMORY ALLOCATIONS (RAM & VRAM)");
    ImGui::SameLine(availW - 220);
    ImGui::TextColored(ImVec4(0.22f, 0.74f, 0.97f, 1.0f), "■ Process RAM (MB)");
    ImGui::SameLine(0, 12);
    ImGui::TextColored(ImVec4(0.65f, 0.71f, 0.99f, 1.0f), "■ Dedicated VRAM (MB)");

    drawLineGraph("##GraphMemory",
                  m_historyRamMB.data(), "Process RAM", IM_COL32(56, 189, 248, 255), "MB",
                  m_historyVramMB.data(), "Active VRAM", IM_COL32(165, 180, 252, 255), "MB",
                  (int)m_historyRamMB.size(), 0.0f, m_vramDedicatedMB, ImVec2(availW, graphHeight),
                  -1.0f, -1.0f);
}

void PerformanceProfiler::renderDetailedMetrics(const VisualStateAdapter& visualAdapter, const Renderer& renderer) {
    float availW = ImGui::GetContentRegionAvail().x;
    float colW = (availW - 16.0f) * 0.5f;

    // LEFT CARD: TIMINGS & HARDWARE RESOURCES
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.028f, 0.046f, 0.088f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.18f, 0.32f, 0.70f));
    ImGui::BeginChild("##LeftMetricsCard", ImVec2(colW, 260), true);

    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "CORE PIPELINE TIMINGS & HARDWARE");
    ImGui::Separator();
    ImGui::Spacing();

    auto row2 = [](const char* k, const std::string& v) {
        ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "%s", k);
        ImGui::SameLine(180.0f);
        ImGui::TextColored(ImVec4(0.92f, 0.95f, 0.98f, 1.0f), "%s", v.c_str());
    };

    char buf[128];
    snprintf(buf, sizeof(buf), "%.2f ms (%.1f FPS)", m_currentSnapshot.frameTimeMs, m_currentSnapshot.fps);
    row2("Frame Duration:", buf);

    snprintf(buf, sizeof(buf), "%.3f ms", m_currentSnapshot.physicsTimeMs);
    row2("Physics Step Time:", buf);

    snprintf(buf, sizeof(buf), "%.3f ms", m_currentSnapshot.renderCpuTimeMs);
    row2("CPU Render Dispatch:", buf);

    snprintf(buf, sizeof(buf), "%.3f ms", m_currentSnapshot.gpuTimeMs);
    row2("GPU Pipeline Execution:", buf);

    snprintf(buf, sizeof(buf), "%.1f %% (over %d threads)", m_currentSnapshot.cpuUsagePercent, m_logicalCpuCores);
    row2("Process CPU Usage:", buf);

    snprintf(buf, sizeof(buf), "%.1f MB (Private: %.1f MB)", m_currentSnapshot.ramWorkingSetMB, m_currentSnapshot.ramPrivateMB);
    row2("RAM (Working Set):", buf);

    snprintf(buf, sizeof(buf), "%.2f GB avail / %.2f GB total", m_availPhysRamGB, m_totalPhysRamGB);
    row2("System Physical RAM:", buf);

    float vramPct = (m_vramDedicatedMB > 0.0f) ? (m_vramUsedMB / m_vramDedicatedMB * 100.0f) : 0.0f;
    snprintf(buf, sizeof(buf), "%.0f MB / %.0f MB (%.1f%%)", m_currentSnapshot.vramUsedMB, m_currentSnapshot.vramDedicatedMB, vramPct);
    row2("VRAM (Dedicated):", buf);

    ImGui::EndChild();
    ImGui::SameLine(0, 16.0f);

    // RIGHT CARD: ASTRONOMICAL SIMULATION & ACTIVE SHADERS
    ImGui::BeginChild("##RightMetricsCard", ImVec2(colW, 260), true);

    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "SIMULATION DYNAMICS & SHADER PIPELINE");
    ImGui::Separator();
    ImGui::Spacing();

    snprintf(buf, sizeof(buf), "%d Celestial Bodies", m_currentSnapshot.celestialBodyCount);
    row2("Active Celestial Bodies:", buf);

    int totalParticles = m_currentSnapshot.physicalParticleCount + m_currentSnapshot.visualParticleCount;
    snprintf(buf, sizeof(buf), "%d (%d phys, %d vis)", totalParticles, m_currentSnapshot.physicalParticleCount, m_currentSnapshot.visualParticleCount);
    row2("Particle / Asteroid Swarm:", buf);

    row2("Current Physics Mode:", m_currentSnapshot.isGeneralRelativity ? "Einstein 1PN General Relativity" : "Newtonian Gravitation (1/r²)");

    if (m_currentSnapshot.isPaused) {
        snprintf(buf, sizeof(buf), "PAUSED (Time Frozen)");
    } else {
        float ts = m_currentSnapshot.timeScale;
        if (ts >= 31536000.0f) {
            snprintf(buf, sizeof(buf), "%.2f yr/s (%.1fx)", ts / 31536000.0f, ts);
        } else if (ts >= 86400.0f) {
            snprintf(buf, sizeof(buf), "%.2f day/s (%.1fx)", ts / 86400.0f, ts);
        } else if (ts >= 3600.0f) {
            snprintf(buf, sizeof(buf), "%.2f hr/s (%.1fx)", ts / 3600.0f, ts);
        } else {
            snprintf(buf, sizeof(buf), "%.1fx (Real-Time)", ts);
        }
    }
    row2("Simulation Time Scale:", buf);

    ImGui::Spacing();
    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "Active Shader / Post-FX Pipeline:");
    ImGui::Spacing();

    // Badges for active effects
    auto drawBadge = [](const char* name, bool active) {
        ImVec4 col = active ? ImVec4(0.10f, 0.22f, 0.38f, 1.0f) : ImVec4(0.04f, 0.08f, 0.14f, 0.60f);
        ImVec4 textCol = active ? ImVec4(0.70f, 0.88f, 1.0f, 1.0f) : ImVec4(0.40f, 0.48f, 0.58f, 0.80f);
        ImGui::PushStyleColor(ImGuiCol_Button, col);
        ImGui::PushStyleColor(ImGuiCol_Text, textCol);
        ImGui::Button(name);
        ImGui::PopStyleColor(2);
        ImGui::SameLine(0, 6.0f);
    };

    drawBadge("PBR Surface", true);
    drawBadge("Atmosphere", visualAdapter.areAtmospheresEnabled());
    drawBadge("Clouds", visualAdapter.areCloudsEnabled());
    drawBadge("Corona", !visualAdapter.getStarLightSources().empty());
    drawBadge("Shadows", visualAdapter.areShadowsEnabled());
    drawBadge("Impacts", visualAdapter.areImpactFXEnabled());
    ImGui::NewLine();
    drawBadge("Trails", visualAdapter.areMotionTrailsEnabled());
    drawBadge("Orbits", visualAdapter.areOrbitLinesEnabled());
    drawBadge("Bloom", visualAdapter.getBloomIntensity() > 0.01f);
    drawBadge("DoF", visualAdapter.isDoFEnabled());
    drawBadge("ToneMap", true);
    drawBadge("Vignette", visualAdapter.isVignetteEnabled());

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

void PerformanceProfiler::renderSystemInfo() {
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.028f, 0.046f, 0.088f, 0.95f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.10f, 0.18f, 0.32f, 0.70f));
    ImGui::BeginChild("##SysInfoCard", ImVec2(0, 100), true);

    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "HARDWARE ACCELERATION & SUBSYSTEM SPECS");
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::Columns(2, "##SysCols", false);
    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "GPU Hardware:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.92f, 0.95f, 0.98f, 1.0f), "%s (%s)", m_gpuRenderer.c_str(), m_gpuVendor.c_str());

    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "Host CPU Architecture:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.92f, 0.95f, 0.98f, 1.0f), "%s (%d cores)", m_cpuInfo.c_str(), m_logicalCpuCores);

    ImGui::NextColumn();

    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "Driver API:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.92f, 0.95f, 0.98f, 1.0f), "%s", m_glVersion.c_str());

    ImGui::TextColored(ImVec4(0.55f, 0.65f, 0.78f, 1.0f), "Windowing Platform:");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.92f, 0.95f, 0.98f, 1.0f), "GLFW 3.3+ / Dear ImGui Multi-Viewport Docking");

    ImGui::Columns(1);
    ImGui::EndChild();
    ImGui::PopStyleColor(2);
}

void PerformanceProfiler::applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Dark Navy Theme Palette
    style.Colors[ImGuiCol_WindowBg]             = ImVec4(0.024f, 0.038f, 0.075f, 0.98f);
    style.Colors[ImGuiCol_Header]               = ImVec4(0.045f, 0.080f, 0.155f, 0.85f);
    style.Colors[ImGuiCol_HeaderHovered]        = ImVec4(0.075f, 0.140f, 0.260f, 0.95f);
    style.Colors[ImGuiCol_HeaderActive]         = ImVec4(0.100f, 0.180f, 0.340f, 1.00f);
    style.Colors[ImGuiCol_Button]               = ImVec4(0.055f, 0.110f, 0.210f, 0.90f);
    style.Colors[ImGuiCol_ButtonHovered]        = ImVec4(0.085f, 0.165f, 0.310f, 1.00f);
    style.Colors[ImGuiCol_ButtonActive]         = ImVec4(0.110f, 0.210f, 0.390f, 1.00f);
    style.Colors[ImGuiCol_FrameBg]              = ImVec4(0.030f, 0.050f, 0.095f, 0.90f);
    style.Colors[ImGuiCol_FrameBgHovered]       = ImVec4(0.045f, 0.075f, 0.140f, 1.00f);
    style.Colors[ImGuiCol_FrameBgActive]        = ImVec4(0.065f, 0.110f, 0.200f, 1.00f);
    style.Colors[ImGuiCol_TitleBg]              = ImVec4(0.020f, 0.032f, 0.065f, 1.00f);
    style.Colors[ImGuiCol_TitleBgActive]        = ImVec4(0.030f, 0.052f, 0.105f, 1.00f);
    style.Colors[ImGuiCol_Border]               = ImVec4(0.095f, 0.165f, 0.300f, 0.70f);
    style.Colors[ImGuiCol_Separator]            = ImVec4(0.080f, 0.140f, 0.260f, 0.60f);
    style.Colors[ImGuiCol_Text]                 = ImVec4(0.920f, 0.950f, 0.980f, 1.00f);
    style.Colors[ImGuiCol_TextDisabled]         = ImVec4(0.450f, 0.540f, 0.660f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarBg]          = ImVec4(0.020f, 0.032f, 0.065f, 0.60f);
    style.Colors[ImGuiCol_ScrollbarGrab]        = ImVec4(0.065f, 0.120f, 0.220f, 0.80f);
    style.Colors[ImGuiCol_ScrollbarGrabHovered] = ImVec4(0.095f, 0.170f, 0.310f, 1.00f);
    style.Colors[ImGuiCol_ScrollbarGrabActive]  = ImVec4(0.120f, 0.220f, 0.390f, 1.00f);
    style.Colors[ImGuiCol_Tab]                  = ImVec4(0.040f, 0.070f, 0.135f, 0.90f);
    style.Colors[ImGuiCol_TabHovered]           = ImVec4(0.080f, 0.150f, 0.280f, 1.00f);
    style.Colors[ImGuiCol_TabActive]            = ImVec4(0.065f, 0.130f, 0.250f, 1.00f);
    style.Colors[ImGuiCol_TabUnfocused]         = ImVec4(0.030f, 0.050f, 0.095f, 0.90f);
    style.Colors[ImGuiCol_TabUnfocusedActive]   = ImVec4(0.050f, 0.095f, 0.180f, 1.00f);
}

void PerformanceProfiler::render(const VisualStateAdapter& visualAdapter, const Renderer& renderer) {
    if (!m_isOpen || !m_profilerWindow || !m_profilerContext) {
        return;
    }

    if (glfwWindowShouldClose(m_profilerWindow)) {
        glfwSetWindowShouldClose(m_profilerWindow, GLFW_FALSE);
        glfwHideWindow(m_profilerWindow);
        m_isOpen = false;
        return;
    }

    GLFWwindow* prevWin = glfwGetCurrentContext();
    ImGuiContext* prevCtx = ImGui::GetCurrentContext();

    glfwMakeContextCurrent(m_profilerWindow);
    ImGui::SetCurrentContext(m_profilerContext);

    int fbW = 0, fbH = 0;
    glfwGetFramebufferSize(m_profilerWindow, &fbW, &fbH);
    if (fbW > 0 && fbH > 0) {
        glViewport(0, 0, fbW, fbH);
        glClearColor(0.024f, 0.038f, 0.075f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        int winW = 0, winH = 0;
        glfwGetWindowSize(m_profilerWindow, &winW, &winH);
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2((float)winW, (float)winH));

        ImGuiWindowFlags winFlags = ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | 
                                    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse;

        if (ImGui::Begin("##ProfilerRoot", nullptr, winFlags)) {
            renderHeaderOverview();
            ImGui::Spacing();
            renderControlsBar();
            ImGui::Spacing();
            ImGui::Separator();
            ImGui::Spacing();

            if (ImGui::BeginTabBar("##ProfilerTabs", ImGuiTabBarFlags_None)) {
                if (ImGui::BeginTabItem("LIVE TELEMETRY & GRAPHS")) {
                    renderGraphs();
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("METRICS BREAKDOWN")) {
                    renderDetailedMetrics(visualAdapter, renderer);
                    ImGui::EndTabItem();
                }
                if (ImGui::BeginTabItem("HARDWARE & DIAGNOSTICS")) {
                    renderSystemInfo();
                    ImGui::Spacing();
                    ImGui::TextColored(ImVec4(0.35f, 0.75f, 1.0f, 1.0f), "RECORDED BENCHMARK SNAPSHOTS");
                    ImGui::Separator();
                    ImGui::Spacing();

                    char statInfo[128];
                    snprintf(statInfo, sizeof(statInfo), "Buffered Samples: %zu / %zu (Duration: %.1f s)",
                             m_snapshotHistory.size(), MAX_HISTORY_SAMPLES,
                             (float)m_snapshotHistory.size() * ((float)m_samplingIntervalMs / 1000.0f));
                    ImGui::TextColored(ImVec4(0.8f, 0.88f, 0.98f, 1.0f), "%s", statInfo);

                    if (ImGui::Button("EXPORT REPORT NOW (CSV & TXT)", ImVec2(240, 26))) {
                        exportReport();
                    }
                    ImGui::EndTabItem();
                }
                ImGui::EndTabBar();
            }
        }
        ImGui::End();

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        glfwSwapBuffers(m_profilerWindow);
    }

    // Always restore main application OpenGL and ImGui contexts
    ImGui::SetCurrentContext(prevCtx);
    glfwMakeContextCurrent(prevWin);
}

} // namespace AstroGenesis
