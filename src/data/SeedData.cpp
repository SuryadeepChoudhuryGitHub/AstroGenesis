#include "data/SeedData.hpp"
#include "data/UnitConverter.hpp"
#include <cmath>
#include <iostream>

namespace AstroGenesis {

bool SeedData::isDatabaseSeeded(ObjectRepository& repo) {
    return repo.getObjectCount() >= 8;
}

bool SeedData::seedDefaultDatabase(ObjectRepository& repo) {
    std::cout << "[SeedData] Seeding database with high-precision baseline astronomical datasets..." << std::endl;

    const std::string SOURCE_NAME = "Bundled Seed Dataset (NASA/JPL baseline)";

    // 1. Sol (Sun)
    CelestialBodyRecord sol;
    sol.sourceName = SOURCE_NAME;
    sol.object.slug = "sol";
    sol.object.name = "Sol";
    sol.object.type = "G2V Star";
    sol.object.category = "Solar System";
    sol.object.color = glm::vec3(1.0f, 0.75f, 0.2f);
    sol.object.texturePath = "";
    sol.physical.massKg = 1.9885e30;
    sol.physical.radiusM = 696340000.0;
    sol.physical.luminosityW = UnitConverter::SOLAR_LUMINOSITY_W;
    sol.physical.albedo = 0.0;
    sol.physical.greenhouseK = 0.0;
    sol.physical.axialTiltDeg = 7.25;
    sol.physical.rotationPeriodHours = 609.12;
    sol.physical.atmosphereSummary = "73.46% H₂, 24.85% He";
    sol.physical.magneticFieldStr = "100–300 µT";
    sol.physical.surfaceGravityMps2 = 274.0;
    sol.physical.escapeVelocityMps = 617700.0;
    sol.physical.surfaceTempK = 5778.0;
    sol.physical.sourceRecordId = "10";
    sol.stateVector.positionM = glm::dvec3(0.0);
    sol.stateVector.velocityMps = glm::dvec3(0.0);
    sol.composition = {
        { 0, 0, "Hydrogen", 73.46f, {1.0f, 0.8f, 0.2f, 1.0f} },
        { 0, 0, "Helium", 24.85f, {0.9f, 0.5f, 0.1f, 1.0f} },
        { 0, 0, "Oxygen", 0.77f, {0.2f, 0.8f, 0.4f, 1.0f} },
        { 0, 0, "Carbon", 0.29f, {0.5f, 0.5f, 0.5f, 1.0f} }
    };
    int64_t solId = 0;
    repo.saveCelestialBodyRecord(sol, &solId);

    struct BodySeedResult {
        int64_t id = 0;
        glm::dvec3 posM{0.0};
        glm::dvec3 velMps{0.0};
        double massKg = 0.0;
    };

    // Lambda helper for planet insertion
    auto addPlanet = [&](const std::string& slug, const std::string& name, const std::string& type,
                         double massKg, double radiusM, double semiMajorAU, double eccentricity,
                         double inclinationDeg, double longAscNodeDeg, double argPeriapsisDeg, double meanAnomalyDeg,
                         double albedo, double greenhouseK,
                         double axialTiltDeg, double rotPeriodHours,
                         const std::string& atmStr, const std::string& pressStr,
                         const std::string& magStr, const std::string& sourceRecId,
                         const std::vector<CompositionRecord>& comp,
                         const glm::vec3& color, const std::string& texPath,
                         const std::string& ringsJson = "",
                         std::optional<int64_t> parentId = std::nullopt,
                         const std::string& category = "Solar System") -> BodySeedResult {
        CelestialBodyRecord b;
        b.sourceName = SOURCE_NAME;
        b.object.slug = slug;
        b.object.name = name;
        b.object.type = type;
        b.object.category = category;
        b.object.color = color;
        b.object.texturePath = texPath;
        b.object.parentObjectId = parentId.has_value() ? parentId : std::optional<int64_t>(solId);

        b.physical.massKg = massKg;
        b.physical.radiusM = radiusM;
        b.physical.albedo = albedo;
        b.physical.greenhouseK = greenhouseK;
        b.physical.axialTiltDeg = axialTiltDeg;
        b.physical.rotationPeriodHours = rotPeriodHours;
        b.physical.atmosphereSummary = atmStr;
        b.physical.magneticFieldStr = magStr;
        b.physical.sourceRecordId = sourceRecId;
        if (!ringsJson.empty()) b.physical.ringsJson = ringsJson;

        // Calculate surface gravity and escape velocity
        if (radiusM > 0.0) {
            b.physical.surfaceGravityMps2 = (UnitConverter::G_CONST * massKg) / (radiusM * radiusM);
            b.physical.escapeVelocityMps = std::sqrt(2.0 * UnitConverter::G_CONST * massKg / radiusM);
            double volM3 = (4.0 / 3.0) * UnitConverter::PI * std::pow(radiusM, 3.0);
            b.physical.meanDensityKgM3 = massKg / volM3;
        }

        b.orbital.epochJd = UnitConverter::J2000_JD;
        b.orbital.semiMajorAxisAU = semiMajorAU;
        double aM = semiMajorAU * UnitConverter::AU_TO_METERS;
        b.orbital.semiMajorAxisM = aM;
        b.orbital.eccentricity = eccentricity;
        b.orbital.inclinationDeg = inclinationDeg;
        b.orbital.longAscendingNodeDeg = longAscNodeDeg;
        b.orbital.argPeriapsisDeg = argPeriapsisDeg;
        b.orbital.meanAnomalyDeg = meanAnomalyDeg;
        b.orbital.orbitalPeriodDays = (2.0 * UnitConverter::PI * std::sqrt(std::pow(aM, 3.0) / (UnitConverter::G_CONST * 1.9885e30))) / UnitConverter::SEC_PER_DAY;

        // True 3D Cartesian State Vector from NASA/JPL Keplerian Elements
        b.stateVector.epochJd = UnitConverter::J2000_JD;
        UnitConverter::keplerianToCartesian(
            aM, eccentricity, inclinationDeg, longAscNodeDeg, argPeriapsisDeg, meanAnomalyDeg,
            1.9885e30, massKg,
            b.stateVector.positionM.x, b.stateVector.positionM.y, b.stateVector.positionM.z,
            b.stateVector.velocityMps.x, b.stateVector.velocityMps.y, b.stateVector.velocityMps.z
        );
        b.stateVector.referenceFrame = "ICRF/Ecliptic_J2000";

        b.composition = comp;
        int64_t outId = 0;
        repo.saveCelestialBodyRecord(b, &outId);
        return { outId, b.stateVector.positionM, b.stateVector.velocityMps, massKg };
    };

    // Lambda helper for moon insertion (parent-relative orbits with true physical quantities)
    auto addMoon = [&](const std::string& slug, const std::string& name,
                       double massKg, double radiusM, double semiMajorKm, double eccentricity,
                       double inclinationDeg, double longAscNodeDeg, double argPeriapsisDeg, double meanAnomalyDeg,
                       double albedo, double greenhouseK,
                       double axialTiltDeg, double rotPeriodHours,
                       const std::string& atmStr, const std::string& pressStr,
                       const std::string& magStr, const std::string& sourceRecId,
                       const std::vector<CompositionRecord>& comp,
                       const glm::vec3& color, const std::string& texPath,
                       const BodySeedResult& parent) -> BodySeedResult {
        CelestialBodyRecord b;
        b.sourceName = SOURCE_NAME;
        b.object.slug = slug;
        b.object.name = name;
        b.object.type = "Natural Satellite (Moon)";
        b.object.category = "Solar System";
        b.object.color = color;
        b.object.texturePath = texPath;
        b.object.parentObjectId = parent.id;

        b.physical.massKg = massKg;
        b.physical.radiusM = radiusM;
        b.physical.albedo = albedo;
        b.physical.greenhouseK = greenhouseK;
        b.physical.axialTiltDeg = axialTiltDeg;
        b.physical.rotationPeriodHours = rotPeriodHours;
        b.physical.atmosphereSummary = atmStr;
        b.physical.magneticFieldStr = magStr;
        b.physical.sourceRecordId = sourceRecId;

        if (radiusM > 0.0) {
            b.physical.surfaceGravityMps2 = (UnitConverter::G_CONST * massKg) / (radiusM * radiusM);
            b.physical.escapeVelocityMps = std::sqrt(2.0 * UnitConverter::G_CONST * massKg / radiusM);
            double volM3 = (4.0 / 3.0) * UnitConverter::PI * std::pow(radiusM, 3.0);
            b.physical.meanDensityKgM3 = massKg / volM3;
        }

        double aM = semiMajorKm * 1000.0;
        b.orbital.epochJd = UnitConverter::J2000_JD;
        b.orbital.semiMajorAxisM = aM;
        b.orbital.semiMajorAxisAU = aM / UnitConverter::AU_TO_METERS;
        b.orbital.eccentricity = eccentricity;
        b.orbital.inclinationDeg = inclinationDeg;
        b.orbital.longAscendingNodeDeg = longAscNodeDeg;
        b.orbital.argPeriapsisDeg = argPeriapsisDeg;
        b.orbital.meanAnomalyDeg = meanAnomalyDeg;
        b.orbital.orbitalPeriodDays = (2.0 * UnitConverter::PI * std::sqrt(std::pow(aM, 3.0) / (UnitConverter::G_CONST * (parent.massKg + massKg)))) / UnitConverter::SEC_PER_DAY;

        glm::dvec3 relPosM(0.0), relVelMps(0.0);
        UnitConverter::keplerianToCartesian(
            aM, eccentricity, inclinationDeg, longAscNodeDeg, argPeriapsisDeg, meanAnomalyDeg,
            parent.massKg, massKg,
            relPosM.x, relPosM.y, relPosM.z,
            relVelMps.x, relVelMps.y, relVelMps.z
        );

        b.stateVector.positionM = parent.posM + relPosM;
        b.stateVector.velocityMps = parent.velMps + relVelMps;
        b.stateVector.epochJd = UnitConverter::J2000_JD;
        b.stateVector.referenceFrame = "ICRF/Ecliptic_J2000";
        b.composition = comp;

        int64_t outId = 0;
        repo.saveCelestialBodyRecord(b, &outId);
        return { outId, b.stateVector.positionM, b.stateVector.velocityMps, massKg };
    };

    // 2. Mercury (NASA JPL DE440 ephemerides at J2000)
    auto mercuryRes = addPlanet("mercury", "Mercury", "Terrestrial Planet",
              3.3011e23, 2439700.0, 0.387098, 0.205630,
              7.0049, 48.331, 29.124, 174.796,
              0.088, 0.0, 0.034, 1407.5,
              "42% O₂, 29% Na, 22% H₂", "10⁻¹⁴ kPa", "0.3 µT", "199",
              { {0,0,"Oxygen",42.0f,{0.2f,0.8f,0.4f,1.0f}}, {0,0,"Sodium",29.0f,{0.9f,0.6f,0.1f,1.0f}}, {0,0,"Hydrogen",22.0f,{0.4f,0.7f,1.0f,1.0f}} },
              glm::vec3(0.7f, 0.6f, 0.5f), "assets/textures/mercury_surface.jpg");

    // 3. Venus
    auto venusRes = addPlanet("venus", "Venus", "Terrestrial Planet",
              4.8675e24, 6051800.0, 0.723332, 0.006772,
              3.3947, 76.680, 54.884, 50.115,
              0.760, 480.0, 177.36, -5832.6,
              "96.5% CO₂, 3.5% N₂", "9,200 kPa", "Induced", "299",
              { {0,0,"Carbon Dioxide",96.5f,{0.9f,0.3f,0.3f,1.0f}}, {0,0,"Nitrogen",3.5f,{0.0f,0.7f,0.9f,1.0f}} },
              glm::vec3(0.9f, 0.7f, 0.3f), "assets/textures/venus_surface.jpg");

    // 4. Earth
    auto earthRes = addPlanet("earth", "Earth", "Terrestrial Planet",
              5.97219e24, 6371000.0, 1.000000, 0.0167086,
              0.00005, 174.9, 288.1, 357.517,
              0.306, 33.0, 23.44, 23.93446,
              "78% N₂, 21% O₂", "101.3 kPa", "25.0–65.0 µT", "399",
              { {0,0,"Nitrogen",78.08f,{0.0f,0.7f,0.9f,1.0f}}, {0,0,"Oxygen",20.95f,{0.0f,0.85f,0.4f,1.0f}}, {0,0,"Argon",0.93f,{0.94f,0.75f,0.12f,1.0f}}, {0,0,"Carbon Dioxide",0.04f,{0.86f,0.31f,0.31f,1.0f}} },
              glm::vec3(0.0f, 0.83f, 1.0f), "assets/textures/earth_daymap.jpg");

    // 4b. Moon (Luna)
    auto moonRes = addMoon("moon", "Moon",
              UnitConverter::LUNAR_MASS_KG, UnitConverter::LUNAR_RADIUS_M, 384400.0, 0.0549,
              5.145, 125.08, 318.15, 135.27,
              0.12, 0.0, 1.54, 655.7,
              "Trace Helium, Neon, Hydrogen", "10⁻¹² kPa", "None", "301",
              { {0,0,"Helium",40.0f,{0.8f,0.8f,0.8f,1.0f}}, {0,0,"Neon",40.0f,{0.9f,0.4f,0.1f,1.0f}}, {0,0,"Hydrogen",20.0f,{0.3f,0.7f,1.0f,1.0f}} },
              glm::vec3(0.75f, 0.75f, 0.75f), "", earthRes);

    // 5. Mars
    auto marsRes = addPlanet("mars", "Mars", "Terrestrial Planet",
              6.4171e23, 3389500.0, 1.523679, 0.093400,
              1.8497, 49.558, 286.502, 19.373,
              0.250, 5.0, 25.19, 24.6229,
              "95.3% CO₂, 2.6% N₂", "0.61 kPa", "Remnant", "499",
              { {0,0,"Carbon Dioxide",95.32f,{0.9f,0.3f,0.2f,1.0f}}, {0,0,"Nitrogen",2.6f,{0.0f,0.7f,0.9f,1.0f}}, {0,0,"Argon",1.9f,{0.9f,0.7f,0.1f,1.0f}} },
              glm::vec3(0.95f, 0.35f, 0.2f), "assets/textures/mars_surface.jpg");

    // 6. Jupiter
    auto jupiterRes = addPlanet("jupiter", "Jupiter", "Gas Giant",
              1.89813e27, 69911000.0, 5.204400, 0.048900,
              1.303, 100.464, 273.867, 20.020,
              0.503, 0.0, 3.13, 9.925,
              "89% H₂, 10% He", "100 kPa", "420 µT", "599",
              { {0,0,"Hydrogen",89.8f,{0.9f,0.8f,0.6f,1.0f}}, {0,0,"Helium",10.2f,{0.9f,0.6f,0.3f,1.0f}} },
              glm::vec3(0.85f, 0.65f, 0.45f), "assets/textures/jupiter_surface.jpg");

    // 6b. Jupiter's Galilean Moons (Io, Europa, Ganymede, Callisto)
    addMoon("io", "Io",
            8.9319e22, 1821600.0, 421700.0, 0.0041,
            0.05, 43.0, 84.0, 200.0,
            0.63, 0.0, 0.0, 42.46,
            "Trace SO₂", "10⁻⁹ kPa", "Induced", "501",
            { {0,0,"Sulfur / Silicates",90.0f,{0.9f,0.8f,0.2f,1.0f}}, {0,0,"Iron Core",10.0f,{0.5f,0.3f,0.1f,1.0f}} },
            glm::vec3(0.95f, 0.85f, 0.2f), "", jupiterRes);

    addMoon("europa", "Europa",
            4.7998e22, 1560800.0, 670900.0, 0.0090,
            0.47, 219.0, 357.0, 90.0,
            0.67, 0.0, 0.1, 85.23,
            "Trace O₂", "10⁻¹¹ kPa", "Induced", "502",
            { {0,0,"Water Ice Crust",85.0f,{0.8f,0.9f,1.0f,1.0f}}, {0,0,"Silicate Mantle",15.0f,{0.6f,0.5f,0.4f,1.0f}} },
            glm::vec3(0.85f, 0.85f, 0.95f), "", jupiterRes);

    addMoon("ganymede", "Ganymede",
            1.4819e23, 2634100.0, 1070400.0, 0.0013,
            0.20, 63.0, 192.0, 45.0,
            0.43, 0.0, 0.2, 171.71,
            "Trace O₂, O₃", "10⁻¹¹ kPa", "1.2 µT (Intrinsic)", "503",
            { {0,0,"Water Ice",50.0f,{0.7f,0.8f,0.9f,1.0f}}, {0,0,"Silicates",40.0f,{0.5f,0.5f,0.5f,1.0f}}, {0,0,"Iron Core",10.0f,{0.6f,0.4f,0.2f,1.0f}} },
            glm::vec3(0.7f, 0.65f, 0.6f), "", jupiterRes);

    addMoon("callisto", "Callisto",
            1.0759e23, 2410300.0, 1882700.0, 0.0074,
            0.28, 298.0, 52.0, 310.0,
            0.22, 0.0, 0.0, 400.54,
            "Trace CO₂", "10⁻¹¹ kPa", "Induced", "504",
            { {0,0,"Water Ice",50.0f,{0.6f,0.6f,0.7f,1.0f}}, {0,0,"Silicates",50.0f,{0.4f,0.4f,0.4f,1.0f}} },
            glm::vec3(0.55f, 0.5f, 0.45f), "", jupiterRes);

    // 7. Saturn with rings
    std::string saturnRing = R"({"hasRing":true,"innerRadiusM":74500000.0,"outerRadiusM":140220000.0,"massKg":1.5e19,"thicknessM":20.0,"colorR":0.88,"colorG":0.82,"colorB":0.70,"texturePath":"assets/textures/saturn_ring_alpha.png"})";
    auto saturnRes = addPlanet("saturn", "Saturn", "Gas Giant",
              5.6834e26, 58232000.0, 9.582600, 0.056500,
              2.485, 113.665, 339.392, 317.020,
              0.342, 0.0, 26.73, 10.656,
              "96% H₂, 3% He", "100 kPa", "21 µT", "699",
              { {0,0,"Hydrogen",96.3f,{0.9f,0.85f,0.5f,1.0f}}, {0,0,"Helium",3.2f,{0.9f,0.7f,0.4f,1.0f}} },
              glm::vec3(0.9f, 0.8f, 0.5f), "assets/textures/saturn_surface.jpg", saturnRing);

    // 7b. Saturn's Titan
    addMoon("titan", "Titan",
            1.3452e23, 2574700.0, 1221870.0, 0.0288,
            0.35, 99.0, 180.0, 15.0,
            0.22, 12.0, 0.0, 382.68,
            "95% N₂, 5% CH₄", "146.7 kPa", "Induced", "606",
            { {0,0,"Nitrogen Atmosphere",60.0f,{0.9f,0.6f,0.2f,1.0f}}, {0,0,"Water Ice Mantle",40.0f,{0.6f,0.7f,0.9f,1.0f}} },
            glm::vec3(0.9f, 0.7f, 0.3f), "", saturnRes);

    // 8. Uranus
    auto uranusRes = addPlanet("uranus", "Uranus", "Ice Giant",
              8.6810e25, 25362000.0, 19.20120, 0.047170,
              0.773, 74.006, 96.999, 142.239,
              0.300, 0.0, 97.77, -17.24,
              "83% H₂, 15% He, 2.3% CH₄", "100 kPa", "23 µT", "799",
              { {0,0,"Hydrogen",83.0f,{0.3f,0.8f,0.9f,1.0f}}, {0,0,"Helium",15.0f,{0.5f,0.7f,0.9f,1.0f}}, {0,0,"Methane",2.3f,{0.1f,0.5f,0.8f,1.0f}} },
              glm::vec3(0.5f, 0.8f, 0.9f), "assets/textures/uranus_surface.jpg");

    // 9. Neptune
    auto neptuneRes = addPlanet("neptune", "Neptune", "Ice Giant",
              1.02413e26, 24622000.0, 30.04720, 0.008678,
              1.770, 131.784, 273.187, 256.228,
              0.290, 0.0, 28.32, 16.11,
              "80% H₂, 19% He, 1.5% CH₄", "100 kPa", "14 µT", "899",
              { {0,0,"Hydrogen",80.0f,{0.1f,0.3f,0.9f,1.0f}}, {0,0,"Helium",19.0f,{0.3f,0.5f,0.9f,1.0f}}, {0,0,"Methane",1.5f,{0.0f,0.2f,0.7f,1.0f}} },
              glm::vec3(0.2f, 0.4f, 0.9f), "assets/textures/neptune_surface.jpg");

    // 10. Pluto
    auto plutoRes = addPlanet("pluto", "Pluto", "Dwarf Planet",
              1.303e22, 1188300.0, 39.482, 0.2488,
              17.16, 110.303, 113.834, 14.882,
              0.52, 0.0, 122.53, -153.3,
              "99% N₂, 0.5% CH₄, 0.5% CO", "0.001 kPa", "None", "999",
              { {0,0,"Nitrogen",99.0f,{0.9f,0.8f,0.7f,1.0f}}, {0,0,"Methane",0.5f,{0.2f,0.5f,0.8f,1.0f}} },
              glm::vec3(0.8f, 0.7f, 0.6f), "");

    // 11. Major Real Asteroids (Asteroid Belt)
    addPlanet("ceres", "1 Ceres", "Dwarf Planet / Asteroid",
              9.3835e20, 469700.0, 2.767, 0.076,
              10.59, 80.33, 73.59, 95.99,
              0.09, 0.0, 4.0, 9.074,
              "Water vapor (transient)", "None", "None", "2000001",
              { {0,0,"Water Ice",73.0f,{0.6f,0.8f,1.0f,1.0f}}, {0,0,"Silicates",27.0f,{0.6f,0.5f,0.4f,1.0f}} },
              glm::vec3(0.7f, 0.65f, 0.6f), "", "", std::nullopt, "Asteroid Belt");

    addPlanet("vesta", "4 Vesta", "Asteroid (Basaltic)",
              2.5908e20, 262700.0, 2.362, 0.089,
              7.14, 103.81, 150.73, 20.86,
              0.423, 0.0, 29.0, 5.342,
              "None", "None", "None", "2000004",
              { {0,0,"Basaltic Pyroxene",65.0f,{0.5f,0.5f,0.5f,1.0f}}, {0,0,"Olivine",35.0f,{0.4f,0.6f,0.3f,1.0f}} },
              glm::vec3(0.78f, 0.75f, 0.7f), "", "", std::nullopt, "Asteroid Belt");

    addPlanet("pallas", "2 Pallas", "Asteroid (Carbonaceous)",
              2.04e20, 256000.0, 2.772, 0.231,
              34.84, 173.08, 310.05, 101.4,
              0.159, 0.0, 84.0, 7.813,
              "None", "None", "None", "2000002",
              { {0,0,"Hydrated Silicates",60.0f,{0.4f,0.4f,0.4f,1.0f}}, {0,0,"Carbon",40.0f,{0.2f,0.2f,0.2f,1.0f}} },
              glm::vec3(0.6f, 0.6f, 0.6f), "", "", std::nullopt, "Asteroid Belt");

    addPlanet("apophis", "99942 Apophis", "Near-Earth Asteroid",
              6.1e10, 185.0, 0.922, 0.191,
              3.33, 204.04, 126.40, 250.0,
              0.23, 0.0, 0.0, 30.56,
              "None", "None", "None", "2099942",
              { {0,0,"LL Chondrite",100.0f,{0.7f,0.6f,0.5f,1.0f}} },
              glm::vec3(0.85f, 0.45f, 0.3f), "", "", std::nullopt, "Asteroid Belt");

    addPlanet("bennu", "101955 Bennu", "Near-Earth Asteroid",
              7.329e10, 245.0, 1.126, 0.2037,
              6.03, 2.06, 66.22, 101.7,
              0.044, 0.0, 178.0, 4.296,
              "None", "None", "None", "2101955",
              { {0,0,"Carbonaceous Magnetite",80.0f,{0.2f,0.2f,0.25f,1.0f}}, {0,0,"Organic compounds",20.0f,{0.4f,0.3f,0.2f,1.0f}} },
              glm::vec3(0.4f, 0.45f, 0.5f), "", "", std::nullopt, "Asteroid Belt");

    // 12. Exoplanet Systems (TRAPPIST-1, Proxima Centauri, Kepler-186)
    // TRAPPIST-1 Host Star (Ultracool Red Dwarf)
    CelestialBodyRecord trappist1;
    trappist1.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    trappist1.object.slug = "trappist_1";
    trappist1.object.name = "TRAPPIST-1";
    trappist1.object.type = "M8V Red Dwarf Star";
    trappist1.object.category = "TRAPPIST-1 System";
    trappist1.object.color = glm::vec3(1.0f, 0.35f, 0.1f);
    trappist1.physical.massKg = 0.0898 * UnitConverter::SOLAR_MASS_KG;
    trappist1.physical.radiusM = 0.1192 * UnitConverter::SOLAR_RADIUS_M;
    trappist1.physical.luminosityW = 0.000553 * UnitConverter::SOLAR_LUMINOSITY_W;
    trappist1.physical.surfaceTempK = 2566.0;
    trappist1.physical.rotationPeriodHours = 79.2;
    trappist1.physical.sourceRecordId = "TRAPPIST-1";
    trappist1.stateVector.positionM = glm::dvec3(0.0);
    trappist1.stateVector.velocityMps = glm::dvec3(0.0);
    int64_t trap1Id = 0;
    repo.saveCelestialBodyRecord(trappist1, &trap1Id);

    // TRAPPIST-1 e (Habitable Zone Exoplanet)
    CelestialBodyRecord trap1e;
    trap1e.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    trap1e.object.slug = "trappist_1_e";
    trap1e.object.name = "TRAPPIST-1 e";
    trap1e.object.type = "Exoplanet (Habitable Zone)";
    trap1e.object.category = "TRAPPIST-1 System";
    trap1e.object.parentObjectId = trap1Id;
    trap1e.object.color = glm::vec3(0.1f, 0.75f, 0.85f);
    trap1e.physical.massKg = 0.692 * UnitConverter::EARTH_MASS_KG;
    trap1e.physical.radiusM = 0.920 * UnitConverter::EARTH_RADIUS_M;
    trap1e.physical.surfaceGravityMps2 = 0.93 * 9.80665;
    trap1e.physical.surfaceTempK = 251.0;
    trap1e.physical.atmosphereSummary = "Compact H₂O/N₂ atmosphere";
    trap1e.physical.sourceRecordId = "TRAPPIST-1 e";
    trap1e.orbital.semiMajorAxisAU = 0.02928;
    trap1e.orbital.semiMajorAxisM = 0.02928 * UnitConverter::AU_TO_METERS;
    trap1e.orbital.eccentricity = 0.0051;
    trap1e.orbital.orbitalPeriodDays = 6.0996;
    double aM_e = trap1e.orbital.semiMajorAxisM.value();
    double v_e = std::sqrt(UnitConverter::G_CONST * (trappist1.physical.massKg.value() + trap1e.physical.massKg.value()) / aM_e);
    trap1e.stateVector.positionM = glm::dvec3(aM_e, 0.0, 0.0);
    trap1e.stateVector.velocityMps = glm::dvec3(0.0, 0.0, v_e);
    repo.saveCelestialBodyRecord(trap1e);

    // Proxima Centauri Host Star
    CelestialBodyRecord proxC;
    proxC.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    proxC.object.slug = "proxima_centauri";
    proxC.object.name = "Proxima Centauri";
    proxC.object.type = "M5.5Ve Red Dwarf Star";
    proxC.object.category = "Proxima Centauri";
    proxC.object.color = glm::vec3(1.0f, 0.45f, 0.15f);
    proxC.physical.massKg = 0.1221 * UnitConverter::SOLAR_MASS_KG;
    proxC.physical.radiusM = 0.1542 * UnitConverter::SOLAR_RADIUS_M;
    proxC.physical.luminosityW = 0.0017 * UnitConverter::SOLAR_LUMINOSITY_W;
    proxC.physical.surfaceTempK = 3042.0;
    proxC.physical.sourceRecordId = "Proxima Centauri";
    int64_t proxCId = 0;
    repo.saveCelestialBodyRecord(proxC, &proxCId);

    // Proxima Centauri b
    CelestialBodyRecord proxCb;
    proxCb.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    proxCb.object.slug = "proxima_centauri_b";
    proxCb.object.name = "Proxima Centauri b";
    proxCb.object.type = "Exoplanet (Habitable Zone)";
    proxCb.object.category = "Proxima Centauri";
    proxCb.object.parentObjectId = proxCId;
    proxCb.object.color = glm::vec3(0.2f, 0.8f, 0.5f);
    proxCb.physical.massKg = 1.07 * UnitConverter::EARTH_MASS_KG;
    proxCb.physical.radiusM = 1.03 * UnitConverter::EARTH_RADIUS_M;
    proxCb.physical.surfaceTempK = 234.0;
    proxCb.physical.sourceRecordId = "Proxima Centauri b";
    proxCb.orbital.semiMajorAxisAU = 0.0485;
    proxCb.orbital.semiMajorAxisM = 0.0485 * UnitConverter::AU_TO_METERS;
    proxCb.orbital.eccentricity = 0.02;
    proxCb.orbital.orbitalPeriodDays = 11.186;
    double aM_pb = proxCb.orbital.semiMajorAxisM.value();
    double v_pb = std::sqrt(UnitConverter::G_CONST * (proxC.physical.massKg.value() + proxCb.physical.massKg.value()) / aM_pb);
    proxCb.stateVector.positionM = glm::dvec3(aM_pb, 0.0, 0.0);
    proxCb.stateVector.velocityMps = glm::dvec3(0.0, 0.0, v_pb);
    repo.saveCelestialBodyRecord(proxCb);

    // Kepler-186 Host Star
    CelestialBodyRecord kep186;
    kep186.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    kep186.object.slug = "kepler_186";
    kep186.object.name = "Kepler-186";
    kep186.object.type = "M1V Red Dwarf Star";
    kep186.object.category = "Kepler-186 System";
    kep186.object.color = glm::vec3(1.0f, 0.55f, 0.2f);
    kep186.physical.massKg = 0.544 * UnitConverter::SOLAR_MASS_KG;
    kep186.physical.radiusM = 0.523 * UnitConverter::SOLAR_RADIUS_M;
    kep186.physical.surfaceTempK = 3755.0;
    kep186.physical.sourceRecordId = "Kepler-186";
    int64_t kep186Id = 0;
    repo.saveCelestialBodyRecord(kep186, &kep186Id);

    // Kepler-186 f (First Earth-sized planet in habitable zone)
    CelestialBodyRecord kep186f;
    kep186f.sourceName = "NASA Exoplanet Archive (Bundled Seed)";
    kep186f.object.slug = "kepler_186_f";
    kep186f.object.name = "Kepler-186 f";
    kep186f.object.type = "Exoplanet (Habitable Zone)";
    kep186f.object.category = "Kepler-186 System";
    kep186f.object.parentObjectId = kep186Id;
    kep186f.object.color = glm::vec3(0.3f, 0.7f, 0.9f);
    kep186f.physical.massKg = 1.44 * UnitConverter::EARTH_MASS_KG;
    kep186f.physical.radiusM = 1.17 * UnitConverter::EARTH_RADIUS_M;
    kep186f.physical.surfaceTempK = 188.0;
    kep186f.physical.sourceRecordId = "Kepler-186 f";
    kep186f.orbital.semiMajorAxisAU = 0.432;
    kep186f.orbital.semiMajorAxisM = 0.432 * UnitConverter::AU_TO_METERS;
    kep186f.orbital.eccentricity = 0.04;
    kep186f.orbital.orbitalPeriodDays = 129.9441;
    double aM_kf = kep186f.orbital.semiMajorAxisM.value();
    double v_kf = std::sqrt(UnitConverter::G_CONST * (kep186.physical.massKg.value() + kep186f.physical.massKg.value()) / aM_kf);
    kep186f.stateVector.positionM = glm::dvec3(aM_kf, 0.0, 0.0);
    kep186f.stateVector.velocityMps = glm::dvec3(0.0, 0.0, v_kf);
    repo.saveCelestialBodyRecord(kep186f);

    std::cout << "[SeedData] Seeded " << repo.getObjectCount() << " astronomical bodies into database." << std::endl;
    return true;
}

} // namespace AstroGenesis
