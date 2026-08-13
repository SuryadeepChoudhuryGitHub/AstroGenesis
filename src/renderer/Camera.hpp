#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace AstroGenesis {

class Camera {
public:
    Camera();

    void update(float deltaTime);

    // Inputs
    void processMouseOrbit(float deltaX, float deltaY);
    void processMouseZoom(float deltaZoom);
    void resetCenter();

    void setTargetPosition(const glm::vec3& target, bool immediate = false);
    void setTargetBodyRadius(float radius3D);
    void setDistance(float dist) { m_distance = dist; }

    // Matrix getters
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Attribute getters
    glm::vec3 getEyePosition() const;
    glm::vec3 getTargetPosition() const { return m_currentTarget; }
    float getDistance() const { return m_distance; }

private:
    glm::vec3 m_desiredTarget{0.0f};
    glm::vec3 m_currentTarget{0.0f};

    float m_yaw = 0.0f;       // In radians
    float m_pitch = 0.3f;     // In radians
    float m_distance = 3.5f;

    float m_fov = 45.0f;       // In degrees
    float m_nearPlane = 0.0000001f;
    float m_farPlane = 500.0f;
    float m_targetRadius = 0.000042587f;
};

} // namespace AstroGenesis
