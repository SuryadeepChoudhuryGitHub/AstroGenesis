#include "data/providers/NASAExoplanetProvider.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "data/UnitConverter.hpp"
#include "simulation/CelestialBody.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace AstroGenesis {

static std::string safeJsonString(const nlohmann::json& j, const std::string& key, const std::string& defVal = "") {
    if (j.contains(key) && !j[key].is_null()) {
        try {
            if (j[key].is_string()) return j[key].get<std::string>();
            if (j[key].is_number()) return std::to_string(j[key].get<double>());
        } catch (...) {}
    }
    return defVal;
}

static CelestialBodyRecord parseHostStarRecord(const nlohmann::json& rec, const std::string& providerName) {
    CelestialBodyRecord star;
    std::string hostName = safeJsonString(rec, "hostname", "Unknown Host Star");
    std::string specType = safeJsonString(rec, "st_spectype", "");

    star.sourceName = providerName;
    star.object.slug = hostName;
    std::transform(star.object.slug.begin(), star.object.slug.end(), star.object.slug.begin(), [](char c){
        return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
    });
    star.object.name = hostName;
    star.object.category = hostName;
    star.object.provenanceStatus = "NASA TAP Verified";

    // Stellar Mass
    double stMassSun = 1.0;
    if (!rec["st_mass"].is_null()) {
        stMassSun = rec["st_mass"].get<double>();
    }
    star.physical.massKg = stMassSun * UnitConverter::SOLAR_MASS_KG;

    // Stellar Radius
    double stRadSun = 1.0;
    if (!rec["st_rad"].is_null()) {
        stRadSun = rec["st_rad"].get<double>();
    }
    star.physical.radiusM = stRadSun * UnitConverter::SOLAR_RADIUS_M;

    // Effective Temperature
    double teff = 5778.0;
    if (!rec["st_teff"].is_null()) {
        teff = rec["st_teff"].get<double>();
    }
    star.physical.surfaceTempK = teff;

    // Spectral Class color and type
    std::string autoSpecType;
    star.object.color = VisualStateAdapter::getSpectralClassColor(teff, autoSpecType);
    if (specType.empty()) specType = autoSpecType;
    star.physical.spectralType = specType;
    star.object.type = specType + " Host Star";

    // Classification
    if (!specType.empty() && (specType[0] == 'M')) {
        star.object.classification = "Star (Red Dwarf)";
    } else if (stRadSun > 100.0) {
        star.object.classification = "Star (Supergiant)";
    } else if (stRadSun > 10.0) {
        star.object.classification = "Star (Giant)";
    } else {
        star.object.classification = "Star (Main Sequence)";
    }

    // Luminosity
    if (!rec["st_lum"].is_null()) {
        double logLum = rec["st_lum"].get<double>();
        star.physical.luminosityW = std::pow(10.0, logLum) * UnitConverter::SOLAR_LUMINOSITY_W;
    } else {
        static constexpr double SIGMA_SB = 5.670374419e-8;
        double rM = star.physical.radiusM.value();
        star.physical.luminosityW = 4.0 * UnitConverter::PI * rM * rM * SIGMA_SB * std::pow(teff, 4.0);
    }

    star.physical.sourceRecordId = hostName;
    star.physical.isEstimated = false;

    // State Vector (barycenter)
    star.stateVector.epochJd = UnitConverter::J2000_JD;
    star.stateVector.positionM = glm::dvec3(0.0);
    star.stateVector.velocityMps = glm::dvec3(0.0);
    star.stateVector.referenceFrame = "System-Barycentric";

    star.physical.atmosphereSummary = "Stellar Photosphere & Chromosphere (" + specType + ")";
    star.composition = {
        { 0, 0, "Hydrogen (H)", 74.0f, {0.9f, 0.4f, 0.2f, 1.0f} },
        { 0, 0, "Helium (He)", 24.0f, {1.0f, 0.8f, 0.3f, 1.0f} },
        { 0, 0, "Heavier Metals", 2.0f, {0.7f, 0.7f, 0.7f, 1.0f} }
    };

    return star;
}

