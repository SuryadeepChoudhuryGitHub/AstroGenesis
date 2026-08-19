#pragma once

#include <string>
#include <vector>
#include <memory>
#include "data/AstronomicalModels.hpp"

namespace AstroGenesis {

class IAstronomicalDataProvider {
public:
    virtual ~IAstronomicalDataProvider() = default;

    virtual std::string getProviderName() const = 0;
    virtual std::string getBaseUrl() const = 0;

    // Search objects from external API
    virtual bool searchObjects(const std::string& query, 
                               std::vector<SearchResult>& outResults, 
                               std::string& outError) = 0;

    // Fetch full object details (physical, orbital, state vectors)
    virtual bool fetchObjectData(const std::string& sourceIdOrName, 
                                CelestialBodyRecord& outRecord, 
                                std::string& outError) = 0;

    // Fetch ephemeris time series for validation
    virtual bool fetchEphemerisSeries(const std::string& sourceIdOrName, 
                                     double startJd, 
                                     double endJd, 
                                     double stepDays, 
                                     std::vector<EphemerisRecord>& outRecords, 
                                     std::string& outError) = 0;
};

} // namespace AstroGenesis
