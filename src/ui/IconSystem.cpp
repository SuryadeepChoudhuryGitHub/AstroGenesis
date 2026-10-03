#include "ui/IconSystem.hpp"
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

static inline ImVec2 RotatePoint(const ImVec2& p, const ImVec2& center, float cosA, float sinA) {
    float dx = p.x - center.x;
    float dy = p.y - center.y;
    return ImVec2(center.x + dx * cosA - dy * sinA, center.y + dx * sinA + dy * cosA);
}

static inline void DrawEllipseArc(ImDrawList* dl, ImVec2 center, float rx, float ry, float angleRad, float a0, float a1, int segments, ImU32 color, float thickness) {
    float cosA = std::cos(angleRad);
    float sinA = std::sin(angleRad);
    for (int i = 0; i < segments; ++i) {
        float t0 = a0 + (a1 - a0) * ((float)i / (float)segments);
        float t1 = a0 + (a1 - a0) * ((float)(i + 1) / (float)segments);
        ImVec2 p0 = RotatePoint(ImVec2(center.x + rx * std::cos(t0), center.y + ry * std::sin(t0)), center, cosA, sinA);
        ImVec2 p1 = RotatePoint(ImVec2(center.x + rx * std::cos(t1), center.y + ry * std::sin(t1)), center, cosA, sinA);
        dl->AddLine(p0, p1, color, thickness);
    }
}

