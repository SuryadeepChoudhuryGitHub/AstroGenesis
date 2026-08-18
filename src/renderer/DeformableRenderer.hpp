#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include <memory>
#include "renderer/Camera.hpp"
#include "simulation/DeformableBody.hpp"
#include "simulation/MatterSystem.hpp"

namespace AstroGenesis {

struct DeformVertex {
    glm::vec3 pos;          // Camera-relative position in AU (or local meters scale)
    glm::vec3 normal;       // Normal vector
    glm::vec4 color;        // Base material color
    float scalarValue;      // Field visualization scalar [0, 1]
};

class DeformableRenderer {
public:
    DeformableRenderer();
    ~DeformableRenderer();

    bool initialize();
    void shutdown();

    void render(
        const Camera& camera,
        float aspect,
        const std::vector<std::shared_ptr<DeformableBody>>& bodies,
        MatterVisualizationMode visMode,
        const glm::vec3& sunPos,
        const glm::vec3& cameraTarget,
        bool drawWireframe = true
    );

private:
    GLuint m_program = 0;
    GLint m_uVPLoc = -1;
    GLint m_uLightPosLoc = -1;
    GLint m_uVisModeLoc = -1;
    GLint m_uBaseColorLoc = -1;
    GLint m_uMetallicLoc = -1;
    GLint m_uRoughnessLoc = -1;

    // Line program for constraints & fractures
    GLuint m_lineProgram = 0;
    GLint m_uLineVPLoc = -1;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_lineVAO = 0;
    GLuint m_lineVBO = 0;

    size_t m_vboCapacity = 0;
    size_t m_lineCapacity = 0;
};

} // namespace AstroGenesis
