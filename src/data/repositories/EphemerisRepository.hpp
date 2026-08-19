#pragma once

#include <vector>
#include <string>
#include <optional>
#include "data/AstronomicalModels.hpp"
#include "data/DatabaseManager.hpp"

namespace AstroGenesis {

class EphemerisRepository {
public:
    explicit EphemerisRepository(DatabaseManager& db);

    bool saveEphemerisRecords(int64_t objectId, const std::vector<EphemerisRecord>& records);
    bool saveEphemerisRecord(const EphemerisRecord& record);

    std::vector<EphemerisRecord> getEphemerisSeries(int64_t objectId, double startJd = 0.0, double endJd = 1e9);
    std::optional<EphemerisRecord> getClosestEphemeris(int64_t objectId, double targetJd);
    
    bool deleteEphemerisForObject(int64_t objectId);
    int getEphemerisCount(int64_t objectId);

private:
    DatabaseManager& m_db;
};

} // namespace AstroGenesis
