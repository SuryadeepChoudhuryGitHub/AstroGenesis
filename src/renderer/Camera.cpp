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

void Camera::processMousePan(float deltaX, float deltaY) {
    float cosPitch = std::cos(m_pitch);
    float sinPitch = std::sin(m_pitch);
    float cosYaw   = std::cos(m_yaw);
    float sinYaw   = std::sin(m_yaw);

    glm::vec3 forward(-cosPitch * sinYaw, -sinPitch, -cosPitch * cosYaw);
    glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
    glm::vec3 right = glm::normalize(glm::cross(forward, worldUp));
    glm::vec3 up    = glm::normalize(glm::cross(right, forward));

    float panSpeed = m_distance * 0.0015f;
    glm::vec3 panOffset = (-right * deltaX + up * deltaY) * panSpeed;

    m_desiredTarget += panOffset;
    m_currentTarget += panOffset;
    m_startPos += panOffset;
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
    m_roll = 0.0f;
    m_distance = std::max(0.0000001f, m_targetRadius * 3.5f);
}

void Camera::resetOverview(const glm::vec3& targetPos, float distance) {
    m_desiredTarget = targetPos;
    m_currentTarget = targetPos;
    m_startPos = targetPos;
    m_yaw = 0.0f;
    m_pitch = 0.45f;
    m_roll = 0.0f;
    m_distance = distance;
    m_targetDistance = distance;
    m_isTransitioning = false;
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
    // Camera looks at origin (0,0,0) — the focused body is placed there
    glm::vec3 up(0.0f, 1.0f, 0.0f);
    if (std::abs(m_roll) > 0.0001f) {
        glm::vec3 forward = glm::normalize(glm::vec3(0.0f) - eye);
        glm::vec3 defaultUp(0.0f, 1.0f, 0.0f);
        if (std::abs(glm::dot(forward, defaultUp)) > 0.999f) {
            defaultUp = glm::vec3(0.0f, 0.0f, 1.0f);
        }
        glm::vec3 right = glm::normalize(glm::cross(forward, defaultUp));
        glm::vec3 baseUp = glm::normalize(glm::cross(right, forward));
        glm::mat4 rollRot = glm::rotate(glm::mat4(1.0f), m_roll, forward);
        up = glm::normalize(glm::vec3(rollRot * glm::vec4(baseUp, 0.0f)));
    }
    return glm::lookAt(eye, glm::vec3(0.0f), up);
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

void Camera::screenToWorldRay(float screenX, float screenY, float vpX, float vpY, float vpW, float vpH,
                              glm::vec3& outRayOrigin, glm::vec3& outRayDir) const {
    if (vpW <= 0.0f || vpH <= 0.0f) {
        outRayOrigin = m_currentTarget;
        outRayDir = glm::vec3(0.0f, 0.0f, -1.0f);
        return;
    }

    float aspect = vpW / vpH;
    glm::mat4 proj = getProjectionMatrix(aspect);
    glm::mat4 view = getViewMatrix();
    glm::mat4 invVP = glm::inverse(proj * view);

    float ndcX = ((screenX - vpX) / vpW) * 2.0f - 1.0f;
    float ndcY = 1.0f - ((screenY - vpY) / vpH) * 2.0f;

    glm::vec4 pNear = invVP * glm::vec4(ndcX, ndcY, -1.0f, 1.0f);
    glm::vec4 pFar  = invVP * glm::vec4(ndcX, ndcY,  1.0f, 1.0f);

    if (std::abs(pNear.w) > 1e-7f) pNear /= pNear.w;
    if (std::abs(pFar.w) > 1e-7f)  pFar  /= pFar.w;

    glm::vec3 relNear = glm::vec3(pNear);
    glm::vec3 relFar  = glm::vec3(pFar);

    outRayOrigin = m_currentTarget + relNear;
    outRayDir = glm::normalize(relFar - relNear);
}

bool Camera::intersectPlane(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                            const glm::vec3& planePoint, const glm::vec3& planeNormal,
                            glm::vec3& outIntersection) const {
    float denom = glm::dot(planeNormal, rayDir);
    if (std::abs(denom) < 1e-5f) return false;

    float t = glm::dot(planePoint - rayOrigin, planeNormal) / denom;
    if (t < 0.0f) return false;

    outIntersection = rayOrigin + t * rayDir;
    return true;
}

int Camera::pickBody(const glm::vec3& rayOrigin, const glm::vec3& rayDir,
                     const std::vector<glm::vec3>& positions,
                     const std::vector<float>& radii3D,
                     float vpH, float pixelTolerance) const {
    if (positions.empty() || positions.size() != radii3D.size() || vpH <= 0.0f) {
        return -1;
    }

    int bestIdx = -1;
    float bestDepth = 1e12f;
    float tanHalfFov = std::tan(glm::radians(m_fov * 0.5f));

    for (size_t i = 0; i < positions.size(); ++i) {
        glm::vec3 toCenter = positions[i] - rayOrigin;
        float tClose = glm::dot(toCenter, rayDir);
        if (tClose <= 0.001f) continue; // Behind camera ray

        glm::vec3 pClose = rayOrigin + tClose * rayDir;
        float perpDist = glm::length(positions[i] - pClose);

        float physRadius = std::max(0.0001f, radii3D[i]);
        // Minimum pixel hitbox translated to world units at distance tClose
        float minWorldRadius = (pixelTolerance / (vpH * 0.5f)) * tClose * tanHalfFov;
        float effectiveRadius = std::max(physRadius, minWorldRadius);

        if (perpDist <= effectiveRadius) {
            float depth = tClose;
            // If physical hit on the sphere, find the entry point
            if (perpDist <= physRadius) {
                depth = tClose - std::sqrt(physRadius * physRadius - perpDist * perpDist);
            }
            if (depth < bestDepth) {
                bestDepth = depth;
                bestIdx = (int)i;
            }
        }
    }

    return bestIdx;
}

} // namespace AstroGenesis
