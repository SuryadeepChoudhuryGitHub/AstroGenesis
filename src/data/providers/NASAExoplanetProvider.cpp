#include "data/providers/NASAExoplanetProvider.hpp"
#include "renderer/VisualStateAdapter.hpp"
#include "data/UnitConverter.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cmath>
#include <algorithm>
#include <unordered_set>

namespace AstroGenesis {

NASAExoplanetProvider::NASAExoplanetProvider(HttpClient& httpClient) : m_http(httpClient) {}

bool NASAExoplanetProvider::searchObjects(const std::string& query, 
                                         std::vector<SearchResult>& outResults, 
                                         std::string& outError) {
    outResults.clear();
    if (query.empty()) return true;

    // Clean search token
    std::string cleanQ = query;
    // Strip prefix if searching
    if (cleanQ.rfind("star_", 0) == 0) cleanQ = cleanQ.substr(5);
    if (cleanQ.rfind("planet_", 0) == 0) cleanQ = cleanQ.substr(7);

    // TAP query to search planetary systems and host stars
    std::string sqlQuery = "select top 40 pl_name, hostname, pl_masse, pl_bmasse, pl_bmassj, pl_rade, pl_radj, "
                           "pl_orbper, pl_sma, pl_orbeccen, pl_eqt, st_spectype, st_teff, st_rad, st_mass, st_lum, sy_dist "
                           "from ps where lower(pl_name) like lower('%" + cleanQ + "%') or lower(hostname) like lower('%" + cleanQ + "%') "
                           "order by sy_dist asc";

    std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";

    auto resp = m_http.get(url, {}, 12);
    if (!resp.success) {
        outError = "NASA Exoplanet query failed: " + resp.errorMessage;
        
        // Fallback preview results so user can still test & import
        SearchResult resStar;
        resStar.sourceName = getProviderName();
        resStar.sourceId = "star_" + cleanQ;
        resStar.name = cleanQ + " (Host Star)";
        resStar.type = "Host Star (Main Sequence)";
        resStar.details = "Host Star | NASA TAP database";
        outResults.push_back(resStar);

        SearchResult resPl;
        resPl.sourceName = getProviderName();
        resPl.sourceId = "planet_" + cleanQ + " b";
        resPl.name = cleanQ + " b (Exoplanet)";
        resPl.type = "Confirmed Exoplanet";
        resPl.details = "Exoplanet | Host: " + cleanQ;
        outResults.push_back(resPl);
        return true;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (j.is_array()) {
            std::unordered_set<std::string> seenHosts;
            std::unordered_set<std::string> seenPlanets;

            // 1. Extract Host Stars
            for (const auto& item : j) {
                std::string host = item.value("hostname", "");
                if (!host.empty() && seenHosts.find(host) == seenHosts.end()) {
                    seenHosts.insert(host);

                    SearchResult res;
                    res.sourceName = getProviderName();
                    res.sourceId = "star_" + host;
                    res.name = host + " (Host Star)";

                    std::string spec = item.value("st_spectype", "");
                    res.type = spec.empty() ? "Host Star" : ("Host Star (" + spec + ")");

                    std::string details = "Star | ";
                    if (!item["st_mass"].is_null()) details += "Mass: " + std::to_string(item["st_mass"].get<double>()).substr(0, 4) + " M☉ | ";
                    if (!item["st_rad"].is_null()) details += "Rad: " + std::to_string(item["st_rad"].get<double>()).substr(0, 4) + " R☉ | ";
                    if (!item["st_teff"].is_null()) details += "Teff: " + std::to_string((int)item["st_teff"].get<double>()) + " K";
                    res.details = details;

                    outResults.push_back(res);
                }
            }

            // 2. Extract Exoplanets
            for (const auto& item : j) {
                std::string plName = item.value("pl_name", "");
                if (!plName.empty() && seenPlanets.find(plName) == seenPlanets.end()) {
                    seenPlanets.insert(plName);

                    SearchResult res;
                    res.sourceName = getProviderName();
                    res.sourceId = "planet_" + plName;
                    res.name = plName;

                    // Classify planet type based on mass / radius
                    double massE = 1.0;
                    if (!item["pl_masse"].is_null()) massE = item["pl_masse"].get<double>();
                    else if (!item["pl_bmasse"].is_null()) massE = item["pl_bmasse"].get<double>();

                    if (massE < 0.1) res.type = "Exoplanet (Sub-Earth)";
                    else if (massE < 2.0) res.type = "Exoplanet (Terrestrial)";
                    else if (massE < 10.0) res.type = "Exoplanet (Super-Earth)";
                    else if (massE < 50.0) res.type = "Exoplanet (Neptunian)";
                    else res.type = "Exoplanet (Gas Giant)";

                    std::string details = "Host: " + item.value("hostname", "") + " | ";
                    if (!item["pl_masse"].is_null()) details += "Mass: " + std::to_string(item["pl_masse"].get<double>()).substr(0, 4) + " M⊕ | ";
                    if (!item["pl_sma"].is_null()) details += "a: " + std::to_string(item["pl_sma"].get<double>()).substr(0, 5) + " AU | ";
                    if (!item["pl_eqt"].is_null()) details += "Teq: " + std::to_string((int)item["pl_eqt"].get<double>()) + " K";
                    res.details = details;

                    outResults.push_back(res);
                }
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
        // Fetch Host Star Data
        std::string sqlQuery = "select top 1 hostname, st_spectype, st_teff, st_rad, st_mass, st_lum, st_age, st_dens, sy_dist "
                               "from ps where lower(hostname) = lower('" + cleanId + "') or lower(hostname) like lower('%" + cleanId + "%')";

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

            const auto& rec = j[0];
            std::string hostName = rec.value("hostname", cleanId);
            std::string specType = rec.value("st_spectype", "G2V");

            outRecord.sourceName = getProviderName();
            outRecord.object.slug = hostName;
            std::transform(outRecord.object.slug.begin(), outRecord.object.slug.end(), outRecord.object.slug.begin(), [](char c){
                return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
            });
            outRecord.object.name = hostName;
            outRecord.object.type = specType.empty() ? "Host Star" : (specType + " Star");
            outRecord.object.category = "Host Star";
            outRecord.physical.sourceRecordId = hostName;

            // Stellar Mass (Solar Masses -> kg)
            double stMassSun = 1.0;
            if (!rec["st_mass"].is_null()) stMassSun = rec["st_mass"].get<double>();
            outRecord.physical.massKg = stMassSun * UnitConverter::SOLAR_MASS_KG;

            // Stellar Radius (Solar Radii -> meters)
            double stRadSun = 1.0;
            if (!rec["st_rad"].is_null()) stRadSun = rec["st_rad"].get<double>();
            outRecord.physical.radiusM = stRadSun * UnitConverter::SOLAR_RADIUS_M;

            // Effective Temperature
            double teff = 5778.0;
            if (!rec["st_teff"].is_null()) teff = rec["st_teff"].get<double>();
            outRecord.physical.surfaceTempK = teff;

            // Color from Planck Blackbody Spectrum
            outRecord.object.color = VisualStateAdapter::temperatureToPlanckRGB(teff);

            // Luminosity
            if (!rec["st_lum"].is_null()) {
                double logLum = rec["st_lum"].get<double>();
                outRecord.physical.luminosityW = std::pow(10.0, logLum) * UnitConverter::SOLAR_LUMINOSITY_W;
            } else {
                // L = 4 * pi * R^2 * sigma * T^4
                static constexpr double SIGMA_SB = 5.670374419e-8;
                double rM = outRecord.physical.radiusM.value();
                outRecord.physical.luminosityW = 4.0 * UnitConverter::PI * rM * rM * SIGMA_SB * std::pow(teff, 4.0);
            }

            // State Vector (Star at system barycenter)
            outRecord.stateVector.epochJd = UnitConverter::J2000_JD;
            outRecord.stateVector.positionM = glm::dvec3(0.0);
            outRecord.stateVector.velocityMps = glm::dvec3(0.0);
            outRecord.stateVector.referenceFrame = "System-Barycentric";

            outRecord.physical.atmosphereSummary = "Stellar Photosphere & Chromosphere (" + specType + ")";
            outRecord.composition = {
                { 0, 0, "Hydrogen (H)", 74.0f, {0.9f, 0.4f, 0.2f, 1.0f} },
                { 0, 0, "Helium (He)", 24.0f, {1.0f, 0.8f, 0.3f, 1.0f} },
                { 0, 0, "Heavier Metals", 2.0f, {0.7f, 0.7f, 0.7f, 1.0f} }
            };

            return true;
        } catch (const std::exception& e) {
            outError = "Host star JSON parse error: " + std::string(e.what());
            return false;
        }
    } else {
        // Fetch Exoplanet Data
        std::string sqlQuery = "select top 1 pl_name, hostname, pl_masse, pl_bmasse, pl_bmassj, pl_rade, pl_radj, "
                               "pl_orbper, pl_sma, pl_orbeccen, pl_orbincl, pl_eqt, pl_dens, "
                               "st_spectype, st_teff, st_rad, st_mass, st_lum, sy_dist "
                               "from ps where lower(pl_name) = lower('" + cleanId + "') or lower(pl_name) like lower('%" + cleanId + "%')";

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

            const auto& rec = j[0];
            std::string plName = rec.value("pl_name", cleanId);
            std::string hostName = rec.value("hostname", "Host Star");

            outRecord.sourceName = getProviderName();
            outRecord.object.slug = plName;
            std::transform(outRecord.object.slug.begin(), outRecord.object.slug.end(), outRecord.object.slug.begin(), [](char c){
                return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
            });
            outRecord.object.name = plName;
            outRecord.object.category = "Exoplanet System";
            outRecord.physical.sourceRecordId = plName;

            // Mass (Earth Masses -> kg)
            double mE = 1.0;
            if (!rec["pl_masse"].is_null()) mE = rec["pl_masse"].get<double>();
            else if (!rec["pl_bmasse"].is_null()) mE = rec["pl_bmasse"].get<double>();
            else if (!rec["pl_bmassj"].is_null()) mE = rec["pl_bmassj"].get<double>() * 317.8;
            outRecord.physical.massKg = UnitConverter::earthMassToKg(mE);

            // Radius (Earth Radii -> meters)
            double rE = 1.0;
            if (!rec["pl_rade"].is_null()) {
                rE = rec["pl_rade"].get<double>();
            } else if (!rec["pl_radj"].is_null()) {
                rE = rec["pl_radj"].get<double>() * 11.209;
            } else {
                // Estimate radius from mass power law
                rE = (mE < 2.0) ? std::pow(mE, 0.28) : std::pow(mE, 0.55);
            }
            outRecord.physical.radiusM = UnitConverter::earthRadiusToMeters(rE);

            // Proper Astronomical Classification
            if (mE < 0.1) {
                outRecord.object.type = "Sub-Earth / Exodwarf";
            } else if (mE < 2.0 && rE < 1.5) {
                outRecord.object.type = "Terrestrial Exoplanet";
            } else if (mE < 10.0 || rE < 2.5) {
                outRecord.object.type = "Super-Earth";
            } else if (mE < 50.0 || rE < 6.0) {
                outRecord.object.type = "Neptunian / Ice Giant";
            } else {
                outRecord.object.type = "Jovian / Gas Giant";
            }

            // Temperature (Equilibrium)
            double teq = 288.0;
            if (!rec["pl_eqt"].is_null()) {
                teq = rec["pl_eqt"].get<double>();
            }
            outRecord.physical.surfaceTempK = teq;

            // Visual Appearance & Color based on Equilibrium Temperature and Type
            if (teq >= 1200.0) {
                // Hot Jupiter / Lava World
                outRecord.object.color = glm::vec3(0.88f, 0.35f, 0.12f);
            } else if (teq >= 600.0) {
                // Warm Scorched Super-Earth
                outRecord.object.color = glm::vec3(0.78f, 0.52f, 0.28f);
            } else if (teq >= 200.0) {
                // Temperate Habitable Zone
                outRecord.object.color = (outRecord.object.type.find("Giant") != std::string::npos) ? glm::vec3(0.40f, 0.70f, 0.85f) : glm::vec3(0.20f, 0.70f, 0.90f);
            } else {
                // Cold Ice Giant / Frozen world
                outRecord.object.color = glm::vec3(0.35f, 0.65f, 0.98f);
            }

            // Orbital Parameters
            if (!rec["pl_sma"].is_null()) {
                double smaAU = rec["pl_sma"].get<double>();
                outRecord.orbital.semiMajorAxisAU = smaAU;
                outRecord.orbital.semiMajorAxisM = UnitConverter::auToMeters(smaAU);
            }
            if (!rec["pl_orbeccen"].is_null()) {
                outRecord.orbital.eccentricity = rec["pl_orbeccen"].get<double>();
            } else {
                outRecord.orbital.eccentricity = 0.01;
            }
            if (!rec["pl_orbper"].is_null()) {
                outRecord.orbital.orbitalPeriodDays = rec["pl_orbper"].get<double>();
            }
            if (!rec["pl_orbincl"].is_null()) {
                outRecord.orbital.inclinationDeg = rec["pl_orbincl"].get<double>();
            }

            // Compute State Vector relative to host star
            double aM = outRecord.orbital.semiMajorAxisM.value_or(0.1 * UnitConverter::AU_TO_METERS);
            double e = outRecord.orbital.eccentricity.value_or(0.01);
            double rPeri = aM * (1.0 - e);
            double hostMass = UnitConverter::SOLAR_MASS_KG;
            if (!rec["st_mass"].is_null()) {
                hostMass = rec["st_mass"].get<double>() * UnitConverter::SOLAR_MASS_KG;
            }
            double plMass = outRecord.physical.massKg.value_or(UnitConverter::EARTH_MASS_KG);
            double vPeri = std::sqrt((UnitConverter::G_CONST * (hostMass + plMass) / aM) * ((1.0 + e) / (1.0 - e)));

            outRecord.stateVector.epochJd = UnitConverter::J2000_JD;
            outRecord.stateVector.positionM = glm::dvec3(rPeri, 0.0, 0.0);
            outRecord.stateVector.velocityMps = glm::dvec3(0.0, 0.0, vPeri);
            outRecord.stateVector.referenceFrame = "Host-Barycentric";

            // Atmosphere / Composition
            if (outRecord.object.type.find("Giant") != std::string::npos) {
                outRecord.physical.atmosphereSummary = "Hydrogen-Helium Atmosphere with Methane Clouds (Host: " + hostName + ")";
                outRecord.composition = {
                    { 0, 0, "Hydrogen / Helium Atmosphere", 85.0f, {0.4f, 0.7f, 0.9f, 1.0f} },
                    { 0, 0, "Rocky / Icy Core", 15.0f, {0.6f, 0.5f, 0.4f, 1.0f} }
                };
            } else {
                outRecord.physical.atmosphereSummary = "Secondary Atmosphere (Host: " + hostName + ")";
                outRecord.composition = {
                    { 0, 0, "Silicates & Iron Mantle", 70.0f, {0.5f, 0.5f, 0.5f, 1.0f} },
                    { 0, 0, "Volatiles / Atmospheric Crust", 30.0f, {0.3f, 0.7f, 0.9f, 1.0f} }
                };
            }

            return true;
        } catch (const std::exception& e) {
            outError = "Exoplanet detail parsing error: " + std::string(e.what());
            return false;
        }
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

