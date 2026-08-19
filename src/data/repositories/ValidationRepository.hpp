#pragma once

#include <vector>
#include <string>
#include <optional>
#include "data/AstronomicalModels.hpp"
#include "data/DatabaseManager.hpp"

namespace AstroGenesis {

class ValidationRepository {
public:
    explicit ValidationRepository(DatabaseManager& db);

    int64_t createSimulationRun(const SimulationRunRecord& run);
    bool updateSimulationRun(const SimulationRunRecord& run);
    
    bool saveValidationResult(const ValidationResultRecord& result);
    bool saveValidationResultsBatch(const std::vector<ValidationResultRecord>& results);

    std::vector<ValidationResultRecord> getValidationResults(int64_t objectId = 0, int64_t runId = 0, int limit = 500);
    std::vector<SimulationRunRecord> getRecentSimulationRuns(int limit = 20);
    
    bool clearValidationHistory(int64_t objectId = 0);

private:
    DatabaseManager& m_db;
};

} // namespace AstroGenesis
