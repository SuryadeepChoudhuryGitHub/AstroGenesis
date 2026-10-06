#pragma once

#include <string>
#include <vector>
#include <memory>
#include <map>
#include <future>
#include <mutex>
#include "data/DatabaseManager.hpp"
#include "data/AstronomicalModels.hpp"
#include "data/repositories/ObjectRepository.hpp"
#include "data/repositories/EphemerisRepository.hpp"
#include "data/repositories/ValidationRepository.hpp"
#include "data/providers/IAstronomicalDataProvider.hpp"
#include "data/providers/JPLHorizonsProvider.hpp"
#include "data/providers/JPLSBDBProvider.hpp"
#include "data/providers/NASAExoplanetProvider.hpp"
#include "data/providers/StellarCatalogProvider.hpp"
#include "net/HttpClient.hpp"

namespace AstroGenesis {

enum class ProviderType {
    Auto_Resolve = 0,     // Automatic cross-catalogue object resolver
    Stellar_Catalog,      // Stellar Catalogue (CDS/SIMBAD & Bright Stars)
    NASA_Exoplanet,       // NASA Exoplanet Archive (Confirmed Exoplanets & Host Stars)
    JPL_Horizons,         // NASA JPL Horizons (Major Solar System Bodies & Moons)
    JPL_SBDB              // NASA JPL Small-Body Database (Asteroids & Comets)
};

struct AsyncJobState {
    bool isRunning = false;
    std::string currentTask;
    float progress = 0.0f;
    std::string lastResult;
    bool lastSuccess = true;
    std::string errorMessage;
};

class DataManager {
public:
    DataManager(DatabaseManager& db, 
                ObjectRepository& objRepo, 
                EphemerisRepository& ephemRepo,
                ValidationRepository& valRepo);
    ~DataManager();

    void initialize();

    // Provider Access
    IAstronomicalDataProvider* getProvider(ProviderType type);
    std::vector<std::string> getProviderNames() const;

    // Cross-Catalog Object Resolution
    std::vector<SearchResult> resolveQuery(const std::string& query, std::string& outError);

    // Asynchronous Search
    void searchAsync(ProviderType provider, const std::string& query);
    bool isSearching() const;
    std::vector<SearchResult> getSearchResults();

    // Asynchronous Import
    void importObjectAsync(ProviderType provider, const std::string& sourceIdOrName, const std::string& categoryOverride = "");
    bool isImporting() const;
    AsyncJobState getImportJobState();

    // Direct Synchronous Import (Worker Thread)
    bool importObject(ProviderType provider, const std::string& sourceIdOrName, const std::string& categoryOverride, std::string& outError);

    // Planetary System Import (Host Star + all orbiting exoplanets)
    bool importExoplanetSystem(const std::string& hostname, std::string& outError);
    void importExoplanetSystemAsync(const std::string& hostname);

    // Multiple-Star / Stellar System Import (e.g. Sirius A + Sirius B, Alpha Centauri A + B)
    bool importStellarSystem(const std::string& systemName, std::string& outError);
    void importStellarSystemAsync(const std::string& systemName);

    // Ephemeris Sync
    bool fetchAndStoreEphemerisSeries(const std::string& sourceIdOrName, int64_t objectId, double startJd, double endJd, double stepDays, std::string& outError);

    // Import History
    std::vector<DataImportRecord> getImportHistory(int limit = 50);
    void logImport(const std::string& providerName, const std::string& targetObject, bool success, int count, const std::string& details);

    // Offline / Online Mode
    bool isOfflineMode() const { return m_offlineMode; }
    void setOfflineMode(bool offline) { m_offlineMode = offline; }

private:
    DatabaseManager& m_db;
    ObjectRepository& m_objRepo;
    EphemerisRepository& m_ephemRepo;
    ValidationRepository& m_valRepo;

    HttpClient m_httpClient;
    std::unique_ptr<JPLHorizonsProvider> m_horizonsProvider;
    std::unique_ptr<JPLSBDBProvider> m_sbdbProvider;
    std::unique_ptr<NASAExoplanetProvider> m_exoplanetProvider;
    std::unique_ptr<StellarCatalogProvider> m_stellarProvider;

    // Threading & Async State
    std::mutex m_searchMutex;
    std::vector<SearchResult> m_cachedSearchResults;
    bool m_isSearching = false;

    std::mutex m_importMutex;
    AsyncJobState m_importJobState;

    bool m_offlineMode = false;
};

} // namespace AstroGenesis
