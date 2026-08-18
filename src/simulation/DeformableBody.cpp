#include "simulation/DeformableBody.hpp"
#include <cmath>
#include <queue>
#include <unordered_set>
#include <algorithm>

namespace AstroGenesis {

static const double G_CONST = 6.67430e-11;
static const double C_LIGHT = 299792458.0;
static const double STEFAN_BOLTZMANN = 5.670374419e-8;

DeformableBody::DeformableBody(const std::string& name, const MaterialDefinition& material)
    : m_name(name), m_material(material) {}

DeformableBody::~DeformableBody() {}

// =========================================================================
// Geometry Factory Constructors
// =========================================================================

std::shared_ptr<DeformableBody> DeformableBody::createCube(
    const std::string& name,
    const MaterialDefinition& material,
    const glm::dvec3& centerM,
    double sizeM,
    int resolution,
    const glm::dvec3& initialVelMps,
    double initialTempK
) {
    auto body = std::make_shared<DeformableBody>(name, material);
    int N = std::max(2, resolution);
    double step = sizeM / (double)(N - 1);
    double totalMass = material.referenceDensityKgM3 * std::pow(sizeM, 3.0);
    int totalNodes = N * N * N;
    double nodeMass = totalMass / (double)totalNodes;

    body->m_nodes.reserve(totalNodes);
    body->m_isFixedNode.assign(totalNodes, false);

    // 1. Create 3D grid of nodes
    for (int z = 0; z < N; ++z) {
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                MaterialNode node;
                glm::dvec3 localPos(
                    ((double)x * step) - (sizeM * 0.5),
                    ((double)y * step) - (sizeM * 0.5),
                    ((double)z * step) - (sizeM * 0.5)
                );
                node.restPositionM = localPos;
                node.positionM = centerM + localPos;
                node.predictedPosM = node.positionM;
                node.velocityMps = initialVelMps;
                node.massKg = nodeMass;
                node.invMass = 1.0 / nodeMass;
                node.temperatureK = initialTempK;
                node.internalEnergyJ = nodeMass * material.specificHeatJPerKgK * initialTempK;
                node.damage = 0.0;
                node.phase = (initialTempK >= material.meltingPointK) ? MaterialPhase::LiquidMolten : MaterialPhase::Solid;
                body->m_nodes.push_back(node);
            }
        }
    }

    auto getNodeIdx = [N](int x, int y, int z) -> int {
        return z * N * N + y * N + x;
    };

    double crossArea = (step * step);

    // 2. Create structural, shear, and cross-diagonal spring constraints
    auto addConstraint = [&](int i, int j) {
        if (i == j || i < 0 || j < 0 || i >= totalNodes || j >= totalNodes) return;
        MaterialConstraint c;
        c.nodeA = i;
        c.nodeB = j;
        double dist = glm::distance(body->m_nodes[i].positionM, body->m_nodes[j].positionM);
        c.restLengthM = dist;
        c.initialLengthM = dist;
        c.crossSectionAreaM2 = crossArea;
        c.damage = 0.0;
        c.isBroken = false;
        body->m_constraints.push_back(c);
    };

    for (int z = 0; z < N; ++z) {
        for (int y = 0; y < N; ++y) {
            for (int x = 0; x < N; ++x) {
                int curr = getNodeIdx(x, y, z);
                // Structural (along axes)
                if (x + 1 < N) addConstraint(curr, getNodeIdx(x + 1, y, z));
                if (y + 1 < N) addConstraint(curr, getNodeIdx(x, y + 1, z));
                if (z + 1 < N) addConstraint(curr, getNodeIdx(x, y, z + 1));

                // Shear (face diagonals)
                if (x + 1 < N && y + 1 < N) addConstraint(curr, getNodeIdx(x + 1, y + 1, z));
                if (x + 1 < N && y - 1 >= 0) addConstraint(curr, getNodeIdx(x + 1, y - 1, z));
                if (x + 1 < N && z + 1 < N) addConstraint(curr, getNodeIdx(x + 1, y, z + 1));
                if (y + 1 < N && z + 1 < N) addConstraint(curr, getNodeIdx(x, y + 1, z + 1));

                // Volumetric (body diagonals)
                if (x + 1 < N && y + 1 < N && z + 1 < N) addConstraint(curr, getNodeIdx(x + 1, y + 1, z + 1));
                if (x + 1 < N && y + 1 < N && z - 1 >= 0) addConstraint(curr, getNodeIdx(x + 1, y + 1, z - 1));
            }
        }
    }

    body->rebuildSurfaceMesh();
    return body;
}