namespace UIIcon {

void Draw(ImDrawList* dl, IconId icon, ImVec2 c, float size, ImU32 color, float thickness) {
    if (icon == IconId::None) return;
    if (color == 0) color = ImGui::GetColorU32(ImGuiCol_Text);

    float r = size * 0.5f;
    float th = (thickness > 0.0f) ? thickness : std::max(1.35f, size * 0.095f);

    switch (icon) {
        case IconId::Logo:
        case IconId::LogoCompact: {
            float rx = r * 0.95f;
            float ry = r * 0.40f;
            float tilt = -0.48f; // ~-27.5 deg
            DrawEllipseArc(dl, c, rx, ry, tilt, 0.0f, 6.28318f, 28, color, th);

            // Orbiting satellite/exoplanet node
            float cosA = std::cos(tilt), sinA = std::sin(tilt);
            float nodeAngle = 0.785f;
            ImVec2 nodePos = RotatePoint(ImVec2(c.x + rx * std::cos(nodeAngle), c.y + ry * std::sin(nodeAngle)), c, cosA, sinA);
            dl->AddCircleFilled(nodePos, std::max(1.8f, r * 0.22f), color, 16);

            // Central luminous star core
            dl->AddCircleFilled(c, std::max(2.2f, r * 0.35f), color, 16);

            if (icon == IconId::Logo) {
                // Subtle corona ring + reticle ticks
                dl->AddCircle(c, r * 0.52f, color, 16, th * 0.7f);
                float tickL = r * 0.18f;
                dl->AddLine(ImVec2(c.x - r, c.y), ImVec2(c.x - r + tickL, c.y), color, th);
                dl->AddLine(ImVec2(c.x + r - tickL, c.y), ImVec2(c.x + r, c.y), color, th);
                dl->AddLine(ImVec2(c.x, c.y - r), ImVec2(c.x, c.y - r + tickL), color, th);
                dl->AddLine(ImVec2(c.x, c.y + r - tickL), ImVec2(c.x, c.y + r), color, th);
            }
            break;
        }

        case IconId::Select: {
            // Sleek arrow cursor
            ImVec2 p0(c.x - r * 0.65f, c.y - r * 0.85f);
            ImVec2 p1(c.x - r * 0.65f, c.y + r * 0.65f);
            ImVec2 p2(c.x - r * 0.15f, c.y + r * 0.25f);
            ImVec2 p3(c.x + r * 0.35f, c.y + r * 0.85f);
            ImVec2 p4(c.x + r * 0.65f, c.y + r * 0.65f);
            ImVec2 p5(c.x + r * 0.15f, c.y + r * 0.05f);
            ImVec2 p6(c.x + r * 0.65f, c.y - r * 0.05f);
            dl->AddLine(p0, p1, color, th);
            dl->AddLine(p1, p2, color, th);
            dl->AddLine(p2, p3, color, th);
            dl->AddLine(p3, p4, color, th);
            dl->AddLine(p4, p5, color, th);
            dl->AddLine(p5, p6, color, th);
            dl->AddLine(p6, p0, color, th);
            break;
        }

        case IconId::Move: {
            // 4-Way translation crosshair
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y), ImVec2(c.x + r * 0.85f, c.y), color, th);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x, c.y + r * 0.85f), color, th);
            float aL = r * 0.32f;
            // Left arrow
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y), ImVec2(c.x - r * 0.85f + aL, c.y - aL), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y), ImVec2(c.x - r * 0.85f + aL, c.y + aL), color, th);
            // Right arrow
            dl->AddLine(ImVec2(c.x + r * 0.85f, c.y), ImVec2(c.x + r * 0.85f - aL, c.y - aL), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.85f, c.y), ImVec2(c.x + r * 0.85f - aL, c.y + aL), color, th);
            // Up arrow
            dl->AddLine(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x - aL, c.y - r * 0.85f + aL), color, th);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.85f), ImVec2(c.x + aL, c.y - r * 0.85f + aL), color, th);
            // Down arrow
            dl->AddLine(ImVec2(c.x, c.y + r * 0.85f), ImVec2(c.x - aL, c.y + r * 0.85f - aL), color, th);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.85f), ImVec2(c.x + aL, c.y + r * 0.85f - aL), color, th);
            break;
        }

        case IconId::Add: {
            float d = r * 0.65f;
            dl->AddLine(ImVec2(c.x - d, c.y), ImVec2(c.x + d, c.y), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x, c.y - d), ImVec2(c.x, c.y + d), color, th * 1.25f);
            break;
        }

        case IconId::Delete: {
            // Trash bin
            dl->AddLine(ImVec2(c.x - r * 0.70f, c.y - r * 0.45f), ImVec2(c.x + r * 0.70f, c.y - r * 0.45f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r * 0.65f), ImVec2(c.x + r * 0.25f, c.y - r * 0.65f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r * 0.65f), ImVec2(c.x - r * 0.25f, c.y - r * 0.45f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.25f, c.y - r * 0.65f), ImVec2(c.x + r * 0.25f, c.y - r * 0.45f), color, th);

            ImVec2 b0(c.x - r * 0.55f, c.y - r * 0.45f);
            ImVec2 b1(c.x - r * 0.42f, c.y + r * 0.75f);
            ImVec2 b2(c.x + r * 0.42f, c.y + r * 0.75f);
            ImVec2 b3(c.x + r * 0.55f, c.y - r * 0.45f);
            dl->AddLine(b0, b1, color, th);
            dl->AddLine(b1, b2, color, th);
            dl->AddLine(b2, b3, color, th);
            // Inner ribs
            dl->AddLine(ImVec2(c.x - r * 0.18f, c.y - r * 0.25f), ImVec2(c.x - r * 0.14f, c.y + r * 0.55f), color, th * 0.85f);
            dl->AddLine(ImVec2(c.x + r * 0.18f, c.y - r * 0.25f), ImVec2(c.x + r * 0.14f, c.y + r * 0.55f), color, th * 0.85f);
            break;
        }

        case IconId::Edit: {
            // Stylized 45 deg technical pencil
            float cos45 = 0.7071f;
            ImVec2 tip(c.x - r * 0.65f, c.y + r * 0.65f);
            ImVec2 topR(c.x + r * 0.55f, c.y - r * 0.55f);
            float w = r * 0.24f;
            ImVec2 n(-cos45 * w, -cos45 * w);
            dl->AddLine(ImVec2(tip.x + n.y, tip.y - n.x), ImVec2(topR.x + n.y, topR.y - n.x), color, th);
            dl->AddLine(ImVec2(tip.x - n.y, tip.y + n.x), ImVec2(topR.x - n.y, topR.y + n.x), color, th);
            dl->AddLine(ImVec2(topR.x + n.y, topR.y - n.x), ImVec2(topR.x - n.y, topR.y + n.x), color, th);
            dl->AddLine(ImVec2(tip.x + n.y, tip.y - n.x), tip, color, th);
            dl->AddLine(ImVec2(tip.x - n.y, tip.y + n.x), tip, color, th);
            break;
        }

        case IconId::Clone: {
            // Two overlapping sheets
            float off = r * 0.25f;
            dl->AddRect(ImVec2(c.x - r * 0.75f + off, c.y - r * 0.75f + off), ImVec2(c.x + r * 0.75f, c.y + r * 0.75f), color, 1.5f, 0, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y - r * 0.75f + off), ImVec2(c.x - r * 0.75f, c.y - r * 0.75f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y - r * 0.75f), ImVec2(c.x + r * 0.75f - off, c.y - r * 0.75f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y + r * 0.75f - off), ImVec2(c.x - r * 0.75f + off, c.y + r * 0.75f - off), color, th);
            break;
        }

        case IconId::Focus:
        case IconId::Target: {
            // Reticle / target
            dl->AddCircle(c, r * 0.72f, color, 24, th);
            dl->AddCircleFilled(c, std::max(1.8f, r * 0.20f), color, 16);
            float t0 = r * 0.48f, t1 = r * 0.95f;
            dl->AddLine(ImVec2(c.x - t1, c.y), ImVec2(c.x - t0, c.y), color, th);
            dl->AddLine(ImVec2(c.x + t0, c.y), ImVec2(c.x + t1, c.y), color, th);
            dl->AddLine(ImVec2(c.x, c.y - t1), ImVec2(c.x, c.y - t0), color, th);
            dl->AddLine(ImVec2(c.x, c.y + t0), ImVec2(c.x, c.y + t1), color, th);
            break;
        }

        case IconId::Play: {
            ImVec2 p0(c.x - r * 0.45f, c.y - r * 0.65f);
            ImVec2 p1(c.x + r * 0.65f, c.y);
            ImVec2 p2(c.x - r * 0.45f, c.y + r * 0.65f);
            dl->AddTriangleFilled(p0, p1, p2, color);
            break;
        }

        case IconId::Pause: {
            float w = r * 0.28f, h = r * 1.20f;
            dl->AddRectFilled(ImVec2(c.x - r * 0.50f, c.y - h * 0.5f), ImVec2(c.x - r * 0.50f + w, c.y + h * 0.5f), color, 1.0f);
            dl->AddRectFilled(ImVec2(c.x + r * 0.50f - w, c.y - h * 0.5f), ImVec2(c.x + r * 0.50f, c.y + h * 0.5f), color, 1.0f);
            break;
        }

        case IconId::StepForward: {
            ImVec2 p0(c.x - r * 0.55f, c.y - r * 0.60f);
            ImVec2 p1(c.x + r * 0.25f, c.y);
            ImVec2 p2(c.x - r * 0.55f, c.y + r * 0.60f);
            dl->AddTriangleFilled(p0, p1, p2, color);
            dl->AddLine(ImVec2(c.x + r * 0.45f, c.y - r * 0.60f), ImVec2(c.x + r * 0.45f, c.y + r * 0.60f), color, th * 1.3f);
            break;
        }

        case IconId::StepBackward: {
            ImVec2 p0(c.x + r * 0.55f, c.y - r * 0.60f);
            ImVec2 p1(c.x - r * 0.25f, c.y);
            ImVec2 p2(c.x + r * 0.55f, c.y + r * 0.60f);
            dl->AddTriangleFilled(p0, p1, p2, color);
            dl->AddLine(ImVec2(c.x - r * 0.45f, c.y - r * 0.60f), ImVec2(c.x - r * 0.45f, c.y + r * 0.60f), color, th * 1.3f);
            break;
        }

        case IconId::Reset:
        case IconId::Refresh: {
            dl->PathArcTo(c, r * 0.68f, 0.7f, 5.6f, 20);
            dl->PathStroke(color, 0, th);
            float endAngle = 5.6f;
            ImVec2 endPt(c.x + r * 0.68f * std::cos(endAngle), c.y + r * 0.68f * std::sin(endAngle));
            float aL = r * 0.32f;
            dl->AddLine(endPt, ImVec2(endPt.x - aL * 0.9f, endPt.y - aL * 0.3f), color, th);
            dl->AddLine(endPt, ImVec2(endPt.x - aL * 0.2f, endPt.y + aL * 0.9f), color, th);
            break;
        }

        case IconId::Undo: {
            dl->PathArcTo(ImVec2(c.x + r * 0.1f, c.y + r * 0.1f), r * 0.62f, 3.2f, 5.8f, 16);
            dl->PathStroke(color, 0, th);
            ImVec2 tip(c.x - r * 0.65f, c.y - r * 0.15f);
            dl->AddLine(tip, ImVec2(tip.x + r * 0.35f, tip.y - r * 0.25f), color, th);
            dl->AddLine(tip, ImVec2(tip.x + r * 0.30f, tip.y + r * 0.30f), color, th);
            break;
        }

        case IconId::Redo: {
            dl->PathArcTo(ImVec2(c.x - r * 0.1f, c.y + r * 0.1f), r * 0.62f, 3.6f, 6.2f, 16);
            dl->PathStroke(color, 0, th);
            ImVec2 tip(c.x + r * 0.65f, c.y - r * 0.15f);
            dl->AddLine(tip, ImVec2(tip.x - r * 0.35f, tip.y - r * 0.25f), color, th);
            dl->AddLine(tip, ImVec2(tip.x - r * 0.30f, tip.y + r * 0.30f), color, th);
            break;
        }

        case IconId::Search: {
            ImVec2 cLens(c.x - r * 0.15f, c.y - r * 0.15f);
            float rLens = r * 0.52f;
            dl->AddCircle(cLens, rLens, color, 20, th);
            dl->AddLine(ImVec2(cLens.x + rLens * 0.707f, cLens.y + rLens * 0.707f),
                        ImVec2(c.x + r * 0.75f, c.y + r * 0.75f), color, th * 1.35f);
            break;
        }

        case IconId::Info: {
            dl->AddCircle(c, r * 0.85f, color, 24, th);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.40f), std::max(1.2f, th * 0.85f), color, 8);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.10f), ImVec2(c.x, c.y + r * 0.48f), color, th * 1.15f);
            break;
        }

        case IconId::Warning: {
            ImVec2 p0(c.x, c.y - r * 0.85f);
            ImVec2 p1(c.x - r * 0.85f, c.y + r * 0.75f);
            ImVec2 p2(c.x + r * 0.85f, c.y + r * 0.75f);
            dl->AddTriangle(p0, p1, p2, color, th);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.28f), ImVec2(c.x, c.y + r * 0.22f), color, th * 1.15f);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.48f), std::max(1.2f, th * 0.85f), color, 8);
            break;
        }

        case IconId::Error:
        case IconId::Close: {
            float d = r * 0.58f;
            dl->AddLine(ImVec2(c.x - d, c.y - d), ImVec2(c.x + d, c.y + d), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x - d, c.y + d), ImVec2(c.x + d, c.y - d), color, th * 1.25f);
            break;
        }

        case IconId::Success:
        case IconId::Check: {
            ImVec2 p0(c.x - r * 0.65f, c.y + r * 0.05f);
            ImVec2 p1(c.x - r * 0.15f, c.y + r * 0.55f);
            ImVec2 p2(c.x + r * 0.65f, c.y - r * 0.50f);
            dl->AddLine(p0, p1, color, th * 1.35f);
            dl->AddLine(p1, p2, color, th * 1.35f);
            break;
        }

        case IconId::CheckCircle: {
            dl->AddCircle(c, r * 0.85f, color, 24, th);
            ImVec2 p0(c.x - r * 0.45f, c.y + r * 0.05f);
            ImVec2 p1(c.x - r * 0.10f, c.y + r * 0.40f);
            ImVec2 p2(c.x + r * 0.48f, c.y - r * 0.35f);
            dl->AddLine(p0, p1, color, th * 1.3f);
            dl->AddLine(p1, p2, color, th * 1.3f);
            break;
        }

        case IconId::Database:
        case IconId::DataManager: {
            float rx = r * 0.80f, ry = r * 0.28f;
            // Top rim
            DrawEllipseArc(dl, ImVec2(c.x, c.y - r * 0.45f), rx, ry, 0.0f, 0.0f, 6.28318f, 20, color, th);
            // Mid arc
            DrawEllipseArc(dl, ImVec2(c.x, c.y), rx, ry, 0.0f, 0.0f, 3.14159f, 16, color, th);
            // Bottom arc
            DrawEllipseArc(dl, ImVec2(c.x, c.y + r * 0.45f), rx, ry, 0.0f, 0.0f, 3.14159f, 16, color, th);
            // Sides
            dl->AddLine(ImVec2(c.x - rx, c.y - r * 0.45f), ImVec2(c.x - rx, c.y + r * 0.45f), color, th);
            dl->AddLine(ImVec2(c.x + rx, c.y - r * 0.45f), ImVec2(c.x + rx, c.y + r * 0.45f), color, th);
            break;
        }

        case IconId::Validation: {
            // Balance scale
            float topY = c.y - r * 0.45f;
            dl->AddLine(ImVec2(c.x, topY), ImVec2(c.x, c.y + r * 0.70f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, topY), ImVec2(c.x + r * 0.75f, topY), color, th);
            // Left pan
            dl->AddLine(ImVec2(c.x - r * 0.65f, topY), ImVec2(c.x - r * 0.85f, topY + r * 0.55f), color, th * 0.8f);
            dl->AddLine(ImVec2(c.x - r * 0.65f, topY), ImVec2(c.x - r * 0.45f, topY + r * 0.55f), color, th * 0.8f);
            dl->AddLine(ImVec2(c.x - r * 0.90f, topY + r * 0.55f), ImVec2(c.x - r * 0.40f, topY + r * 0.55f), color, th);
            // Right pan
            dl->AddLine(ImVec2(c.x + r * 0.65f, topY), ImVec2(c.x + r * 0.45f, topY + r * 0.55f), color, th * 0.8f);
            dl->AddLine(ImVec2(c.x + r * 0.65f, topY), ImVec2(c.x + r * 0.85f, topY + r * 0.55f), color, th * 0.8f);
            dl->AddLine(ImVec2(c.x + r * 0.40f, topY + r * 0.55f), ImVec2(c.x + r * 0.90f, topY + r * 0.55f), color, th);
            // Base
            dl->AddLine(ImVec2(c.x - r * 0.40f, c.y + r * 0.70f), ImVec2(c.x + r * 0.40f, c.y + r * 0.70f), color, th);
            break;
        }

        case IconId::Physics:
        case IconId::Simulation: {
            // Atom with central nucleus + 2 tilted orbital electron paths
            dl->AddCircleFilled(c, std::max(2.0f, r * 0.22f), color, 16);
            DrawEllipseArc(dl, c, r * 0.88f, r * 0.35f, 0.61f, 0.0f, 6.28318f, 24, color, th);
            DrawEllipseArc(dl, c, r * 0.88f, r * 0.35f, -0.61f, 0.0f, 6.28318f, 24, color, th);
            break;
        }

        case IconId::Orbit:
        case IconId::Universe: {
            dl->AddCircleFilled(c, std::max(2.2f, r * 0.28f), color, 16);
            DrawEllipseArc(dl, c, r * 0.85f, r * 0.46f, -0.35f, 0.0f, 6.28318f, 24, color, th);
            // Planet node
            float cosA = std::cos(-0.35f), sinA = std::sin(-0.35f);
            ImVec2 pNode = RotatePoint(ImVec2(c.x + r * 0.85f * std::cos(0.9f), c.y + r * 0.46f * std::sin(0.9f)), c, cosA, sinA);
            dl->AddCircleFilled(pNode, std::max(1.8f, r * 0.18f), color, 12);
            break;
        }

        case IconId::Time: {
            dl->AddCircle(c, r * 0.85f, color, 24, th);
            dl->AddLine(c, ImVec2(c.x, c.y - r * 0.52f), color, th * 1.15f);
            dl->AddLine(c, ImVec2(c.x + r * 0.40f, c.y), color, th * 1.15f);
            break;
        }

        case IconId::Camera: {
            dl->AddRect(ImVec2(c.x - r * 0.80f, c.y - r * 0.45f), ImVec2(c.x + r * 0.80f, c.y + r * 0.65f), color, 2.0f, 0, th);
            dl->AddCircle(ImVec2(c.x, c.y + r * 0.10f), r * 0.35f, color, 20, th);
            dl->AddRectFilled(ImVec2(c.x - r * 0.35f, c.y - r * 0.70f), ImVec2(c.x + r * 0.10f, c.y - r * 0.45f), color, 1.0f);
            break;
        }

        case IconId::Cinematic: {
            // Clapperboard
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.65f), ImVec2(c.x + r * 0.85f, c.y + r * 0.65f), color, 2.0f, 0, th);
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y - r * 0.20f), ImVec2(c.x + r * 0.85f, c.y - r * 0.20f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.50f, c.y - r * 0.65f), ImVec2(c.x - r * 0.35f, c.y - r * 0.20f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.10f, c.y - r * 0.65f), ImVec2(c.x + r * 0.05f, c.y - r * 0.20f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.30f, c.y - r * 0.65f), ImVec2(c.x + r * 0.45f, c.y - r * 0.20f), color, th);
            break;
        }

        case IconId::PhotoMode: {
            // Camera + sparkle flash
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.40f), ImVec2(c.x + r * 0.70f, c.y + r * 0.70f), color, 2.0f, 0, th);
            dl->AddCircle(ImVec2(c.x - r * 0.08f, c.y + r * 0.15f), r * 0.32f, color, 18, th);
            // Flash star at top-right
            ImVec2 fC(c.x + r * 0.55f, c.y - r * 0.55f);
            float fR = r * 0.32f;
            dl->AddLine(ImVec2(fC.x - fR, fC.y), ImVec2(fC.x + fR, fC.y), color, th * 0.9f);
            dl->AddLine(ImVec2(fC.x, fC.y - fR), ImVec2(fC.x, fC.y + fR), color, th * 0.9f);
            break;
        }

        case IconId::AI:
        case IconId::AIAssistant: {
            // Central hub + 6 hexagonal constellation spokes
            dl->AddCircleFilled(c, std::max(2.0f, r * 0.26f), color, 16);
            for (int i = 0; i < 6; ++i) {
                float a = (float)i * 1.047197f; // 60 deg
                ImVec2 p(c.x + r * 0.75f * std::cos(a), c.y + r * 0.75f * std::sin(a));
                dl->AddLine(c, p, color, th * 0.85f);
                dl->AddCircleFilled(p, std::max(1.4f, r * 0.14f), color, 10);
            }
            break;
        }

        case IconId::Settings: {
            // Mechanical precision gear
            dl->AddCircle(c, r * 0.42f, color, 16, th);
            for (int i = 0; i < 6; ++i) {
                float a = (float)i * 1.047197f;
                float w = 0.22f;
                ImVec2 p0(c.x + r * 0.55f * std::cos(a - w), c.y + r * 0.55f * std::sin(a - w));
                ImVec2 p1(c.x + r * 0.85f * std::cos(a - w * 0.6f), c.y + r * 0.85f * std::sin(a - w * 0.6f));
                ImVec2 p2(c.x + r * 0.85f * std::cos(a + w * 0.6f), c.y + r * 0.85f * std::sin(a + w * 0.6f));
                ImVec2 p3(c.x + r * 0.55f * std::cos(a + w), c.y + r * 0.55f * std::sin(a + w));
                dl->AddLine(p0, p1, color, th);
                dl->AddLine(p1, p2, color, th);
                dl->AddLine(p2, p3, color, th);
            }
            break;
        }

        case IconId::Hierarchy: {
            // 3 horizontal tree list lines
            for (int i = 0; i < 3; ++i) {
                float y = c.y - r * 0.50f + (float)i * r * 0.50f;
                dl->AddCircleFilled(ImVec2(c.x - r * 0.60f, y), std::max(1.4f, th * 0.85f), color, 8);
                dl->AddLine(ImVec2(c.x - r * 0.35f, y), ImVec2(c.x + r * 0.70f, y), color, th);
            }
            break;
        }

        case IconId::Details: {
            // Sliders / Inspector tracks
            for (int i = 0; i < 3; ++i) {
                float y = c.y - r * 0.55f + (float)i * r * 0.55f;
                dl->AddLine(ImVec2(c.x - r * 0.75f, y), ImVec2(c.x + r * 0.75f, y), color, th * 0.8f);
                float kx = (i == 0) ? -r * 0.25f : ((i == 1) ? r * 0.35f : -r * 0.05f);
                dl->AddRectFilled(ImVec2(c.x + kx - r * 0.12f, y - r * 0.20f),
                                  ImVec2(c.x + kx + r * 0.12f, y + r * 0.20f), color, 1.0f);
            }
            break;
        }

        case IconId::Save: {
            // Floppy disk outline
            dl->AddRect(ImVec2(c.x - r * 0.75f, c.y - r * 0.75f), ImVec2(c.x + r * 0.75f, c.y + r * 0.75f), color, 2.0f, 0, th);
            // Sliding shutter
            dl->AddRect(ImVec2(c.x - r * 0.45f, c.y - r * 0.75f), ImVec2(c.x + r * 0.25f, c.y - r * 0.30f), color, 1.0f, 0, th);
            // Label box
            dl->AddRect(ImVec2(c.x - r * 0.50f, c.y + r * 0.15f), ImVec2(c.x + r * 0.50f, c.y + r * 0.75f), color, 1.0f, 0, th);
            break;
        }

        case IconId::Import: {
            // Tray + down arrow
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y + r * 0.25f), ImVec2(c.x - r * 0.75f, c.y + r * 0.75f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y + r * 0.75f), ImVec2(c.x + r * 0.75f, c.y + r * 0.75f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.75f, c.y + r * 0.75f), ImVec2(c.x + r * 0.75f, c.y + r * 0.25f), color, th);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.70f), ImVec2(c.x, c.y + r * 0.40f), color, th * 1.2f);
            float aL = r * 0.35f;
            dl->AddLine(ImVec2(c.x, c.y + r * 0.40f), ImVec2(c.x - aL, c.y + r * 0.40f - aL), color, th * 1.2f);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.40f), ImVec2(c.x + aL, c.y + r * 0.40f - aL), color, th * 1.2f);
            break;
        }

        case IconId::Folder: {
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y - r * 0.45f), ImVec2(c.x - r * 0.25f, c.y - r * 0.45f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r * 0.45f), ImVec2(c.x - r * 0.10f, c.y - r * 0.25f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.10f, c.y - r * 0.25f), ImVec2(c.x + r * 0.85f, c.y - r * 0.25f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.85f, c.y - r * 0.25f), ImVec2(c.x + r * 0.85f, c.y + r * 0.65f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.85f, c.y + r * 0.65f), ImVec2(c.x - r * 0.85f, c.y + r * 0.65f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.85f, c.y + r * 0.65f), ImVec2(c.x - r * 0.85f, c.y - r * 0.45f), color, th);
            break;
        }

        case IconId::Star: {
            // 4-Pointed astronomical radiant star
            float l = r * 0.90f, w = r * 0.25f;
            ImVec2 pts[8] = {
                ImVec2(c.x, c.y - l), ImVec2(c.x + w, c.y - w),
                ImVec2(c.x + l, c.y), ImVec2(c.x + w, c.y + w),
                ImVec2(c.x, c.y + l), ImVec2(c.x - w, c.y + w),
                ImVec2(c.x - l, c.y), ImVec2(c.x - w, c.y - w)
            };
            dl->AddConvexPolyFilled(pts, 8, color);
            break;
        }

        case IconId::Planet: {
            dl->AddCircleFilled(c, r * 0.50f, color, 20);
            DrawEllipseArc(dl, c, r * 0.90f, r * 0.35f, -0.32f, 0.0f, 6.28318f, 24, color, th);
            break;
        }

        case IconId::Moon: {
            // Crescent moon
            dl->PathArcTo(c, r * 0.75f, 0.8f, 5.48f, 20);
            dl->PathArcTo(ImVec2(c.x + r * 0.35f, c.y), r * 0.55f, 4.8f, 1.48f, 16);
            dl->PathStroke(color, 0, th);
            break;
        }

        case IconId::Asteroid:
        case IconId::AsteroidBelt: {
            // Irregular faceted rock polygon
            ImVec2 pts[6] = {
                ImVec2(c.x - r * 0.65f, c.y - r * 0.55f),
                ImVec2(c.x + r * 0.45f, c.y - r * 0.75f),
                ImVec2(c.x + r * 0.85f, c.y - r * 0.05f),
                ImVec2(c.x + r * 0.60f, c.y + r * 0.75f),
                ImVec2(c.x - r * 0.35f, c.y + r * 0.80f),
                ImVec2(c.x - r * 0.85f, c.y + r * 0.15f)
            };
            dl->AddPolyline(pts, 6, color, ImDrawFlags_Closed, th);
            dl->AddLine(pts[0], pts[3], color, th * 0.65f);
            break;
        }

        case IconId::Comet: {
            // Nucleus + trailing wisps
            ImVec2 n(c.x + r * 0.45f, c.y - r * 0.45f);
            dl->AddCircleFilled(n, std::max(2.0f, r * 0.28f), color, 14);
            dl->AddLine(ImVec2(n.x - r * 0.25f, n.y + r * 0.15f), ImVec2(c.x - r * 0.85f, c.y + r * 0.55f), color, th);
            dl->AddLine(ImVec2(n.x + r * 0.10f, n.y + r * 0.35f), ImVec2(c.x - r * 0.55f, c.y + r * 0.85f), color, th);
            dl->AddLine(ImVec2(n.x - r * 0.35f, n.y - r * 0.10f), ImVec2(c.x - r * 0.85f, c.y + r * 0.20f), color, th);
            break;
        }

        case IconId::BlackHole: {
            dl->AddCircleFilled(c, r * 0.40f, ImGui::ColorConvertFloat4ToU32(ImVec4(0.02f, 0.03f, 0.05f, 1.0f)), 24);
            DrawEllipseArc(dl, c, r * 0.85f, r * 0.35f, 0.0f, 0.0f, 6.28318f, 24, color, th * 1.25f);
            dl->AddCircle(c, r * 0.55f, color, 24, th);
            break;
        }

        case IconId::Matter:
        case IconId::MatterLab: {
            // Hexagonal crystal cell
            ImVec2 pts[6];
            for (int i = 0; i < 6; ++i) {
                float a = (float)i * 1.047197f - 0.523598f;
                pts[i] = ImVec2(c.x + r * 0.85f * std::cos(a), c.y + r * 0.85f * std::sin(a));
            }
            dl->AddPolyline(pts, 6, color, ImDrawFlags_Closed, th);
            dl->AddLine(c, pts[0], color, th * 0.85f);
            dl->AddLine(c, pts[2], color, th * 0.85f);
            dl->AddLine(c, pts[4], color, th * 0.85f);
            break;
        }

        case IconId::Energy: {
            // Lightning bolt
            ImVec2 pts[6] = {
                ImVec2(c.x + r * 0.15f, c.y - r * 0.85f),
                ImVec2(c.x - r * 0.55f, c.y + r * 0.05f),
                ImVec2(c.x - r * 0.05f, c.y + r * 0.05f),
                ImVec2(c.x - r * 0.20f, c.y + r * 0.85f),
                ImVec2(c.x + r * 0.55f, c.y - r * 0.05f),
                ImVec2(c.x + r * 0.05f, c.y - r * 0.05f)
            };
            dl->AddConvexPolyFilled(pts, 6, color);
            break;
        }

        case IconId::Speed: {
            // Speedometer gauge
            dl->PathArcTo(c, r * 0.75f, 2.5f, 6.9f, 20);
            dl->PathStroke(color, 0, th);
            dl->AddLine(c, ImVec2(c.x + r * 0.48f, c.y - r * 0.35f), color, th * 1.3f);
            dl->AddCircleFilled(c, std::max(1.8f, th * 1.1f), color, 8);
            break;
        }

        case IconId::Ruler: {
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y), ImVec2(c.x + r * 0.75f, c.y), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.75f, c.y - r * 0.35f), ImVec2(c.x - r * 0.75f, c.y + r * 0.35f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.75f, c.y - r * 0.35f), ImVec2(c.x + r * 0.75f, c.y + r * 0.35f), color, th);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.22f), ImVec2(c.x, c.y + r * 0.22f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.37f, c.y - r * 0.15f), ImVec2(c.x - r * 0.37f, c.y + r * 0.15f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.37f, c.y - r * 0.15f), ImVec2(c.x + r * 0.37f, c.y + r * 0.15f), color, th);
            break;
        }

        case IconId::Thermometer: {
            dl->AddCircle(ImVec2(c.x, c.y + r * 0.45f), r * 0.38f, color, 16, th);
            dl->AddLine(ImVec2(c.x - r * 0.18f, c.y - r * 0.75f), ImVec2(c.x - r * 0.18f, c.y + r * 0.20f), color, th);
            dl->AddLine(ImVec2(c.x + r * 0.18f, c.y - r * 0.75f), ImVec2(c.x + r * 0.18f, c.y + r * 0.20f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.18f, c.y - r * 0.75f), ImVec2(c.x + r * 0.18f, c.y - r * 0.75f), color, th);
            dl->AddCircleFilled(ImVec2(c.x, c.y + r * 0.45f), r * 0.22f, color, 12);
            break;
        }

        case IconId::Pressure: {
            dl->AddCircle(c, r * 0.80f, color, 24, th);
            dl->AddLine(c, ImVec2(c.x - r * 0.30f, c.y - r * 0.40f), color, th * 1.2f);
            dl->AddCircleFilled(c, std::max(1.6f, th), color, 8);
            break;
        }

        case IconId::Density: {
            // Packed mass nodes
            dl->AddCircleFilled(ImVec2(c.x - r * 0.35f, c.y + r * 0.30f), r * 0.26f, color, 10);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.35f, c.y + r * 0.30f), r * 0.26f, color, 10);
            dl->AddCircleFilled(ImVec2(c.x, c.y - r * 0.35f), r * 0.26f, color, 10);
            break;
        }

        case IconId::Gravity: {
            dl->AddLine(ImVec2(c.x, c.y - r * 0.70f), ImVec2(c.x, c.y + r * 0.55f), color, th * 1.25f);
            float aL = r * 0.38f;
            dl->AddLine(ImVec2(c.x, c.y + r * 0.55f), ImVec2(c.x - aL, c.y + r * 0.55f - aL), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.55f), ImVec2(c.x + aL, c.y + r * 0.55f - aL), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x - r * 0.55f, c.y + r * 0.75f), ImVec2(c.x + r * 0.55f, c.y + r * 0.75f), color, th);
            break;
        }

        case IconId::Atmosphere: {
            // Horizon + 2 concentric glowing shell arcs
            dl->PathArcTo(ImVec2(c.x, c.y + r * 0.75f), r * 0.75f, 3.14159f, 6.28318f, 16);
            dl->PathStroke(color, 0, th);
            dl->PathArcTo(ImVec2(c.x, c.y + r * 0.75f), r * 1.10f, 3.4f, 6.0f, 16);
            dl->PathStroke(color, 0, th * 0.8f);
            dl->AddLine(ImVec2(c.x - r, c.y + r * 0.75f), ImVec2(c.x + r, c.y + r * 0.75f), color, th);
            break;
        }

        case IconId::Clouds: {
            // Fluffy cloud contour
            dl->PathArcTo(ImVec2(c.x - r * 0.40f, c.y + r * 0.20f), r * 0.32f, 1.8f, 4.6f, 10);
            dl->PathArcTo(ImVec2(c.x - r * 0.05f, c.y - r * 0.15f), r * 0.45f, 3.0f, 5.8f, 12);
            dl->PathArcTo(ImVec2(c.x + r * 0.45f, c.y + r * 0.15f), r * 0.35f, 4.4f, 7.8f, 10);
            dl->PathLineTo(ImVec2(c.x - r * 0.40f, c.y + r * 0.50f));
            dl->PathStroke(color, 0, th);
            break;
        }

        case IconId::Radiation: {
            dl->AddCircleFilled(c, std::max(2.0f, r * 0.20f), color, 12);
            for (int i = 0; i < 3; ++i) {
                float a = (float)i * 2.094395f;
                dl->PathArcTo(c, r * 0.80f, a - 0.42f, a + 0.42f, 8);
                dl->PathLineTo(c);
                dl->PathFillConvex(color);
            }
            break;
        }

        case IconId::Terminal: {
            dl->AddRect(ImVec2(c.x - r * 0.85f, c.y - r * 0.65f), ImVec2(c.x + r * 0.85f, c.y + r * 0.65f), color, 2.0f, 0, th);
            dl->AddLine(ImVec2(c.x - r * 0.60f, c.y - r * 0.30f), ImVec2(c.x - r * 0.20f, c.y), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.20f, c.y), ImVec2(c.x - r * 0.60f, c.y + r * 0.30f), color, th);
            dl->AddLine(ImVec2(c.x - r * 0.05f, c.y + r * 0.30f), ImVec2(c.x + r * 0.45f, c.y + r * 0.30f), color, th);
            break;
        }

        case IconId::Eye: {
            dl->PathArcTo(ImVec2(c.x, c.y - r * 0.65f), r * 1.15f, 0.78f, 2.36f, 16);
            dl->PathStroke(color, 0, th);
            dl->PathArcTo(ImVec2(c.x, c.y + r * 0.65f), r * 1.15f, 3.92f, 5.50f, 16);
            dl->PathStroke(color, 0, th);
            dl->AddCircleFilled(c, std::max(1.8f, r * 0.30f), color, 12);
            break;
        }

        case IconId::EyeSlash: {
            Draw(dl, IconId::Eye, c, size, color, th);
            dl->AddLine(ImVec2(c.x - r * 0.80f, c.y - r * 0.80f), ImVec2(c.x + r * 0.80f, c.y + r * 0.80f), color, th * 1.2f);
            break;
        }

        case IconId::Filter: {
            ImVec2 pts[6] = {
                ImVec2(c.x - r * 0.80f, c.y - r * 0.70f),
                ImVec2(c.x + r * 0.80f, c.y - r * 0.70f),
                ImVec2(c.x + r * 0.18f, c.y + r * 0.05f),
                ImVec2(c.x + r * 0.18f, c.y + r * 0.75f),
                ImVec2(c.x - r * 0.18f, c.y + r * 0.55f),
                ImVec2(c.x - r * 0.18f, c.y + r * 0.05f)
            };
            dl->AddPolyline(pts, 6, color, ImDrawFlags_Closed, th);
            break;
        }

        case IconId::Maximize: {
            float d = r * 0.80f, cL = r * 0.35f;
            // Top-left
            dl->AddLine(ImVec2(c.x - d, c.y - d + cL), ImVec2(c.x - d, c.y - d), color, th);
            dl->AddLine(ImVec2(c.x - d, c.y - d), ImVec2(c.x - d + cL, c.y - d), color, th);
            // Top-right
            dl->AddLine(ImVec2(c.x + d - cL, c.y - d), ImVec2(c.x + d, c.y - d), color, th);
            dl->AddLine(ImVec2(c.x + d, c.y - d), ImVec2(c.x + d, c.y - d + cL), color, th);
            // Bottom-left
            dl->AddLine(ImVec2(c.x - d, c.y + d - cL), ImVec2(c.x - d, c.y + d), color, th);
            dl->AddLine(ImVec2(c.x - d, c.y + d), ImVec2(c.x - d + cL, c.y + d), color, th);
            // Bottom-right
            dl->AddLine(ImVec2(c.x + d - cL, c.y + d), ImVec2(c.x + d, c.y + d), color, th);
            dl->AddLine(ImVec2(c.x + d, c.y + d), ImVec2(c.x + d, c.y + d - cL), color, th);
            break;
        }

        case IconId::ChevronDown: {
            dl->AddLine(ImVec2(c.x - r * 0.60f, c.y - r * 0.25f), ImVec2(c.x, c.y + r * 0.35f), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x, c.y + r * 0.35f), ImVec2(c.x + r * 0.60f, c.y - r * 0.25f), color, th * 1.25f);
            break;
        }

        case IconId::ChevronUp: {
            dl->AddLine(ImVec2(c.x - r * 0.60f, c.y + r * 0.25f), ImVec2(c.x, c.y - r * 0.35f), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x, c.y - r * 0.35f), ImVec2(c.x + r * 0.60f, c.y + r * 0.25f), color, th * 1.25f);
            break;
        }

        case IconId::ChevronLeft: {
            dl->AddLine(ImVec2(c.x + r * 0.25f, c.y - r * 0.60f), ImVec2(c.x - r * 0.35f, c.y), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x - r * 0.35f, c.y), ImVec2(c.x + r * 0.25f, c.y + r * 0.60f), color, th * 1.25f);
            break;
        }

        case IconId::ChevronRight: {
            dl->AddLine(ImVec2(c.x - r * 0.25f, c.y - r * 0.60f), ImVec2(c.x + r * 0.35f, c.y), color, th * 1.25f);
            dl->AddLine(ImVec2(c.x + r * 0.35f, c.y), ImVec2(c.x - r * 0.25f, c.y + r * 0.60f), color, th * 1.25f);
            break;
        }

        case IconId::Tools: {
            // Crossed wrench & screwdriver
            dl->AddLine(ImVec2(c.x - r * 0.70f, c.y - r * 0.70f), ImVec2(c.x + r * 0.70f, c.y + r * 0.70f), color, th * 1.2f);
            dl->AddLine(ImVec2(c.x + r * 0.70f, c.y - r * 0.70f), ImVec2(c.x - r * 0.70f, c.y + r * 0.70f), color, th * 1.2f);
            dl->AddCircle(ImVec2(c.x - r * 0.60f, c.y - r * 0.60f), r * 0.25f, color, 12, th);
            dl->AddCircle(ImVec2(c.x + r * 0.60f, c.y - r * 0.60f), r * 0.25f, color, 12, th);
            break;
        }

        case IconId::Presets:
        case IconId::System: {
            // Multi-body star system
            dl->AddCircleFilled(c, r * 0.32f, color, 16);
            DrawEllipseArc(dl, c, r * 0.85f, r * 0.40f, 0.0f, 0.0f, 6.28318f, 20, color, th * 0.85f);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.85f, c.y), std::max(1.6f, r * 0.16f), color, 8);
            dl->AddCircleFilled(ImVec2(c.x - r * 0.45f, c.y - r * 0.34f), std::max(1.4f, r * 0.13f), color, 8);
            break;
        }

        case IconId::Objects: {
            // Celestial sphere with grid latitude
            dl->AddCircle(c, r * 0.82f, color, 24, th);
            dl->AddLine(ImVec2(c.x - r * 0.82f, c.y), ImVec2(c.x + r * 0.82f, c.y), color, th * 0.85f);
            DrawEllipseArc(dl, c, r * 0.82f, r * 0.35f, 0.0f, 0.0f, 6.28318f, 20, color, th * 0.85f);
            break;
        }

        case IconId::Explore: {
            // Galaxy spiral / telescope reticle
            DrawEllipseArc(dl, c, r * 0.90f, r * 0.45f, -0.6f, 0.0f, 6.28318f, 24, color, th);
            dl->AddCircleFilled(c, std::max(2.2f, r * 0.28f), color, 16);
            dl->AddCircleFilled(ImVec2(c.x + r * 0.55f, c.y + r * 0.25f), std::max(1.4f, r * 0.14f), color, 8);
            dl->AddCircleFilled(ImVec2(c.x - r * 0.55f, c.y - r * 0.25f), std::max(1.4f, r * 0.14f), color, 8);
            break;
        }

        default:
            break;
    }
}

