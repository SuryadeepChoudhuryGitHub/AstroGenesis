#include "data/DataManager.hpp"
#include "data/UnitConverter.hpp"
#include <iostream>
#include <thread>

namespace AstroGenesis {

DataManager::DataManager(DatabaseManager& db, 
                         ObjectRepository& objRepo, 
                         EphemerisRepository& ephemRepo,
                         ValidationRepository& valRepo)
    : m_db(db), m_objRepo(objRepo), m_ephemRepo(ephemRepo), m_valRepo(valRepo) {}

DataManager::~DataManager() {}

void DataManager::initialize() {
    m_horizonsProvider = std::make_unique<JPLHorizonsProvider>(m_httpClient);
    m_sbdbProvider = std::make_unique<JPLSBDBProvider>(m_httpClient);
    m_exoplanetProvider = std::make_unique<NASAExoplanetProvider>(m_httpClient);

    std::cout << "[DataManager] Initialized Astronomical Data Providers (JPL Horizons, JPL SBDB, NASA Exoplanet)." << std::endl;
}

IAstronomicalDataProvider* DataManager::getProvider(ProviderType type) {
    switch (type) {
        case ProviderType::JPL_Horizons: return m_horizonsProvider.get();
        case ProviderType::JPL_SBDB:     return m_sbdbProvider.get();
        case ProviderType::NASA_Exoplanet: return m_exoplanetProvider.get();
    }
    return m_horizonsProvider.get();
}

std::vector<std::string> DataManager::getProviderNames() const {
    return {
        "NASA JPL Horizons (Ephemerides & Major Bodies)",
        "NASA JPL Small-Body Database (Asteroids & Comets)",
        "NASA Exoplanet Archive (Exoplanetary Systems)"
    };
}

void DataManager::searchAsync(ProviderType providerType, const std::string& query) {
    {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        m_isSearching = true;
        m_cachedSearchResults.clear();
    }

    std::thread([this, providerType, query]() {
        IAstronomicalDataProvider* prov = getProvider(providerType);
        std::vector<SearchResult> results;
        std::string err;

        if (prov && !m_offlineMode) {
            prov->searchObjects(query, results, err);
        } else if (m_offlineMode) {
            // In offline mode, search local SQLite database
            auto localObjs = m_objRepo.getAllObjects("", true, query);
            for (const auto& obj : localObjs) {
                SearchResult res;
                res.sourceName = "Local Database (Offline)";
                res.sourceId = obj.slug;
                res.name = obj.name;
                res.type = obj.type;
                res.details = "Category: " + obj.category;
                res.alreadyInDatabase = true;
                results.push_back(res);
            }
        }

        // Check if results are already in DB
        for (auto& r : results) {
            if (m_objRepo.getObjectBySlug(r.sourceId).has_value() || 
                m_objRepo.getObjectBySlug("sbdb_" + r.sourceId).has_value()) {
                r.alreadyInDatabase = true;
            }
        }

        {
            std::lock_guard<std::mutex> lock(m_searchMutex);
            m_cachedSearchResults = results;
            m_isSearching = false;
        }
    }).detach();
}

bool DataManager::isSearching() const {
    return m_isSearching;
}

std::vector<SearchResult> DataManager::getSearchResults() {
    std::lock_guard<std::mutex> lock(m_searchMutex);
    return m_cachedSearchResults;
}

void DataManager::importObjectAsync(ProviderType providerType, const std::string& sourceIdOrName, const std::string& categoryOverride) {
    {
        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = true;
        m_importJobState.currentTask = "Importing " + sourceIdOrName + "...";
        m_importJobState.progress = 0.1f;
        m_importJobState.lastSuccess = true;
        m_importJobState.errorMessage.clear();
    }

    std::thread([this, providerType, sourceIdOrName, categoryOverride]() {
        std::string err;
        bool ok = importObject(providerType, sourceIdOrName, categoryOverride, err);

        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = false;
        m_importJobState.lastSuccess = ok;
        m_importJobState.progress = 1.0f;
        if (ok) {
            m_importJobState.lastResult = "Successfully imported: " + sourceIdOrName;
        } else {
            m_importJobState.errorMessage = err;
            m_importJobState.lastResult = "Import failed: " + err;
        }
    }).detach();
}

bool DataManager::isImporting() const {
    return m_importJobState.isRunning;
}

AsyncJobState DataManager::getImportJobState() {
    std::lock_guard<std::mutex> lock(m_importMutex);
    return m_importJobState;
}

bool DataManager::importObject(ProviderType providerType, const std::string& sourceIdOrName, const std::string& categoryOverride, std::string& outError) {
    if (m_offlineMode) {
        outError = "Application is in Offline Mode. Enable online mode to import from external APIs.";
        return false;
    }

    IAstronomicalDataProvider* prov = getProvider(providerType);
    if (!prov) {
        outError = "Invalid provider selected.";
        return false;
    }

    CelestialBodyRecord rec;
    if (!prov->fetchObjectData(sourceIdOrName, rec, outError)) {
        logImport(prov->getProviderName(), sourceIdOrName, false, 0, outError);
        return false;
    }

    if (!categoryOverride.empty()) {
        rec.object.category = categoryOverride;
    }

    // Input Validation & Normalization
    if (rec.object.slug.empty()) rec.object.slug = sourceIdOrName;
    if (rec.object.name.empty()) rec.object.name = sourceIdOrName;

    // Check if body is a moon and link to parent body
    auto metaOpt = JPLHorizonsProvider::getBodyMetadata(sourceIdOrName);
    if (metaOpt.has_value() && !metaOpt.value().parentSlug.empty()) {
        auto parentObj = m_objRepo.getObjectBySlug(metaOpt.value().parentSlug);
        if (parentObj.has_value()) {
            rec.object.parentObjectId = parentObj.value().id;
            rec.object.category = parentObj.value().category;
            
            auto parentState = m_objRepo.getStateVector(parentObj.value().id);
            if (parentState.has_value()) {
                rec.stateVector.positionM = parentState.value().positionM + rec.stateVector.positionM;
                rec.stateVector.velocityMps = parentState.value().velocityMps + rec.stateVector.velocityMps;
            }
        }
    }

    // Validate Physical Properties
    if (!rec.physical.massKg.has_value() || rec.physical.massKg.value() <= 0.0) {
        rec.physical.massKg = 1.0e15; // default for small bodies
    }
    if (!rec.physical.radiusM.has_value() || rec.physical.radiusM.value() <= 0.0) {
        rec.physical.radiusM = 500.0;
    }

    // Ensure state vector is populated
    if (glm::length(rec.stateVector.positionM) < 1.0) {
        double sma = rec.orbital.semiMajorAxisM.value_or(2.5 * UnitConverter::AU_TO_METERS);
        rec.stateVector.positionM = glm::dvec3(sma, 0.0, 0.0);
        double v = std::sqrt(UnitConverter::G_CONST * 1.9885e30 / sma);
        rec.stateVector.velocityMps = glm::dvec3(0.0, 0.0, v);
    }

    int64_t newObjId = 0;
    bool saved = m_objRepo.saveCelestialBodyRecord(rec, &newObjId);
    if (!saved) {
        outError = "Database error: " + m_db.getLastError();
        logImport(prov->getProviderName(), sourceIdOrName, false, 0, outError);
        return false;
    }

    // Log success
    logImport(prov->getProviderName(), rec.object.name + " (" + sourceIdOrName + ")", true, 1, "Imported into " + rec.object.category);
    std::cout << "[DataManager] Successfully imported " << rec.object.name << " (ID: " << newObjId << ")" << std::endl;
    return true;
}

bool DataManager::fetchAndStoreEphemerisSeries(const std::string& sourceIdOrName, int64_t objectId, double startJd, double endJd, double stepDays, std::string& outError) {
    if (!m_horizonsProvider) {
        outError = "JPL Horizons provider not available.";
        return false;
    }

    std::vector<EphemerisRecord> records;
    if (!m_horizonsProvider->fetchEphemerisSeries(sourceIdOrName, startJd, endJd, stepDays, records, outError)) {
        return false;
    }

    for (auto& r : records) {
        r.objectId = objectId;
    }

    bool ok = m_ephemRepo.saveEphemerisRecords(objectId, records);
    if (!ok) {
        outError = "Failed to save ephemeris records to database.";
        return false;
    }

    logImport("NASA JPL Horizons", sourceIdOrName + " Ephemeris Series", true, (int)records.size(), "Stored " + std::to_string(records.size()) + " ephemeris steps.");
    return true;
}

void DataManager::logImport(const std::string& providerName, const std::string& targetObject, bool success, int count, const std::string& details) {
    int64_t srcId = m_objRepo.getOrCreateSourceId(providerName);
    std::string sql = "INSERT INTO data_imports (source_id, target_object, status, records_count, details, timestamp) "
                      "VALUES (?, ?, ?, ?, ?, datetime('now'));";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return;

    sqlite3_bind_int64(stmt, 1, srcId);
    sqlite3_bind_text(stmt, 2, targetObject.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, success ? "SUCCESS" : "FAILED", -1, SQLITE_TRANSIENT);
    sqlite3_bind_int(stmt, 4, count);
    sqlite3_bind_text(stmt, 5, details.c_str(), -1, SQLITE_TRANSIENT);

    sqlite3_step(stmt);
    m_db.finalize(stmt);
}

std::vector<DataImportRecord> DataManager::getImportHistory(int limit) {
    std::vector<DataImportRecord> list;
    std::string sql = "SELECT id, source_id, target_object, status, records_count, details, timestamp "
                      "FROM data_imports ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        DataImportRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.sourceId = sqlite3_column_int64(stmt, 1);
        r.targetObject = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.status = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.recordsCount = sqlite3_column_int(stmt, 4);
        const unsigned char* d = sqlite3_column_text(stmt, 5);
        if (d) r.details = reinterpret_cast<const char*>(d);
        r.timestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

} // namespace AstroGenesis
