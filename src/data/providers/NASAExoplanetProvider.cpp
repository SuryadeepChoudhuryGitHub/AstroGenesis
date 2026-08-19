#include "data/providers/NASAExoplanetProvider.hpp"
#include "data/UnitConverter.hpp"
#include <nlohmann/json.hpp>
#include <iostream>
#include <cmath>

namespace AstroGenesis {

NASAExoplanetProvider::NASAExoplanetProvider(HttpClient& httpClient) : m_http(httpClient) {}

bool NASAExoplanetProvider::searchObjects(const std::string& query, 
                                         std::vector<SearchResult>& outResults, 
                                         std::string& outError) {
    outResults.clear();
    
    // TAP query to search planetary systems
    std::string sqlQuery = "select top 25 pl_name, hostname, pl_masse, pl_rade, pl_orbper, pl_sma, pl_orbeccen, sy_dist "
                           "from ps where pl_name like '%" + query + "%' or hostname like '%" + query + "%'";
    
    std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";

    auto resp = m_http.get(url, {}, 12);
    if (!resp.success) {
        outError = "NASA Exoplanet query failed: " + resp.errorMessage;
        // Fallback preview result
        SearchResult res;
        res.sourceName = getProviderName();
        res.sourceId = query;
        res.name = query;
        res.type = "Exoplanet";
        res.details = "Search NASA TAP database";
        outResults.push_back(res);
        return true;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (j.is_array()) {
            for (const auto& item : j) {
                SearchResult res;
                res.sourceName = getProviderName();
                res.sourceId = item.value("pl_name", query);
                res.name = item.value("pl_name", query);
                res.type = "Confirmed Exoplanet";

                std::string details = "Host: " + item.value("hostname", "") + " | ";
                if (!item["pl_masse"].is_null()) details += "Mass: " + std::to_string(item["pl_masse"].get<double>()).substr(0, 4) + " M⊕ | ";
                if (!item["pl_orbper"].is_null()) details += "Period: " + std::to_string(item["pl_orbper"].get<double>()).substr(0, 5) + " d";
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
    std::string sqlQuery = "select top 1 pl_name, hostname, pl_masse, pl_rade, pl_orbper, pl_sma, pl_orbeccen, pl_eqt, "
                           "st_mass, st_rad, st_teff, st_lum, st_spectype, sy_dist "
                           "from ps where pl_name = '" + sourceIdOrName + "' or pl_name like '%" + sourceIdOrName + "%'";

    std::string url = getBaseUrl() + "?query=" + HttpClient::urlEncode(sqlQuery) + "&format=json";

    auto resp = m_http.get(url, {}, 15);
    if (!resp.success) {
        outError = "NASA Exoplanet TAP request failed: " + resp.errorMessage;
        return false;
    }

    try {
        auto j = nlohmann::json::parse(resp.body);
        if (!j.is_array() || j.empty()) {
            outError = "No exoplanet record found matching: " + sourceIdOrName;
            return false;
        }

        const auto& rec = j[0];
        std::string plName = rec.value("pl_name", sourceIdOrName);
        std::string hostName = rec.value("hostname", "Host Star");

        outRecord.sourceName = getProviderName();
        outRecord.object.slug = plName;
        std::transform(outRecord.object.slug.begin(), outRecord.object.slug.end(), outRecord.object.slug.begin(), [](char c){
            return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
        });
        outRecord.object.name = plName;
        outRecord.object.type = "Confirmed Exoplanet";
        outRecord.object.category = "Exoplanet System";
        outRecord.object.color = glm::vec3(0.2f, 0.75f, 0.85f);
        outRecord.physical.sourceRecordId = plName;

        // Mass (Earth Masses -> kg)
        if (!rec["pl_masse"].is_null()) {
            double mE = rec["pl_masse"].get<double>();
            outRecord.physical.massKg = UnitConverter::earthMassToKg(mE);
        } else {
            outRecord.physical.massKg = UnitConverter::earthMassToKg(1.0);
        }

        // Radius (Earth Radii -> meters)
        if (!rec["pl_rade"].is_null()) {
            double rE = rec["pl_rade"].get<double>();
            outRecord.physical.radiusM = UnitConverter::earthRadiusToMeters(rE);
        } else {
            outRecord.physical.radiusM = UnitConverter::earthRadiusToMeters(1.0);
        }

        // Temperature
        if (!rec["pl_eqt"].is_null()) {
            outRecord.physical.surfaceTempK = rec["pl_eqt"].get<double>();
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

        // Compute State Vector
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

        // Atmosphere / Composition placeholder
        outRecord.physical.atmosphereSummary = "Exoplanet Atmosphere (Host: " + hostName + ")";
        outRecord.composition = {
            { 0, 0, "Silicates / Iron Core", 70.0f, {0.3f, 0.7f, 0.8f, 1.0f} },
            { 0, 0, "Atmospheric Volatiles", 30.0f, {0.6f, 0.8f, 1.0f, 1.0f} }
        };

        return true;
    } catch (const std::exception& e) {
        outError = "Exoplanet detail parsing error: " + std::string(e.what());
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