void Icon(IconId icon, float size, ImU32 color, float spacingAfter) {
    if (icon == IconId::None) return;
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 cp = ImGui::GetCursorScreenPos();
    float lineH = ImGui::GetTextLineHeight();
    ImVec2 center(cp.x + size * 0.5f, cp.y + lineH * 0.5f);
    Draw(dl, icon, center, size, color);
    ImGui::Dummy(ImVec2(size + spacingAfter, lineH));
}

bool Button(const char* str_id, IconId icon, const char* label, const ImVec2& size, bool active, const ImVec4* customBg, const ImVec4* customFg) {
    char buttonId[128];
    if (str_id && str_id[0] != '\0') {
        if (str_id[0] == '#' && str_id[1] == '#') {
            snprintf(buttonId, sizeof(buttonId), "%s", str_id);
        } else {
            snprintf(buttonId, sizeof(buttonId), "##%s", str_id);
        }
    } else if (label && label[0] != '\0') {
        snprintf(buttonId, sizeof(buttonId), "##%s_%d", label, (int)icon);
    } else {
        snprintf(buttonId, sizeof(buttonId), "##icn_btn_%d", (int)icon);
    }

    ImVec2 minSize = size;
    float iconSize = IconSize::Standard;
    float padX = 10.0f;
    bool hasText = (label && label[0] != '\0');
    float spacing = hasText ? 7.0f : 0.0f;
    float textW = hasText ? ImGui::CalcTextSize(label).x : 0.0f;

    if (minSize.x <= 0.0f) {
        minSize.x = iconSize + textW + padX * 2.0f + spacing;
    }
    if (minSize.y <= 0.0f) {
        minSize.y = 28.0f;
    }

    int pushedColors = 0;
    if (active) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.00f, 0.55f, 0.80f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
        pushedColors = 2;
    } else if (customBg) {
        ImGui::PushStyleColor(ImGuiCol_Button, *customBg);
        ImVec4 hov(std::min(1.0f, customBg->x * 1.25f), std::min(1.0f, customBg->y * 1.25f), std::min(1.0f, customBg->z * 1.25f), customBg->w);
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, hov);
        if (customFg) {
            ImGui::PushStyleColor(ImGuiCol_Text, *customFg);
        } else {
            ImGui::PushStyleColor(ImGuiCol_Text, UIThemeCol::TextPrimary);
        }
        pushedColors = 3;
    }

    bool pressed = ImGui::Button(buttonId, minSize);

    if (pushedColors > 0) {
        ImGui::PopStyleColor(pushedColors);
    }

    // Draw the icon and label centered inside the actual button rect
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    float actualW = bMax.x - bMin.x;
    float actualH = bMax.y - bMin.y;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 fgCol = 0;
    if (active) {
        fgCol = ImGui::ColorConvertFloat4ToU32(ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    } else if (customFg) {
        fgCol = ImGui::ColorConvertFloat4ToU32(*customFg);
    } else if (ImGui::IsItemHovered()) {
        fgCol = ImGui::ColorConvertFloat4ToU32(UIThemeCol::Accent);
    } else {
        fgCol = ImGui::GetColorU32(ImGuiCol_Text);
    }

    float totalW = iconSize + (hasText ? (textW + spacing) : 0.0f);
    float startX = std::max(bMin.x + 4.0f, bMin.x + (actualW - totalW) * 0.5f);
    float centerY = bMin.y + actualH * 0.5f;

    dl->PushClipRect(bMin, bMax, true);

    ImVec2 iconCenter(startX + iconSize * 0.5f, centerY);
    Draw(dl, icon, iconCenter, iconSize, fgCol);

    if (hasText) {
        float textY = centerY - ImGui::GetTextLineHeight() * 0.5f;
        dl->AddText(ImVec2(startX + iconSize + spacing, textY), fgCol, label);
    }

    dl->PopClipRect();

    return pressed;
}

