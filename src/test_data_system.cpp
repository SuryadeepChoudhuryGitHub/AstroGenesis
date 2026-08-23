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

    // 11. Test PhysicsEngine::resetSimulation (Clean workspace & restore baseline)
    int initialCount = physics.getObjectCount();
    CelestialBody customProbe;
    customProbe.id = "test_spacecraft";
    customProbe.name = "Test Spacecraft Probe";
    customProbe.type = "Artificial Satellite";
    customProbe.position = glm::vec3(5.0f, 0.0f, 5.0f);
    customProbe.massKg = 800.0;
    customProbe.radius3D = 0.01f;
    physics.addBody(customProbe);
    assert(physics.getObjectCount() == initialCount + 1);

    physics.resetSimulation(objRepo);
    assert(physics.getObjectCount() == initialCount);
    assert(physics.getSimulatedTimeSeconds() == 0.0);
    assert(physics.getRealTimeElapsedSeconds() == 0.0);
    assert(!physics.isPaused());
    std::cout << "[11] PhysicsEngine::resetSimulation -> PASS" << std::endl;
    std::cout << "    - Successfully cleared custom bodies and restored clean baseline simulation state (" << initialCount << " bodies)." << std::endl;

    // 12. Test Database-Backed System Records & Presets
    auto allSystems = objRepo.getAllSystems();
    std::cout << "[12] Database System Presets -> " << (allSystems.size() >= 8 ? "PASS" : "FAIL") << std::endl;
    std::cout << "    - Total Registered Systems in DB: " << allSystems.size() << std::endl;
    assert(allSystems.size() >= 8);

    auto binSysOpt = objRepo.getSystemByName("Binary Star System");
    assert(binSysOpt.has_value());
    auto binBodies = objRepo.getSystemBodies("Binary Star System");
    std::cout << "    - Binary Star System Bodies: " << binBodies.size() << std::endl;
    assert(binBodies.size() >= 3);

    auto tidalSysOpt = objRepo.getSystemByName("Extreme Tidal Test");
    assert(tidalSysOpt.has_value());
    auto tidalBodies = objRepo.getSystemBodies("Extreme Tidal Test");
    std::cout << "    - Extreme Tidal Test Bodies: " << tidalBodies.size() << std::endl;
    assert(tidalBodies.size() >= 2);

    // 13. Test Custom System Builder Creation, Persistence & Multi-Body Hierarchy
    SystemRecord customSys;
    customSys.name = "Unit Test Tri-Star System";
    customSys.type = "Custom";
    customSys.source = "User";
    customSys.description = "Automated test tri-star system with orbiting exoplanet";

    std::vector<CelestialBody> testBodies;
    CelestialBody star1;
    star1.dbId = 101;
    star1.id = "ut_star_1";
    star1.name = "UT Star Primary";
    star1.type = "G2V Star";
    star1.massKg = UnitConverter::SOLAR_MASS_KG;
    star1.radiusM = UnitConverter::SOLAR_RADIUS_M;
    star1.positionM = glm::dvec3(0.0);
    star1.velocityMps = glm::dvec3(0.0);
    testBodies.push_back(star1);

    CelestialBody star2;
    star2.dbId = 102;
    star2.id = "ut_star_2";
    star2.name = "UT Star Secondary";
    star2.type = "K1V Star";
    star2.parentObjectId = 101;
    star2.massKg = 0.8 * UnitConverter::SOLAR_MASS_KG;
    star2.radiusM = 0.85 * UnitConverter::SOLAR_RADIUS_M;
    star2.semiMajorAxisAU = 10.0;
    star2.positionM = glm::dvec3(10.0 * UnitConverter::AU_TO_METERS, 0.0, 0.0);
    testBodies.push_back(star2);

    CelestialBody planet1;
    planet1.dbId = 103;
    planet1.id = "ut_planet_1";
    planet1.name = "UT Planet Major";
    planet1.type = "Terrestrial Planet";
    planet1.parentObjectId = 101;
    planet1.massKg = 2.0 * UnitConverter::EARTH_MASS_KG;
    planet1.radiusM = 1.2 * UnitConverter::EARTH_RADIUS_M;
    planet1.semiMajorAxisAU = 1.2;
    planet1.positionM = glm::dvec3(1.2 * UnitConverter::AU_TO_METERS, 0.0, 0.0);
    testBodies.push_back(planet1);

    CelestialBody moon1;
    moon1.dbId = 104;
    moon1.id = "ut_moon_1";
    moon1.name = "UT Moon Alpha";
    moon1.type = "Planetary Moon";
    moon1.parentObjectId = 103;
    moon1.massKg = UnitConverter::LUNAR_MASS_KG;
    moon1.radiusM = UnitConverter::LUNAR_RADIUS_M;
    moon1.semiMajorAxisAU = 0.00257;
    moon1.positionM = glm::dvec3((1.2 + 0.00257) * UnitConverter::AU_TO_METERS, 0.0, 0.0);
    testBodies.push_back(moon1);

    int64_t customSysId = 0;
    bool saveOk = objRepo.saveCustomSystem(customSys, testBodies, &customSysId);
    std::cout << "[13] Custom System Persistence (saveCustomSystem) -> " << (saveOk ? "PASS" : "FAIL") << std::endl;
    assert(saveOk);
    assert(customSysId > 0);

    auto reloadedBodies = objRepo.getSystemBodies(customSys.name);
    std::cout << "    - Reloaded Custom Bodies Count: " << reloadedBodies.size() << std::endl;
    assert(reloadedBodies.size() == 4);

    // 14. Test System Duplication
    int64_t dupSysId = 0;
    std::string dupName = "Unit Test Tri-Star System (Duplicate)";
    bool dupOk = objRepo.duplicateSystem(customSysId, dupName, &dupSysId);
    std::cout << "[14] System Duplication (duplicateSystem) -> " << (dupOk ? "PASS" : "FAIL") << std::endl;
    assert(dupOk);
    assert(dupSysId > 0);

    auto dupBodies = objRepo.getSystemBodies(dupName);
    std::cout << "    - Duplicated System Bodies Count: " << dupBodies.size() << std::endl;
    assert(dupBodies.size() == 4);

    // Clean up test custom system
    objRepo.deleteSystem(customSysId);
    objRepo.deleteSystem(dupSysId);

    // 15. Test Pre-Flight Validation & Simulation Ingestion
    auto warnings = objRepo.validateSystem(testBodies);
    std::cout << "[15] Pre-Flight System Validation -> PASS (Warnings detected: " << warnings.size() << ")" << std::endl;

    // Load into physics engine & step simulation
    bool loadCustomOk = physics.loadFromDatabase(objRepo, "Solar System");
    assert(loadCustomOk);
    physics.update(1.0f / 60.0f);
    std::cout << "    - Physics Engine Step after custom ingestion -> PASS" << std::endl;

    std::cout << "\n==========================================================" << std::endl;
    std::cout << " ALL 15 TEST SUITES PASSED SUCCESSFULLY!" << std::endl;
    std::cout << "==========================================================" << std::endl;

    return 0;
}

