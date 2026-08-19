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
#include "net/HttpClient.hpp"

namespace AstroGenesis {

enum class ProviderType {
    JPL_Horizons,
    JPL_SBDB,
    NASA_Exoplanet
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

    // Threading & Async State
    std::mutex m_searchMutex;
    std::vector<SearchResult> m_cachedSearchResults;
    bool m_isSearching = false;

    std::mutex m_importMutex;
    AsyncJobState m_importJobState;

    bool m_offlineMode = false;
};

} // namespace AstroGenesis
