#include "data/providers/JPLHorizonsProvider.hpp"
#include "data/UnitConverter.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace AstroGenesis {

static const std::map<std::string, JPLHorizonsProvider::HorizonsBodyMeta> HORIZONS_CATALOG = {
    // Sun & Terrestrial Planets
    { "sun",       { "10",  "Sol (Sun)",        "G2V Main Sequence Star", "",        "",    696340.0, 1.9885e30 } },
    { "sol",       { "10",  "Sol (Sun)",        "G2V Main Sequence Star", "",        "",    696340.0, 1.9885e30 } },
    { "mercury",   { "199", "Mercury",          "Terrestrial Planet",     "",        "",    2439.7,   3.3011e23 } },
    { "venus",     { "299", "Venus",            "Terrestrial Planet",     "",        "",    6051.8,   4.8675e24 } },
    { "earth",     { "399", "Earth",            "Terrestrial Planet",     "",        "",    6371.0,   5.9722e24 } },
    { "mars",      { "499", "Mars",             "Terrestrial Planet",     "",        "",    3389.5,   6.4171e23 } },

    // Gas & Ice Giants
    { "jupiter",   { "599", "Jupiter",          "Gas Giant",              "",        "",    69911.0,  1.8982e27 } },
    { "saturn",    { "699", "Saturn",           "Gas Giant",              "",        "",    58232.0,  5.6834e26 } },
    { "uranus",    { "799", "Uranus",           "Ice Giant",              "",        "",    25362.0,  8.6810e25 } },
    { "neptune",   { "899", "Neptune",          "Ice Giant",              "",        "",    24622.0,  1.0241e26 } },
    { "pluto",     { "999", "Pluto",            "Dwarf Planet",           "",        "",    1188.3,   1.3030e22 } },

    // Earth's Moon
    { "moon",      { "301", "Moon (Luna)",      "Natural Satellite (Moon)","earth",  "399", 1737.4,   7.3420e22 } },
    { "luna",      { "301", "Moon (Luna)",      "Natural Satellite (Moon)","earth",  "399", 1737.4,   7.3420e22 } },

    // Mars's Moons
    { "phobos",    { "401", "Phobos",           "Natural Satellite (Moon)","mars",   "499", 11.26,    1.0659e16 } },
    { "deimos",    { "402", "Deimos",           "Natural Satellite (Moon)","mars",   "499", 6.20,     1.4762e15 } },

    // Jupiter's Galilean & Inner Moons
    { "io",        { "501", "Io",               "Natural Satellite (Moon)","jupiter","599", 1821.6,   8.9319e22 } },
    { "europa",    { "502", "Europa",           "Natural Satellite (Moon)","jupiter","599", 1560.8,   4.7998e22 } },
    { "ganymede",  { "503", "Ganymede",         "Natural Satellite (Moon)","jupiter","599", 2634.1,   1.4819e23 } },
    { "callisto",  { "504", "Callisto",         "Natural Satellite (Moon)","jupiter","599", 2410.3,   1.0759e23 } },
    { "amalthea",  { "505", "Amalthea",         "Natural Satellite (Moon)","jupiter","599", 83.5,     2.0800e18 } },
    { "himalia",   { "506", "Himalia",          "Natural Satellite (Moon)","jupiter","599", 85.0,     4.2000e18 } },
    { "thebe",     { "514", "Thebe",            "Natural Satellite (Moon)","jupiter","599", 49.3,     4.3000e17 } },

    // Saturn's Major Moons
    { "mimas",     { "601", "Mimas",            "Natural Satellite (Moon)","saturn", "699", 198.2,    3.7500e19 } },
    { "enceladus", { "602", "Enceladus",        "Natural Satellite (Moon)","saturn", "699", 252.1,    1.0800e20 } },
    { "tethys",    { "603", "Tethys",           "Natural Satellite (Moon)","saturn", "699", 531.1,    6.1700e20 } },
    { "dione",     { "604", "Dione",            "Natural Satellite (Moon)","saturn", "699", 561.4,    1.0950e21 } },
    { "rhea",      { "605", "Rhea",             "Natural Satellite (Moon)","saturn", "699", 763.8,    2.3060e21 } },
    { "titan",     { "606", "Titan",            "Natural Satellite (Moon)","saturn", "699", 2574.7,   1.3452e23 } },
    { "hyperion",  { "607", "Hyperion",         "Natural Satellite (Moon)","saturn", "699", 135.0,    5.6200e18 } },
    { "iapetus",   { "608", "Iapetus",          "Natural Satellite (Moon)","saturn", "699", 734.5,    1.8050e21 } },
    { "phoebe",    { "609", "Phoebe",           "Natural Satellite (Moon)","saturn", "699", 106.5,    8.2920e18 } },
    { "janus",     { "610", "Janus",            "Natural Satellite (Moon)","saturn", "699", 89.5,     1.8975e18 } },
    { "epimetheus",{ "611", "Epimetheus",       "Natural Satellite (Moon)","saturn", "699", 58.1,     5.2660e17 } },

    // Uranus's Moons
    { "miranda",   { "705", "Miranda",          "Natural Satellite (Moon)","uranus", "799", 235.8,    6.4000e19 } },
    { "ariel",     { "701", "Ariel",            "Natural Satellite (Moon)","uranus", "799", 578.9,    1.2500e21 } },
    { "umbriel",   { "702", "Umbriel",          "Natural Satellite (Moon)","uranus", "799", 584.7,    1.2700e21 } },
    { "titania",   { "703", "Titania",          "Natural Satellite (Moon)","uranus", "799", 788.4,    3.4000e21 } },
    { "oberon",    { "704", "Oberon",           "Natural Satellite (Moon)","uranus", "799", 761.4,    3.0000e21 } },
    { "puck",      { "715", "Puck",             "Natural Satellite (Moon)","uranus", "799", 81.0,     2.9000e18 } },

    // Neptune's Moons
    { "triton",    { "801", "Triton",           "Natural Satellite (Moon)","neptune","899", 1353.4,   2.1400e22 } },
    { "nereid",    { "802", "Nereid",           "Natural Satellite (Moon)","neptune","899", 170.0,    3.1000e19 } },
    { "proteus",   { "808", "Proteus",          "Natural Satellite (Moon)","neptune","899", 210.0,    4.4000e19 } },

    // Pluto's Moon
    { "charon",    { "901", "Charon",           "Natural Satellite (Moon)","pluto",  "999", 606.0,    1.5860e21 } },
    { "nix",       { "902", "Nix",              "Natural Satellite (Moon)","pluto",  "999", 24.9,     4.5000e16 } },
    { "hydra",     { "903", "Hydra",            "Natural Satellite (Moon)","pluto",  "999", 27.5,     4.8000e16 } },

    // Asteroids & Comets
    { "ceres",     { "2000001", "1 Ceres",      "Dwarf Planet / Asteroid","",        "",    473.0,     9.3835e20 } },
    { "pallas",    { "2000002", "2 Pallas",     "Asteroid",               "",        "",    256.0,     2.1100e20 } },
    { "juno",      { "2000003", "3 Juno",       "Asteroid",               "",        "",    127.0,     2.6700e19 } },
    { "vesta",     { "2000004", "4 Vesta",      "Asteroid",               "",        "",    262.7,     2.5900e20 } },
    { "bennu",     { "2101955", "101955 Bennu", "Near-Earth Asteroid",    "",        "",    0.245,     7.3290e10 } },
    { "apophis",   { "2099942", "99942 Apophis","Near-Earth Asteroid",    "",        "",    0.170,     6.1000e10 } },
    { "eros",      { "2000433", "433 Eros",     "Near-Earth Asteroid",    "",        "",    8.4,       6.6870e15 } },
    { "halley",    { "90000030","1P/Halley",    "Periodic Comet",         "",        "",    5.5,       2.2000e14 } }
};

