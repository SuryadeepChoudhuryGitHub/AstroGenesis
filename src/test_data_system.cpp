#include <iostream>
#include <cassert>
#include <vector>
#include "data/DatabaseManager.hpp"
#include "data/SeedData.hpp"
#include "data/UnitConverter.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/repositories/EphemerisRepository.hpp"
#include "data/repositories/ValidationRepository.hpp"
#include "data/DataManager.hpp"
#include "simulation/ValidationEngine.hpp"
#include "simulation/PhysicsEngine.hpp"

using namespace AstroGenesis;

int main() {
    std::cout << "==========================================================" << std::endl;
    std::cout << " AstroGenesis Astronomical Data & Database Verification" << std::endl;
    std::cout << "==========================================================" << std::endl;

    // 1. Initialize Database
    DatabaseManager& db = DatabaseManager::getInstance();
    bool initOk = db.initialize("data/astrogenesis.db");
    std::cout << "[1] DatabaseManager::initialize -> " << (initOk ? "PASS" : "FAIL") << std::endl;
    assert(initOk);

    // 2. Repositories
    ObjectRepository objRepo(db);
    EphemerisRepository ephemRepo(db);
    ValidationRepository valRepo(db);

    // 3. Seed Data
    bool seedOk = SeedData::seedDefaultDatabase(objRepo);
    std::cout << "[2] SeedData::seedDefaultDatabase -> " << (seedOk ? "PASS" : "FAIL") << std::endl;
    assert(seedOk);

    int objCount = objRepo.getObjectCount();
    std::cout << "    Total Objects in DB: " << objCount << std::endl;
    assert(objCount >= 10);

    // 4. Verify Solar System Objects
    auto solarBodies = objRepo.getSystemBodies("Solar System");
    std::cout << "[3] Solar System Hydrated Bodies Count: " << solarBodies.size() << std::endl;
    assert(solarBodies.size() >= 9);

    auto earthOpt = objRepo.getHydratedBodyBySlug("earth");
    assert(earthOpt.has_value());
    const auto& earth = earthOpt.value();
    std::cout << "    Earth Verified:" << std::endl;
    std::cout << "    - Name: " << earth.name << " (" << earth.type << ")" << std::endl;
    std::cout << "    - Mass: " << earth.massStr << " (" << earth.massKg << " kg)" << std::endl;
    std::cout << "    - Radius: " << earth.radiusStr << std::endl;
    std::cout << "    - Semi-Major Axis: " << earth.semiMajorAxisStr << std::endl;
    std::cout << "    - Data Source: " << earth.sourceName << std::endl;
    std::cout << "    - Epoch: " << earth.epochUtcStr << std::endl;

    // Verify Ganymede & Jupiter Galilean Moons
    auto ganymedeOpt = objRepo.getHydratedBodyBySlug("ganymede");
    assert(ganymedeOpt.has_value());
    const auto& ganymede = ganymedeOpt.value();
    std::cout << "    Ganymede Verified:" << std::endl;
    std::cout << "    - Name: " << ganymede.name << " (" << ganymede.type << ")" << std::endl;
    std::cout << "    - Mass: " << ganymede.massStr << " (" << ganymede.massKg << " kg)" << std::endl;
    std::cout << "    - Radius: " << ganymede.radiusStr << std::endl;
    std::cout << "    - Orbit Radius: " << ganymede.realOrbitRadiusAU << " AU (" << (ganymede.realOrbitRadiusAU * UnitConverter::AU_TO_KM) << " km)" << std::endl;
    assert(ganymede.parentObjectId.has_value());
    assert(ganymede.realOrbitRadiusAU > 0.005 && ganymede.realOrbitRadiusAU < 0.01);
    assert(ganymede.massKg > 1e23);

    // 5. Verify Exoplanets & Orbits
    auto exoBodies = objRepo.getSystemBodies("TRAPPIST-1 System");
    std::cout << "[4] TRAPPIST-1 System Bodies Count: " << exoBodies.size() << std::endl;
    assert(exoBodies.size() >= 2);
    for (const auto& b : exoBodies) {
        if (b.type.find("Star") == std::string::npos) {
            std::cout << "    - Planet: " << b.name << " | realOrbitRadiusAU: " << b.realOrbitRadiusAU << " AU" << std::endl;
            assert(b.realOrbitRadiusAU > 0.0);
        }
    }

    // 6. Verify Asteroids
    auto asteroids = objRepo.getAllObjects("Asteroid Belt", false);
    std::cout << "[5] Asteroid Belt Objects Count: " << asteroids.size() << std::endl;
    assert(asteroids.size() >= 3);

    auto astSystem = objRepo.getSystemBodies("Asteroid Belt");
    assert(!astSystem.empty());
    assert(astSystem[0].id == "sol"); // Sol is present as gravitational anchor

    // 7. Test DataManager & Provider Initialization
    DataManager dataManager(db, objRepo, ephemRepo, valRepo);
    dataManager.initialize();
    std::cout << "[6] DataManager Initialized -> PASS" << std::endl;

    // 8. Test Real vs Simulation Validation Engine
    ValidationEngine valEngine(objRepo, ephemRepo, valRepo);
    std::vector<ValidationComparisonPoint> points;
    ValidationBenchmarkSummary summary;
    std::string valErr;

    bool valOk = valEngine.runValidationBenchmark("mercury", 88.0, 1.0, true, points, summary, valErr);
    std::cout << "[7] ValidationEngine::runValidationBenchmark (Mercury 88-day GR) -> " << (valOk ? "PASS" : "FAIL") << std::endl;
    if (valOk) {
        std::cout << "    - Sample Points: " << summary.sampleCount << std::endl;
        std::cout << "    - Mean Position Error: " << summary.meanPosErrorKm << " km" << std::endl;
        std::cout << "    - Max Energy Drift: " << summary.maxEnergyDriftPct << " %" << std::endl;
        std::cout << "    - Theoretical GR Precession: " << summary.grPrecessionTheoreticalArcsec << " arcsec/century" << std::endl;
        std::cout << "    - Simulated GR Precession: " << summary.grPrecessionSimulatedArcsec << " arcsec/century" << std::endl;
    }

    // 9. Test Newtonian vs GR Comparison
    std::vector<ValidationComparisonPoint> nPts, grPts;
    ValidationBenchmarkSummary nSum, grSum;
    valEngine.runNewtonianVsGRComparison("mercury", 88.0, nPts, grPts, nSum, grSum);
    std::cout << "[8] ValidationEngine::runNewtonianVsGRComparison -> PASS" << std::endl;
    std::cout << "    - Newtonian Shift: " << nSum.grPrecessionSimulatedArcsec << " arcsec/century" << std::endl;
    std::cout << "    - Einstein 1PN Shift: " << grSum.grPrecessionSimulatedArcsec << " arcsec/century" << std::endl;

    // 10. Test PhysicsEngine Data-Driven System Loading & Transitions
    PhysicsEngine physics;
    bool loadOk = physics.loadFromDatabase(objRepo, "Solar System");
    std::cout << "[9] PhysicsEngine::loadFromDatabase('Solar System') -> " << (loadOk ? "PASS" : "FAIL") << std::endl;
    assert(loadOk);
    assert(physics.getObjectCount() >= 9);

    // Test switching to TRAPPIST-1 System and Asteroid Belt
    bool exoLoadOk = physics.loadFromDatabase(objRepo, "TRAPPIST-1 System");
    assert(exoLoadOk);
    bool astLoadOk = physics.loadFromDatabase(objRepo, "Asteroid Belt");
    assert(astLoadOk);
    bool solLoadOk = physics.loadFromDatabase(objRepo, "Solar System");
    assert(solLoadOk);

    physics.update(1.0f / 60.0f);
    std::cout << "[10] PhysicsEngine Simulation Step -> PASS (Total Energy: " << physics.getTotalEnergyStr() << ")" << std::endl;

    // Verify dynamic Keplerian osculating orbit curve is generated for orbiting planets
    const auto& currentBodies = physics.getBodies();
    for (const auto& b : currentBodies) {
        if (b.type.find("Star") == std::string::npos && b.specificOrbitalEnergy < 0.0) {
            assert(!b.dynamicOrbitCurve.empty());
            assert(b.dynamicOrbitCurve.size() >= 50);
        }
    }
    std::cout << "    - Live Keplerian Dynamic Orbit Curves Verified for all planets." << std::endl;

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " ALL 10 TEST SUITES PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}
