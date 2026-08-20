cmake_minimum_required(VERSION 3.16)
project(AstroGenesis LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ── GLFW ────────────────────────────────────────────────────────────────────────
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS     OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL        OFF CACHE BOOL "" FORCE)
add_subdirectory(external/glfw)

# ── GLM (header-only) ──────────────────────────────────────────────────────────
add_subdirectory(external/glm)

# ── GLAD ────────────────────────────────────────────────────────────────────────
add_library(glad STATIC external/glad/src/gl.c)
target_include_directories(glad PUBLIC external/glad/include)

# ── SQLite3 ────────────────────────────────────────────────────────────────────
add_library(sqlite3 STATIC external/sqlite/sqlite3.c)
target_include_directories(sqlite3 PUBLIC external/sqlite)

# ── Dear ImGui ──────────────────────────────────────────────────────────────────
set(IMGUI_DIR ${CMAKE_SOURCE_DIR}/external/imgui)
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/imgui_demo.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)
target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)
target_link_libraries(imgui PUBLIC glfw glad)

# ── Core Engine Library ────────────────────────────────────────────────────────
set(ASTROGENESIS_CORE_SOURCES
    src/net/HttpClient.cpp
    src/data/UnitConverter.cpp
    src/data/DatabaseManager.cpp
    src/data/SeedData.cpp
    src/data/DataManager.cpp
    src/data/repositories/ObjectRepository.cpp
    src/data/repositories/EphemerisRepository.cpp
    src/data/repositories/ValidationRepository.cpp
    src/data/providers/JPLHorizonsProvider.cpp
    src/data/providers/JPLSBDBProvider.cpp
    src/data/providers/NASAExoplanetProvider.cpp
    src/simulation/PhysicsEngine.cpp
    src/simulation/ParticleSystem.cpp
    src/simulation/MaterialModel.cpp
    src/simulation/DeformableBody.cpp
    src/simulation/MatterSystem.cpp
    src/simulation/ValidationEngine.cpp
    src/renderer/Camera.cpp
    src/renderer/Renderer.cpp
    src/renderer/ParticleRenderer.cpp
    src/renderer/DeformableRenderer.cpp
    src/ui/UIManager.cpp
    src/ui/DataManagerUI.cpp
    src/ui/ValidationUI.cpp
)

# ── Platform-Specific Dependencies ─────────────────────────────────────────────
if (WIN32)
    set(PLATFORM_LIBS opengl32 winhttp)
elseif (APPLE)
    find_package(OpenGL REQUIRED)
    set(PLATFORM_LIBS
        OpenGL::GL
        "-framework Cocoa"
        "-framework IOKit"
        "-framework CoreVideo"
        "-framework CoreFoundation"
    )
else()
    find_package(OpenGL REQUIRED)
    set(PLATFORM_LIBS OpenGL::GL pthread dl)
endif()

# ── Main Executable ────────────────────────────────────────────────────────────
add_executable(AstroGenesis 
    src/main.cpp
    src/core/Application.cpp
    ${ASTROGENESIS_CORE_SOURCES}
)

target_include_directories(AstroGenesis PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/sqlite
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)

target_link_libraries(AstroGenesis PRIVATE 
    imgui 
    glfw 
    glad 
    glm::glm 
    sqlite3
    ${PLATFORM_LIBS}
)

# ── Automated Test Suite Executable ───────────────────────────────────────────
add_executable(test_data_system
    src/test_data_system.cpp
    ${ASTROGENESIS_CORE_SOURCES}
)

target_include_directories(test_data_system PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
    ${CMAKE_SOURCE_DIR}/external/sqlite
    ${CMAKE_SOURCE_DIR}/external/nlohmann
)

target_link_libraries(test_data_system PRIVATE 
    imgui 
    glfw 
    glad 
    glm::glm 
    sqlite3
    ${PLATFORM_LIBS}
)

# ── Assets Sync ────────────────────────────────────────────────────────────────
add_custom_command(TARGET AstroGenesis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:AstroGenesis>/assets
)