JPLHorizonsProvider::JPLHorizonsProvider(HttpClient& httpClient) : m_http(httpClient) {}

std::string JPLHorizonsProvider::resolveHorizonsId(const std::string& nameOrId) {
    std::string lower = nameOrId;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = HORIZONS_CATALOG.find(lower);
    if (it != HORIZONS_CATALOG.end()) {
        return it->second.id;
    }
    return nameOrId;
}

std::optional<JPLHorizonsProvider::HorizonsBodyMeta> JPLHorizonsProvider::getBodyMetadata(const std::string& nameOrId) {
    std::string lower = nameOrId;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = HORIZONS_CATALOG.find(lower);
    if (it != HORIZONS_CATALOG.end()) {
        return it->second;
    }
    for (const auto& kv : HORIZONS_CATALOG) {
        if (kv.second.id == nameOrId) return kv.second;
    }

    // Dynamic numeric satellite detection from JPL 3-digit planet/moon numbering
    try {
        int code = std::stoi(nameOrId);
        if (code >= 301 && code <= 398) return HorizonsBodyMeta{ nameOrId, "Earth Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "earth", "399", 50.0, 1e16 };
        if (code >= 401 && code <= 498) return HorizonsBodyMeta{ nameOrId, "Mars Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "mars", "499", 10.0, 1e15 };
        if (code >= 501 && code <= 598) return HorizonsBodyMeta{ nameOrId, "Jovian Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "jupiter", "599", 100.0, 1e18 };
        if (code >= 601 && code <= 698) return HorizonsBodyMeta{ nameOrId, "Saturnian Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "saturn", "699", 100.0, 1e18 };
        if (code >= 701 && code <= 798) return HorizonsBodyMeta{ nameOrId, "Uranian Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "uranus", "799", 80.0, 1e18 };
        if (code >= 801 && code <= 898) return HorizonsBodyMeta{ nameOrId, "Neptunian Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "neptune", "899", 80.0, 1e18 };
        if (code >= 901 && code <= 998) return HorizonsBodyMeta{ nameOrId, "Plutonian Satellite (" + nameOrId + ")", "Natural Satellite (Moon)", "pluto", "999", 50.0, 1e16 };
    } catch (...) {}

    return std::nullopt;
}

bool JPLHorizonsProvider::searchObjects(const std::string& query, 
                                       std::vector<SearchResult>& outResults, 
                                       std::string& outError) {
    outResults.clear();
    std::string lowerQ = query;
    std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);

    // 1. Check known lookup dictionary
    for (const auto& kv : HORIZONS_CATALOG) {
        if (kv.first.find(lowerQ) != std::string::npos || kv.second.name.find(query) != std::string::npos) {
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = kv.second.id;
            res.name = kv.second.name;
            res.type = kv.second.type;
            res.details = "NASA JPL Horizons target code: " + kv.second.id + (!kv.second.parentSlug.empty() ? (" (Orbiting " + kv.second.parentSlug + ")") : "");
            outResults.push_back(res);
        }
    }

    // 2. Query Horizons API live for search query if not directly matched or small body
    std::string url = getBaseUrl() + "?format=json&COMMAND=" + HttpClient::urlEncode("'" + query + "'");
    auto resp = m_http.get(url, {}, 8);
    if (resp.success && !resp.body.empty()) {
        try {
            auto j = nlohmann::json::parse(resp.body);
            if (j.contains("result")) {
                std::string resText = j["result"].get<std::string>();
                std::istringstream stream(resText);
                std::string line;
                while (std::getline(stream, line)) {
                    if (line.find("Multiple major-bodies match") != std::string::npos || 
                        line.find("Matching small-bodies") != std::string::npos) {
                        while (std::getline(stream, line) && !line.empty() && line.find("---") == std::string::npos) {
                            if (line.length() > 10) {
                                SearchResult res;
                                res.sourceName = getProviderName();
                                res.sourceId = query;
                                res.name = line.substr(0, std::min((size_t)35, line.length()));
                                res.type = "JPL Match";
                                res.details = line;
                                outResults.push_back(res);
                            }
                        }
                    }
                }
            }
        } catch (...) {}
    }

    if (outResults.empty()) {
        SearchResult res;
        res.sourceName = getProviderName();
        res.sourceId = query;
        res.name = query;
        res.type = "Solar System Target";
        res.details = "Query via target ID or name";
        outResults.push_back(res);
    }

    return true;
}

bool JPLHorizonsProvider::fetchObjectData(const std::string& sourceIdOrName, 
                                         CelestialBodyRecord& outRecord, 
                                         std::string& outError) {
    std::string targetId = resolveHorizonsId(sourceIdOrName);
    auto metaOpt = getBodyMetadata(sourceIdOrName);

    // Center selection:
    // If the body is a moon (has parent planet), query parent-relative vectors (e.g. '@599' for Jupiter moons)
    std::string centerParam = "'@0'"; // Default: Solar System Barycenter
    if (metaOpt.has_value() && !metaOpt.value().parentId.empty()) {
        centerParam = "'@" + metaOpt.value().parentId + "'";
    }

    std::string url = getBaseUrl() + "?format=json"
                      "&COMMAND=" + HttpClient::urlEncode("'" + targetId + "'") +
                      "&CENTER=" + HttpClient::urlEncode(centerParam) +
                      "&EPHEM_TYPE='VECTORS'"
                      "&VEC_TABLE='2'"
                      "&REF_PLANE='ECLIPTIC'"
                      "&START_TIME='2000-01-01%2012:00:00'"
                      "&STOP_TIME='2000-01-02%2012:00:00'"
                      "&STEP_SIZE='1d'"
                      "&OUT_UNITS='KM-S'";

    auto resp = m_http.get(url, {}, 15);
    if (!resp.success) {
        outError = "JPL Horizons HTTP request failed: " + resp.errorMessage;
        return false;
    }

    std::string rawResult;
    try {
        auto j = nlohmann::json::parse(resp.body);
        if (j.contains("result")) {
            rawResult = j["result"].get<std::string>();
        } else {
            outError = "JPL Horizons response missing 'result' field.";
            return false;
        }
    } catch (const std::exception& e) {
        outError = "Failed to parse JPL Horizons JSON: " + std::string(e.what());
        return false;
    }

    outRecord.sourceName = getProviderName();
    outRecord.physical.sourceRecordId = targetId;
    outRecord.object.slug = sourceIdOrName;
    std::transform(outRecord.object.slug.begin(), outRecord.object.slug.end(), outRecord.object.slug.begin(), ::tolower);
    outRecord.object.name = sourceIdOrName;
    outRecord.object.category = "Solar System";

    if (metaOpt.has_value()) {
        outRecord.object.name = metaOpt.value().name;
        outRecord.object.type = metaOpt.value().type;
        if (metaOpt.value().defaultRadiusKm > 0.0) {
            outRecord.physical.radiusM = metaOpt.value().defaultRadiusKm * 1000.0;
        }
        if (metaOpt.value().defaultMassKg > 0.0) {
            outRecord.physical.massKg = metaOpt.value().defaultMassKg;
        }
    }

    return parseHorizonsResponse(rawResult, outRecord, outError);
}

bool JPLHorizonsProvider::parseHorizonsResponse(const std::string& rawText, 
                                               CelestialBodyRecord& outRecord, 
                                               std::string& outError) {
    // 1. Extract Target Body Name from Header
    size_t targetIdx = rawText.find("Target body name:");
    if (targetIdx != std::string::npos) {
        size_t endLine = rawText.find('\n', targetIdx);
        std::string line = rawText.substr(targetIdx + 17, endLine - (targetIdx + 17));
        size_t braceIdx = line.find('{');
        if (braceIdx != std::string::npos) line = line.substr(0, braceIdx);
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r')) line.pop_back();
        while (!line.empty() && line.front() == ' ') line.erase(line.begin());
        if (!line.empty()) outRecord.object.name = line;
    }

    // 2. Extract Physical Parameters
    // Radius (km)
    std::regex radRegex(R"(Radius\s*\(?km\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex radMeanRegex(R"(Mean\s+Radius\s*\(?km\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex volRadRegex(R"(Vol\.\s*Mean\s*Radius\s*\(?km\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(rawText, match, volRadRegex) || 
        std::regex_search(rawText, match, radMeanRegex) || 
        std::regex_search(rawText, match, radRegex)) {
        try {
            double radKm = std::stod(match[1].str());
            outRecord.physical.radiusM = radKm * 1000.0;
        } catch (...) {}
    }

    // Mass (Supports 10^24, 10^23, 10^22, 10^21, 10^20, 10^19, 10^18, 10^15, kg)
    std::regex massExpRegex(R"(Mass[^\n=]*10\^(\d+)\s*kg\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex massKgRegex(R"(Mass\s*\(?kg\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, massExpRegex)) {
        try {
            int exp = std::stoi(match[1].str());
            double val = std::stod(match[2].str());
            outRecord.physical.massKg = val * std::pow(10.0, (double)exp);
        } catch (...) {}
    } else if (std::regex_search(rawText, match, massKgRegex)) {
        try {
            outRecord.physical.massKg = std::stod(match[1].str());
        } catch (...) {}
    }

    // Density (g/cm^3 -> kg/m^3 (* 1000.0))
    std::regex densRegex(R"(density[^\n=]*g/cm\^3\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, densRegex)) {
        try {
            outRecord.physical.meanDensityKgM3 = std::stod(match[1].str()) * 1000.0;
        } catch (...) {}
    }

    // Geometric / Bond Albedo
    std::regex albedoRegex(R"(albedo\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, albedoRegex)) {
        try {
            outRecord.physical.albedo = std::stod(match[1].str());
        } catch (...) {}
    }

    // Rotation Period (hours or days)
    std::regex rotPerDRegex(R"(Rot\.\s*Per\.\s*\(?d\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex rotPerHRegex(R"(Rot\.\s*Per\.\s*\(?h\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, rotPerHRegex)) {
        try {
            outRecord.physical.rotationPeriodHours = std::stod(match[1].str());
        } catch (...) {}
    } else if (std::regex_search(rawText, match, rotPerDRegex)) {
        try {
            outRecord.physical.rotationPeriodHours = std::stod(match[1].str()) * 24.0;
        } catch (...) {}
    }

    // Axial tilt (degrees)
    std::regex tiltRegex(R"(Obliquity\s*to\s*orbit\s*\(?deg\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, tiltRegex)) {
        try {
            outRecord.physical.axialTiltDeg = std::stod(match[1].str());
        } catch (...) {}
    }

    // 3. Extract Vector Block between $$SOE and $$EOE
    size_t soeIdx = rawText.find("$$SOE");
    size_t eoeIdx = rawText.find("$$EOE");

    if (soeIdx == std::string::npos || eoeIdx == std::string::npos) {
        outError = "Could not find $$SOE / $$EOE vector data block in JPL Horizons response.";
        return false;
    }

    std::string vecBlock = rawText.substr(soeIdx + 5, eoeIdx - (soeIdx + 5));
    std::vector<EphemerisRecord> ephemList;
    if (parseVectorBlock(vecBlock, ephemList, outRecord.object.name) && !ephemList.empty()) {
        const auto& firstState = ephemList[0];
        outRecord.stateVector.epochJd = firstState.epochJd;
        outRecord.stateVector.positionM = firstState.positionM;
        outRecord.stateVector.velocityMps = firstState.velocityMps;
        outRecord.stateVector.referenceFrame = "ICRF/Barycentric";

        outRecord.orbital.epochJd = firstState.epochJd;
        double rM = glm::length(firstState.positionM);
        outRecord.orbital.semiMajorAxisM = rM;
        outRecord.orbital.semiMajorAxisAU = rM / UnitConverter::AU_TO_METERS;

        // Calculate surface gravity and escape velocity if physical parameters exist
        if (outRecord.physical.radiusM.has_value() && outRecord.physical.massKg.has_value()) {
            double r = outRecord.physical.radiusM.value();
            double m = outRecord.physical.massKg.value();
            if (r > 0.0) {
                outRecord.physical.surfaceGravityMps2 = (UnitConverter::G_CONST * m) / (r * r);
                outRecord.physical.escapeVelocityMps = std::sqrt(2.0 * UnitConverter::G_CONST * m / r);
            }
        }

        return true;
    }

    outError = "Failed to parse vectors from Horizons block.";
    return false;
}

bool JPLHorizonsProvider::parseVectorBlock(const std::string& vecBlock, 
                                          std::vector<EphemerisRecord>& outRecords, 
                                          const std::string& targetName) {
    outRecords.clear();
    std::istringstream stream(vecBlock);
    std::string line;

    double curJd = UnitConverter::J2000_JD;
    std::string curUtc;

    while (std::getline(stream, line)) {
        // Line with JD and Calendar date (e.g. 2451545.500000000 = A.D. 2000-Jan-01 12:00:00.0000 TDB)
        if (line.find('=') != std::string::npos && line.find("A.D.") != std::string::npos) {
            size_t eqIdx = line.find('=');
            try {
                curJd = std::stod(line.substr(0, eqIdx));
                curUtc = line.substr(eqIdx + 1);
            } catch (...) {}
        }

        // Line with X, Y, Z
        if (line.find("X =") != std::string::npos || line.find("X=") != std::string::npos) {
            double xKm = 0.0, yKm = 0.0, zKm = 0.0;
            if (sscanf(line.c_str(), " X =%lf Y =%lf Z =%lf", &xKm, &yKm, &zKm) >= 3 ||
                sscanf(line.c_str(), " X=%lf Y=%lf Z=%lf", &xKm, &yKm, &zKm) >= 3) {
                // Next line has VX, VY, VZ
                if (std::getline(stream, line)) {
                    double vxKmS = 0.0, vyKmS = 0.0, vzKmS = 0.0;
                    if (sscanf(line.c_str(), " VX=%lf VY=%lf VZ=%lf", &vxKmS, &vyKmS, &vzKmS) >= 3 ||
                        sscanf(line.c_str(), " VX =%lf VY =%lf VZ =%lf", &vxKmS, &vyKmS, &vzKmS) >= 3) {
                        EphemerisRecord rec;
                        rec.targetName = targetName;
                        rec.epochJd = curJd;
                        rec.epochUtc = curUtc;
                        // Map JPL Horizons Ecliptic (X, Y in-plane, Z normal) into AstroGenesis (X horizontal, Y normal/up, Z in-plane depth)
                        rec.positionM = glm::dvec3(xKm * 1000.0, zKm * 1000.0, yKm * 1000.0);
                        rec.velocityMps = glm::dvec3(vxKmS * 1000.0, vzKmS * 1000.0, vyKmS * 1000.0);
                        rec.referenceFrame = "ICRF/Ecliptic_J2000";
                        rec.sourceId = 1;
                        outRecords.push_back(rec);
                    }
                }
            }
        }
    }

    return !outRecords.empty();
}

bool JPLHorizonsProvider::fetchEphemerisSeries(const std::string& sourceIdOrName, 
                                              double startJd, 
                                              double endJd, 
                                              double stepDays, 
                                              std::vector<EphemerisRecord>& outRecords, 
                                              std::string& outError) {
    std::string targetId = resolveHorizonsId(sourceIdOrName);
    
    // Format start and stop dates in Julian Date format for Horizons (e.g. JD 2451545.0)
    char startBuf[64], stopBuf[64], stepBuf[32];
    snprintf(startBuf, sizeof(startBuf), "'JD %.2f'", startJd);
    snprintf(stopBuf, sizeof(stopBuf), "'JD %.2f'", endJd);
    snprintf(stepBuf, sizeof(stepBuf), "'%.2fd'", std::max(0.1, stepDays));

    std::string url = getBaseUrl() + "?format=json"
                      "&COMMAND=" + HttpClient::urlEncode("'" + targetId + "'") +
                      "&CENTER='@0'"
                      "&EPHEM_TYPE='VECTORS'"
                      "&VEC_TABLE='2'"
                      "&REF_PLANE='ECLIPTIC'"
                      "&START_TIME=" + HttpClient::urlEncode(startBuf) +
                      "&STOP_TIME=" + HttpClient::urlEncode(stopBuf) +
                      "&STEP_SIZE=" + HttpClient::urlEncode(stepBuf) +
                      "&OUT_UNITS='KM-S'";

    auto resp = m_http.get(url, {}, 25);
    if (!resp.success) {
        outError = "JPL Horizons request failed: " + resp.errorMessage;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (j.contains("result")) {
            std::string resText = j["result"].get<std::string>();
            size_t soeIdx = resText.find("$$SOE");
            size_t eoeIdx = resText.find("$$EOE");
            if (soeIdx != std::string::npos && eoeIdx != std::string::npos) {
                std::string block = resText.substr(soeIdx + 5, eoeIdx - (soeIdx + 5));
                return parseVectorBlock(block, outRecords, sourceIdOrName);
            }
        }
    } catch (const std::exception& e) {
        outError = "Ephemeris parsing failed: " + std::string(e.what());
        return false;
    }

    outError = "No ephemeris vectors returned from Horizons.";
    return false;
}

} // namespace AstroGenesis
