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
    void resetOverview(const glm::vec3& targetPos = glm::vec3(0.0f), float distance = 6.0f);

    void setTargetPosition(const glm::vec3& target, bool immediate = false);
    void setTargetBodyRadius(float radius3D);
    void focusOnBody(const glm::vec3& targetPos, float targetRadius3D, float durationSeconds = 0.85f);
    void setDistance(float dist) { m_distance = dist; }

    bool isTransitioning() const { return m_isTransitioning; }

    // Matrix getters
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;

    // Screen projection
    bool projectToScreen(const glm::vec3& worldPos, const glm::vec3& cameraTarget,
                         float vpX, float vpY, float vpW, float vpH,
                         glm::vec2& outScreenPos, float& outScreenRadius, float bodyRadius3D = 1.0f) const;

    float getFOV() const { return m_fov; }
    void setFOV(float fovDeg) { m_fov = glm::clamp(fovDeg, 10.0f, 120.0f); }

    // Roll Controls (for Photo Mode and Cinematic Camera)
    float getRoll() const { return m_roll; }
    void setRoll(float rollRad) { m_roll = rollRad; }
    void roll(float deltaRad) { m_roll += deltaRad; }
    void resetRoll() { m_roll = 0.0f; }

    // Exposure & Depth of Field Controls
    float getExposure() const { return m_exposure; }
    void setExposure(float exp) { m_exposure = glm::clamp(exp, 0.05f, 20.0f); }

    float getFocusDistance() const { return m_focusDistance; }
    void setFocusDistance(float dist) { m_focusDistance = std::max(0.0001f, dist); }

    bool isDoFEnabled() const { return m_enableDoF; }
    void setDoFEnabled(bool enable) { m_enableDoF = enable; }

    float getDoFAperture() const { return m_dofAperture; }
    void setDoFAperture(float ap) { m_dofAperture = glm::clamp(ap, 0.001f, 0.2f); }

    // Attribute getters
    glm::vec3 getEyePosition() const;
    glm::vec3 getTargetPosition() const { return m_currentTarget; }
    float getDistance() const { return m_distance; }
    float getNearPlane() const { return m_nearPlane; }
    float getFarPlane() const { return m_farPlane; }

private:
    glm::vec3 m_desiredTarget{0.0f};
    glm::vec3 m_currentTarget{0.0f};

    // Smooth logarithmic travel transition
    glm::vec3 m_startPos{0.0f};
    float m_startDistance = 3.5f;
    float m_targetDistance = 3.5f;
    float m_transitionTimer = 0.0f;
    float m_transitionDuration = 0.85f;
    bool m_isTransitioning = false;

    float m_yaw = 0.0f;       // In radians
    float m_pitch = 0.3f;     // In radians
    float m_roll = 0.0f;      // In radians
    float m_distance = 3.5f;

    float m_fov = 45.0f;       // In degrees
    float m_nearPlane = 0.0000001f;
    float m_farPlane = 500.0f;
    float m_targetRadius = 0.000042587f;

    // Cinematic & Photographic Parameters
    float m_exposure = 1.0f;
    float m_focusDistance = 3.5f;
    bool m_enableDoF = false;
    float m_dofAperture = 0.035f;
};

} // namespace AstroGenesis
