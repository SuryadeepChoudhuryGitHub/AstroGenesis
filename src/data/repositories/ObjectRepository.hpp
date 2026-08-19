#pragma once

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include "data/AstronomicalModels.hpp"
#include "data/DatabaseManager.hpp"
#include "simulation/CelestialBody.hpp"

namespace AstroGenesis {

class ObjectRepository {
public:
    explicit ObjectRepository(DatabaseManager& db);

    // Save full celestial body record (Object + Physical + Orbital + State Vector + Composition)
    bool saveCelestialBodyRecord(const CelestialBodyRecord& record, int64_t* outId = nullptr);
    bool saveCelestialBody(const CelestialBody& body, int64_t* outId = nullptr);

    // Retrieve object list
    std::vector<ObjectRecord> getAllObjects(const std::string& category = "", 
                                           bool includeSynthetic = true, 
                                           const std::string& searchQuery = "");

    std::optional<ObjectRecord> getObjectById(int64_t id);
    std::optional<ObjectRecord> getObjectBySlug(const std::string& slug);

    // Hydrate complete CelestialBody for simulation/rendering from DB
    std::vector<CelestialBody> getSystemBodies(const std::string& systemCategory = "Solar System");
    std::optional<CelestialBody> getHydratedBody(int64_t id);
    std::optional<CelestialBody> getHydratedBodyBySlug(const std::string& slug);

    // Component retrieval
    std::optional<PhysicalPropertiesRecord> getPhysicalProperties(int64_t objectId);
    std::optional<OrbitalElementsRecord> getOrbitalElements(int64_t objectId);
    std::optional<StateVectorRecord> getStateVector(int64_t objectId);
    std::vector<CompositionRecord> getComposition(int64_t objectId);

    // Delete
    bool deleteObject(int64_t id);
    bool deleteObjectBySlug(const std::string& slug);

    // Categories & counts
    std::vector<std::string> getAvailableCategories();
    int getObjectCount(const std::string& category = "");

    // Quick Data Source lookup
    std::string getSourceName(int64_t sourceId);
    int64_t getOrCreateSourceId(const std::string& sourceName, const std::string& baseUrl = "", const std::string& description = "");

private:
    DatabaseManager& m_db;
    void hydrateCelestialBodyFields(CelestialBody& body, 
                                   const ObjectRecord& obj, 
                                   const std::optional<PhysicalPropertiesRecord>& phys,
                                   const std::optional<OrbitalElementsRecord>& orb,
                                   const std::optional<StateVectorRecord>& state,
                                   const std::vector<CompositionRecord>& comp);
};

} // namespace AstroGenesis
