#include "simulation/MatterSystem.hpp"
#include <cmath>
#include <algorithm>

namespace AstroGenesis {

MatterSystem::MatterSystem() {}

MatterSystem::~MatterSystem() {}

void MatterSystem::initialize() {
    clearAllBodies();
    // Default initial demonstration: A realistic deformable asteroid in space
    const auto& basalt = MaterialLibrary::instance().getMaterial("Basalt Rock");
    auto asteroid = DeformableBody::createAsteroid(
        "Ceres_Test_Deformable",
        basalt,
        glm::dvec3(0.0, 0.0, 0.0),
        50000.0, // 50 km radius
        5,
        glm::dvec3(0.0),
        180.0
    );
    addBody(asteroid);
}

void MatterSystem::addBody(std::shared_ptr<DeformableBody> body) {
    if (body) {
        m_bodies.push_back(body);
    }
}

void MatterSystem::removeBody(size_t index) {
    if (index < m_bodies.size()) {
        m_bodies.erase(m_bodies.begin() + index);
    }
}

void MatterSystem::clearAllBodies() {
    m_bodies.clear();
    m_diagnostics = {};
}

void MatterSystem::update(double deltaSeconds, const std::vector<CelestialBody>& attractors, bool enableGR) {
    if (m_bodies.empty() || deltaSeconds <= 0.0) return;

    double remainingTime = deltaSeconds;
    int maxSubSteps = 32;
    int stepCount = 0;

    std::vector<std::shared_ptr<DeformableBody>> newFragments;

    while (remainingTime > 1.0e-6 && stepCount < maxSubSteps) {
        double subDt = std::min(m_subStepTimeS, remainingTime);
        remainingTime -= subDt;
        stepCount++;

        // 1. External Forces: True differential gravity across every individual node
        for (auto& body : m_bodies) {
            body->computeExternalForces(attractors, enableGR);
        }

        // 2. Sub-stepped XPBD Constraint Solvers
        for (auto& body : m_bodies) {
            body->subStepXPBD(subDt, 4);
        }

        // 3. Continuum Plastic Flow & Damage Accumulation
        for (auto& body : m_bodies) {
            body->updatePlasticityAndDamage(subDt);
        }

        // 4. Fourier Conduction & Stefan-Boltzmann Radiative Cooling
        for (auto& body : m_bodies) {
            body->updateThermalConductionAndRadiation(subDt);
        }

        // 5. Inter-body node-to-node contact resolution
        resolveInterBodyCollisions(subDt);

        // 6. Topological Fracture & Multi-Body Fragmentation Check
        for (auto& body : m_bodies) {
            body->checkAndPerformFragmentation(newFragments);
        }
    }

    // Append newly spawned child fragments to the active bodies list
    for (auto& frag : newFragments) {
        if (frag) {
            m_bodies.push_back(frag);
        }
    }

    updateDiagnostics();
    updateNodeVisualizationScalars();
}

void MatterSystem::resolveInterBodyCollisions(double subDt) {
    size_t numBodies = m_bodies.size();
    if (numBodies < 2) return;

    for (size_t b0 = 0; b0 < numBodies; ++b0) {
        for (size_t b1 = b0 + 1; b1 < numBodies; ++b1) {
            auto& bodyA = m_bodies[b0];
            auto& bodyB = m_bodies[b1];

            auto& nodesA = bodyA->getNodes();
            auto& nodesB = bodyB->getNodes();

            // Broad-phase distance check
            glm::dvec3 comA = bodyA->getCenterOfMass();
            glm::dvec3 comB = bodyB->getCenterOfMass();
            double distCOM = glm::distance(comA, comB);

            // Approximate bounding radius
            double maxDimA = 1000.0, maxDimB = 1000.0;
            if (!nodesA.empty()) maxDimA = glm::distance(nodesA[0].positionM, comA) * 2.5;
            if (!nodesB.empty()) maxDimB = glm::distance(nodesB[0].positionM, comB) * 2.5;

            if (distCOM > maxDimA + maxDimB) continue;

            // Narrow-phase node-node contact response
            for (auto& nA : nodesA) {
                for (auto& nB : nodesB) {
                    glm::dvec3 delta = nA.positionM - nB.positionM;
                    double d = glm::length(delta);
                    double minSeparation = 100.0; // Contact sphere radius

                    if (d < minSeparation && d > 1.0e-5) {
                        glm::dvec3 normal = delta / d;
                        double penetration = minSeparation - d;

                        double wA = nA.invMass;
                        double wB = nB.invMass;
                        double wSum = wA + wB;
                        if (wSum == 0.0) continue;

                        glm::dvec3 correction = normal * (penetration / wSum);
                        nA.positionM += wA * correction;
                        nB.positionM -= wB * correction;

                        // Contact velocity & impact heating
                        glm::dvec3 vRel = nA.velocityMps - nB.velocityMps;
                        double vNorm = glm::dot(vRel, normal);

                        if (vNorm < 0.0) {
                            double restitution = 0.25; // Inelastic collision
                            glm::dvec3 impulse = normal * (-(1.0 + restitution) * vNorm / wSum);
                            nA.velocityMps += impulse * wA;
                            nB.velocityMps -= impulse * wB;

                            // Convert impact energy loss to heat
                            double impactEnergyLoss = 0.5 * (nA.massKg * nB.massKg / (nA.massKg + nB.massKg)) * (1.0 - restitution * restitution) * (vNorm * vNorm);
                            double heatA = impactEnergyLoss * 0.5;
                            nA.temperatureK += heatA / (nA.massKg * bodyA->getMaterial().specificHeatJPerKgK);
                            nB.temperatureK += heatA / (nB.massKg * bodyB->getMaterial().specificHeatJPerKgK);
                        }
                    }
                }
            }
        }
    }
}

void MatterSystem::updateDiagnostics() {
    m_diagnostics.totalDeformableBodies = (int)m_bodies.size();
    m_diagnostics.totalNodes = 0;
    m_diagnostics.totalConstraints = 0;
    m_diagnostics.totalBrokenConstraints = 0;

    m_diagnostics.totalMassKg = 0.0;
    m_diagnostics.totalLinearMomentum = glm::dvec3(0.0);
    m_diagnostics.totalAngularMomentum = glm::dvec3(0.0);

    m_diagnostics.kineticEnergyJ = 0.0;
    m_diagnostics.elasticPotentialEnergyJ = 0.0;
    m_diagnostics.thermalInternalEnergyJ = 0.0;

    m_diagnostics.maxVonMisesStressPa = 0.0;
    m_diagnostics.maxTemperatureK = 0.0;
    m_diagnostics.maxDamage = 0.0;

    for (const auto& body : m_bodies) {
        m_diagnostics.totalNodes += (int)body->getNodes().size();
        m_diagnostics.totalConstraints += (int)body->getConstraints().size();
        m_diagnostics.totalBrokenConstraints += body->getBrokenConstraintCount();

        m_diagnostics.totalMassKg += body->getTotalMass();
        m_diagnostics.totalLinearMomentum += body->getLinearMomentum();
        m_diagnostics.totalAngularMomentum += body->getAngularMomentum();

        m_diagnostics.kineticEnergyJ += body->getKineticEnergy();
        m_diagnostics.elasticPotentialEnergyJ += body->getElasticPotentialEnergy();
        m_diagnostics.thermalInternalEnergyJ += body->getThermalInternalEnergy();

        m_diagnostics.maxVonMisesStressPa = std::max(m_diagnostics.maxVonMisesStressPa, body->getMaxVonMisesStress());
        m_diagnostics.maxTemperatureK = std::max(m_diagnostics.maxTemperatureK, body->getMeanTemperature());
        m_diagnostics.maxDamage = std::max(m_diagnostics.maxDamage, body->getMaxDamage());
    }

    m_diagnostics.totalEnergyJ = m_diagnostics.kineticEnergyJ +
                                 m_diagnostics.elasticPotentialEnergyJ +
                                 m_diagnostics.thermalInternalEnergyJ;

    if (m_diagnostics.initialTotalEnergyJ == 0.0 && m_diagnostics.totalEnergyJ != 0.0) {
        m_diagnostics.initialTotalEnergyJ = m_diagnostics.totalEnergyJ;
    }

    if (std::abs(m_diagnostics.initialTotalEnergyJ) > 1.0e-5) {
        m_diagnostics.energyConservationDriftPct = (std::abs(m_diagnostics.totalEnergyJ - m_diagnostics.initialTotalEnergyJ) /
                                                    std::abs(m_diagnostics.initialTotalEnergyJ)) * 100.0;
    }
}

void MatterSystem::updateNodeVisualizationScalars() {
    for (auto& body : m_bodies) {
        const auto& mat = body->getMaterial();
        auto& nodes = body->getNodes();

        for (auto& n : nodes) {
            float val = 0.0f;
            switch (m_visMode) {
                case MatterVisualizationMode::RealisticMaterial:
                    val = (float)n.temperatureK;
                    break;
                case MatterVisualizationMode::VonMisesStress:
                    val = (float)glm::clamp(n.vonMisesStressPa / std::max(1.0e6, mat.yieldStrengthPa * 1.5), 0.0, 1.0);
                    break;
                case MatterVisualizationMode::MechanicalStrain:
                    val = (float)glm::clamp(n.equivalentStrain / std::max(0.01, mat.failureStrain), 0.0, 1.0);
                    break;
                case MatterVisualizationMode::TemperatureHeatmap:
                    val = (float)glm::clamp((n.temperatureK - 100.0) / std::max(100.0, mat.meltingPointK * 1.2), 0.0, 1.0);
                    break;
                case MatterVisualizationMode::DamageAndFracture:
                    val = (float)glm::clamp(n.damage, 0.0, 1.0);
                    break;
                case MatterVisualizationMode::PlasticDeformation:
                    val = (float)glm::clamp(n.plasticStrain / 0.10, 0.0, 1.0);
                    break;
                case MatterVisualizationMode::TidalGravityVectors:
                    val = (float)glm::clamp(glm::length(n.externalForceN) / std::max(1.0, n.massKg * 50.0), 0.0, 1.0);
                    break;
            }
            n.scalarValue = val;
        }
    }
}

// =========================================================================
// Sandbox Scenario Presets
// =========================================================================

void MatterSystem::spawnBlackHoleTidalDisruptionLab(const glm::dvec3& originM) {
    clearAllBodies();

    const auto& rock = MaterialLibrary::instance().getMaterial("Basalt Rock");

    // Spawn an asteroid on a high-eccentricity close approach to Sol / Black Hole
    // Center at (0.05 AU, 0, 0) with periapsis orbital velocity
    double rMeters = 7.0e9; // 7,000,000 km (extreme tidal field)
    glm::dvec3 initialPos = originM + glm::dvec3(rMeters, 0.0, 0.0);
    glm::dvec3 initialVel(0.0, 0.0, 140000.0); // 140 km/s orbital speed

    auto asteroid = DeformableBody::createAsteroid(
        "Tidal_Disrupted_Asteroid",
        rock,
        initialPos,
        150000.0, // 150 km diameter
        6,
        initialVel,
        220.0
    );

    addBody(asteroid);
    setVisualizationMode(MatterVisualizationMode::VonMisesStress);
}

void MatterSystem::spawnHypervelocityCollision(const glm::dvec3& originM) {
    clearAllBodies();

    const auto& iron = MaterialLibrary::instance().getMaterial("Iron / Structural Steel");
    const auto& rock = MaterialLibrary::instance().getMaterial("Basalt Rock");

    // Projectile (Iron Sphere) flying right at +600 m/s
    auto projectile = DeformableBody::createSphere(
        "Iron_Impactor",
        iron,
        originM - glm::dvec3(8000.0, 0.0, 0.0),
        2500.0,
        5, 6,
        glm::dvec3(600.0, 0.0, 0.0),
        293.15
    );

    // Target (Basalt Rock Cube) flying left at -150 m/s
    auto target = DeformableBody::createCube(
        "Rock_Target",
        rock,
        originM + glm::dvec3(8000.0, 0.0, 0.0),
        7000.0,
        5,
        glm::dvec3(-150.0, 0.0, 0.0),
        293.15
    );

    addBody(projectile);
    addBody(target);
    setVisualizationMode(MatterVisualizationMode::DamageAndFracture);
}

void MatterSystem::spawnTensileTest(const glm::dvec3& originM) {
    clearAllBodies();

    const auto& ti = MaterialLibrary::instance().getMaterial("Titanium Alloy");

    // Elongated rectangular test specimen
    auto bar = DeformableBody::createCube(
        "Tensile_Specimen",
        ti,
        originM,
        4000.0,
        4,
        glm::dvec3(0.0),
        293.15
    );

    // Anchor left boundary nodes
    auto& nodes = bar->getNodes();
    for (size_t i = 0; i < nodes.size(); ++i) {
        if (nodes[i].restPositionM.x < -1800.0) {
            bar->setFixedAnchor((int)i, true);
        } else if (nodes[i].restPositionM.x > 1800.0) {
            // Apply tensile pulling velocity to right boundary
            nodes[i].velocityMps = glm::dvec3(120.0, 0.0, 0.0);
        }
    }

    addBody(bar);
    setVisualizationMode(MatterVisualizationMode::VonMisesStress);
}

void MatterSystem::spawnThermalMeltingLab(const glm::dvec3& originM) {
    clearAllBodies();

    const auto& ice = MaterialLibrary::instance().getMaterial("Water Ice");

    auto iceBlock = DeformableBody::createCube(
        "Melting_Ice_Block",
        ice,
        originM,
        5000.0,
        4,
        glm::dvec3(0.0),
        250.0 // -23 °C
    );

    // Inject high initial heat on one side (320 K / 47 °C) to demonstrate phase boundary propagation
    auto& nodes = iceBlock->getNodes();
    for (auto& n : nodes) {
        if (n.restPositionM.x > 1000.0) {
            n.temperatureK = 330.0;
        }
    }

    addBody(iceBlock);
    setVisualizationMode(MatterVisualizationMode::TemperatureHeatmap);
}

} // namespace AstroGenesis
