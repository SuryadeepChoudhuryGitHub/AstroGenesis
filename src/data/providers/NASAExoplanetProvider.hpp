#pragma once

#include "data/providers/IAstronomicalDataProvider.hpp"
#include "net/HttpClient.hpp"

namespace AstroGenesis {

class NASAExoplanetProvider : public IAstronomicalDataProvider {
public:
    explicit NASAExoplanetProvider(HttpClient& httpClient);

    std::string getProviderName() const override { return "NASA Exoplanet Archive"; }
    std::string getBaseUrl() const override { return "https://exoplanetarchive.ipac.caltech.edu/TAP/sync"; }

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