bool SmallButton(const char* str_id, IconId icon, const char* label) {
    char buttonId[128];
    if (str_id && str_id[0] != '\0') {
        if (str_id[0] == '#' && str_id[1] == '#') {
            snprintf(buttonId, sizeof(buttonId), "%s", str_id);
        } else {
            snprintf(buttonId, sizeof(buttonId), "##%s", str_id);
        }
    } else if (label && label[0] != '\0') {
        snprintf(buttonId, sizeof(buttonId), "##sm_%s_%d", label, (int)icon);
    } else {
        snprintf(buttonId, sizeof(buttonId), "##sm_btn_%d", (int)icon);
    }

    float iconSize = IconSize::Small;
    bool hasText = (label && label[0] != '\0');
    float textW = hasText ? ImGui::CalcTextSize(label).x : 0.0f;
    float spacing = hasText ? 5.0f : 0.0f;
    float btnW = iconSize + textW + spacing + 12.0f;
    float btnH = 22.0f;

    bool pressed = ImGui::Button(buttonId, ImVec2(btnW, btnH));
    ImVec2 bMin = ImGui::GetItemRectMin();
    ImVec2 bMax = ImGui::GetItemRectMax();
    float actualW = bMax.x - bMin.x;
    float actualH = bMax.y - bMin.y;
    ImDrawList* dl = ImGui::GetWindowDrawList();

    ImU32 fgCol = ImGui::IsItemHovered() ? ImGui::ColorConvertFloat4ToU32(UIThemeCol::Accent) : ImGui::GetColorU32(ImGuiCol_Text);
    float totalW = iconSize + (hasText ? (textW + spacing) : 0.0f);
    float startX = std::max(bMin.x + 3.0f, bMin.x + (actualW - totalW) * 0.5f);
    float centerY = bMin.y + actualH * 0.5f;

    dl->PushClipRect(bMin, bMax, true);
    Draw(dl, icon, ImVec2(startX + iconSize * 0.5f, centerY), iconSize, fgCol);

    if (hasText) {
        float textY = centerY - ImGui::GetTextLineHeight() * 0.5f;
        dl->AddText(ImVec2(startX + iconSize + spacing, textY), fgCol, label);
    }
    dl->PopClipRect();

    return pressed;
}