std::shared_ptr<DeformableBody> DeformableBody::createSphere(
    const std::string& name,
    const MaterialDefinition& material,
    const glm::dvec3& centerM,
    double radiusM,
    int rings,
    int sectors,
    const glm::dvec3& initialVelMps,
    double initialTempK
) {
    auto body = std::make_shared<DeformableBody>(name, material);
    double volume = (4.0 / 3.0) * 3.1415926535 * std::pow(radiusM, 3.0);
    double totalMass = material.referenceDensityKgM3 * volume;

    // Center core node
    MaterialNode core;
    core.restPositionM = glm::dvec3(0.0);
    core.positionM = centerM;
    core.predictedPosM = centerM;
    core.velocityMps = initialVelMps;
    core.temperatureK = initialTempK;
    body->m_nodes.push_back(core);

    // Surface rings
    for (int r = 1; r <= rings; ++r) {
        double phi = 3.1415926535 * (double)r / (double)(rings + 1);
        double layerRadius = radiusM * (double)r / (double)rings;

        for (int s = 0; s < sectors; ++s) {
            double theta = 2.0 * 3.1415926535 * (double)s / (double)sectors;
            glm::dvec3 localPos(
                layerRadius * std::sin(phi) * std::cos(theta),
                layerRadius * std::cos(phi),
                layerRadius * std::sin(phi) * std::sin(theta)
            );
            MaterialNode n;
            n.restPositionM = localPos;
            n.positionM = centerM + localPos;
            n.predictedPosM = n.positionM;
            n.velocityMps = initialVelMps;
            n.temperatureK = initialTempK;
            body->m_nodes.push_back(n);
        }
    }

    double nodeMass = totalMass / (double)body->m_nodes.size();
    for (auto& n : body->m_nodes) {
        n.massKg = nodeMass;
        n.invMass = 1.0 / nodeMass;
        n.internalEnergyJ = nodeMass * material.specificHeatJPerKgK * initialTempK;
        n.damage = 0.0;
    }

    body->m_isFixedNode.assign(body->m_nodes.size(), false);

    // Connect nodes with compliant constraints
    size_t numNodes = body->m_nodes.size();
    double maxConnDist = (radiusM * 2.0 / (double)rings) * 1.5;
    double crossArea = (radiusM / (double)rings) * (radiusM / (double)rings);

    for (size_t i = 0; i < numNodes; ++i) {
        for (size_t j = i + 1; j < numNodes; ++j) {
            double dist = glm::distance(body->m_nodes[i].positionM, body->m_nodes[j].positionM);
            if (dist <= maxConnDist) {
                MaterialConstraint c;
                c.nodeA = (int)i;
                c.nodeB = (int)j;
                c.restLengthM = dist;
                c.initialLengthM = dist;
                c.crossSectionAreaM2 = crossArea;
                body->m_constraints.push_back(c);
            }
        }
    }

    body->rebuildSurfaceMesh();
    return body;
}

std::shared_ptr<DeformableBody> DeformableBody::createAsteroid(
    const std::string& name,
    const MaterialDefinition& material,
    const glm::dvec3& centerM,
    double radiusM,
    int resolution,
    const glm::dvec3& initialVelMps,
    double initialTempK
) {
    auto body = createSphere(name, material, centerM, radiusM, resolution, resolution * 2, initialVelMps, initialTempK);

    // Apply procedural irregularity to rest positions
    for (size_t i = 1; i < body->m_nodes.size(); ++i) {
        auto& n = body->m_nodes[i];
        glm::dvec3 dir = glm::normalize(n.restPositionM);
        double noise = 0.85 + 0.30 * std::sin(dir.x * 5.0 + dir.y * 3.0 + dir.z * 4.0);
        n.restPositionM *= noise;
        n.positionM = centerM + n.restPositionM;
        n.predictedPosM = n.positionM;
    }

    // Recalculate rest lengths
    for (auto& c : body->m_constraints) {
        double dist = glm::distance(body->m_nodes[c.nodeA].positionM, body->m_nodes[c.nodeB].positionM);
        c.restLengthM = dist;
        c.initialLengthM = dist;
    }

    body->rebuildSurfaceMesh();
    return body;
}

