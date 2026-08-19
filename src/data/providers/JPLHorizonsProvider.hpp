#pragma once

#include "data/providers/IAstronomicalDataProvider.hpp"
#include "net/HttpClient.hpp"
#include <map>

namespace AstroGenesis {

class JPLHorizonsProvider : public IAstronomicalDataProvider {
public:
    explicit JPLHorizonsProvider(HttpClient& httpClient);

    std::string getProviderName() const override { return "NASA JPL Horizons"; }
    std::string getBaseUrl() const override { return "https://ssd.jpl.nasa.gov/api/horizons.api"; }

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

    // Helper: Map common body names to JPL Horizons IDs
    static std::string resolveHorizonsId(const std::string& nameOrId);

private:
    HttpClient& m_http;
    bool parseHorizonsResponse(const std::string& rawText, CelestialBodyRecord& outRecord, std::string& outError);
    bool parseVectorBlock(const std::string& rawText, std::vector<EphemerisRecord>& outRecords, const std::string& targetName);
};

} // namespace AstroGenesis