static CelestialBodyRecord parsePlanetRecord(const nlohmann::json& rec, const std::string& providerName, double hostMassKg) {
    CelestialBodyRecord planet;
    std::string plName = safeJsonString(rec, "pl_name", "Unknown Exoplanet");
    std::string hostName = safeJsonString(rec, "hostname", "Unknown Host");

    planet.sourceName = providerName;
    planet.object.slug = plName;
    std::transform(planet.object.slug.begin(), planet.object.slug.end(), planet.object.slug.begin(), [](char c){
        return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
    });
    planet.object.name = plName;
    planet.object.category = hostName;
    planet.hostStarName = hostName;
    planet.object.provenanceStatus = "NASA Exoplanet Archive Verified";
    planet.physical.sourceRecordId = plName;

    // Mass (Earth Masses -> kg)
    double mE = 0.0;
    bool massEstimated = false;
    if (!rec["pl_bmasse"].is_null()) {
        mE = rec["pl_bmasse"].get<double>();
    } else if (!rec["pl_masse"].is_null()) {
        mE = rec["pl_masse"].get<double>();
    }

    // Radius (Earth Radii -> meters)
    double rE = 0.0;
    bool radiusEstimated = false;
    if (!rec["pl_rade"].is_null()) {
        rE = rec["pl_rade"].get<double>();
    } else if (!rec["pl_radj"].is_null()) {
        rE = rec["pl_radj"].get<double>() * 11.209;
    }

    // If one is missing, estimate from the other
    if (mE <= 0.0 && rE > 0.0) {
        massEstimated = true;
        // Empirical Chen & Kipping (2017) mass-radius relation
        if (rE < 1.23) {
            mE = std::pow(rE, 3.57);
        } else if (rE < 14.26) {
            mE = std::pow(rE / 1.008, 1.0 / 0.59);
        } else {
            mE = 317.8;
        }
    } else if (rE <= 0.0 && mE > 0.0) {
        radiusEstimated = true;
        if (mE < 2.0) {
            rE = std::pow(mE, 0.28);
        } else if (mE < 130.0) {
            rE = std::pow(mE, 0.55);
        } else {
            rE = 11.2;
        }
    } else if (mE <= 0.0 && rE <= 0.0) {
        mE = 1.0;
        rE = 1.0;
        massEstimated = true;
        radiusEstimated = true;
    }

    planet.physical.massKg = UnitConverter::earthMassToKg(mE);
    planet.physical.radiusM = UnitConverter::earthRadiusToMeters(rE);
    planet.physical.isEstimated = (massEstimated || radiusEstimated);

    // Classification
    if (mE < 0.1) {
        planet.object.classification = "Planet (Terrestrial)";
        planet.object.type = "Sub-Earth / Exodwarf";
    } else if (mE < 2.0 && rE < 1.5) {
        planet.object.classification = "Planet (Terrestrial)";
        planet.object.type = "Terrestrial Exoplanet";
    } else if (mE < 10.0 || rE < 2.5) {
        planet.object.classification = "Planet (Super-Earth)";
        planet.object.type = "Super-Earth";
    } else if (mE < 50.0 || rE < 6.0) {
        planet.object.classification = "Planet (Ice Giant)";
        planet.object.type = "Neptunian / Ice Giant";
    } else {
        planet.object.classification = "Planet (Gas Giant)";
        planet.object.type = "Jovian / Gas Giant";
    }

    // Temperature (Equilibrium)
    double teq = 288.0;
    if (!rec["pl_eqt"].is_null()) {
        teq = rec["pl_eqt"].get<double>();
    }
    planet.physical.surfaceTempK = teq;

    // Appearance Color: Physically grounded based on temperature & classification
    // (Preserve authentic planetary hue, never wash out with blinding white)
    if (planet.object.classification == "Planet (Gas Giant)") {
        if (teq >= 1400.0) {
            // Ultra-Hot Jupiter: Thermal glow / silicates vaporized
            planet.object.color = glm::vec3(0.35f, 0.15f, 0.12f);
        } else if (teq >= 800.0) {
            // Hot Jupiter: Dark alkali absorption / reddish-brown
            planet.object.color = glm::vec3(0.55f, 0.35f, 0.22f);
        } else if (teq >= 350.0) {
            // Warm Jupiter: Water cloud deck / pale tan
            planet.object.color = glm::vec3(0.80f, 0.70f, 0.55f);
        } else {
            // Cold Jovian: Ammonia clouds / Jupiter-like cream and ochre bands
            planet.object.color = glm::vec3(0.78f, 0.65f, 0.48f);
        }
    } else if (planet.object.classification == "Planet (Ice Giant)") {
        if (teq >= 400.0) {
            planet.object.color = glm::vec3(0.30f, 0.55f, 0.75f);
        } else {
            // Cold Ice Giant: Methane absorption azure blue (like Uranus / Neptune)
            planet.object.color = glm::vec3(0.28f, 0.52f, 0.82f);
        }
    } else {
        // Terrestrial / Super-Earth
        if (teq >= 1000.0) {
            // Lava world: Scorched basalt
            planet.object.color = glm::vec3(0.45f, 0.22f, 0.15f);
        } else if (teq >= 450.0) {
            // Scorched rocky / desert rust
            planet.object.color = glm::vec3(0.72f, 0.58f, 0.38f);
        } else if (teq >= 240.0) {
            // Temperate terrestrial / habitable zone
            planet.object.color = glm::vec3(0.25f, 0.48f, 0.65f);
        } else {
            // Cold rocky / icy world: Grey-brown silicate with subtle frost tint (NOT blinding white)
            planet.object.color = glm::vec3(0.55f, 0.58f, 0.62f);
        }
    }

    // Orbital Parameters
    if (!rec["pl_orbsmax"].is_null()) {
        double smaAU = rec["pl_orbsmax"].get<double>();
        planet.orbital.semiMajorAxisAU = smaAU;
        planet.orbital.semiMajorAxisM = UnitConverter::auToMeters(smaAU);
    }
    if (!rec["pl_orbeccen"].is_null()) {
        planet.orbital.eccentricity = rec["pl_orbeccen"].get<double>();
    } else {
        planet.orbital.eccentricity = 0.01;
    }
    if (!rec["pl_orbper"].is_null()) {
        planet.orbital.orbitalPeriodDays = rec["pl_orbper"].get<double>();
    }
    if (!rec["pl_orbincl"].is_null()) {
        planet.orbital.inclinationDeg = rec["pl_orbincl"].get<double>();
    }

    // State Vector
    double aM = planet.orbital.semiMajorAxisM.value_or(0.1 * UnitConverter::AU_TO_METERS);
    double e = planet.orbital.eccentricity.value_or(0.01);
    double rPeri = aM * (1.0 - e);
    double plMass = planet.physical.massKg.value_or(UnitConverter::EARTH_MASS_KG);
    double vPeri = std::sqrt((UnitConverter::G_CONST * (hostMassKg + plMass) / aM) * ((1.0 + e) / (1.0 - e)));

    planet.stateVector.epochJd = UnitConverter::J2000_JD;
    planet.stateVector.positionM = glm::dvec3(rPeri, 0.0, 0.0);
    planet.stateVector.velocityMps = glm::dvec3(0.0, 0.0, vPeri);
    planet.stateVector.referenceFrame = "Host-Barycentric";

    // Atmosphere and Composition
    if (planet.object.classification == "Planet (Gas Giant)" || planet.object.classification == "Planet (Ice Giant)") {
        planet.physical.atmosphereSummary = "Hydrogen-Helium Atmosphere (Host: " + hostName + ")";
        planet.composition = {
            { 0, 0, "Hydrogen / Helium", 85.0f, {0.4f, 0.7f, 0.9f, 1.0f} },
            { 0, 0, "Rocky / Metallic Core", 15.0f, {0.6f, 0.5f, 0.4f, 1.0f} }
        };
    } else {
        planet.physical.atmosphereSummary = "Silicate Mantle & Volatiles (Host: " + hostName + ")";
        planet.composition = {
            { 0, 0, "Silicates & Iron Mantle", 70.0f, {0.5f, 0.5f, 0.5f, 1.0f} },
            { 0, 0, "Atmosphere / Crust", 30.0f, {0.3f, 0.7f, 0.9f, 1.0f} }
        };
    }

    return planet;
}

