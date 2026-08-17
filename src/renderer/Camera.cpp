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
        m_startPos = target;
        m_isTransitioning = false;
    }
}

void Camera::setTargetBodyRadius(float radius3D) {
    m_targetRadius = radius3D;
    if (!m_isTransitioning) {
        m_distance = std::max(0.0000001f, radius3D * 3.5f);
    }
}

void Camera::focusOnBody(const glm::vec3& targetPos, float targetRadius3D, float durationSeconds) {
    if (!m_isTransitioning && glm::distance(m_currentTarget, targetPos) < 0.000001f) {
        return; // Already focused on this object
    }
    m_startPos = m_currentTarget;
    m_startDistance = m_distance;
    m_targetRadius = targetRadius3D;
    m_targetDistance = std::max(0.0000001f, targetRadius3D * 3.5f);
    m_desiredTarget = targetPos;
    m_transitionDuration = std::max(0.05f, durationSeconds);
    m_transitionTimer = 0.0f;
    m_isTransitioning = true;
}

void Camera::update(float deltaTime) {
    if (m_isTransitioning) {
        m_transitionTimer += deltaTime;
        float rawT = m_transitionTimer / m_transitionDuration;

        if (rawT >= 1.0f) {
            // Flight completed: seamlessly lock to moving target at final distance
            m_isTransitioning = false;
            m_currentTarget = m_desiredTarget;
            m_distance = m_targetDistance;
        } else {
            // Logarithmic deceleration easing: E(t) = ln(1 + k*t) / ln(1 + k) with k = 9.0
            const float k = 9.0f;
            float ease = std::log(1.0f + k * rawT) / std::log(1.0f + k);

            // Interpolate position from starting point to the moving destination planet
            m_currentTarget = glm::mix(m_startPos, m_desiredTarget, ease);

            // Logarithmic (exponential) distance zoom interpolation
            float logStartDist = std::log(std::max(0.00000001f, m_startDistance));
            float logTargetDist = std::log(std::max(0.00000001f, m_targetDistance));
            m_distance = std::exp(glm::mix(logStartDist, logTargetDist, ease));
        }
    } else {
        // Locked onto active body
        m_currentTarget = m_desiredTarget;
    }
}

void Camera::processMouseOrbit(float deltaX, float deltaY) {
    float sensitivity = 0.005f;
    m_yaw -= deltaX * sensitivity;
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

bool Camera::projectToScreen(const glm::vec3& worldPos, const glm::vec3& cameraTarget,
                             float vpX, float vpY, float vpW, float vpH,
                             glm::vec2& outScreenPos, float& outScreenRadius, float bodyRadius3D) const {
    if (vpH <= 0.0f || vpW <= 0.0f) return false;

    float aspect = vpW / vpH;
    glm::mat4 proj = getProjectionMatrix(aspect);
    glm::mat4 view = getViewMatrix();

    // Camera-relative position
    glm::vec3 relPos = worldPos - cameraTarget;
    glm::vec4 clip = proj * view * glm::vec4(relPos, 1.0f);

    // Behind near plane
    if (clip.w <= 0.000001f) {
        return false;
    }

    glm::vec3 ndc = glm::vec3(clip) / clip.w;

    // Out of depth range
    if (ndc.z < -1.0f || ndc.z > 1.0f) {
        return false;
    }

    // Convert NDC [-1, 1] to ImGui screen coordinates (top-left is (0,0))
    float sx = vpX + (ndc.x * 0.5f + 0.5f) * vpW;
    float sy = vpY + (1.0f - (ndc.y * 0.5f + 0.5f)) * vpH;
    outScreenPos = glm::vec2(sx, sy);

    // Calculate approximate screen radius in pixels based on body scale and distance
    float tanHalfFov = std::tan(glm::radians(m_fov * 0.5f));
    float distToCam = clip.w;
    outScreenRadius = (bodyRadius3D / distToCam) * (vpH * 0.5f / tanHalfFov);

    return true;
}

} // namespace AstroGenesis
