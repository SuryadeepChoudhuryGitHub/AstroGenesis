#pragma once

#include "data/providers/IAstronomicalDataProvider.hpp"
#include "net/HttpClient.hpp"

namespace AstroGenesis {

class JPLSBDBProvider : public IAstronomicalDataProvider {
public:
    explicit JPLSBDBProvider(HttpClient& httpClient);

    std::string getProviderName() const override { return "NASA JPL Small-Body Database (SBDB)"; }
    std::string getBaseUrl() const override { return "https://ssd-api.jpl.nasa.gov/sbdb.api"; }

    bool searchObjects(const std::string& query, 
                       std::vector<SearchResult>& outResults, 
                       std::string& outError) override;

    bool fetchObjectData(const std::string& sourceIdOrName, 
                         CelestialBodyRecord& outRecord, 
                         std::string& outError) override;

    bool fetchEphemerisSeries(const std::string& sourceIdOrName, 
                              double startJd, 
                              double endJd, 
                              double stepDays, 
                              std::vector<EphemerisRecord>& outRecords, 
                              std::string& outError) override;

private:
    HttpClient& m_http;
};

} // namespace AstroGenesis
