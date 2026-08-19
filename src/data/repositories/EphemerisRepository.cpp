#include "data/repositories/EphemerisRepository.hpp"
#include <iostream>

namespace AstroGenesis {

EphemerisRepository::EphemerisRepository(DatabaseManager& db) : m_db(db) {}

bool EphemerisRepository::saveEphemerisRecords(int64_t objectId, const std::vector<EphemerisRecord>& records) {
    if (records.empty()) return true;

    DatabaseManager::ScopedTransaction tx(m_db);
    std::string sql = "INSERT OR REPLACE INTO ephemeris_records ("
                      "object_id, target_name, epoch_utc, epoch_jd, pos_x_m, pos_y_m, pos_z_m, "
                      "vel_x_mps, vel_y_mps, vel_z_mps, reference_frame, source_id) "
                      "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    for (const auto& r : records) {
        sqlite3_bind_int64(stmt, 1, (r.objectId > 0) ? r.objectId : objectId);
        sqlite3_bind_text(stmt, 2, r.targetName.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, r.epochUtc.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_double(stmt, 4, r.epochJd);
        sqlite3_bind_double(stmt, 5, r.positionM.x);
        sqlite3_bind_double(stmt, 6, r.positionM.y);
        sqlite3_bind_double(stmt, 7, r.positionM.z);
        sqlite3_bind_double(stmt, 8, r.velocityMps.x);
        sqlite3_bind_double(stmt, 9, r.velocityMps.y);
        sqlite3_bind_double(stmt, 10, r.velocityMps.z);
        sqlite3_bind_text(stmt, 11, r.referenceFrame.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 12, r.sourceId);

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

bool EphemerisRepository::saveEphemerisRecord(const EphemerisRecord& record) {
    return saveEphemerisRecords(record.objectId, { record });
}

std::vector<EphemerisRecord> EphemerisRepository::getEphemerisSeries(int64_t objectId, double startJd, double endJd) {
    std::vector<EphemerisRecord> list;
    std::string sql = "SELECT id, object_id, target_name, epoch_utc, epoch_jd, pos_x_m, pos_y_m, pos_z_m, "
                      "vel_x_mps, vel_y_mps, vel_z_mps, reference_frame, source_id "
                      "FROM ephemeris_records WHERE object_id = ? AND epoch_jd >= ? AND epoch_jd <= ? ORDER BY epoch_jd ASC;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    sqlite3_bind_int64(stmt, 1, objectId);
    sqlite3_bind_double(stmt, 2, startJd);
    sqlite3_bind_double(stmt, 3, endJd);

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        EphemerisRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.objectId = sqlite3_column_int64(stmt, 1);
        r.targetName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.epochUtc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.epochJd = sqlite3_column_double(stmt, 4);
        r.positionM.x = sqlite3_column_double(stmt, 5);
        r.positionM.y = sqlite3_column_double(stmt, 6);
        r.positionM.z = sqlite3_column_double(stmt, 7);
        r.velocityMps.x = sqlite3_column_double(stmt, 8);
        r.velocityMps.y = sqlite3_column_double(stmt, 9);
        r.velocityMps.z = sqlite3_column_double(stmt, 10);
        r.referenceFrame = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        r.sourceId = sqlite3_column_int64(stmt, 12);
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

std::optional<EphemerisRecord> EphemerisRepository::getClosestEphemeris(int64_t objectId, double targetJd) {
    std::string sql = "SELECT id, object_id, target_name, epoch_utc, epoch_jd, pos_x_m, pos_y_m, pos_z_m, "
                      "vel_x_mps, vel_y_mps, vel_z_mps, reference_frame, source_id, ABS(epoch_jd - ?) AS diff "
                      "FROM ephemeris_records WHERE object_id = ? ORDER BY diff ASC LIMIT 1;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_double(stmt, 1, targetJd);
    sqlite3_bind_int64(stmt, 2, objectId);

    std::optional<EphemerisRecord> res;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        EphemerisRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.objectId = sqlite3_column_int64(stmt, 1);
        r.targetName = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        r.epochUtc = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        r.epochJd = sqlite3_column_double(stmt, 4);
        r.positionM.x = sqlite3_column_double(stmt, 5);
        r.positionM.y = sqlite3_column_double(stmt, 6);
        r.positionM.z = sqlite3_column_double(stmt, 7);
        r.velocityMps.x = sqlite3_column_double(stmt, 8);
        r.velocityMps.y = sqlite3_column_double(stmt, 9);
        r.velocityMps.z = sqlite3_column_double(stmt, 10);
        r.referenceFrame = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        r.sourceId = sqlite3_column_int64(stmt, 12);
        res = r;
    }
    m_db.finalize(stmt);
    return res;
}

bool EphemerisRepository::deleteEphemerisForObject(int64_t objectId) {
    std::string sql = "DELETE FROM ephemeris_records WHERE object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, objectId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

int EphemerisRepository::getEphemerisCount(int64_t objectId) {
    std::string sql = "SELECT COUNT(*) FROM ephemeris_records WHERE object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return 0;

    sqlite3_bind_int64(stmt, 1, objectId);
    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    m_db.finalize(stmt);
    return count;
}

} // namespace AstroGenesis
