#include "data/providers/JPLHorizonsProvider.hpp"
#include "data/UnitConverter.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <algorithm>
#include <regex>

namespace AstroGenesis {

static const std::map<std::string, std::pair<std::string, std::string>> KNOWN_HORIZONS_BODIES = {
    { "sun",      { "10",  "Sol (Sun)" } },
    { "sol",      { "10",  "Sol (Sun)" } },
    { "mercury",  { "199", "Mercury" } },
    { "venus",    { "299", "Venus" } },
    { "earth",    { "399", "Earth" } },
    { "moon",     { "301", "Moon (Luna)" } },
    { "mars",     { "499", "Mars" } },
    { "jupiter",  { "599", "Jupiter" } },
    { "saturn",   { "699", "Saturn" } },
    { "uranus",   { "799", "Uranus" } },
    { "neptune",  { "899", "Neptune" } },
    { "pluto",    { "999", "Pluto" } },
    { "ceres",    { "2000001", "1 Ceres" } },
    { "pallas",   { "2000002", "2 Pallas" } },
    { "juno",     { "2000003", "3 Juno" } },
    { "vesta",    { "2000004", "4 Vesta" } },
    { "bennu",    { "2101955", "101955 Bennu" } },
    { "apophis",  { "2099942", "99942 Apophis" } },
    { "eros",     { "2000433", "433 Eros" } },
    { "halley",   { "90000030", "1P/Halley" } }
};

JPLHorizonsProvider::JPLHorizonsProvider(HttpClient& httpClient) : m_http(httpClient) {}

std::string JPLHorizonsProvider::resolveHorizonsId(const std::string& nameOrId) {
    std::string lower = nameOrId;
    std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    auto it = KNOWN_HORIZONS_BODIES.find(lower);
    if (it != KNOWN_HORIZONS_BODIES.end()) {
        return it->second.first;
    }
    return nameOrId;
}

bool JPLHorizonsProvider::searchObjects(const std::string& query, 
                                       std::vector<SearchResult>& outResults, 
                                       std::string& outError) {
    outResults.clear();
    std::string lowerQ = query;
    std::transform(lowerQ.begin(), lowerQ.end(), lowerQ.begin(), ::tolower);

    // 1. Check known lookup dictionary
    for (const auto& kv : KNOWN_HORIZONS_BODIES) {
        if (kv.first.find(lowerQ) != std::string::npos || kv.second.second.find(query) != std::string::npos) {
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = kv.second.first;
            res.name = kv.second.second;
            res.type = (kv.second.first.length() > 5) ? "Asteroid / Small Body" : "Major Body / Planet";
            res.details = "NASA JPL Horizons target code: " + kv.second.first;
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
                // Extract match lines if multiple records match
                std::istringstream stream(resText);
                std::string line;
                while (std::getline(stream, line)) {
                    if (line.find("Multiple major-bodies match") != std::string::npos || 
                        line.find("Matching small-bodies") != std::string::npos) {
                        // Scan subsequent lines for matches
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

    // Build URL for J2000 state vectors from Solar System Barycenter (@0)
    std::string url = getBaseUrl() + "?format=json"
                      "&COMMAND=" + HttpClient::urlEncode("'" + targetId + "'") +
                      "&CENTER='@0'"
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
        // Trim whitespace
        while (!line.empty() && (line.back() == ' ' || line.back() == '\r')) line.pop_back();
        while (!line.empty() && line.front() == ' ') line.erase(line.begin());
        if (!line.empty()) outRecord.object.name = line;
    }

    // 2. Extract Physical Parameters
    // Radius
    std::regex radRegex(R"(Radius\s*\(?km\)?\s*=\s*([0-9.+-E]+))", std::regex::icase);
    std::regex radMeanRegex(R"(Mean\s+Radius\s*\(?km\)?\s*=\s*([0-9.+-E]+))", std::regex::icase);
    std::smatch match;
    if (std::regex_search(rawText, match, radMeanRegex) || std::regex_search(rawText, match, radRegex)) {
        try {
            double radKm = std::stod(match[1].str());
            outRecord.physical.radiusM = radKm * 1000.0;
        } catch (...) {}
    }

    // Mass (e.g. Mass, 10^24 kg = 5.97219 or Mass (10^20 kg) = 9.38)
    std::regex mass24Regex(R"(Mass.*10\^24\s*kg\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex mass20Regex(R"(Mass.*10\^20\s*kg\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    std::regex massKgRegex(R"(Mass\s*\(?kg\)?\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, mass24Regex)) {
        try {
            outRecord.physical.massKg = std::stod(match[1].str()) * 1e24;
        } catch (...) {}
    } else if (std::regex_search(rawText, match, mass20Regex)) {
        try {
            outRecord.physical.massKg = std::stod(match[1].str()) * 1e20;
        } catch (...) {}
    } else if (std::regex_search(rawText, match, massKgRegex)) {
        try {
            outRecord.physical.massKg = std::stod(match[1].str());
        } catch (...) {}
    }

    // Density (g/cm^3 -> kg/m^3 (* 1000.0))
    std::regex densRegex(R"(density.*g/cm\^3\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, densRegex)) {
        try {
            outRecord.physical.meanDensityKgM3 = std::stod(match[1].str()) * 1000.0;
        } catch (...) {}
    }

    // Albedo
    std::regex albedoRegex(R"(albedo\s*=\s*~?([0-9.+-E]+))", std::regex::icase);
    if (std::regex_search(rawText, match, albedoRegex)) {
        try {
            outRecord.physical.albedo = std::stod(match[1].str());
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
        double vMps = glm::length(firstState.velocityMps);
        outRecord.orbital.semiMajorAxisM = rM;
        outRecord.orbital.semiMajorAxisAU = rM / UnitConverter::AU_TO_METERS;
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
                        rec.positionM = glm::dvec3(xKm * 1000.0, yKm * 1000.0, zKm * 1000.0);
                        rec.velocityMps = glm::dvec3(vxKmS * 1000.0, vyKmS * 1000.0, vzKmS * 1000.0);
                        rec.referenceFrame = "ICRF/Barycentric";
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