NASAExoplanetProvider::NASAExoplanetProvider(HttpClient& httpClient) : m_http(httpClient) {}

bool NASAExoplanetProvider::searchObjects(const std::string& query, 
                                         std::vector<SearchResult>& outResults, 
                                         std::string& outError) {
    outResults.clear();
    if (query.empty()) return true;

    // Clean search token
    std::string cleanQ = query;
    if (cleanQ.rfind("star_", 0) == 0) cleanQ = cleanQ.substr(5);
    if (cleanQ.rfind("planet_", 0) == 0) cleanQ = cleanQ.substr(7);

    // TAP query to search planetary systems and host stars using composite parameters table
    std::string sqlQuery = "select top 40 pl_name, hostname, pl_bmasse, pl_rade, "
                           "pl_orbper, pl_orbsmax, pl_orbeccen, pl_eqt, st_spectype, st_teff, st_rad, st_mass, st_lum, sy_dist "
                           "from pscomppars where lower(pl_name) like lower('%" + cleanQ + "%') or lower(hostname) like lower('%" + cleanQ + "%') "
                           "order by sy_dist asc";

    std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";

    auto resp = m_http.get(url, {}, 12);
    if (!resp.success) {
        outError = "NASA Exoplanet query failed: " + resp.errorMessage;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (!j.is_array()) {
            outError = "Unexpected response from NASA TAP";
            return false;
        }

        std::unordered_set<std::string> seenHosts;
        std::unordered_set<std::string> seenPlanets;

        // 1. Extract Host Stars
        for (const auto& item : j) {
            std::string host = safeJsonString(item, "hostname", "");
            if (!host.empty() && seenHosts.find(host) == seenHosts.end()) {
                seenHosts.insert(host);

                SearchResult res;
                res.sourceName = getProviderName();
                res.sourceId = "star_" + host;
                res.name = host + " (Host Star)";

                std::string spec = safeJsonString(item, "st_spectype", "");
                res.type = spec.empty() ? "Host Star" : ("Host Star (" + spec + ")");

                std::string details = "Host Star | ";
                if (!item["st_mass"].is_null()) details += "Mass: " + std::to_string(item["st_mass"].get<double>()).substr(0, 4) + " M☉ | ";
                if (!item["st_rad"].is_null()) details += "Rad: " + std::to_string(item["st_rad"].get<double>()).substr(0, 4) + " R☉ | ";
                if (!item["st_teff"].is_null()) details += "Teff: " + std::to_string((int)item["st_teff"].get<double>()) + " K";
                res.details = details;

                outResults.push_back(res);
            }
        }

        // 2. Extract Exoplanets
        for (const auto& item : j) {
            std::string plName = safeJsonString(item, "pl_name", "");
            if (!plName.empty() && seenPlanets.find(plName) == seenPlanets.end()) {
                seenPlanets.insert(plName);

                SearchResult res;
                res.sourceName = getProviderName();
                res.sourceId = "planet_" + plName;
                res.name = plName;

                double massE = 1.0;
                if (!item["pl_bmasse"].is_null()) massE = item["pl_bmasse"].get<double>();

                if (massE < 0.1) res.type = "Exoplanet (Sub-Earth)";
                else if (massE < 2.0) res.type = "Exoplanet (Terrestrial)";
                else if (massE < 10.0) res.type = "Exoplanet (Super-Earth)";
                else if (massE < 50.0) res.type = "Exoplanet (Neptunian)";
                else res.type = "Exoplanet (Gas Giant)";

                std::string details = "Host: " + safeJsonString(item, "hostname", "") + " | ";
                if (!item["pl_bmasse"].is_null()) details += "Mass: " + std::to_string(item["pl_bmasse"].get<double>()).substr(0, 4) + " M⊕ | ";
                if (!item["pl_orbsmax"].is_null()) details += "a: " + std::to_string(item["pl_orbsmax"].get<double>()).substr(0, 5) + " AU | ";
                if (!item["pl_eqt"].is_null()) details += "Teq: " + std::to_string((int)item["pl_eqt"].get<double>()) + " K";
                res.details = details;

                outResults.push_back(res);
            }
        }
    } catch (const std::exception& e) {
        outError = "Exoplanet JSON parse error: " + std::string(e.what());
        return false;
    }

    return true;
}