void DeformableBody::rebuildSurfaceMesh() {
    m_surfaceTriangles.clear();
    if (m_nodes.size() < 3) return;

    // Fast convex hull / nearest neighbor surface triangulation
    for (size_t i = 0; i < m_constraints.size(); ++i) {
        if (m_constraints[i].isBroken) continue;
        int a = m_constraints[i].nodeA;
        int b = m_constraints[i].nodeB;

        for (size_t j = i + 1; j < m_constraints.size(); ++j) {
            if (m_constraints[j].isBroken) continue;
            int c1 = m_constraints[j].nodeA;
            int c2 = m_constraints[j].nodeB;

            int common = -1, other = -1;
            if (c1 == a || c1 == b) { common = c1; other = c2; }
            else if (c2 == a || c2 == b) { common = c2; other = c1; }

            if (common != -1 && other != a && other != b) {
                int third = (common == a) ? b : a;
                // Form a triangle (common, third, other)
                MeshTriangle tri;
                tri.i0 = common;
                tri.i1 = third;
                tri.i2 = other;

                glm::vec3 v0 = m_nodes[tri.i0].positionM;
                glm::vec3 v1 = m_nodes[tri.i1].positionM;
                glm::vec3 v2 = m_nodes[tri.i2].positionM;
                tri.normal = glm::normalize(glm::cross(v1 - v0, v2 - v0));

                m_surfaceTriangles.push_back(tri);
                if (m_surfaceTriangles.size() > 600) break;
            }
        }
        if (m_surfaceTriangles.size() > 600) break;
    }
}

void DeformableBody::setFixedAnchor(int nodeIndex, bool fixed) {
    if (nodeIndex >= 0 && nodeIndex < (int)m_nodes.size()) {
        m_isFixedNode[nodeIndex] = fixed;
        m_nodes[nodeIndex].invMass = fixed ? 0.0 : (1.0 / m_nodes[nodeIndex].massKg);
    }
}

// =========================================================================
// Physics Solvers: Differential Gravity, XPBD, Plasticity, Damage & Heat
// =========================================================================

void DeformableBody::computeExternalForces(const std::vector<CelestialBody>& attractors, bool enableGR) {
    size_t numNodes = m_nodes.size();
    size_t numAttractors = attractors.size();

    for (size_t i = 0; i < numNodes; ++i) {
        auto& node = m_nodes[i];
        if (node.invMass == 0.0) {
            node.externalForceN = glm::dvec3(0.0);
            continue;
        }

        glm::dvec3 totalAcc{0.0};
        glm::dvec3 nodePos = node.positionM;
        glm::dvec3 nodeVel = node.velocityMps;

        // Evaluate true differential gravitational acceleration at this node's exact 3D position
        for (size_t a = 0; a < numAttractors; ++a) {
            const auto& body = attractors[a];
            glm::dvec3 r_vec = body.positionM - nodePos;
            double r2 = glm::dot(r_vec, r_vec);
            double r = std::sqrt(r2);

            if (r < 100.0) continue;

            // Newtonian gravity
            double newtonCoeff = (G_CONST * body.massKg) / (r2 * r);
            glm::dvec3 acc = newtonCoeff * r_vec;

            // Einstein 1PN Post-Newtonian relativistic correction
            if (enableGR && body.id == "sol" && r > 1.0e6) {
                double c2 = C_LIGHT * C_LIGHT;
                double v2 = glm::dot(nodeVel, nodeVel);
                double rDotV = glm::dot(r_vec, nodeVel);
                double grFactor1 = (4.0 * G_CONST * body.massKg / r) - v2;
                glm::dvec3 gr1 = (G_CONST * body.massKg / (c2 * r2 * r)) * (grFactor1 * r_vec);
                glm::dvec3 gr2 = (4.0 * G_CONST * body.massKg / (c2 * r2 * r)) * (rDotV * nodeVel);
                acc += (gr1 + gr2);
            }

            totalAcc += acc;
        }

        node.externalForceN = node.massKg * totalAcc;
    }
}

