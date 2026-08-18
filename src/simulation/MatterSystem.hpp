#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "simulation/DeformableBody.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

enum class MatterVisualizationMode {
    RealisticMaterial = 0,
    VonMisesStress = 1,
    MechanicalStrain = 2,
    TemperatureHeatmap = 3,
    DamageAndFracture = 4,
    PlasticDeformation = 5,
    TidalGravityVectors = 6
};

struct MatterSystemDiagnostics {
    int totalDeformableBodies = 0;
    int totalNodes = 0;
    int totalConstraints = 0;
    int totalBrokenConstraints = 0;

    double totalMassKg = 0.0;
    glm::dvec3 totalLinearMomentum{0.0};
    glm::dvec3 totalAngularMomentum{0.0};

    double kineticEnergyJ = 0.0;
    double elasticPotentialEnergyJ = 0.0;
    double thermalInternalEnergyJ = 0.0;
    double totalEnergyJ = 0.0;
    double initialTotalEnergyJ = 0.0;
    double energyConservationDriftPct = 0.0;

    double maxVonMisesStressPa = 0.0;
    double maxTemperatureK = 0.0;
    double maxDamage = 0.0;
};

class MatterSystem {
public:
    MatterSystem();
    ~MatterSystem();

    void initialize();
    void update(double deltaSeconds, const std::vector<CelestialBody>& attractors, bool enableGR);

    // Body Management
    void addBody(std::shared_ptr<DeformableBody> body);
    void removeBody(size_t index);
    void clearAllBodies();

    const std::vector<std::shared_ptr<DeformableBody>>& getBodies() const { return m_bodies; }
    std::vector<std::shared_ptr<DeformableBody>>& getBodies() { return m_bodies; }

    // Visualization Mode
    MatterVisualizationMode getVisualizationMode() const { return m_visMode; }
    void setVisualizationMode(MatterVisualizationMode mode) { m_visMode = mode; }

    // Sandbox Scenario Spawners
    void spawnBlackHoleTidalDisruptionLab(const glm::dvec3& originM = glm::dvec3(0.0));
    void spawnHypervelocityCollision(const glm::dvec3& originM = glm::dvec3(0.0));
    void spawnTensileTest(const glm::dvec3& originM = glm::dvec3(0.0));
    void spawnThermalMeltingLab(const glm::dvec3& originM = glm::dvec3(0.0));

    // Diagnostics & Conservation
    const MatterSystemDiagnostics& getDiagnostics() const { return m_diagnostics; }

private:
    void resolveInterBodyCollisions(double subDt);
    void updateDiagnostics();
    void updateNodeVisualizationScalars();

    std::vector<std::shared_ptr<DeformableBody>> m_bodies;
    MatterVisualizationMode m_visMode = MatterVisualizationMode::RealisticMaterial;
    MatterSystemDiagnostics m_diagnostics;

    double m_subStepTimeS = 0.004; // 4ms physical sub-step for continuum stability
};

} // namespace AstroGenesis
