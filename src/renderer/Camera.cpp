#include "renderer/Camera.hpp"
#include <algorithm>
#include <cmath>

namespace AstroGenesis {

Camera::Camera() {
    m_desiredTarget = glm::vec3(0.0f);
    m_currentTarget = glm::vec3(0.0f);
}

void Camera::setTargetPosition(const glm::vec3& target, bool immediate) {
    m_desiredTarget = target;
    if (immediate) {
        m_currentTarget = target;
    }
}

void Camera::update(float deltaTime) {
    // Smooth interpolation towards desired target position
    float lerpSpeed = 10.0f;
    m_currentTarget = glm::mix(m_currentTarget, m_desiredTarget, std::min(1.0f, deltaTime * lerpSpeed));
}

void Camera::processMouseOrbit(float deltaX, float deltaY) {
    float sensitivity = 0.005f;
    m_yaw -= deltaX * sensitivity;
    m_pitch += deltaY * sensitivity;

    // Clamp pitch to avoid flipping over pole
    const float maxPitch = 1.55f; // ~89 degrees
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);
}

void Camera::setTargetBodyRadius(float radius3D) {
    m_targetRadius = radius3D;
    // Set default initial viewing distance proportional to target body size
    m_distance = std::max(0.0000001f, radius3D * 3.5f);
}

void Camera::processMouseZoom(float deltaZoom) {
    if (deltaZoom == 0.0f) return;

    // Exponential/proportional zooming for smooth scaling around celestial object
    float zoomFactor = std::pow(0.88f, deltaZoom);
    m_distance *= zoomFactor;

    // Adaptive clamping based on target radius (min 1.05x body radius, max 200 AU)
    float minDist = std::max(0.00000001f, m_targetRadius * 1.05f);
    float maxDist = 200.0f;
    m_distance = std::clamp(m_distance, minDist, maxDist);
}

void Camera::resetCenter() {
    m_yaw = 0.0f;
    m_pitch = 0.3f;
    m_distance = std::max(0.0000001f, m_targetRadius * 3.5f);
}

glm::vec3 Camera::getEyePosition() const {
    // Returns camera eye position relative to origin (0,0,0) — NOT world space.
    // The focused body is always at origin in view space.
    float cosPitch = std::cos(m_pitch);
    float sinPitch = std::sin(m_pitch);
    float cosYaw   = std::cos(m_yaw);
    float sinYaw   = std::sin(m_yaw);

    glm::vec3 offset;
    offset.x = m_distance * cosPitch * sinYaw;
    offset.y = m_distance * sinPitch;
    offset.z = m_distance * cosPitch * cosYaw;

    return offset; // Camera-relative: no world offset added
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 eye = getEyePosition();
    // Camera always looks at origin (0,0,0) — the focused body is placed there
    return glm::lookAt(eye, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    // Dynamic near/far clip planes scaled to camera distance for maximum depth precision
    float nearPlane = std::max(0.000000001f, m_distance * 0.0001f);
    float farPlane  = std::max(200.0f, m_distance * 100.0f);
    return glm::perspective(glm::radians(m_fov), std::max(aspectRatio, 0.1f), nearPlane, farPlane);
}

} // namespace AstroGenesis