void DeformableBody::subStepXPBD(double subDt, int constraintIterations) {
    if (m_nodes.empty() || subDt <= 0.0) return;

    size_t numNodes = m_nodes.size();

    // 1. Explicit Integration: Velocity & Predicted Position
    for (size_t i = 0; i < numNodes; ++i) {
        auto& node = m_nodes[i];
        if (node.invMass == 0.0) continue;

        glm::dvec3 acc = node.externalForceN * node.invMass;
        node.velocityMps += acc * subDt;
        node.predictedPosM = node.positionM + node.velocityMps * subDt;
    }

    // Reset constraint multipliers for this sub-step
    for (auto& c : m_constraints) {
        c.lambda = 0.0;
    }

    // 2. XPBD Constraint Projection Iterations
    for (int iter = 0; iter < constraintIterations; ++iter) {
        for (auto& c : m_constraints) {
            if (c.isBroken) continue;

            auto& nA = m_nodes[c.nodeA];
            auto& nB = m_nodes[c.nodeB];

            double wA = nA.invMass;
            double wB = nB.invMass;
            double wSum = wA + wB;
            if (wSum == 0.0) continue;

            glm::dvec3 delta = nA.predictedPosM - nB.predictedPosM;
            double currentDist = glm::length(delta);
            if (currentDist < 1.0e-7) continue;

            glm::dvec3 dir = delta / currentDist;
            double C = currentDist - c.restLengthM; // Constraint error

            // Coupled temperature & damage compliance
            double avgTemp = (nA.temperatureK + nB.temperatureK) * 0.5;
            double avgDamage = (c.damage + (nA.damage + nB.damage) * 0.5) * 0.5;

            double E_eff, G_eff, sigY_eff, rho_eff;
            MaterialPhase phase;
            MaterialModel::evaluateCoupledProperties(
                m_material, avgTemp, 101325.0, 0.0, avgDamage,
                E_eff, G_eff, sigY_eff, rho_eff, phase
            );

            double k_stiffness = (E_eff * c.crossSectionAreaM2) / std::max(1.0e-4, c.restLengthM);
            double alpha = 1.0 / (std::max(1.0e-5, k_stiffness) * (subDt * subDt)); // Inverse compliance

            // XPBD Lagrange Multiplier Update
            double deltaLambda = (-C - alpha * c.lambda) / (wSum + alpha);
            c.lambda += deltaLambda;

            glm::dvec3 correction = dir * deltaLambda;
            nA.predictedPosM += wA * correction;
            nB.predictedPosM -= wB * correction;
        }
    }

    // 3. Velocity Update & Position Finalization
    double invDt = 1.0 / subDt;
    for (size_t i = 0; i < numNodes; ++i) {
        auto& node = m_nodes[i];
        if (node.invMass == 0.0) continue;

        node.velocityMps = (node.predictedPosM - node.positionM) * invDt;
        node.positionM = node.predictedPosM;
    }
}

