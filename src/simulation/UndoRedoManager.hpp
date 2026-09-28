#pragma once

#include <vector>
#include <string>
#include <memory>
#include <algorithm>
#include "simulation/CelestialBody.hpp"
#include "simulation/PhysicsEngine.hpp"

namespace AstroGenesis {

enum class EditActionType {
    Reposition,
    PropertyChange,
    AddBody,
    DeleteBody
};

struct EditAction {
    EditActionType type;
    std::string description;

    int bodyIndex = -1;
    std::string bodyId;

    // For Reposition
    glm::vec3 oldPosAU{0.0f};
    glm::vec3 newPosAU{0.0f};
    glm::dvec3 oldVelMps{0.0};
    glm::dvec3 newVelMps{0.0};

    // Full snapshots for structural edits or property changes
    CelestialBody beforeSnapshot;
    CelestialBody afterSnapshot;
};

class UndoRedoManager {
public:
    UndoRedoManager() = default;

    void recordReposition(int bodyIndex, const std::string& bodyId, 
                          const glm::vec3& oldPosAU, const glm::vec3& newPosAU,
                          const glm::dvec3& oldVelMps, const glm::dvec3& newVelMps,
                          const std::string& name) {
        if (glm::distance(oldPosAU, newPosAU) < 1e-5f) return;
        EditAction act;
        act.type = EditActionType::Reposition;
        act.description = "Move " + name;
        act.bodyIndex = bodyIndex;
        act.bodyId = bodyId;
        act.oldPosAU = oldPosAU;
        act.newPosAU = newPosAU;
        act.oldVelMps = oldVelMps;
        act.newVelMps = newVelMps;

        pushAction(act);
    }

    void recordReposition(int bodyIndex, const glm::vec3& oldPosAU, const glm::vec3& newPosAU,
                          const glm::dvec3& oldVelMps, const glm::dvec3& newVelMps,
                          const std::string& name = "") {
        recordReposition(bodyIndex, "", oldPosAU, newPosAU, oldVelMps, newVelMps, name);
    }

    void recordPropertyChange(int bodyIndex, const CelestialBody& before, const CelestialBody& after, const std::string& propName) {
        EditAction act;
        act.type = EditActionType::PropertyChange;
        act.description = "Change " + before.name + " " + propName;
        act.bodyIndex = bodyIndex;
        act.bodyId = before.id;
        act.beforeSnapshot = before;
        act.afterSnapshot = after;

        pushAction(act);
    }

    void recordAddBody(int bodyIndex, const CelestialBody& body) {
        EditAction act;
        act.type = EditActionType::AddBody;
        act.description = "Add " + body.name;
        act.bodyIndex = bodyIndex;
        act.bodyId = body.id;
        act.afterSnapshot = body;

        pushAction(act);
    }

    void recordDeleteBody(int bodyIndex, const CelestialBody& body) {
        EditAction act;
        act.type = EditActionType::DeleteBody;
        act.description = "Delete " + body.name;
        act.bodyIndex = bodyIndex;
        act.bodyId = body.id;
        act.beforeSnapshot = body;

        pushAction(act);
    }

    bool canUndo() const {
        return !m_undoStack.empty();
    }

    bool canRedo() const {
        return !m_redoStack.empty();
    }

    std::string getLastUndoDescription() const {
        return m_undoStack.empty() ? "" : m_undoStack.back().description;
    }

    std::string getLastRedoDescription() const {
        return m_redoStack.empty() ? "" : m_redoStack.back().description;
    }

    bool undo(PhysicsEngine& physics, std::string& outMessage) {
        if (m_undoStack.empty()) return false;
        EditAction act = m_undoStack.back();
        m_undoStack.pop_back();

        int targetIdx = findBodyIndex(physics, act.bodyId, act.bodyIndex);

        switch (act.type) {
            case EditActionType::Reposition: {
                if (targetIdx >= 0) {
                    physics.setBodyPositionAU(targetIdx, act.oldPosAU, true);
                    physics.setBodyVelocity(targetIdx, act.oldVelMps);
                    physics.selectBody(targetIdx);
                }
                break;
            }
            case EditActionType::PropertyChange: {
                if (targetIdx >= 0) {
                    physics.getBodies()[targetIdx] = act.beforeSnapshot;
                    physics.updateBodyScales();
                    physics.forceUpdatePhysicalQuantities();
                    physics.selectBody(targetIdx);
                }
                break;
            }
            case EditActionType::AddBody: {
                if (targetIdx >= 0) {
                    physics.removeBody(targetIdx);
                }
                break;
            }
            case EditActionType::DeleteBody: {
                physics.addBody(act.beforeSnapshot);
                physics.selectBody((int)physics.getBodies().size() - 1);
                break;
            }
        }

        outMessage = "Undo: " + act.description;
        m_redoStack.push_back(act);
        return true;
    }

    bool redo(PhysicsEngine& physics, std::string& outMessage) {
        if (m_redoStack.empty()) return false;
        EditAction act = m_redoStack.back();
        m_redoStack.pop_back();

        int targetIdx = findBodyIndex(physics, act.bodyId, act.bodyIndex);

        switch (act.type) {
            case EditActionType::Reposition: {
                if (targetIdx >= 0) {
                    physics.setBodyPositionAU(targetIdx, act.newPosAU, true);
                    physics.setBodyVelocity(targetIdx, act.newVelMps);
                    physics.selectBody(targetIdx);
                }
                break;
            }
            case EditActionType::PropertyChange: {
                if (targetIdx >= 0) {
                    physics.getBodies()[targetIdx] = act.afterSnapshot;
                    physics.updateBodyScales();
                    physics.forceUpdatePhysicalQuantities();
                    physics.selectBody(targetIdx);
                }
                break;
            }
            case EditActionType::AddBody: {
                physics.addBody(act.afterSnapshot);
                physics.selectBody((int)physics.getBodies().size() - 1);
                break;
            }
            case EditActionType::DeleteBody: {
                if (targetIdx >= 0) {
                    physics.removeBody(targetIdx);
                }
                break;
            }
        }

        outMessage = "Redo: " + act.description;
        m_undoStack.push_back(act);
        return true;
    }

    bool undo(PhysicsEngine& physics) {
        std::string msg;
        return undo(physics, msg);
    }

    bool redo(PhysicsEngine& physics) {
        std::string msg;
        return redo(physics, msg);
    }

    std::string getLastActionName() const {
        return getLastUndoDescription();
    }

    void clear() {
        m_undoStack.clear();
        m_redoStack.clear();
    }

private:
    void pushAction(const EditAction& act) {
        m_undoStack.push_back(act);
        m_redoStack.clear(); // Clear redo branch on new edit
        if (m_undoStack.size() > 50) {
            m_undoStack.erase(m_undoStack.begin());
        }
    }

    int findBodyIndex(PhysicsEngine& physics, const std::string& id, int fallbackIdx) {
        const auto& bodies = physics.getBodies();
        for (size_t i = 0; i < bodies.size(); ++i) {
            if (bodies[i].id == id) return (int)i;
        }
        if (fallbackIdx >= 0 && fallbackIdx < (int)bodies.size()) return fallbackIdx;
        return -1;
    }

    std::vector<EditAction> m_undoStack;
    std::vector<EditAction> m_redoStack;
};

} // namespace AstroGenesis
