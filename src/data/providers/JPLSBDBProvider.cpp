#include "data/providers/JPLSBDBProvider.hpp"
#include "data/UnitConverter.hpp"
#include <nlohmann/json.hpp>
#include <sstream>
#include <iostream>
#include <cmath>

namespace AstroGenesis {

JPLSBDBProvider::JPLSBDBProvider(HttpClient& httpClient) : m_http(httpClient) {}

bool JPLSBDBProvider::searchObjects(const std::string& query, 
                                    std::vector<SearchResult>& outResults, 
                                    std::string& outError) {
    outResults.clear();
    std::string url = getBaseUrl() + "?sstr=" + HttpClient::urlEncode(query) + "&phys-par=1";

    auto resp = m_http.get(url, {}, 10);
    if (!resp.success) {
        outError = "SBDB query failed: " + resp.errorMessage;
        // Provide fallback preview
        SearchResult res;
        res.sourceName = getProviderName();
        res.sourceId = query;
        res.name = query;
        res.type = "Asteroid / Comet";
        res.details = "Search directly via name or designation";
        outResults.push_back(res);
        return true;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);

        if (j.contains("object")) {
            SearchResult res;
            res.sourceName = getProviderName();
            res.sourceId = j["object"].value("spkid", query);
            res.name = j["object"].value("fullname", j["object"].value("shortname", query));
            
            std::string orbClass = "Asteroid";
            if (j["object"].contains("orbit_class") && j["object"]["orbit_class"].contains("name")) {
                orbClass = j["object"]["orbit_class"]["name"].get<std::string>();
            }
            res.type = orbClass;

            std::string details;
            if (j.contains("phys_par")) {
                for (const auto& p : j["phys_par"]) {
                    if (p.value("name", "") == "diameter") {
                        details += "Diameter: " + p.value("value", "") + " km | ";
                    }
                }
            }
            if (j.contains("orbit") && j["orbit"].contains("elements")) {
                for (const auto& elem : j["orbit"]["elements"]) {
                    if (elem.value("name", "") == "a") {
                        details += "a: " + elem.value("value", "") + " AU | ";
                    }
                    if (elem.value("name", "") == "e") {
                        details += "e: " + elem.value("value", "");
                    }
                }
            }
            res.details = details;
            outResults.push_back(res);
        } else if (j.contains("list")) {
            // Multiple matches returned in a list
            for (const auto& item : j["list"]) {
                SearchResult res;
                res.sourceName = getProviderName();
                res.sourceId = item.value("pdes", query);
                res.name = item.value("name", item.value("fullname", query));
                res.type = "Small Body";
                res.details = "Desig: " + item.value("pdes", "");
                outResults.push_back(res);
            }
        }
    } catch (const std::exception& e) {
        outError = "JSON parse error: " + std::string(e.what());
        return false;
    }

    return true;
}