void DeformableBody::updatePlasticityAndDamage(double subDt) {
    for (auto& c : m_constraints) {
        if (c.isBroken) continue;

        auto& nA = m_nodes[c.nodeA];
        auto& nB = m_nodes[c.nodeB];

        double currentDist = glm::distance(nA.positionM, nB.positionM);
        double axialStrain = (currentDist - c.initialLengthM) / std::max(1.0e-4, c.initialLengthM);

        double avgTemp = (nA.temperatureK + nB.temperatureK) * 0.5;
        double E_eff, G_eff, sigY_eff, rho_eff;
        MaterialPhase phase;
        MaterialModel::evaluateCoupledProperties(
            m_material, avgTemp, 101325.0, 0.0, c.damage,
            E_eff, G_eff, sigY_eff, rho_eff, phase
        );

        double stressPa = E_eff * std::abs(currentDist - c.restLengthM) / std::max(1.0e-4, c.restLengthM);
        c.currentStressPa = stressPa;
        nA.vonMisesStressPa = std::max(nA.vonMisesStressPa, stressPa);
        nB.vonMisesStressPa = std::max(nB.vonMisesStressPa, stressPa);

        // 1. Plastic Flow: Permanent rest length change when stress exceeds yield
        if (stressPa > sigY_eff) {
            double overStress = stressPa - sigY_eff;
            double plasticFlow = (overStress / (E_eff + m_material.strainHardeningModulusPa)) * c.restLengthM;

            if (currentDist > c.restLengthM) {
                c.restLengthM += plasticFlow;
            } else {
                c.restLengthM -= plasticFlow;
            }

            c.plasticStretchM += std::abs(plasticFlow);
            nA.plasticStrain += std::abs(plasticFlow) / c.initialLengthM;
            nB.plasticStrain += std::abs(plasticFlow) / c.initialLengthM;

            // Inelastic dissipated work -> Thermal heat generation Delta T = W_p / (m * C_p)
            double plasticWorkJ = sigY_eff * std::abs(plasticFlow) * c.crossSectionAreaM2;
            double heatPerNode = plasticWorkJ * 0.5;

            double cp = std::max(1.0, m_material.specificHeatJPerKgK);
            nA.temperatureK += heatPerNode / (nA.massKg * cp);
            nB.temperatureK += heatPerNode / (nB.massKg * cp);
            nA.internalEnergyJ += heatPerNode;
            nB.internalEnergyJ += heatPerNode;
        }

        // 2. Continuous Damage Accumulation & Fracture
        if (std::abs(axialStrain) > m_material.failureStrain || stressPa > m_material.ultimateTensileStrengthPa) {
            double overRatio = std::max(
                std::abs(axialStrain) / std::max(1.0e-4, m_material.failureStrain),
                stressPa / std::max(1.0e-4, m_material.ultimateTensileStrengthPa)
            );
            double damageRate = (overRatio - 1.0) * 12.0 * subDt;
            c.damage = glm::clamp(c.damage + damageRate, 0.0, 1.0);
            nA.damage = glm::clamp(nA.damage + damageRate * 0.5, 0.0, 1.0);
            nB.damage = glm::clamp(nB.damage + damageRate * 0.5, 0.0, 1.0);

            // Fracture condition: when D >= 1.0, permanent severance occurs
            if (c.damage >= 1.0) {
                c.isBroken = true;
            }
        }
    }
}

void DeformableBody::updateThermalConductionAndRadiation(double subDt) {
    double k_th = m_material.thermalConductivityWPerMK;
    double cp = std::max(1.0, m_material.specificHeatJPerKgK);

    // 1. Node-to-node Fourier Conduction
    for (const auto& c : m_constraints) {
        if (c.isBroken) continue;

        auto& nA = m_nodes[c.nodeA];
        auto& nB = m_nodes[c.nodeB];

        double deltaT = nB.temperatureK - nA.temperatureK;
        double dist = std::max(1.0e-4, glm::distance(nA.positionM, nB.positionM));

        // Fourier's Law: dQ/dt = k * A * (T_B - T_A) / L
        double heatFluxRateWatts = k_th * c.crossSectionAreaM2 * (deltaT / dist);
        double heatTransferJ = heatFluxRateWatts * subDt;

        nA.temperatureK += heatTransferJ / (nA.massKg * cp);
        nB.temperatureK -= heatTransferJ / (nB.massKg * cp);
    }

    // 2. Stefan-Boltzmann Radiative Cooling into Deep Space (T_space = 2.7 K)
    for (auto& node : m_nodes) {
        double T = node.temperatureK;
        if (T > 10.0) {
            double nodeAreaM2 = std::pow(node.massKg / m_material.referenceDensityKgM3, 2.0 / 3.0);
            double radPowerWatts = m_material.emissivity * STEFAN_BOLTZMANN * nodeAreaM2 * (std::pow(T, 4.0) - std::pow(2.7, 4.0));
            double heatLossJ = radPowerWatts * subDt;
            node.temperatureK = std::max(2.7, node.temperatureK - (heatLossJ / (node.massKg * cp)));
        }

        // Phase update
        if (node.temperatureK >= m_material.meltingPointK) {
            node.phase = MaterialPhase::LiquidMolten;
        } else if (node.temperatureK >= 0.82 * m_material.meltingPointK) {
            node.phase = MaterialPhase::SoftenedPlastic;
        } else {
            node.phase = MaterialPhase::Solid;
        }
    }
}

