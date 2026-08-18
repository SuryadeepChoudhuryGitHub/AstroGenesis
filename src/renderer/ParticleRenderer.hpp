#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>
#include "renderer/Camera.hpp"
#include "simulation/ParticleSystem.hpp"

namespace AstroGenesis {

class ParticleRenderer {
public:
    ParticleRenderer();
    ~ParticleRenderer();

    bool initialize();
    void shutdown();

    void render(const Camera& camera, float aspect,
                const std::vector<ParticleInstanceData>& instances,
                const glm::vec3& sunPos, const glm::vec3& cameraTarget);

private:
    void createParticleMesh();
    void ensureInstanceBufferCapacity(size_t count);

    GLuint m_program = 0;
    GLint m_uVPLoc = -1;
    GLint m_uLightPosLoc = -1;

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    GLuint m_instanceVBO = 0;
    int m_indexCount = 0;
    size_t m_instanceBufferCapacity = 0;
};

} // namespace AstroGenesis