void Icon(IconId icon, float size, const ImVec4& color, float spacingAfter) {
    Icon(icon, size, ImGui::ColorConvertFloat4ToU32(color), spacingAfter);
}

bool Button(IconId icon, const char* label, const ImVec2& size, bool active, const ImVec4* customBg, const ImVec4* customFg) {
    char autoId[64];
    const char* effectiveLabel = (label && label[0] != '\0') ? label : nullptr;
    if (effectiveLabel) {
        return Button(effectiveLabel, icon, effectiveLabel, size, active, customBg, customFg);
    } else {
        snprintf(autoId, sizeof(autoId), "##btn_icn_%d", (int)icon);
        return Button(autoId, icon, nullptr, size, active, customBg, customFg);
    }
}

bool SmallButton(IconId icon, const char* label) {
    char autoId[64];
    const char* effectiveLabel = (label && label[0] != '\0') ? label : nullptr;
    if (effectiveLabel) {
        return SmallButton(effectiveLabel, icon, effectiveLabel);
    } else {
        snprintf(autoId, sizeof(autoId), "##sm_btn_%d", (int)icon);
        return SmallButton(autoId, icon, nullptr);
    }
}

void StatItem(IconId icon, const char* label, const char* value, float iconSize, const ImVec4* iconColor) {
    ImGui::BeginGroup();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 curPos = ImGui::GetCursorScreenPos();
    float lineH = ImGui::GetTextLineHeight();
    ImVec2 iconCenter(curPos.x + iconSize * 0.5f, curPos.y + lineH * 0.5f);

    ImU32 colU32 = iconColor ? ImGui::ColorConvertFloat4ToU32(*iconColor) : ImGui::ColorConvertFloat4ToU32(UIThemeCol::Accent);
    Draw(dl, icon, iconCenter, iconSize, colU32);

    ImGui::SetCursorScreenPos(ImVec2(curPos.x + iconSize + 6.0f, curPos.y));
    ImGui::TextColored(UIThemeCol::TextSecondary, "%s", label);
    ImGui::SameLine();
    ImGui::TextColored(UIThemeCol::TextPrimary, "%s", value);
    ImGui::EndGroup();
}

void DrawLogo(ImDrawList* dl, ImVec2 center, float radius, ImU32 accentCol, ImU32 secondaryCol) {
    Draw(dl, IconId::Logo, center, radius * 2.0f, accentCol);
}

void DrawBranding(float height, bool compact) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImVec2 pos = ImGui::GetCursorScreenPos();
    float iconSize = height * 0.90f;
    ImVec2 iconCenter(pos.x + iconSize * 0.5f, pos.y + height * 0.5f);
    ImU32 accentCol = ImGui::ColorConvertFloat4ToU32(UIThemeCol::Accent);

    Draw(dl, IconId::Logo, iconCenter, iconSize, accentCol);

    if (!compact) {
        float textX = pos.x + iconSize + 10.0f;
        float textY = pos.y + (height - ImGui::GetTextLineHeight()) * 0.5f;
        dl->AddText(ImVec2(textX, textY), accentCol, "ASTROGENESIS");
        ImGui::Dummy(ImVec2(iconSize + 10.0f + ImGui::CalcTextSize("ASTROGENESIS").x + 12.0f, height));
    } else {
        ImGui::Dummy(ImVec2(iconSize + 6.0f, height));
    }
}

} // namespace UIIcon

} // namespace AstroGenesis