// =========================================================================
// Topological Fracture & Multi-Body Fragmentation Engine
// =========================================================================

bool DeformableBody::checkAndPerformFragmentation(std::vector<std::shared_ptr<DeformableBody>>& outNewFragments) {
    size_t numNodes = m_nodes.size();
    if (numNodes < 4) return false;

    // 1. Build adjacency list of active unbroken constraints
    std::vector<std::vector<int>> adj(numNodes);
    for (const auto& c : m_constraints) {
        if (!c.isBroken) {
            adj[c.nodeA].push_back(c.nodeB);
            adj[c.nodeB].push_back(c.nodeA);
        }
    }

    // 2. Breadth-First Search (BFS) to find connected components
    std::vector<int> componentId(numNodes, -1);
    std::vector<std::vector<int>> components;

    for (size_t i = 0; i < numNodes; ++i) {
        if (componentId[i] != -1) continue;

        int currentCompId = (int)components.size();
        components.push_back({});
        std::queue<int> q;
        q.push((int)i);
        componentId[i] = currentCompId;

        while (!q.empty()) {
            int u = q.front();
            q.pop();
            components.back().push_back(u);

            for (int v : adj[u]) {
                if (componentId[v] == -1) {
                    componentId[v] = currentCompId;
                    q.push(v);
                }
            }
        }
    }

    // If all nodes remain in one connected component, no fragmentation occurred
    if (components.size() <= 1) return false;

    // 3. Fragmentation: Sort components by size (largest remains in this body)
    std::sort(components.begin(), components.end(), [](const std::vector<int>& a, const std::vector<int>& b) {
        return a.size() > b.size();
    });

    // Create child DeformableBody instances for secondary fragments
    for (size_t k = 1; k < components.size(); ++k) {
        const auto& compNodes = components[k];
        if (compNodes.empty()) continue;

        auto fragment = std::make_shared<DeformableBody>(
            m_name + "_Frag" + std::to_string(k),
            m_material
        );

        std::unordered_map<int, int> oldToNewMap;
        for (int oldIdx : compNodes) {
            oldToNewMap[oldIdx] = (int)fragment->m_nodes.size();
            fragment->m_nodes.push_back(m_nodes[oldIdx]);
            fragment->m_isFixedNode.push_back(m_isFixedNode[oldIdx]);
        }

        // Copy active constraints belonging to this fragment
        for (const auto& c : m_constraints) {
            if (c.isBroken) continue;
            auto itA = oldToNewMap.find(c.nodeA);
            auto itB = oldToNewMap.find(c.nodeB);
            if (itA != oldToNewMap.end() && itB != oldToNewMap.end()) {
                MaterialConstraint newC = c;
                newC.nodeA = itA->second;
                newC.nodeB = itB->second;
                fragment->m_constraints.push_back(newC);
            }
        }

        fragment->rebuildSurfaceMesh();
        fragment->setFragmented(true);
        outNewFragments.push_back(fragment);
    }

    // 4. Retain only largest component in this body
    const auto& primaryComp = components[0];
    std::vector<MaterialNode> newPrimaryNodes;
    std::vector<bool> newPrimaryFixed;
    std::unordered_map<int, int> oldToPrimaryMap;

    for (int oldIdx : primaryComp) {
        oldToPrimaryMap[oldIdx] = (int)newPrimaryNodes.size();
        newPrimaryNodes.push_back(m_nodes[oldIdx]);
        newPrimaryFixed.push_back(m_isFixedNode[oldIdx]);
    }

    std::vector<MaterialConstraint> newPrimaryConstraints;
    for (const auto& c : m_constraints) {
        if (c.isBroken) continue;
        auto itA = oldToPrimaryMap.find(c.nodeA);
        auto itB = oldToPrimaryMap.find(c.nodeB);
        if (itA != oldToPrimaryMap.end() && itB != oldToPrimaryMap.end()) {
            MaterialConstraint newC = c;
            newC.nodeA = itA->second;
            newC.nodeB = itB->second;
            newPrimaryConstraints.push_back(newC);
        }
    }

    m_nodes = std::move(newPrimaryNodes);
    m_isFixedNode = std::move(newPrimaryFixed);
    m_constraints = std::move(newPrimaryConstraints);
    rebuildSurfaceMesh();
    m_isFragmented = true;

    return true;
}

