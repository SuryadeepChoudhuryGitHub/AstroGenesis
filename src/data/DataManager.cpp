#include "data/DataManager.hpp"
#include "data/UnitConverter.hpp"
#include <iostream>
#include <thread>
#include <algorithm>

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
    m_stellarProvider = std::make_unique<StellarCatalogProvider>(m_httpClient);

    std::cout << "[DataManager] Initialized Astronomical Data Providers (Stellar Catalogue, NASA Exoplanet, JPL Horizons, JPL SBDB)." << std::endl;
}

IAstronomicalDataProvider* DataManager::getProvider(ProviderType type) {
    switch (type) {
        case ProviderType::Stellar_Catalog: return m_stellarProvider.get();
        case ProviderType::NASA_Exoplanet:  return m_exoplanetProvider.get();
        case ProviderType::JPL_Horizons:    return m_horizonsProvider.get();
        case ProviderType::JPL_SBDB:        return m_sbdbProvider.get();
        case ProviderType::Auto_Resolve:
        default:                            return m_stellarProvider.get();
    }
}

std::vector<std::string> DataManager::getProviderNames() const {
    return {
        "Auto-Resolve (All Astronomical Catalogues)",
        "Stellar Catalogue (CDS/SIMBAD & Bright Stars)",
        "NASA Exoplanet Archive (Confirmed Exoplanets & Host Stars)",
        "NASA JPL Horizons (Planets, Moons, Major Bodies)",
        "NASA JPL Small-Body Database (Asteroids & Comets)"
    };
}

std::vector<SearchResult> DataManager::resolveQuery(const std::string& query, std::string& outError) {
    std::vector<SearchResult> results;
    if (query.empty()) return results;

    std::string err;

    // 1. If query is likely stellar or a star/system name, query Stellar Catalogue first
    if (StellarCatalogProvider::isLikelyStellarQuery(query)) {
        if (m_stellarProvider && m_stellarProvider->searchObjects(query, results, err) && !results.empty()) {
            return results;
        }
    }

    // 2. Check Exoplanet Archive if query matches exoplanet patterns (Kepler, TRAPPIST, TOI, HD)
    std::string lowQ = query;
    std::transform(lowQ.begin(), lowQ.end(), lowQ.begin(), ::tolower);
    if (lowQ.find("kepler") != std::string::npos || lowQ.find("trappist") != std::string::npos ||
        lowQ.find("toi") != std::string::npos || lowQ.find("wasp") != std::string::npos ||
        lowQ.find("hat-p") != std::string::npos || lowQ.find("exoplanet") != std::string::npos) {
        if (m_exoplanetProvider && m_exoplanetProvider->searchObjects(query, results, err) && !results.empty()) {
            return results;
        }
    }

    // 3. Check Solar System bodies & minor bodies (Horizons / SBDB)
    if (m_horizonsProvider) {
        std::vector<SearchResult> horizRes;
        if (m_horizonsProvider->searchObjects(query, horizRes, err) && !horizRes.empty()) {
            results.insert(results.end(), horizRes.begin(), horizRes.end());
        }
    }

    // 4. Also check Stellar Catalogue if not already matched
    if (m_stellarProvider) {
        std::vector<SearchResult> starRes;
        if (m_stellarProvider->searchObjects(query, starRes, err) && !starRes.empty()) {
            results.insert(results.end(), starRes.begin(), starRes.end());
        }
    }

    // 5. Check NASA Exoplanet archive as well
    if (m_exoplanetProvider && results.empty()) {
        std::vector<SearchResult> exoRes;
        if (m_exoplanetProvider->searchObjects(query, exoRes, err) && !exoRes.empty()) {
            results.insert(results.end(), exoRes.begin(), exoRes.end());
        }
    }

    // 6. Check JPL Small-Body Database for asteroids/comets
    if (m_sbdbProvider && results.empty()) {
        std::vector<SearchResult> sbdbRes;
        if (m_sbdbProvider->searchObjects(query, sbdbRes, err) && !sbdbRes.empty()) {
            results.insert(results.end(), sbdbRes.begin(), sbdbRes.end());
        }
    }

    return results;
}