bool NASAExoplanetProvider::fetchObjectData(const std::string& sourceIdOrName, 
                                           CelestialBodyRecord& outRecord, 
                                           std::string& outError) {
    std::string cleanId = sourceIdOrName;
    bool isStarTarget = false;

    if (cleanId.rfind("star_", 0) == 0) {
        cleanId = cleanId.substr(5);
        isStarTarget = true;
    } else if (cleanId.rfind("planet_", 0) == 0) {
        cleanId = cleanId.substr(7);
        isStarTarget = false;
    }

    if (isStarTarget) {
        // Fetch Host Star Data from pscomppars
        std::string sqlQuery = "select top 1 hostname, st_spectype, st_teff, st_rad, st_mass, st_lum, st_age, st_dens, sy_dist "
                               "from pscomppars where lower(hostname) = lower('" + cleanId + "') or lower(hostname) like lower('%" + cleanId + "%')";

        std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";
        auto resp = m_http.get(url, {}, 15);
        if (!resp.success) {
            outError = "NASA Exoplanet Host Star TAP request failed: " + resp.errorMessage;
            return false;
        }

        try {
            auto j = nlohmann::json::parse(resp.body);
            if (!j.is_array() || j.empty()) {
                outError = "No star record found matching: " + cleanId;
                return false;
            }

            outRecord = parseHostStarRecord(j[0], getProviderName());
            return true;
        } catch (const std::exception& e) {
            outError = "Host star JSON parse error: " + std::string(e.what());
            return false;
        }
    } else {
        // Fetch Exoplanet Data from pscomppars
        std::string sqlQuery = "select top 1 pl_name, hostname, pl_bmasse, pl_masse, pl_rade, pl_radj, "
                               "pl_orbper, pl_orbsmax, pl_orbeccen, pl_orbincl, pl_eqt, pl_dens, "
                               "st_spectype, st_teff, st_rad, st_mass, st_lum, sy_dist "
                               "from pscomppars where lower(pl_name) = lower('" + cleanId + "') or lower(pl_name) like lower('%" + cleanId + "%')";

        std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";
        auto resp = m_http.get(url, {}, 15);
        if (!resp.success) {
            outError = "NASA Exoplanet TAP request failed: " + resp.errorMessage;
            return false;
        }

        try {
            auto j = nlohmann::json::parse(resp.body);
            if (!j.is_array() || j.empty()) {
                outError = "No exoplanet record found matching: " + cleanId;
                return false;
            }

            double hostMass = UnitConverter::SOLAR_MASS_KG;
            if (!j[0]["st_mass"].is_null()) {
                hostMass = j[0]["st_mass"].get<double>() * UnitConverter::SOLAR_MASS_KG;
            }

            outRecord = parsePlanetRecord(j[0], getProviderName(), hostMass);
            return true;
        } catch (const std::exception& e) {
            outError = "Exoplanet detail parsing error: " + std::string(e.what());
            return false;
        }
    }
}

