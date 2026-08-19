#include "data/repositories/ValidationRepository.hpp"
#include <iostream>

namespace AstroGenesis {

ValidationRepository::ValidationRepository(DatabaseManager& db) : m_db(db) {}

int64_t ValidationRepository::createSimulationRun(const SimulationRunRecord& run) {
    std::string sql = "INSERT INTO simulation_runs (name, integrator_type, start_epoch_jd, time_scale, gr_enabled, start_timestamp, total_sim_seconds) "
                      "VALUES (?, ?, ?, ?, ?, datetime('now'), ?);";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return 0;

    sqlite3_bind_text(stmt, 1, run.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, run.integratorType.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_double(stmt, 3, run.startEpochJd);
    sqlite3_bind_double(stmt, 4, run.timeScale);
    sqlite3_bind_int(stmt, 5, run.grEnabled ? 1 : 0);
    sqlite3_bind_double(stmt, 6, run.totalSimSeconds);

    int64_t runId = 0;
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        runId = m_db.getLastInsertId();
    }
    m_db.finalize(stmt);
    return runId;
}

bool ValidationRepository::updateSimulationRun(const SimulationRunRecord& run) {
    std::string sql = "UPDATE simulation_runs SET end_timestamp = datetime('now'), total_sim_seconds = ? WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_double(stmt, 1, run.totalSimSeconds);
    sqlite3_bind_int64(stmt, 2, run.id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

bool ValidationRepository::saveValidationResultsBatch(const std::vector<ValidationResultRecord>& results) {
    if (results.empty()) return true;

    DatabaseManager::ScopedTransaction tx(m_db);
    std::string sql = "INSERT INTO validation_results ("
                      "run_id, object_id, object_name, epoch_jd, sim_pos_x, sim_pos_y, sim_pos_z, "
                      "real_pos_x, real_pos_y, real_pos_z, sim_vel_x, sim_vel_y, sim_vel_z, "
                      "real_vel_x, real_vel_y, real_vel_z, pos_error_m, pos_relative_error, "
                      "vel_error_mps, energy_drift_pct, angular_momentum_drift_pct, evaluated_at, gr_mode) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'), ?);";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    for (const auto& r : results) {
        if (r.runId > 0) sqlite3_bind_int64(stmt, 1, r.runId); else sqlite3_bind_null(stmt, 1);
        sqlite3_bind_int64(stmt, 2, r.objectId);
        sqlite3_bind_text(stmt, 3, r.objectName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, r.epochJd);
        sqlite3_bind_double(stmt, 5, r.simPosM.x);
        sqlite3_bind_double(stmt, 6, r.simPosM.y);
        sqlite3_bind_double(stmt, 7, r.simPosM.z);
        sqlite3_bind_double(stmt, 8, r.realPosM.x);
        sqlite3_bind_double(stmt, 9, r.realPosM.y);
        sqlite3_bind_double(stmt, 10, r.realPosM.z);
        sqlite3_bind_double(stmt, 11, r.simVelMps.x);
        sqlite3_bind_double(stmt, 12, r.simVelMps.y);
        sqlite3_bind_double(stmt, 13, r.simVelMps.z);
        sqlite3_bind_double(stmt, 14, r.realVelMps.x);
        sqlite3_bind_double(stmt, 15, r.realVelMps.y);
        sqlite3_bind_double(stmt, 16, r.realVelMps.z);
        sqlite3_bind_double(stmt, 17, r.posErrorM);
        sqlite3_bind_double(stmt, 18, r.posRelativeError);
        sqlite3_bind_double(stmt, 19, r.velErrorMps);
        sqlite3_bind_double(stmt, 20, r.energyDriftPct);
        sqlite3_bind_double(stmt, 21, r.angularMomentumDriftPct);
        sqlite3_bind_int(stmt, 22, r.grMode ? 1 : 0);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            m_db.finalize(stmt);
            return false;
        }
        sqlite3_reset(stmt);
    }

    m_db.finalize(stmt);
    tx.commit();
    return true;
}

bool ValidationRepository::saveValidationResult(const ValidationResultRecord& result) {
    return saveValidationResultsBatch({ result });
}

std::vector<ValidationResultRecord> ValidationRepository::getValidationResults(int64_t objectId, int64_t runId, int limit) {
    std::vector<ValidationResultRecord> list;
    std::string sql = "SELECT id, run_id, object_id, object_name, epoch_jd, sim_pos_x, sim_pos_y, sim_pos_z, "
                      "real_pos_x, real_pos_y, real_pos_z, sim_vel_x, sim_vel_y, sim_vel_z, "
                      "real_vel_x, real_vel_y, real_vel_z, pos_error_m, pos_relative_error, "
                      "vel_error_mps, energy_drift_pct, angular_momentum_drift_pct, evaluated_at, gr_mode "
                      "FROM validation_results WHERE 1=1 ";
    if (objectId > 0) sql += " AND object_id = ? ";
    if (runId > 0) sql += " AND run_id = ? ";
    sql += " ORDER BY epoch_jd ASC LIMIT ?;";

    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    int bindIdx = 1;
    if (objectId > 0) sqlite3_bind_int64(stmt, bindIdx++, objectId);
    if (runId > 0) sqlite3_bind_int64(stmt, bindIdx++, runId);
    sqlite3_bind_int(stmt, bindIdx++, limit);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ValidationResultRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        if (sqlite3_column_type(stmt, 1) != SQLITE_NULL) r.runId = sqlite3_column_int64(stmt, 1);
        r.objectId = sqlite3_column_int64(stmt, 2);
        r.objectName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.epochJd = sqlite3_column_double(stmt, 4);
        r.simPosM.x = sqlite3_column_double(stmt, 5);
        r.simPosM.y = sqlite3_column_double(stmt, 6);
        r.simPosM.z = sqlite3_column_double(stmt, 7);
        r.realPosM.x = sqlite3_column_double(stmt, 8);
        r.realPosM.y = sqlite3_column_double(stmt, 9);
        r.realPosM.z = sqlite3_column_double(stmt, 10);
        r.simVelMps.x = sqlite3_column_double(stmt, 11);
        r.simVelMps.y = sqlite3_column_double(stmt, 12);
        r.simVelMps.z = sqlite3_column_double(stmt, 13);
        r.realVelMps.x = sqlite3_column_double(stmt, 14);
        r.realVelMps.y = sqlite3_column_double(stmt, 15);
        r.realVelMps.z = sqlite3_column_double(stmt, 16);
        r.posErrorM = sqlite3_column_double(stmt, 17);
        r.posRelativeError = sqlite3_column_double(stmt, 18);
        r.velErrorMps = sqlite3_column_double(stmt, 19);
        r.energyDriftPct = sqlite3_column_double(stmt, 20);
        r.angularMomentumDriftPct = sqlite3_column_double(stmt, 21);
        r.evaluatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 22));
        r.grMode = (sqlite3_column_int(stmt, 23) != 0);
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

std::vector<SimulationRunRecord> ValidationRepository::getRecentSimulationRuns(int limit) {
    std::vector<SimulationRunRecord> list;
    std::string sql = "SELECT id, name, integrator_type, start_epoch_jd, time_scale, gr_enabled, "
                      "start_timestamp, end_timestamp, total_sim_seconds FROM simulation_runs ORDER BY id DESC LIMIT ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    sqlite3_bind_int(stmt, 1, limit);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SimulationRunRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        r.integratorType = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.startEpochJd = sqlite3_column_double(stmt, 3);
        r.timeScale = sqlite3_column_double(stmt, 4);
        r.grEnabled = (sqlite3_column_int(stmt, 5) != 0);
        r.startTimestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        const unsigned char* endTs = sqlite3_column_text(stmt, 7);
        if (endTs) r.endTimestamp = reinterpret_cast<const char*>(endTs);
        r.totalSimSeconds = sqlite3_column_double(stmt, 8);
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

bool ValidationRepository::clearValidationHistory(int64_t objectId) {
    std::string sql = "DELETE FROM validation_results";
    if (objectId > 0) sql += " WHERE object_id = ?";
    sql += ";";

    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    if (objectId > 0) sqlite3_bind_int64(stmt, 1, objectId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

} // namespace AstroGenesis
