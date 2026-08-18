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

# ── Main Executable ────────────────────────────────────────────────────────────
set(ASTROGENESIS_SOURCES
    src/main.cpp
    src/core/Application.cpp
    src/simulation/PhysicsEngine.cpp
    src/simulation/ParticleSystem.cpp
    src/simulation/MaterialModel.cpp
    src/simulation/DeformableBody.cpp
    src/simulation/MatterSystem.cpp
    src/renderer/Camera.cpp
    src/renderer/Renderer.cpp
    src/renderer/ParticleRenderer.cpp
    src/renderer/DeformableRenderer.cpp
    src/ui/UIManager.cpp
)

add_executable(AstroGenesis ${ASTROGENESIS_SOURCES})

target_include_directories(AstroGenesis PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/external
)

target_link_libraries(AstroGenesis PRIVATE imgui glfw glad glm::glm opengl32)

# ── Assets Sync ────────────────────────────────────────────────────────────────
add_custom_command(TARGET AstroGenesis POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:AstroGenesis>/assets
)

