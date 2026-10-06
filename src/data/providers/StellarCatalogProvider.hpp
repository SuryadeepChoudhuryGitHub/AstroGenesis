#pragma once

#include "data/providers/IAstronomicalDataProvider.hpp"
#include "net/HttpClient.hpp"
#include <unordered_map>
#include <vector>
#include <string>

namespace AstroGenesis {

class StellarCatalogProvider : public IAstronomicalDataProvider {
public:
    explicit StellarCatalogProvider(HttpClient& httpClient);

    std::string getProviderName() const override { return "Stellar Catalogue (CDS/SIMBAD & Bright Stars)"; }
    std::string getBaseUrl() const override { return "https://cds.unistra.fr/cgi-bin/nph-sesame/-oI/SNVA"; }

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

    bool fetchSystemComponents(const std::string& systemNameOrId,
                               std::vector<CelestialBodyRecord>& outComponents,
                               std::string& outError) override;

    // Helper: Determine if a query token is likely targeted for a star or stellar system
    static bool isLikelyStellarQuery(const std::string& query);

private:
    HttpClient& m_http;

    // Curated catalog entries for major navigational and astrophysical benchmark stars
    struct CuratedStarEntry {
        std::string slug;
        std::string name;
        std::string systemName;
        std::string spectralType;
        std::string classification;
        double massMsun = 1.0;
        double radiusRsun = 1.0;
        double teffK = 5778.0;
        double lumLsun = 1.0;
        double distanceLy = 10.0;
        double vMag = 0.0;
        std::string mainAlias;
        std::string crossIdents;
        glm::vec3 color{1.0f};
        // Binary / Multiple system parameters if component of a bound orbit
        bool isBinaryComponent = false;
        double binarySemiMajorAU = 0.0;
        double binaryPeriodYr = 0.0;
        double binaryEcc = 0.0;
        glm::dvec3 barycentricPosM{0.0};
        glm::dvec3 barycentricVelMps{0.0};
    };

    struct CuratedSystemEntry {
        std::string systemName;
        std::string type; // "Binary Star System", "Triple Star System", "Single Star System"
        std::string description;
        std::vector<std::string> componentSlugs;
    };

    void initializeCuratedCatalog();
    bool queryCDSSesame(const std::string& query, CelestialBodyRecord& outRecord, std::vector<SearchResult>& outResults, std::string& outError);

    std::unordered_map<std::string, CuratedStarEntry> m_curatedStars;
    std::unordered_map<std::string, CuratedSystemEntry> m_curatedSystems;
    std::unordered_map<std::string, std::string> m_aliasToSlug;
};

} // namespace AstroGenesis