bool NASAExoplanetProvider::fetchSystemPlanets(const std::string& hostname, 
                                              std::vector<CelestialBodyRecord>& outPlanets, 
                                              CelestialBodyRecord& outHostStar, 
                                              std::string& outError) {
    outPlanets.clear();
    std::string cleanHost = hostname;
    if (cleanHost.rfind("star_", 0) == 0) cleanHost = cleanHost.substr(5);
    if (cleanHost.rfind("planet_", 0) == 0) cleanHost = cleanHost.substr(7);

    std::string sqlQuery = "select pl_name, hostname, pl_bmasse, pl_masse, pl_rade, pl_radj, pl_orbper, pl_orbsmax, pl_orbeccen, pl_orbincl, pl_eqt, pl_dens, "
                           "st_spectype, st_teff, st_rad, st_mass, st_lum, st_age, st_dens, sy_dist "
                           "from pscomppars where lower(hostname) = lower('" + cleanHost + "') or lower(hostname) like lower('%" + cleanHost + "%') "
                           "order by pl_orbsmax asc";

    std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";
    auto resp = m_http.get(url, {}, 15);
    if (!resp.success) {
        outError = "NASA TAP query failed: " + resp.errorMessage;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (!j.is_array() || j.empty()) {
            outError = "No system found for host star: " + cleanHost;
            return false;
        }

        // 1. Build Host Star from first record
        outHostStar = parseHostStarRecord(j[0], getProviderName());
        double hostMassKg = outHostStar.physical.massKg.value_or(UnitConverter::SOLAR_MASS_KG);

        // 2. Build each planet
        std::unordered_set<std::string> seenPlanets;
        for (const auto& item : j) {
            std::string plName = safeJsonString(item, "pl_name", "");
            if (plName.empty() || seenPlanets.count(plName)) continue;
            seenPlanets.insert(plName);

            auto plRec = parsePlanetRecord(item, getProviderName(), hostMassKg);
            outPlanets.push_back(plRec);
        }

        return true;
    } catch (const std::exception& e) {
        outError = "NASA TAP parse error: " + std::string(e.what());
        return false;
    }
}

bool NASAExoplanetProvider::fetchEphemerisSeries(const std::string& sourceIdOrName, 
                                                double startJd, 
                                                double endJd, 
                                                double stepDays, 
                                                std::vector<EphemerisRecord>& outRecords, 
                                                std::string& outError) {
    outError = "Ephemeris time series for exoplanets is computed numerically in AstroGenesis.";
    return false;
}

} // namespace AstroGenesis