bool JPLSBDBProvider::fetchObjectData(const std::string& sourceIdOrName, 
                                     CelestialBodyRecord& outRecord, 
                                     std::string& outError) {
    std::string url = getBaseUrl() + "?sstr=" + HttpClient::urlEncode(sourceIdOrName) + "&phys-par=1&cov=mat";

    auto resp = m_http.get(url, {}, 15);
    if (!resp.success) {
        outError = "JPL SBDB HTTP Request failed: " + resp.errorMessage;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);

        if (!j.contains("object")) {
            outError = "SBDB returned no object data for: " + sourceIdOrName;
            return false;
        }

        const auto& objNode = j["object"];
        std::string spkid = objNode.value("spkid", sourceIdOrName);
        std::string shortName = objNode.value("shortname", sourceIdOrName);
        std::string fullName = objNode.value("fullname", shortName);

        outRecord.sourceName = getProviderName();
        outRecord.object.slug = "sbdb_" + spkid;
        outRecord.object.name = shortName;
        outRecord.object.category = "Asteroid Belt";
        outRecord.object.isSynthetic = false;
        outRecord.object.color = glm::vec3(0.7f, 0.65f, 0.6f);

        std::string orbClass = "Asteroid";
        if (objNode.contains("orbit_class") && objNode["orbit_class"].contains("name")) {
            orbClass = objNode["orbit_class"]["name"].get<std::string>();
        }
        outRecord.object.type = orbClass;
        outRecord.physical.sourceRecordId = spkid;

        // Parse Physical Parameters
        if (j.contains("phys_par")) {
            for (const auto& p : j["phys_par"]) {
                std::string name = p.value("name", "");
                std::string valStr = p.value("value", "");
                if (valStr.empty()) continue;

                try {
                    double val = std::stod(valStr);
                    if (name == "diameter") {
                        outRecord.physical.radiusM = (val * 1000.0) / 2.0; // diameter km -> radius m
                    } else if (name == "GM") {
                        // GM in km^3 / s^2 -> Mass = GM * 1e9 / G_CONST
                        outRecord.physical.massKg = (val * 1e9) / UnitConverter::G_CONST;
                    } else if (name == "density") {
                        outRecord.physical.meanDensityKgM3 = val * 1000.0; // g/cm^3 -> kg/m^3
                    } else if (name == "albedo") {
                        outRecord.physical.albedo = val;
                    } else if (name == "rot_per") {
                        outRecord.physical.rotationPeriodHours = val;
                    }
                } catch (...) {}
            }
        }

        // Estimate mass if diameter and density are known or fallback to default asteroid density (2.0 g/cm^3)
        if (!outRecord.physical.massKg.has_value() && outRecord.physical.radiusM.has_value()) {
            double rM = outRecord.physical.radiusM.value();
            double dens = outRecord.physical.meanDensityKgM3.value_or(2000.0);
            double volM3 = (4.0 / 3.0) * UnitConverter::PI * std::pow(rM, 3.0);
            outRecord.physical.massKg = dens * volM3;
        }

        // Parse Orbital Elements
        if (j.contains("orbit")) {
            const auto& orbNode = j["orbit"];
            if (orbNode.contains("epoch")) {
                try {
                    outRecord.orbital.epochJd = std::stod(orbNode["epoch"].get<std::string>());
                } catch (...) {}
            }

            if (orbNode.contains("elements")) {
                for (const auto& elem : orbNode["elements"]) {
                    std::string name = elem.value("name", "");
                    std::string valStr = elem.value("value", "");
                    if (valStr.empty()) continue;

                    try {
                        double val = std::stod(valStr);
                        if (name == "a") {
                            outRecord.orbital.semiMajorAxisAU = val;
                            outRecord.orbital.semiMajorAxisM = val * UnitConverter::AU_TO_METERS;
                        } else if (name == "e") {
                            outRecord.orbital.eccentricity = val;
                        } else if (name == "i") {
                            outRecord.orbital.inclinationDeg = val;
                        } else if (name == "om") {
                            outRecord.orbital.longAscendingNodeDeg = val;
                        } else if (name == "w") {
                            outRecord.orbital.argPeriapsisDeg = val;
                        } else if (name == "ma") {
                            outRecord.orbital.meanAnomalyDeg = val;
                        } else if (name == "per") {
                            outRecord.orbital.orbitalPeriodDays = val;
                        }
                    } catch (...) {}
                }
            }
        }

        // Compute 3D State Vector from full Keplerian elements at epoch
        double aM = outRecord.orbital.semiMajorAxisM.value_or(2.7 * UnitConverter::AU_TO_METERS);
        double e = outRecord.orbital.eccentricity.value_or(0.08);
        double incDeg = outRecord.orbital.inclinationDeg.value_or(0.0);
        double nodeDeg = outRecord.orbital.longAscendingNodeDeg.value_or(0.0);
        double argDeg = outRecord.orbital.argPeriapsisDeg.value_or(0.0);
        double maDeg = outRecord.orbital.meanAnomalyDeg.value_or(0.0);
        double massSol = 1.9885e30;
        double massObj = outRecord.physical.massKg.value_or(1e15);

        outRecord.stateVector.epochJd = outRecord.orbital.epochJd;
        UnitConverter::keplerianToCartesian(
            aM, e, incDeg, nodeDeg, argDeg, maDeg,
            massSol, massObj,
            outRecord.stateVector.positionM.x, outRecord.stateVector.positionM.y, outRecord.stateVector.positionM.z,
            outRecord.stateVector.velocityMps.x, outRecord.stateVector.velocityMps.y, outRecord.stateVector.velocityMps.z
        );
        outRecord.stateVector.referenceFrame = "ICRF/Ecliptic_J2000";

        // Composition
        outRecord.composition = {
            { 0, 0, "Silicates / Pyroxene", 70.0f, {0.6f, 0.6f, 0.5f, 1.0f} },
            { 0, 0, "Carbonaceous Material", 30.0f, {0.3f, 0.3f, 0.3f, 1.0f} }
        };

        return true;
    } catch (const std::exception& e) {
        outError = "Error parsing SBDB JSON response: " + std::string(e.what());
        return false;
    }
}

bool JPLSBDBProvider::fetchEphemerisSeries(const std::string& sourceIdOrName, 
                                          double startJd, 
                                          double endJd, 
                                          double stepDays, 
                                          std::vector<EphemerisRecord>& outRecords, 
                                          std::string& outError) {
    outError = "Time-series ephemeris vector generation for SBDB asteroids should be queried via JPL Horizons.";
    return false;
}

} // namespace AstroGenesis