void DataManager::searchAsync(ProviderType providerType, const std::string& query) {
    {
        std::lock_guard<std::mutex> lock(m_searchMutex);
        m_isSearching = true;
        m_cachedSearchResults.clear();
    }

    std::thread([this, providerType, query]() {
        std::vector<SearchResult> results;
        std::string err;

        if (m_offlineMode) {
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
        } else if (providerType == ProviderType::Auto_Resolve) {
            results = resolveQuery(query, err);
        } else {
            IAstronomicalDataProvider* prov = getProvider(providerType);
            if (prov) {
                prov->searchObjects(query, results, err);
            }
        }

        // Check if results are already in DB
        for (auto& r : results) {
            if (m_objRepo.getObjectBySlug(r.sourceId).has_value() || 
                m_objRepo.getObjectBySlug("sbdb_" + r.sourceId).has_value() ||
                (r.sourceId.rfind("star_", 0) == 0 && m_objRepo.getObjectBySlug(r.sourceId.substr(5)).has_value())) {
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

    if (sourceIdOrName.rfind("system_", 0) == 0) {
        return importStellarSystem(sourceIdOrName, outError);
    }

    IAstronomicalDataProvider* prov = nullptr;
    if (providerType == ProviderType::Auto_Resolve) {
        if (sourceIdOrName.rfind("star_", 0) == 0 || StellarCatalogProvider::isLikelyStellarQuery(sourceIdOrName)) {
            prov = m_stellarProvider.get();
        } else if (sourceIdOrName.rfind("planet_", 0) == 0) {
            prov = m_exoplanetProvider.get();
        } else if (sourceIdOrName.rfind("200000", 0) == 0 || sourceIdOrName.rfind("sbdb_", 0) == 0) {
            prov = m_sbdbProvider.get();
        } else if (JPLHorizonsProvider::getBodyMetadata(sourceIdOrName).has_value()) {
            prov = m_horizonsProvider.get();
        } else {
            prov = m_stellarProvider.get();
        }
    } else {
        prov = getProvider(providerType);
    }

    if (!prov) {
        outError = "Invalid provider selected.";
        return false;
    }

    CelestialBodyRecord rec;
    if (!prov->fetchObjectData(sourceIdOrName, rec, outError)) {
        // Fallback for auto-resolve: try other providers if first failed
        bool fetched = false;
        if (providerType == ProviderType::Auto_Resolve) {
            if (prov != m_stellarProvider.get() && m_stellarProvider && m_stellarProvider->fetchObjectData(sourceIdOrName, rec, outError)) {
                prov = m_stellarProvider.get();
                fetched = true;
            } else if (prov != m_exoplanetProvider.get() && m_exoplanetProvider && m_exoplanetProvider->fetchObjectData(sourceIdOrName, rec, outError)) {
                prov = m_exoplanetProvider.get();
                fetched = true;
            }
        }
        if (!fetched) {
            logImport(prov->getProviderName(), sourceIdOrName, false, 0, outError);
            return false;
        }
    }

    if (!categoryOverride.empty()) {
        rec.object.category = categoryOverride;
    }

    // Input Validation & Normalization
    if (rec.object.slug.empty()) rec.object.slug = sourceIdOrName;
    if (rec.object.name.empty()) rec.object.name = sourceIdOrName;

    // Check if body is a moon and link to parent body
    auto metaOpt = JPLHorizonsProvider::getBodyMetadata(sourceIdOrName);
    if (!metaOpt.has_value()) {
        metaOpt = JPLHorizonsProvider::getBodyMetadata(rec.object.slug);
    }
    if (!metaOpt.has_value()) {
        metaOpt = JPLHorizonsProvider::getBodyMetadata(rec.object.name);
    }
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

    // Validate Physical Properties according to object classification
    bool isStar = (rec.object.type.find("Star") != std::string::npos || 
                   rec.object.category == "Host Star" || 
                   rec.object.classification.find("Star") != std::string::npos ||
                   rec.object.classification.find("Black Hole") != std::string::npos);
    bool isPlanet = (rec.object.type.find("Planet") != std::string::npos || 
                     rec.object.classification.find("Planet") != std::string::npos || 
                     rec.object.category == "Exoplanet System");

    if (!rec.physical.massKg.has_value() || rec.physical.massKg.value() <= 0.0) {
        if (isStar) {
            rec.physical.massKg = UnitConverter::SOLAR_MASS_KG;
            rec.physical.isEstimated = true;
        } else if (isPlanet) {
            rec.physical.massKg = UnitConverter::EARTH_MASS_KG;
            rec.physical.isEstimated = true;
        } else {
            rec.physical.massKg = 1.0e15; // default for minor asteroids
        }
    }
    if (!rec.physical.radiusM.has_value() || rec.physical.radiusM.value() <= 0.0) {
        if (isStar) {
            rec.physical.radiusM = UnitConverter::SOLAR_RADIUS_M;
            rec.physical.isEstimated = true;
        } else if (isPlanet) {
            rec.physical.radiusM = UnitConverter::EARTH_RADIUS_M;
            rec.physical.isEstimated = true;
        } else {
            rec.physical.radiusM = 500.0;
        }
    }

    // For Exoplanets: Link host star if available
    if (providerType == ProviderType::NASA_Exoplanet && !rec.hostStarName.empty()) {
        std::string hostSlug = rec.hostStarName;
        std::transform(hostSlug.begin(), hostSlug.end(), hostSlug.begin(), [](char c){
            return (isalnum((unsigned char)c)) ? (char)tolower(c) : '_';
        });

        int64_t hostId = 0;
        auto existingHost = m_objRepo.getObjectBySlug(hostSlug);
        if (existingHost.has_value()) {
            hostId = existingHost.value().id;
        } else {
            CelestialBodyRecord hostRec;
            std::string hostErr;
            if (m_exoplanetProvider && m_exoplanetProvider->fetchObjectData("star_" + rec.hostStarName, hostRec, hostErr)) {
                m_objRepo.saveCelestialBodyRecord(hostRec, &hostId);
            }
        }

        if (hostId > 0) {
            rec.object.parentObjectId = hostId;
            rec.object.category = rec.hostStarName;
        }
    }

    // Ensure state vector is populated for orbiting bodies, but NEVER overwrite stationary or barycentric state for stars!
    if (!isStar && !rec.object.parentObjectId.has_value() && glm::length(rec.stateVector.positionM) < 1.0) {
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

    // If imported exoplanet has a host system, register link in systems & system_objects
    if (providerType == ProviderType::NASA_Exoplanet && !rec.object.category.empty() && rec.object.category != "Exoplanet System") {
        auto sysOpt = m_objRepo.getSystemByName(rec.object.category);
        int64_t sysId = 0;
        if (!sysOpt.has_value()) {
            SystemRecord newSys;
            newSys.name = rec.object.category;
            newSys.type = "Exoplanetary System";
            newSys.source = "NASA Exoplanet Archive";
            newSys.description = "Exoplanetary system " + rec.object.category;
            m_objRepo.createSystem(newSys, &sysId);
        } else {
            sysId = sysOpt.value().id;
        }

        if (sysId > 0) {
            if (rec.object.parentObjectId.has_value()) {
                m_objRepo.addSystemObject(sysId, rec.object.parentObjectId.value(), std::nullopt, 0);
            }
            m_objRepo.addSystemObject(sysId, newObjId, rec.object.parentObjectId, 1);
        }
    }

    // If imported star has a system category, register in systems & system_objects
    if (isStar && !rec.object.category.empty() && rec.object.category != "Solar System" && rec.object.category != "Host Star") {
        auto sysOpt = m_objRepo.getSystemByName(rec.object.category);
        int64_t sysId = 0;
        if (!sysOpt.has_value()) {
            SystemRecord newSys;
            newSys.name = rec.object.category;
            newSys.type = (rec.stateVector.positionM != glm::dvec3(0.0)) ? "Binary Star System" : "Single Star System";
            newSys.source = prov->getProviderName();
            newSys.description = "Astronomical stellar system " + rec.object.category;
            m_objRepo.createSystem(newSys, &sysId);
        } else {
            sysId = sysOpt.value().id;
        }

        if (sysId > 0) {
            m_objRepo.addSystemObject(sysId, newObjId, rec.object.parentObjectId, 0);
        }
    }

    // Log success
    logImport(prov->getProviderName(), rec.object.name + " (" + sourceIdOrName + ")", true, 1, "Imported into " + rec.object.category);
    std::cout << "[DataManager] Successfully imported " << rec.object.name << " (ID: " << newObjId << ")" << std::endl;
    return true;
}

void DataManager::importExoplanetSystemAsync(const std::string& hostname) {
    {
        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = true;
        m_importJobState.currentTask = "Importing planetary system " + hostname + "...";
        m_importJobState.progress = 0.1f;
        m_importJobState.lastSuccess = true;
        m_importJobState.errorMessage.clear();
    }

    std::thread([this, hostname]() {
        std::string err;
        bool ok = importExoplanetSystem(hostname, err);

        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = false;
        m_importJobState.lastSuccess = ok;
        m_importJobState.progress = 1.0f;
        if (ok) {
            m_importJobState.lastResult = "Successfully imported system: " + hostname;
        } else {
            m_importJobState.errorMessage = err;
            m_importJobState.lastResult = "System import failed: " + err;
        }
    }).detach();
}

bool DataManager::importExoplanetSystem(const std::string& hostname, std::string& outError) {
    if (m_offlineMode) {
        outError = "Application is in Offline Mode. Enable online mode to import from NASA TAP.";
        return false;
    }
    if (!m_exoplanetProvider) {
        outError = "NASA Exoplanet provider is not initialized.";
        return false;
    }

    std::vector<CelestialBodyRecord> planets;
    CelestialBodyRecord hostStar;
    if (!m_exoplanetProvider->fetchSystemPlanets(hostname, planets, hostStar, outError)) {
        logImport("NASA Exoplanet Archive", hostname + " (System)", false, 0, outError);
        return false;
    }

    std::string sysName = hostStar.object.name;

    // 1. Save or update Host Star
    int64_t hostStarId = 0;
    hostStar.object.category = sysName;
    if (!m_objRepo.saveCelestialBodyRecord(hostStar, &hostStarId)) {
        outError = "Failed to save host star: " + m_db.getLastError();
        logImport("NASA Exoplanet Archive", sysName + " (Host Star)", false, 0, outError);
        return false;
    }

    // 2. Create or find System record
    int64_t sysId = 0;
    auto sysOpt = m_objRepo.getSystemByName(sysName);
    if (sysOpt.has_value()) {
        sysId = sysOpt.value().id;
    } else {
        SystemRecord sysRec;
        sysRec.name = sysName;
        sysRec.type = "Exoplanetary System";
        sysRec.source = "NASA Exoplanet Archive";
        sysRec.description = "Planetary system orbiting " + sysName + " (" + hostStar.object.type + ") with " +
                             std::to_string(planets.size()) + " confirmed exoplanets.";
        if (!m_objRepo.createSystem(sysRec, &sysId)) {
            outError = "Failed to create system entry: " + m_db.getLastError();
            return false;
        }
    }

    // Link host star in system_objects at order 0
    m_objRepo.addSystemObject(sysId, hostStarId, std::nullopt, 0);

    // 3. Save each planet and link in system_objects
    int order = 1;
    int importedPlanets = 0;
    for (auto& pl : planets) {
        pl.object.parentObjectId = hostStarId;
        pl.object.category = sysName;

        int64_t plId = 0;
        if (m_objRepo.saveCelestialBodyRecord(pl, &plId)) {
            m_objRepo.addSystemObject(sysId, plId, hostStarId, order++);
            importedPlanets++;
        }
    }

    std::string details = "Imported host star " + sysName + " and " + std::to_string(importedPlanets) + " confirmed planets.";
    logImport("NASA Exoplanet Archive", sysName + " (System)", true, 1 + importedPlanets, details);
    std::cout << "[DataManager] Successfully imported system " << sysName << " (" << details << ")" << std::endl;
    return true;
}

void DataManager::importStellarSystemAsync(const std::string& systemName) {
    {
        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = true;
        m_importJobState.currentTask = "Importing stellar system " + systemName + "...";
        m_importJobState.progress = 0.1f;
        m_importJobState.lastSuccess = true;
        m_importJobState.errorMessage.clear();
    }

    std::thread([this, systemName]() {
        std::string err;
        bool ok = importStellarSystem(systemName, err);

        std::lock_guard<std::mutex> lock(m_importMutex);
        m_importJobState.isRunning = false;
        m_importJobState.lastSuccess = ok;
        m_importJobState.progress = 1.0f;
        if (ok) {
            m_importJobState.lastResult = "Successfully imported stellar system: " + systemName;
        } else {
            m_importJobState.errorMessage = err;
            m_importJobState.lastResult = "Stellar system import failed: " + err;
        }
    }).detach();
}

bool DataManager::importStellarSystem(const std::string& systemName, std::string& outError) {
    if (!m_stellarProvider) {
        outError = "Stellar Catalog provider is not initialized.";
        return false;
    }

    std::vector<CelestialBodyRecord> components;
    if (!m_stellarProvider->fetchSystemComponents(systemName, components, outError)) {
        logImport("Stellar Catalogue", systemName + " (Stellar System)", false, 0, outError);
        return false;
    }

    if (components.empty()) {
        outError = "No stellar components resolved for " + systemName;
        logImport("Stellar Catalogue", systemName + " (Stellar System)", false, 0, outError);
        return false;
    }

    // Determine system name and system type
    std::string sysName = components[0].object.category;
    if (sysName.empty() || sysName == "Single Star System") {
        sysName = components[0].object.name;
        if (components.size() > 1 && sysName.find("System") == std::string::npos) {
            sysName += " System";
        }
    }

    std::string sysType = (components.size() == 1) ? "Single Star System" : 
                          (components.size() == 2) ? "Binary Star System" : "Multiple Star System";

    // 1. Create or find System record
    int64_t sysId = 0;
    auto sysOpt = m_objRepo.getSystemByName(sysName);
    if (sysOpt.has_value()) {
        sysId = sysOpt.value().id;
    } else {
        SystemRecord sysRec;
        sysRec.name = sysName;
        sysRec.type = sysType;
        sysRec.source = "Stellar Catalogue (CDS/SIMBAD)";
        sysRec.description = "Astronomical stellar system containing " + std::to_string(components.size()) + 
                             " stellar component(s): ";
        for (size_t i = 0; i < components.size(); ++i) {
            if (i > 0) sysRec.description += ", ";
            sysRec.description += components[i].object.name;
            if (components[i].physical.spectralType.has_value()) {
                sysRec.description += " (" + components[i].physical.spectralType.value() + ")";
            }
        }
        if (!m_objRepo.createSystem(sysRec, &sysId)) {
            outError = "Failed to create system entry: " + m_db.getLastError();
            return false;
        }
    }

    // 2. Save each star component and link into system_objects
    int order = 0;
    int importedStars = 0;
    for (auto& comp : components) {
        comp.object.category = sysName;
        int64_t starId = 0;
        if (m_objRepo.saveCelestialBodyRecord(comp, &starId)) {
            m_objRepo.addSystemObject(sysId, starId, std::nullopt, order++);
            importedStars++;
        }
    }

    std::string details = "Imported " + std::to_string(importedStars) + " stellar component(s) into " + sysName;
    logImport("Stellar Catalogue", sysName + " (Stellar System)", true, importedStars, details);
    std::cout << "[DataManager] Successfully imported stellar system " << sysName << " (" << details << ")" << std::endl;
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
        r.targetObject = columnTextSafe(stmt, 2);
        r.status = columnTextSafe(stmt, 3);
        r.recordsCount = sqlite3_column_int(stmt, 4);
        r.details = columnTextSafe(stmt, 5);
        r.timestamp = columnTextSafe(stmt, 6);
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

} // namespace AstroGenesis
