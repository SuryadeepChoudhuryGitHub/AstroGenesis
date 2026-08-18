#pragma once

#include <vector>
#include <memory>
#include <string>
#include <glm/glm.hpp>
#include "simulation/MaterialModel.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

struct MaterialNode {
    glm::dvec3 positionM{0.0};          // Current world position in meters
    glm::dvec3 restPositionM{0.0};      // Rest position in initial body local frame
    glm::dvec3 predictedPosM{0.0};      // Sub-step predicted position
    glm::dvec3 velocityMps{0.0};        // Velocity in m/s
    glm::dvec3 externalForceN{0.0};     // Accumulated external forces (differential gravity, contact)

    double massKg = 1.0;
    double invMass = 1.0;
    double temperatureK = 293.15;       // Temperature in Kelvin
    double internalEnergyJ = 0.0;       // Internal thermal energy
    double damage = 0.0;                // Continuous damage D in [0, 1]
    double equivalentStrain = 0.0;      // Accumulated strain
    double plasticStrain = 0.0;         // Permanent plastic strain
    double vonMisesStressPa = 0.0;      // Instantaneous equivalent stress
    MaterialPhase phase = MaterialPhase::Solid;

    float scalarValue = 0.0f;           // Normalized GPU visualization scalar
};

struct MaterialConstraint {
    int nodeA = 0;
    int nodeB = 0;
    double restLengthM = 1.0;           // Current rest length (permanently modified by plastic flow)
    double initialLengthM = 1.0;        // Pristine original reference length
    double crossSectionAreaM2 = 1.0;    // Cross-sectional element area
    double plasticStretchM = 0.0;       // Accumulated plastic deformation
    double damage = 0.0;                // Continuous damage D in [0, 1]
    bool isBroken = false;              // Permanent fracture flag
    double currentStressPa = 0.0;       // Instantaneous axial/shear stress
    double lambda = 0.0;                // XPBD Lagrange multiplier
};

struct MeshTriangle {
    int i0 = 0;
    int i1 = 0;
    int i2 = 0;
    glm::vec3 normal{0.0f, 1.0f, 0.0f};
};

class DeformableBody {
public:
    DeformableBody(const std::string& name, const MaterialDefinition& material);
    ~DeformableBody();

    // Factory geometry constructors
    static std::shared_ptr<DeformableBody> createCube(
        const std::string& name,
        const MaterialDefinition& material,
        const glm::dvec3& centerM,
        double sizeM,
        int resolution = 4,
        const glm::dvec3& initialVelMps = glm::dvec3(0.0),
        double initialTempK = 293.15
    );

    static std::shared_ptr<DeformableBody> createSphere(
        const std::string& name,
        const MaterialDefinition& material,
        const glm::dvec3& centerM,
        double radiusM,
        int rings = 6,
        int sectors = 8,
        const glm::dvec3& initialVelMps = glm::dvec3(0.0),
        double initialTempK = 293.15
    );

    static std::shared_ptr<DeformableBody> createAsteroid(
        const std::string& name,
        const MaterialDefinition& material,
        const glm::dvec3& centerM,
        double radiusM,
        int resolution = 5,
        const glm::dvec3& initialVelMps = glm::dvec3(0.0),
        double initialTempK = 180.0
    );

    // Physics Simulation Pipeline
    void computeExternalForces(const std::vector<CelestialBody>& attractors, bool enableGR);
    void subStepXPBD(double subDt, int constraintIterations = 4);
    void updateThermalConductionAndRadiation(double subDt);
    void updatePlasticityAndDamage(double subDt);

    // Topological Fracture & Fragmentation
    bool checkAndPerformFragmentation(std::vector<std::shared_ptr<DeformableBody>>& outNewFragments);

    // Diagnostics & Conservation Measurements
    glm::dvec3 getCenterOfMass() const;
    glm::dvec3 getLinearMomentum() const;
    glm::dvec3 getAngularMomentum() const;
    double getTotalMass() const;
    double getKineticEnergy() const;
    double getElasticPotentialEnergy() const;
    double getThermalInternalEnergy() const;
    double getMaxVonMisesStress() const;
    double getMeanTemperature() const;
    double getMaxDamage() const;
    int getActiveConstraintCount() const;
    int getBrokenConstraintCount() const;

    // Getters & Setters
    const std::string& getName() const { return m_name; }
    void setName(const std::string& name) { m_name = name; }

    const MaterialDefinition& getMaterial() const { return m_material; }
    void setMaterial(const MaterialDefinition& mat) { m_material = mat; }

    const std::vector<MaterialNode>& getNodes() const { return m_nodes; }
    std::vector<MaterialNode>& getNodes() { return m_nodes; }

    const std::vector<MaterialConstraint>& getConstraints() const { return m_constraints; }
    std::vector<MaterialConstraint>& getConstraints() { return m_constraints; }

    const std::vector<MeshTriangle>& getSurfaceTriangles() const { return m_surfaceTriangles; }

    bool isFragmented() const { return m_isFragmented; }
    void setFragmented(bool val) { m_isFragmented = val; }

    void setFixedAnchor(int nodeIndex, bool fixed);

private:
    void rebuildSurfaceMesh();

    std::string m_name;
    MaterialDefinition m_material;

    std::vector<MaterialNode> m_nodes;
    std::vector<MaterialConstraint> m_constraints;
    std::vector<MeshTriangle> m_surfaceTriangles;
    std::vector<bool> m_isFixedNode;

    bool m_isFragmented = false;
};

} // namespace AstroGenesis
