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
    m_yaw += deltaX * sensitivity;
    m_pitch += deltaY * sensitivity;

    // Clamp pitch to avoid flipping over pole
    const float maxPitch = 1.55f; // ~89 degrees
    m_pitch = std::clamp(m_pitch, -maxPitch, maxPitch);
}

void Camera::processMouseZoom(float deltaZoom) {
    if (deltaZoom == 0.0f) return;

    // Exponential/proportional zooming for smooth scaling around celestial object
    float zoomFactor = std::pow(0.88f, deltaZoom);
    m_distance *= zoomFactor;

    // Clamp camera distance (min 0.2f, max 500.0f)
    m_distance = std::clamp(m_distance, 0.2f, 500.0f);
}

void Camera::resetCenter() {
    m_yaw = 0.0f;
    m_pitch = 0.3f;
    m_distance = 3.5f;
}

glm::vec3 Camera::getEyePosition() const {
    float cosPitch = std::cos(m_pitch);
    float sinPitch = std::sin(m_pitch);
    float cosYaw   = std::cos(m_yaw);
    float sinYaw   = std::sin(m_yaw);

    glm::vec3 offset;
    offset.x = m_distance * cosPitch * sinYaw;
    offset.y = m_distance * sinPitch;
    offset.z = m_distance * cosPitch * cosYaw;

    // Camera eye is positioned around exact celestial object center
    return m_currentTarget + offset;
}

glm::mat4 Camera::getViewMatrix() const {
    glm::vec3 eye = getEyePosition();
    // Look directly at the center of the celestial object
    return glm::lookAt(eye, m_currentTarget, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(m_fov), std::max(aspectRatio, 0.1f), m_nearPlane, m_farPlane);
}

} // namespace AstroGenesis