// =========================================================================
// Diagnostics & Conservation Measurements
// =========================================================================

glm::dvec3 DeformableBody::getCenterOfMass() const {
    glm::dvec3 com(0.0);
    double totalMass = 0.0;
    for (const auto& n : m_nodes) {
        com += n.positionM * n.massKg;
        totalMass += n.massKg;
    }
    return (totalMass > 0.0) ? (com / totalMass) : glm::dvec3(0.0);
}

glm::dvec3 DeformableBody::getLinearMomentum() const {
    glm::dvec3 p(0.0);
    for (const auto& n : m_nodes) {
        p += n.velocityMps * n.massKg;
    }
    return p;
}

glm::dvec3 DeformableBody::getAngularMomentum() const {
    glm::dvec3 com = getCenterOfMass();
    glm::dvec3 L(0.0);
    for (const auto& n : m_nodes) {
        glm::dvec3 r = n.positionM - com;
        L += glm::cross(r, n.velocityMps * n.massKg);
    }
    return L;
}

double DeformableBody::getTotalMass() const {
    double m = 0.0;
    for (const auto& n : m_nodes) m += n.massKg;
    return m;
}

double DeformableBody::getKineticEnergy() const {
    double ke = 0.0;
    for (const auto& n : m_nodes) {
        ke += 0.5 * n.massKg * glm::dot(n.velocityMps, n.velocityMps);
    }
    return ke;
}

double DeformableBody::getElasticPotentialEnergy() const {
    double pe = 0.0;
    for (const auto& c : m_constraints) {
        if (c.isBroken) continue;
        double currentDist = glm::distance(m_nodes[c.nodeA].positionM, m_nodes[c.nodeB].positionM);
        double deltaL = currentDist - c.restLengthM;
        double k = (m_material.youngsModulusPa * c.crossSectionAreaM2) / std::max(1.0e-4, c.restLengthM);
        pe += 0.5 * k * (deltaL * deltaL);
    }
    return pe;
}

double DeformableBody::getThermalInternalEnergy() const {
    double u = 0.0;
    for (const auto& n : m_nodes) u += n.internalEnergyJ;
    return u;
}

double DeformableBody::getMaxVonMisesStress() const {
    double maxStress = 0.0;
    for (const auto& n : m_nodes) maxStress = std::max(maxStress, n.vonMisesStressPa);
    return maxStress;
}

double DeformableBody::getMeanTemperature() const {
    if (m_nodes.empty()) return 293.15;
    double sumT = 0.0;
    for (const auto& n : m_nodes) sumT += n.temperatureK;
    return sumT / (double)m_nodes.size();
}

double DeformableBody::getMaxDamage() const {
    double maxD = 0.0;
    for (const auto& n : m_nodes) maxD = std::max(maxD, n.damage);
    return maxD;
}

int DeformableBody::getActiveConstraintCount() const {
    int active = 0;
    for (const auto& c : m_constraints) if (!c.isBroken) active++;
    return active;
}

int DeformableBody::getBrokenConstraintCount() const {
    int broken = 0;
    for (const auto& c : m_constraints) if (c.isBroken) broken++;
    return broken;
}

} // namespace AstroGenesis
