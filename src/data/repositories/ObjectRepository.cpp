#include "data/repositories/ObjectRepository.hpp"
#include "data/UnitConverter.hpp"
#include <iostream>
#include <sstream>
#include <cmath>
#include <nlohmann/json.hpp>

namespace AstroGenesis {

ObjectRepository::ObjectRepository(DatabaseManager& db) : m_db(db) {}

int64_t ObjectRepository::getOrCreateSourceId(const std::string& sourceName, const std::string& baseUrl, const std::string& description) {
    std::string q = "SELECT id FROM data_sources WHERE name = ?;";
    sqlite3_stmt* stmt = m_db.prepare(q);
    if (!stmt) return 1;

    sqlite3_bind_text(stmt, 1, sourceName.c_str(), -1, SQLITE_TRANSIENT);
    int64_t sourceId = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        sourceId = sqlite3_column_int64(stmt, 0);
    }
    m_db.finalize(stmt);

    if (sourceId > 0) return sourceId;

    std::string ins = "INSERT INTO data_sources (name, base_url, description, is_official) VALUES (?, ?, ?, 1);";
    stmt = m_db.prepare(ins);
    if (!stmt) return 1;

    sqlite3_bind_text(stmt, 1, sourceName.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, baseUrl.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, description.c_str(), -1, SQLITE_TRANSIENT);
    if (sqlite3_step(stmt) == SQLITE_DONE) {
        sourceId = m_db.getLastInsertId();
    }
    m_db.finalize(stmt);

    return (sourceId > 0) ? sourceId : 1;
}

std::string ObjectRepository::getSourceName(int64_t sourceId) {
    std::string q = "SELECT name FROM data_sources WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(q);
    if (!stmt) return "Bundled Seed Dataset";

    sqlite3_bind_int64(stmt, 1, sourceId);
    std::string name = "Bundled Seed Dataset";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char* text = sqlite3_column_text(stmt, 0);
        if (text) name = reinterpret_cast<const char*>(text);
    }
    m_db.finalize(stmt);
    return name;
}

bool ObjectRepository::saveCelestialBodyRecord(const CelestialBodyRecord& record, int64_t* outId) {
    DatabaseManager::ScopedTransaction tx(m_db);

    int64_t sourceId = getOrCreateSourceId(record.sourceName);

    // 1. Check if object already exists by slug
    int64_t objId = 0;
    std::string findSql = "SELECT id FROM objects WHERE slug = ?;";
    sqlite3_stmt* stmt = m_db.prepare(findSql);
    if (stmt) {
        sqlite3_bind_text(stmt, 1, record.object.slug.c_str(), -1, SQLITE_TRANSIENT);
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            objId = sqlite3_column_int64(stmt, 0);
        }
        m_db.finalize(stmt);
    }

    if (objId > 0) {
        // Update existing object
        std::string updateSql = "UPDATE objects SET name = ?, type = ?, parent_object_id = ?, category = ?, "
                                "is_synthetic = ?, color_r = ?, color_g = ?, color_b = ?, texture_path = ?, updated_at = datetime('now') WHERE id = ?;";
        stmt = m_db.prepare(updateSql);
        if (!stmt) return false;

        sqlite3_bind_text(stmt, 1, record.object.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, record.object.type.c_str(), -1, SQLITE_TRANSIENT);
        if (record.object.parentObjectId.has_value()) sqlite3_bind_int64(stmt, 3, record.object.parentObjectId.value());
        else sqlite3_bind_null(stmt, 3);
        sqlite3_bind_text(stmt, 4, record.object.category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 5, record.object.isSynthetic ? 1 : 0);
        sqlite3_bind_double(stmt, 6, record.object.color.r);
        sqlite3_bind_double(stmt, 7, record.object.color.g);
        sqlite3_bind_double(stmt, 8, record.object.color.b);
        sqlite3_bind_text(stmt, 9, record.object.texturePath.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int64(stmt, 10, objId);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            m_db.finalize(stmt);
            return false;
        }
        m_db.finalize(stmt);
    } else {
        // Insert new object
        std::string insertSql = "INSERT INTO objects (slug, name, type, parent_object_id, category, is_synthetic, color_r, color_g, color_b, texture_path) "
                                "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?);";
        stmt = m_db.prepare(insertSql);
        if (!stmt) return false;

        sqlite3_bind_text(stmt, 1, record.object.slug.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, record.object.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, record.object.type.c_str(), -1, SQLITE_TRANSIENT);
        if (record.object.parentObjectId.has_value()) sqlite3_bind_int64(stmt, 4, record.object.parentObjectId.value());
        else sqlite3_bind_null(stmt, 4);
        sqlite3_bind_text(stmt, 5, record.object.category.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_int(stmt, 6, record.object.isSynthetic ? 1 : 0);
        sqlite3_bind_double(stmt, 7, record.object.color.r);
        sqlite3_bind_double(stmt, 8, record.object.color.g);
        sqlite3_bind_double(stmt, 9, record.object.color.b);
        sqlite3_bind_text(stmt, 10, record.object.texturePath.c_str(), -1, SQLITE_TRANSIENT);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            m_db.finalize(stmt);
            return false;
        }
        objId = m_db.getLastInsertId();
        m_db.finalize(stmt);
    }

    if (outId) *outId = objId;

    // 2. Insert or replace physical_properties
    std::string physSql = "INSERT OR REPLACE INTO physical_properties ("
                          "object_id, mass_kg, radius_m, albedo, greenhouse_k, luminosity_w, axial_tilt_deg, "
                          "rotation_period_hours, mean_density_kg_m3, surface_gravity_mps2, escape_velocity_mps, "
                          "surface_temp_k, surface_pressure_kpa, magnetic_field_str, atmosphere_summary, rings_json, "
                          "source_id, source_record_id, import_timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";
    stmt = m_db.prepare(physSql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, objId);
    if (record.physical.massKg.has_value()) sqlite3_bind_double(stmt, 2, record.physical.massKg.value()); else sqlite3_bind_null(stmt, 2);
    if (record.physical.radiusM.has_value()) sqlite3_bind_double(stmt, 3, record.physical.radiusM.value()); else sqlite3_bind_null(stmt, 3);
    if (record.physical.albedo.has_value()) sqlite3_bind_double(stmt, 4, record.physical.albedo.value()); else sqlite3_bind_null(stmt, 4);
    if (record.physical.greenhouseK.has_value()) sqlite3_bind_double(stmt, 5, record.physical.greenhouseK.value()); else sqlite3_bind_null(stmt, 5);
    if (record.physical.luminosityW.has_value()) sqlite3_bind_double(stmt, 6, record.physical.luminosityW.value()); else sqlite3_bind_null(stmt, 6);
    if (record.physical.axialTiltDeg.has_value()) sqlite3_bind_double(stmt, 7, record.physical.axialTiltDeg.value()); else sqlite3_bind_null(stmt, 7);
    if (record.physical.rotationPeriodHours.has_value()) sqlite3_bind_double(stmt, 8, record.physical.rotationPeriodHours.value()); else sqlite3_bind_null(stmt, 8);
    if (record.physical.meanDensityKgM3.has_value()) sqlite3_bind_double(stmt, 9, record.physical.meanDensityKgM3.value()); else sqlite3_bind_null(stmt, 9);
    if (record.physical.surfaceGravityMps2.has_value()) sqlite3_bind_double(stmt, 10, record.physical.surfaceGravityMps2.value()); else sqlite3_bind_null(stmt, 10);
    if (record.physical.escapeVelocityMps.has_value()) sqlite3_bind_double(stmt, 11, record.physical.escapeVelocityMps.value()); else sqlite3_bind_null(stmt, 11);
    if (record.physical.surfaceTempK.has_value()) sqlite3_bind_double(stmt, 12, record.physical.surfaceTempK.value()); else sqlite3_bind_null(stmt, 12);
    if (record.physical.surfacePressureKpa.has_value()) sqlite3_bind_double(stmt, 13, record.physical.surfacePressureKpa.value()); else sqlite3_bind_null(stmt, 13);
    if (record.physical.magneticFieldStr.has_value()) sqlite3_bind_text(stmt, 14, record.physical.magneticFieldStr.value().c_str(), -1, SQLITE_TRANSIENT); else sqlite3_bind_null(stmt, 14);
    if (record.physical.atmosphereSummary.has_value()) sqlite3_bind_text(stmt, 15, record.physical.atmosphereSummary.value().c_str(), -1, SQLITE_TRANSIENT); else sqlite3_bind_null(stmt, 15);
    if (record.physical.ringsJson.has_value()) sqlite3_bind_text(stmt, 16, record.physical.ringsJson.value().c_str(), -1, SQLITE_TRANSIENT); else sqlite3_bind_null(stmt, 16);
    sqlite3_bind_int64(stmt, 17, sourceId);
    sqlite3_bind_text(stmt, 18, record.physical.sourceRecordId.c_str(), -1, SQLITE_TRANSIENT);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        m_db.finalize(stmt);
        return false;
    }
    m_db.finalize(stmt);

    // 3. Insert or replace orbital_elements
    std::string orbSql = "INSERT OR REPLACE INTO orbital_elements ("
                         "object_id, epoch_jd, semi_major_axis_m, semi_major_axis_au, eccentricity, inclination_deg, "
                         "long_ascending_node_deg, arg_periapsis_deg, mean_anomaly_deg, true_anomaly_deg, orbital_period_days, "
                         "reference_frame, source_id, import_timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";
    stmt = m_db.prepare(orbSql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, objId);
    sqlite3_bind_double(stmt, 2, record.orbital.epochJd);
    if (record.orbital.semiMajorAxisM.has_value()) sqlite3_bind_double(stmt, 3, record.orbital.semiMajorAxisM.value()); else sqlite3_bind_null(stmt, 3);
    if (record.orbital.semiMajorAxisAU.has_value()) sqlite3_bind_double(stmt, 4, record.orbital.semiMajorAxisAU.value()); else sqlite3_bind_null(stmt, 4);
    if (record.orbital.eccentricity.has_value()) sqlite3_bind_double(stmt, 5, record.orbital.eccentricity.value()); else sqlite3_bind_null(stmt, 5);
    if (record.orbital.inclinationDeg.has_value()) sqlite3_bind_double(stmt, 6, record.orbital.inclinationDeg.value()); else sqlite3_bind_null(stmt, 6);
    if (record.orbital.longAscendingNodeDeg.has_value()) sqlite3_bind_double(stmt, 7, record.orbital.longAscendingNodeDeg.value()); else sqlite3_bind_null(stmt, 7);
    if (record.orbital.argPeriapsisDeg.has_value()) sqlite3_bind_double(stmt, 8, record.orbital.argPeriapsisDeg.value()); else sqlite3_bind_null(stmt, 8);
    if (record.orbital.meanAnomalyDeg.has_value()) sqlite3_bind_double(stmt, 9, record.orbital.meanAnomalyDeg.value()); else sqlite3_bind_null(stmt, 9);
    if (record.orbital.trueAnomalyDeg.has_value()) sqlite3_bind_double(stmt, 10, record.orbital.trueAnomalyDeg.value()); else sqlite3_bind_null(stmt, 10);
    if (record.orbital.orbitalPeriodDays.has_value()) sqlite3_bind_double(stmt, 11, record.orbital.orbitalPeriodDays.value()); else sqlite3_bind_null(stmt, 11);
    sqlite3_bind_text(stmt, 12, record.orbital.referenceFrame.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 13, sourceId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        m_db.finalize(stmt);
        return false;
    }
    m_db.finalize(stmt);

    // 4. Insert or replace state_vectors
    std::string svSql = "INSERT OR REPLACE INTO state_vectors ("
                        "object_id, epoch_jd, pos_x_m, pos_y_m, pos_z_m, vel_x_mps, vel_y_mps, vel_z_mps, "
                        "reference_frame, source_id, import_timestamp) VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, datetime('now'));";
    stmt = m_db.prepare(svSql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, objId);
    sqlite3_bind_double(stmt, 2, record.stateVector.epochJd);
    sqlite3_bind_double(stmt, 3, record.stateVector.positionM.x);
    sqlite3_bind_double(stmt, 4, record.stateVector.positionM.y);
    sqlite3_bind_double(stmt, 5, record.stateVector.positionM.z);
    sqlite3_bind_double(stmt, 6, record.stateVector.velocityMps.x);
    sqlite3_bind_double(stmt, 7, record.stateVector.velocityMps.y);
    sqlite3_bind_double(stmt, 8, record.stateVector.velocityMps.z);
    sqlite3_bind_text(stmt, 9, record.stateVector.referenceFrame.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 10, sourceId);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        m_db.finalize(stmt);
        return false;
    }
    m_db.finalize(stmt);

    // 5. Update composition
    std::string delComp = "DELETE FROM composition WHERE object_id = ?;";
    stmt = m_db.prepare(delComp);
    if (stmt) {
        sqlite3_bind_int64(stmt, 1, objId);
        sqlite3_step(stmt);
        m_db.finalize(stmt);
    }

    if (!record.composition.empty()) {
        std::string insComp = "INSERT INTO composition (object_id, element_or_compound, percentage, color_r, color_g, color_b, color_a) "
                             "VALUES (?, ?, ?, ?, ?, ?, ?);";
        stmt = m_db.prepare(insComp);
        if (stmt) {
            for (const auto& comp : record.composition) {
                sqlite3_bind_int64(stmt, 1, objId);
                sqlite3_bind_text(stmt, 2, comp.elementOrCompound.c_str(), -1, SQLITE_TRANSIENT);
                sqlite3_bind_double(stmt, 3, comp.percentage);
                sqlite3_bind_double(stmt, 4, comp.color.r);
                sqlite3_bind_double(stmt, 5, comp.color.g);
                sqlite3_bind_double(stmt, 6, comp.color.b);
                sqlite3_bind_double(stmt, 7, comp.color.a);
                sqlite3_step(stmt);
                sqlite3_reset(stmt);
            }
            m_db.finalize(stmt);
        }
    }

    tx.commit();
    return true;
}

bool ObjectRepository::saveCelestialBody(const CelestialBody& body, int64_t* outId) {
    CelestialBodyRecord rec;
    rec.object.slug = body.id;
    rec.object.name = body.name;
    rec.object.type = body.type;
    rec.object.category = body.category;
    rec.object.isSynthetic = body.isSynthetic;
    rec.object.color = body.color;
    rec.object.texturePath = body.texturePath;
    rec.object.parentObjectId = body.parentObjectId;
    rec.sourceName = body.sourceName;

    rec.physical.massKg = (body.massKg > 0.0) ? std::optional<double>(body.massKg) : std::nullopt;
    rec.physical.radiusM = (body.radiusM > 0.0) ? std::optional<double>(body.radiusM) : std::nullopt;
    rec.physical.albedo = body.albedo;
    rec.physical.greenhouseK = body.greenhouseK;
    rec.physical.luminosityW = body.luminosityW;
    rec.physical.axialTiltDeg = body.axialTiltDeg;
    rec.physical.rotationPeriodHours = body.rotationPeriodHours;
    rec.physical.meanDensityKgM3 = (body.meanDensityKgM3 > 0.0) ? std::optional<double>(body.meanDensityKgM3) : std::nullopt;
    rec.physical.surfaceGravityMps2 = (body.surfaceGravityMps2 > 0.0) ? std::optional<double>(body.surfaceGravityMps2) : std::nullopt;
    rec.physical.escapeVelocityMps = (body.escapeVelocityKmpS > 0.0) ? std::optional<double>(body.escapeVelocityKmpS * 1000.0) : std::nullopt;
    rec.physical.surfaceTempK = (body.surfaceTempK > 0.0) ? std::optional<double>(body.surfaceTempK) : std::nullopt;
    rec.physical.sourceRecordId = body.sourceObjectId;

    if (!body.atmosphereStr.empty()) rec.physical.atmosphereSummary = body.atmosphereStr;
    if (!body.magneticFieldStr.empty()) rec.physical.magneticFieldStr = body.magneticFieldStr;

    // Rings
    if (body.ring.hasRing) {
        nlohmann::json ringJson;
        ringJson["hasRing"] = true;
        ringJson["innerRadiusM"] = body.ring.innerRadiusM;
        ringJson["outerRadiusM"] = body.ring.outerRadiusM;
        ringJson["massKg"] = body.ring.massKg;
        ringJson["thicknessM"] = body.ring.thicknessM;
        ringJson["colorR"] = body.ring.baseColor.r;
        ringJson["colorG"] = body.ring.baseColor.g;
        ringJson["colorB"] = body.ring.baseColor.b;
        ringJson["texturePath"] = body.ring.texturePath;
        rec.physical.ringsJson = ringJson.dump();
    }

    rec.orbital.epochJd = body.epochJd;
    if (body.semiMajorAxisM > 0.0) rec.orbital.semiMajorAxisM = body.semiMajorAxisM;
    if (body.semiMajorAxisAU > 0.0) rec.orbital.semiMajorAxisAU = body.semiMajorAxisAU;
    if (body.eccentricity >= 0.0) rec.orbital.eccentricity = body.eccentricity;
    if (body.orbitalPeriodDays > 0.0) rec.orbital.orbitalPeriodDays = body.orbitalPeriodDays;
    rec.orbital.referenceFrame = body.referenceFrame;

    rec.stateVector.epochJd = body.epochJd;
    rec.stateVector.positionM = body.positionM;
    rec.stateVector.velocityMps = body.velocityMps;
    rec.stateVector.referenceFrame = body.referenceFrame;

    for (const auto& item : body.composition) {
        CompositionRecord c;
        c.elementOrCompound = item.name;
        c.percentage = item.percentage;
        c.color = item.color;
        rec.composition.push_back(c);
    }

    return saveCelestialBodyRecord(rec, outId);
}

std::vector<ObjectRecord> ObjectRepository::getAllObjects(const std::string& category, bool includeSynthetic, const std::string& searchQuery) {
    std::vector<ObjectRecord> results;
    std::string sql = "SELECT id, slug, name, type, parent_object_id, category, is_synthetic, color_r, color_g, color_b, texture_path, created_at, updated_at "
                      "FROM objects WHERE 1=1 ";

    if (!category.empty()) {
        sql += " AND category = ? ";
    }
    if (!includeSynthetic) {
        sql += " AND is_synthetic = 0 ";
    }
    if (!searchQuery.empty()) {
        sql += " AND (name LIKE ? OR slug LIKE ? OR type LIKE ?) ";
    }
    sql += " ORDER BY id ASC;";

    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return results;

    int bindIdx = 1;
    if (!category.empty()) {
        sqlite3_bind_text(stmt, bindIdx++, category.c_str(), -1, SQLITE_TRANSIENT);
    }
    if (!searchQuery.empty()) {
        std::string wild = "%" + searchQuery + "%";
        sqlite3_bind_text(stmt, bindIdx++, wild.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, wild.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, bindIdx++, wild.c_str(), -1, SQLITE_TRANSIENT);
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        ObjectRecord obj;
        obj.id = sqlite3_column_int64(stmt, 0);
        obj.slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        obj.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        obj.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) obj.parentObjectId = sqlite3_column_int64(stmt, 4);
        obj.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        obj.isSynthetic = (sqlite3_column_int(stmt, 6) != 0);
        obj.color.r = (float)sqlite3_column_double(stmt, 7);
        obj.color.g = (float)sqlite3_column_double(stmt, 8);
        obj.color.b = (float)sqlite3_column_double(stmt, 9);
        const unsigned char* tex = sqlite3_column_text(stmt, 10);
        if (tex) obj.texturePath = reinterpret_cast<const char*>(tex);
        obj.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        obj.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        results.push_back(obj);
    }

    m_db.finalize(stmt);
    return results;
}

std::optional<ObjectRecord> ObjectRepository::getObjectById(int64_t id) {
    std::string sql = "SELECT id, slug, name, type, parent_object_id, category, is_synthetic, color_r, color_g, color_b, texture_path, created_at, updated_at "
                      "FROM objects WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);
    std::optional<ObjectRecord> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ObjectRecord obj;
        obj.id = sqlite3_column_int64(stmt, 0);
        obj.slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        obj.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        obj.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) obj.parentObjectId = sqlite3_column_int64(stmt, 4);
        obj.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        obj.isSynthetic = (sqlite3_column_int(stmt, 6) != 0);
        obj.color.r = (float)sqlite3_column_double(stmt, 7);
        obj.color.g = (float)sqlite3_column_double(stmt, 8);
        obj.color.b = (float)sqlite3_column_double(stmt, 9);
        const unsigned char* tex = sqlite3_column_text(stmt, 10);
        if (tex) obj.texturePath = reinterpret_cast<const char*>(tex);
        obj.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        obj.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        result = obj;
    }
    m_db.finalize(stmt);
    return result;
}

std::optional<ObjectRecord> ObjectRepository::getObjectBySlug(const std::string& slug) {
    std::string sql = "SELECT id, slug, name, type, parent_object_id, category, is_synthetic, color_r, color_g, color_b, texture_path, created_at, updated_at "
                      "FROM objects WHERE slug = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_text(stmt, 1, slug.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<ObjectRecord> result;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        ObjectRecord obj;
        obj.id = sqlite3_column_int64(stmt, 0);
        obj.slug = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        obj.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        obj.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) obj.parentObjectId = sqlite3_column_int64(stmt, 4);
        obj.category = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        obj.isSynthetic = (sqlite3_column_int(stmt, 6) != 0);
        obj.color.r = (float)sqlite3_column_double(stmt, 7);
        obj.color.g = (float)sqlite3_column_double(stmt, 8);
        obj.color.b = (float)sqlite3_column_double(stmt, 9);
        const unsigned char* tex = sqlite3_column_text(stmt, 10);
        if (tex) obj.texturePath = reinterpret_cast<const char*>(tex);
        obj.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        obj.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        result = obj;
    }
    m_db.finalize(stmt);
    return result;
}

std::optional<PhysicalPropertiesRecord> ObjectRepository::getPhysicalProperties(int64_t objectId) {
    std::string sql = "SELECT id, object_id, mass_kg, radius_m, albedo, greenhouse_k, luminosity_w, axial_tilt_deg, "
                      "rotation_period_hours, mean_density_kg_m3, surface_gravity_mps2, escape_velocity_mps, "
                      "surface_temp_k, surface_pressure_kpa, magnetic_field_str, atmosphere_summary, rings_json, "
                      "source_id, source_record_id, import_timestamp FROM physical_properties WHERE object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, objectId);
    std::optional<PhysicalPropertiesRecord> res;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        PhysicalPropertiesRecord p;
        p.id = sqlite3_column_int64(stmt, 0);
        p.objectId = sqlite3_column_int64(stmt, 1);
        if (sqlite3_column_type(stmt, 2) != SQLITE_NULL) p.massKg = sqlite3_column_double(stmt, 2);
        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) p.radiusM = sqlite3_column_double(stmt, 3);
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) p.albedo = sqlite3_column_double(stmt, 4);
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) p.greenhouseK = sqlite3_column_double(stmt, 5);
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) p.luminosityW = sqlite3_column_double(stmt, 6);
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) p.axialTiltDeg = sqlite3_column_double(stmt, 7);
        if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) p.rotationPeriodHours = sqlite3_column_double(stmt, 8);
        if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) p.meanDensityKgM3 = sqlite3_column_double(stmt, 9);
        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) p.surfaceGravityMps2 = sqlite3_column_double(stmt, 10);
        if (sqlite3_column_type(stmt, 11) != SQLITE_NULL) p.escapeVelocityMps = sqlite3_column_double(stmt, 11);
        if (sqlite3_column_type(stmt, 12) != SQLITE_NULL) p.surfaceTempK = sqlite3_column_double(stmt, 12);
        if (sqlite3_column_type(stmt, 13) != SQLITE_NULL) p.surfacePressureKpa = sqlite3_column_double(stmt, 13);
        if (sqlite3_column_type(stmt, 14) != SQLITE_NULL) p.magneticFieldStr = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        if (sqlite3_column_type(stmt, 15) != SQLITE_NULL) p.atmosphereSummary = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 15));
        if (sqlite3_column_type(stmt, 16) != SQLITE_NULL) p.ringsJson = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 16));
        p.sourceId = sqlite3_column_int64(stmt, 17);
        const unsigned char* srcRec = sqlite3_column_text(stmt, 18);
        if (srcRec) p.sourceRecordId = reinterpret_cast<const char*>(srcRec);
        p.importTimestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 19));
        res = p;
    }
    m_db.finalize(stmt);
    return res;
}

std::optional<OrbitalElementsRecord> ObjectRepository::getOrbitalElements(int64_t objectId) {
    std::string sql = "SELECT id, object_id, epoch_jd, semi_major_axis_m, semi_major_axis_au, eccentricity, inclination_deg, "
                      "long_ascending_node_deg, arg_periapsis_deg, mean_anomaly_deg, true_anomaly_deg, orbital_period_days, "
                      "reference_frame, source_id, import_timestamp FROM orbital_elements WHERE object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, objectId);
    std::optional<OrbitalElementsRecord> res;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        OrbitalElementsRecord o;
        o.id = sqlite3_column_int64(stmt, 0);
        o.objectId = sqlite3_column_int64(stmt, 1);
        o.epochJd = sqlite3_column_double(stmt, 2);
        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) o.semiMajorAxisM = sqlite3_column_double(stmt, 3);
        if (sqlite3_column_type(stmt, 4) != SQLITE_NULL) o.semiMajorAxisAU = sqlite3_column_double(stmt, 4);
        if (sqlite3_column_type(stmt, 5) != SQLITE_NULL) o.eccentricity = sqlite3_column_double(stmt, 5);
        if (sqlite3_column_type(stmt, 6) != SQLITE_NULL) o.inclinationDeg = sqlite3_column_double(stmt, 6);
        if (sqlite3_column_type(stmt, 7) != SQLITE_NULL) o.longAscendingNodeDeg = sqlite3_column_double(stmt, 7);
        if (sqlite3_column_type(stmt, 8) != SQLITE_NULL) o.argPeriapsisDeg = sqlite3_column_double(stmt, 8);
        if (sqlite3_column_type(stmt, 9) != SQLITE_NULL) o.meanAnomalyDeg = sqlite3_column_double(stmt, 9);
        if (sqlite3_column_type(stmt, 10) != SQLITE_NULL) o.trueAnomalyDeg = sqlite3_column_double(stmt, 10);
        if (sqlite3_column_type(stmt, 11) != SQLITE_NULL) o.orbitalPeriodDays = sqlite3_column_double(stmt, 11);
        o.referenceFrame = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 12));
        o.sourceId = sqlite3_column_int64(stmt, 13);
        o.importTimestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 14));
        res = o;
    }
    m_db.finalize(stmt);
    return res;
}

std::optional<StateVectorRecord> ObjectRepository::getStateVector(int64_t objectId) {
    std::string sql = "SELECT id, object_id, epoch_jd, pos_x_m, pos_y_m, pos_z_m, vel_x_mps, vel_y_mps, vel_z_mps, "
                      "reference_frame, source_id, import_timestamp FROM state_vectors WHERE object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, objectId);
    std::optional<StateVectorRecord> res;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        StateVectorRecord s;
        s.id = sqlite3_column_int64(stmt, 0);
        s.objectId = sqlite3_column_int64(stmt, 1);
        s.epochJd = sqlite3_column_double(stmt, 2);
        s.positionM.x = sqlite3_column_double(stmt, 3);
        s.positionM.y = sqlite3_column_double(stmt, 4);
        s.positionM.z = sqlite3_column_double(stmt, 5);
        s.velocityMps.x = sqlite3_column_double(stmt, 6);
        s.velocityMps.y = sqlite3_column_double(stmt, 7);
        s.velocityMps.z = sqlite3_column_double(stmt, 8);
        s.referenceFrame = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 9));
        s.sourceId = sqlite3_column_int64(stmt, 10);
        s.importTimestamp = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 11));
        res = s;
    }
    m_db.finalize(stmt);
    return res;
}

std::vector<CompositionRecord> ObjectRepository::getComposition(int64_t objectId) {
    std::vector<CompositionRecord> list;
    std::string sql = "SELECT id, object_id, element_or_compound, percentage, color_r, color_g, color_b, color_a "
                      "FROM composition WHERE object_id = ? ORDER BY percentage DESC;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    sqlite3_bind_int64(stmt, 1, objectId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        CompositionRecord c;
        c.id = sqlite3_column_int64(stmt, 0);
        c.objectId = sqlite3_column_int64(stmt, 1);
        c.elementOrCompound = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        c.percentage = (float)sqlite3_column_double(stmt, 3);
        c.color.r = (float)sqlite3_column_double(stmt, 4);
        c.color.g = (float)sqlite3_column_double(stmt, 5);
        c.color.b = (float)sqlite3_column_double(stmt, 6);
        c.color.a = (float)sqlite3_column_double(stmt, 7);
        list.push_back(c);
    }
    m_db.finalize(stmt);
    return list;
}

void ObjectRepository::hydrateCelestialBodyFields(CelestialBody& body, 
                                                 const ObjectRecord& obj, 
                                                 const std::optional<PhysicalPropertiesRecord>& phys,
                                                 const std::optional<OrbitalElementsRecord>& orb,
                                                 const std::optional<StateVectorRecord>& state,
                                                 const std::vector<CompositionRecord>& comp) {
    body.dbId = obj.id;
    body.id = obj.slug;
    body.name = obj.name;
    body.type = obj.type;
    body.category = obj.category;
    body.isSynthetic = obj.isSynthetic;
    body.color = obj.color;
    body.texturePath = obj.texturePath;
    body.parentObjectId = obj.parentObjectId;

    if (phys.has_value()) {
        const auto& p = phys.value();
        body.massKg = p.massKg.value_or(0.0);
        body.radiusM = p.radiusM.value_or(0.0);
        body.albedo = p.albedo.value_or(0.3);
        body.greenhouseK = p.greenhouseK.value_or(0.0);
        body.luminosityW = p.luminosityW.value_or(0.0);
        body.axialTiltDeg = (float)p.axialTiltDeg.value_or(0.0);
        body.rotationPeriodHours = p.rotationPeriodHours.value_or(24.0);
        body.meanDensityKgM3 = p.meanDensityKgM3.value_or(0.0);
        body.surfaceGravityMps2 = p.surfaceGravityMps2.value_or(0.0);
        body.escapeVelocityKmpS = p.escapeVelocityMps.value_or(0.0) / 1000.0;
        body.surfaceTempK = p.surfaceTempK.value_or(288.0);
        body.sourceObjectId = p.sourceRecordId;
        body.importTimestamp = p.importTimestamp;
        body.sourceName = getSourceName(p.sourceId);

        if (p.magneticFieldStr.has_value()) body.magneticFieldStr = p.magneticFieldStr.value();
        if (p.atmosphereSummary.has_value()) body.atmosphereStr = p.atmosphereSummary.value();

        char tiltBuf[32], rotBuf[32], radBuf[64], massBuf[64], gravBuf[32], escBuf[32], tempBuf[32], pressBuf[32], densBuf[32];
        snprintf(tiltBuf, sizeof(tiltBuf), "%.2f°", body.axialTiltDeg);
        body.axialTiltStr = tiltBuf;

        if (std::abs(body.rotationPeriodHours) >= 24.0) {
            int d = (int)(std::abs(body.rotationPeriodHours) / 24.0);
            int h = (int)(std::fmod(std::abs(body.rotationPeriodHours), 24.0));
            snprintf(rotBuf, sizeof(rotBuf), "%dd %dh", d, h);
        } else {
            int h = (int)std::abs(body.rotationPeriodHours);
            int m = (int)((std::abs(body.rotationPeriodHours) - h) * 60.0);
            snprintf(rotBuf, sizeof(rotBuf), "%dh %dm", h, m);
        }
        body.rotationPeriodStr = rotBuf;

        snprintf(radBuf, sizeof(radBuf), "%'.1f km", body.radiusM / 1000.0);
        body.radiusStr = radBuf;

        body.massStr = UnitConverter::formatMass(body.massKg);
        snprintf(gravBuf, sizeof(gravBuf), "%.2f m/s²", body.surfaceGravityMps2);
        body.gravityStr = gravBuf;
        snprintf(escBuf, sizeof(escBuf), "%.2f km/s", body.escapeVelocityKmpS);
        body.escapeVelocityStr = escBuf;
        snprintf(tempBuf, sizeof(tempBuf), "%.0f K", body.surfaceTempK);
        body.tempStr = tempBuf;

        if (p.surfacePressureKpa.has_value()) {
            snprintf(pressBuf, sizeof(pressBuf), "%.1f kPa", p.surfacePressureKpa.value());
            body.pressureStr = pressBuf;
        } else {
            body.pressureStr = "N/A";
        }

        if (body.meanDensityKgM3 > 0.0) {
            snprintf(densBuf, sizeof(densBuf), "%'.1f kg/m³", body.meanDensityKgM3);
            body.densityStr = densBuf;
        }

        // Parse rings JSON if present
        if (p.ringsJson.has_value() && !p.ringsJson.value().empty()) {
            try {
                auto j = nlohmann::json::parse(p.ringsJson.value());
                body.ring.hasRing = j.value("hasRing", true);
                body.ring.innerRadiusM = j.value("innerRadiusM", 74500000.0);
                body.ring.outerRadiusM = j.value("outerRadiusM", 140220000.0);
                body.ring.innerRadiusAU = body.ring.innerRadiusM / UnitConverter::AU_TO_METERS;
                body.ring.outerRadiusAU = body.ring.outerRadiusM / UnitConverter::AU_TO_METERS;
                body.ring.massKg = j.value("massKg", 1.5e19);
                body.ring.thicknessM = j.value("thicknessM", 20.0);
                body.ring.baseColor = glm::vec3(j.value("colorR", 0.88f), j.value("colorG", 0.82f), j.value("colorB", 0.70f));
                body.ring.texturePath = j.value("texturePath", "assets/textures/saturn_ring_alpha.png");
            } catch (...) {}
        }
    }

    if (orb.has_value()) {
        const auto& o = orb.value();
        body.epochJd = o.epochJd;
        body.epochUtcStr = UnitConverter::julianDateToUtcString(o.epochJd);
        body.semiMajorAxisM = o.semiMajorAxisM.value_or(0.0);
        body.semiMajorAxisAU = o.semiMajorAxisAU.value_or(body.semiMajorAxisM / UnitConverter::AU_TO_METERS);
        body.eccentricity = o.eccentricity.value_or(0.0);
        body.orbitalPeriodDays = o.orbitalPeriodDays.value_or(0.0);
        body.referenceFrame = o.referenceFrame;

        char smaBuf[64], eccBuf[32], perBuf[64];
        snprintf(smaBuf, sizeof(smaBuf), "%.3f AU (%.1fM km)", body.semiMajorAxisAU, (body.semiMajorAxisAU * UnitConverter::AU_TO_KM / 1e6));
        body.semiMajorAxisStr = smaBuf;
        snprintf(eccBuf, sizeof(eccBuf), "%.4f", body.eccentricity);
        body.eccentricityStr = eccBuf;

        if (body.orbitalPeriodDays >= 365.25 * 1.5) {
            snprintf(perBuf, sizeof(perBuf), "%.2f years", body.orbitalPeriodDays / 365.256);
        } else if (body.orbitalPeriodDays > 0.0) {
            snprintf(perBuf, sizeof(perBuf), "%.2f days", body.orbitalPeriodDays);
        } else {
            snprintf(perBuf, sizeof(perBuf), "N/A");
        }
        body.orbitalPeriodStr = perBuf;
        body.yearLengthStr = perBuf;

        double aM = body.semiMajorAxisAU * UnitConverter::AU_TO_METERS;
        double periM = aM * (1.0 - body.eccentricity);
        double apoM  = aM * (1.0 + body.eccentricity);
        char periBuf[64], apoBuf[64];
        snprintf(periBuf, sizeof(periBuf), "%.3f AU", periM / UnitConverter::AU_TO_METERS);
        snprintf(apoBuf, sizeof(apoBuf), "%.3f AU", apoM / UnitConverter::AU_TO_METERS);
        body.periapsisStr = periBuf;
        body.apoapsisStr = apoBuf;
    }

    if (state.has_value()) {
        const auto& s = state.value();
        body.positionM = s.positionM;
        body.velocityMps = s.velocityMps;
        body.position = glm::vec3((float)(body.positionM.x / UnitConverter::AU_TO_METERS),
                                  (float)(body.positionM.y / UnitConverter::AU_TO_METERS),
                                  (float)(body.positionM.z / UnitConverter::AU_TO_METERS));
        body.velocity = glm::vec3((float)(body.velocityMps.x / UnitConverter::AU_TO_METERS),
                                  (float)(body.velocityMps.y / UnitConverter::AU_TO_METERS),
                                  (float)(body.velocityMps.z / UnitConverter::AU_TO_METERS));
    }

    body.composition.clear();
    for (const auto& c : comp) {
        body.composition.push_back({ c.elementOrCompound, c.percentage, c.color });
    }

    body.realRadiusAU = (body.radiusM > 0.0) ? (body.radiusM / UnitConverter::AU_TO_METERS) : 0.0000426;
    if (body.semiMajorAxisAU > 0.0) {
        body.realOrbitRadiusAU = body.semiMajorAxisAU;
    } else if (body.semiMajorAxisM > 0.0) {
        body.realOrbitRadiusAU = body.semiMajorAxisM / UnitConverter::AU_TO_METERS;
    } else {
        body.realOrbitRadiusAU = (double)glm::length(body.position);
    }

    if (body.rotationPeriodHours != 0.0) {
        body.rotationSpeedRadPerSec = (2.0 * UnitConverter::PI) / (std::abs(body.rotationPeriodHours) * 3600.0);
        if (body.rotationPeriodHours < 0.0) body.rotationSpeedRadPerSec = -body.rotationSpeedRadPerSec;
    }
}

std::vector<CelestialBody> ObjectRepository::getSystemBodies(const std::string& systemCategory) {
    std::vector<CelestialBody> bodies;

    // 1. Try to find a registered system in the `systems` table
    auto sysOpt = getSystemByName(systemCategory);
    if (sysOpt.has_value()) {
        int64_t sysId = sysOpt.value().id;
        auto links = getSystemObjectLinks(sysId);
        if (!links.empty()) {
            // Map old parent IDs to hydrated bodies
            for (const auto& link : links) {
                auto bodyOpt = getHydratedBody(link.objectId);
                if (bodyOpt.has_value()) {
                    auto b = bodyOpt.value();
                    b.parentObjectId = link.parentObjectId;
                    b.category = sysOpt.value().name;
                    bodies.push_back(b);
                }
            }
        }
    }

    // 2. Fallback to category-based query if not found via systems table or if links empty
    if (bodies.empty()) {
        std::vector<ObjectRecord> objects;

        if (systemCategory == "Solar System" || systemCategory.empty()) {
            objects = getAllObjects("Solar System", false, "");
            auto asts = getAllObjects("Asteroid Belt", false, "");
            for (const auto& ast : asts) {
                bool exists = false;
                for (const auto& o : objects) { if (o.id == ast.id) { exists = true; break; } }
                if (!exists) objects.push_back(ast);
            }
        } else if (systemCategory == "Asteroid Belt") {
            auto solObj = getObjectBySlug("sol");
            if (solObj.has_value()) objects.push_back(solObj.value());
            auto asts = getAllObjects("Asteroid Belt", false, "");
            for (const auto& ast : asts) {
                if (ast.slug != "sol") objects.push_back(ast);
            }
        } else if (systemCategory == "Exoplanet System") {
            objects = getAllObjects("TRAPPIST-1 System", false, "");
            if (objects.empty()) objects = getAllObjects("Exoplanet System", false, "");
        } else {
            objects = getAllObjects(systemCategory, false, "");
        }

        for (const auto& obj : objects) {
            auto phys = getPhysicalProperties(obj.id);
            auto orb  = getOrbitalElements(obj.id);
            auto state = getStateVector(obj.id);
            auto comp = getComposition(obj.id);

            CelestialBody body;
            hydrateCelestialBodyFields(body, obj, phys, orb, state, comp);
            bodies.push_back(body);
        }
    }

    // Calculate moon counts for host planets
    for (auto& b : bodies) {
        int moonCount = 0;
        for (const auto& other : bodies) {
            if (other.parentObjectId.has_value() && other.parentObjectId.value() == b.dbId) {
                moonCount++;
            }
        }
        b.moons = moonCount;
    }

    return bodies;
}

std::optional<CelestialBody> ObjectRepository::getHydratedBody(int64_t id) {
    auto obj = getObjectById(id);
    if (!obj.has_value()) return std::nullopt;

    auto phys = getPhysicalProperties(id);
    auto orb  = getOrbitalElements(id);
    auto state = getStateVector(id);
    auto comp = getComposition(id);

    CelestialBody body;
    hydrateCelestialBodyFields(body, obj.value(), phys, orb, state, comp);
    return body;
}

std::optional<CelestialBody> ObjectRepository::getHydratedBodyBySlug(const std::string& slug) {
    auto obj = getObjectBySlug(slug);
    if (!obj.has_value()) return std::nullopt;

    int64_t id = obj.value().id;
    auto phys = getPhysicalProperties(id);
    auto orb  = getOrbitalElements(id);
    auto state = getStateVector(id);
    auto comp = getComposition(id);

    CelestialBody body;
    hydrateCelestialBodyFields(body, obj.value(), phys, orb, state, comp);
    return body;
}

bool ObjectRepository::deleteObject(int64_t id) {
    std::string sql = "DELETE FROM objects WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, id);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

bool ObjectRepository::deleteObjectBySlug(const std::string& slug) {
    std::string sql = "DELETE FROM objects WHERE slug = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, slug.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

std::vector<std::string> ObjectRepository::getAvailableCategories() {
    std::vector<std::string> categories;
    std::string sql = "SELECT DISTINCT category FROM objects ORDER BY category ASC;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return categories;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        categories.push_back(reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0)));
    }
    m_db.finalize(stmt);
    return categories;
}

int ObjectRepository::getObjectCount(const std::string& category) {
    std::string sql = "SELECT COUNT(*) FROM objects";
    if (!category.empty()) sql += " WHERE category = ?";
    sql += ";";

    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return 0;

    if (!category.empty()) {
        sqlite3_bind_text(stmt, 1, category.c_str(), -1, SQLITE_TRANSIENT);
    }

    int count = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        count = sqlite3_column_int(stmt, 0);
    }
    m_db.finalize(stmt);
    return count;
}

// ── Systems Management Implementations ───────────────────────────────────────

bool ObjectRepository::createSystem(const SystemRecord& sys, int64_t* outId) {
    std::string sql = "INSERT OR REPLACE INTO systems (name, type, source, description, created_at, updated_at) "
                      "VALUES (?, ?, ?, ?, datetime('now'), datetime('now'));";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, sys.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sys.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, sys.source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, sys.description.c_str(), -1, SQLITE_TRANSIENT);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    if (ok && outId) {
        *outId = m_db.getLastInsertId();
    }
    m_db.finalize(stmt);
    return ok;
}

bool ObjectRepository::updateSystem(const SystemRecord& sys) {
    std::string sql = "UPDATE systems SET name = ?, type = ?, source = ?, description = ?, updated_at = datetime('now') WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, sys.name.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, sys.type.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, sys.source.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 4, sys.description.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_int64(stmt, 5, sys.id);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

bool ObjectRepository::deleteSystem(int64_t systemId) {
    std::string sql = "DELETE FROM systems WHERE id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, systemId);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

bool ObjectRepository::deleteSystemByName(const std::string& name) {
    std::string sql = "DELETE FROM systems WHERE name = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

std::vector<SystemRecord> ObjectRepository::getAllSystems() {
    std::vector<SystemRecord> list;
    std::string sql = "SELECT s.id, s.name, s.type, s.source, s.description, s.created_at, s.updated_at, "
                      "(SELECT COUNT(*) FROM system_objects so WHERE so.system_id = s.id) AS obj_count "
                      "FROM systems s ORDER BY s.updated_at DESC, s.id DESC;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SystemRecord s;
        s.id = sqlite3_column_int64(stmt, 0);
        s.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const unsigned char* desc = sqlite3_column_text(stmt, 4);
        if (desc) s.description = reinterpret_cast<const char*>(desc);
        s.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        s.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        s.objectCount = sqlite3_column_int(stmt, 7);
        list.push_back(s);
    }
    m_db.finalize(stmt);
    return list;
}

std::optional<SystemRecord> ObjectRepository::getSystemById(int64_t id) {
    std::string sql = "SELECT s.id, s.name, s.type, s.source, s.description, s.created_at, s.updated_at, "
                      "(SELECT COUNT(*) FROM system_objects so WHERE so.system_id = s.id) AS obj_count "
                      "FROM systems s WHERE s.id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_int64(stmt, 1, id);
    std::optional<SystemRecord> res;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SystemRecord s;
        s.id = sqlite3_column_int64(stmt, 0);
        s.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const unsigned char* desc = sqlite3_column_text(stmt, 4);
        if (desc) s.description = reinterpret_cast<const char*>(desc);
        s.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        s.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        s.objectCount = sqlite3_column_int(stmt, 7);
        res = s;
    }
    m_db.finalize(stmt);
    return res;
}

std::optional<SystemRecord> ObjectRepository::getSystemByName(const std::string& name) {
    std::string sql = "SELECT s.id, s.name, s.type, s.source, s.description, s.created_at, s.updated_at, "
                      "(SELECT COUNT(*) FROM system_objects so WHERE so.system_id = s.id) AS obj_count "
                      "FROM systems s WHERE s.name = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return std::nullopt;

    sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT);
    std::optional<SystemRecord> res;

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        SystemRecord s;
        s.id = sqlite3_column_int64(stmt, 0);
        s.name = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
        s.type = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 2));
        s.source = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3));
        const unsigned char* desc = sqlite3_column_text(stmt, 4);
        if (desc) s.description = reinterpret_cast<const char*>(desc);
        s.createdAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 5));
        s.updatedAt = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 6));
        s.objectCount = sqlite3_column_int(stmt, 7);
        res = s;
    }
    m_db.finalize(stmt);
    return res;
}

bool ObjectRepository::addSystemObject(int64_t systemId, int64_t objectId, std::optional<int64_t> parentObjectId, int orbitalOrder) {
    std::string sql = "INSERT OR REPLACE INTO system_objects (system_id, object_id, parent_object_id, orbital_order) "
                      "VALUES (?, ?, ?, ?);";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, systemId);
    sqlite3_bind_int64(stmt, 2, objectId);
    if (parentObjectId.has_value()) sqlite3_bind_int64(stmt, 3, parentObjectId.value());
    else sqlite3_bind_null(stmt, 3);
    sqlite3_bind_int(stmt, 4, orbitalOrder);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

bool ObjectRepository::removeSystemObject(int64_t systemId, int64_t objectId) {
    std::string sql = "DELETE FROM system_objects WHERE system_id = ? AND object_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return false;

    sqlite3_bind_int64(stmt, 1, systemId);
    sqlite3_bind_int64(stmt, 2, objectId);

    bool ok = (sqlite3_step(stmt) == SQLITE_DONE);
    m_db.finalize(stmt);
    return ok;
}

std::vector<SystemObjectRecord> ObjectRepository::getSystemObjectLinks(int64_t systemId) {
    std::vector<SystemObjectRecord> list;
    std::string sql = "SELECT id, system_id, object_id, parent_object_id, orbital_order FROM system_objects "
                      "WHERE system_id = ? ORDER BY orbital_order ASC, id ASC;";
    sqlite3_stmt* stmt = m_db.prepare(sql);
    if (!stmt) return list;

    sqlite3_bind_int64(stmt, 1, systemId);
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SystemObjectRecord r;
        r.id = sqlite3_column_int64(stmt, 0);
        r.systemId = sqlite3_column_int64(stmt, 1);
        r.objectId = sqlite3_column_int64(stmt, 2);
        if (sqlite3_column_type(stmt, 3) != SQLITE_NULL) r.parentObjectId = sqlite3_column_int64(stmt, 3);
        r.orbitalOrder = sqlite3_column_int(stmt, 4);
        list.push_back(r);
    }
    m_db.finalize(stmt);
    return list;
}

bool ObjectRepository::saveCustomSystem(const SystemRecord& sys, const std::vector<CelestialBody>& bodies, int64_t* outSystemId) {
    DatabaseManager::ScopedTransaction tx(m_db);

    int64_t systemId = sys.id;
    if (systemId <= 0) {
        auto existing = getSystemByName(sys.name);
        if (existing.has_value()) {
            systemId = existing.value().id;
        }
    }

    if (systemId > 0) {
        SystemRecord updatedSys = sys;
        updatedSys.id = systemId;
        if (!updateSystem(updatedSys)) return false;
    } else {
        if (!createSystem(sys, &systemId)) return false;
    }

    if (outSystemId) *outSystemId = systemId;

    // Clear old links for this system
    std::string clearLinks = "DELETE FROM system_objects WHERE system_id = ?;";
    sqlite3_stmt* stmt = m_db.prepare(clearLinks);
    if (stmt) {
        sqlite3_bind_int64(stmt, 1, systemId);
        sqlite3_step(stmt);
        m_db.finalize(stmt);
    }

    // Save each body and create links
    std::map<int64_t, int64_t> oldToNewIdMap;
    std::vector<std::pair<int64_t, std::optional<int64_t>>> savedObjParentPairs;

    for (const auto& b : bodies) {
        CelestialBody copyBody = b;
        copyBody.category = sys.name;
        copyBody.parentObjectId = std::nullopt; // clear during base object insert to avoid foreign key errors on uninserted parents
        int64_t newObjId = 0;

        if (!saveCelestialBody(copyBody, &newObjId)) {
            return false;
        }

        if (b.dbId > 0) {
            oldToNewIdMap[b.dbId] = newObjId;
        } else {
            oldToNewIdMap[newObjId] = newObjId;
        }

        savedObjParentPairs.push_back({ newObjId, b.parentObjectId });
    }

    // Pass 2: Link hierarchy in system_objects and update parent_object_id in objects table
    int order = 0;
    for (const auto& pair : savedObjParentPairs) {
        int64_t objId = pair.first;
        std::optional<int64_t> origParent = pair.second;
        std::optional<int64_t> mappedParent = std::nullopt;

        if (origParent.has_value()) {
            int64_t op = origParent.value();
            if (oldToNewIdMap.find(op) != oldToNewIdMap.end()) {
                mappedParent = oldToNewIdMap[op];
            } else if (getObjectById(op).has_value()) {
                mappedParent = op;
            }
        }

        if (mappedParent.has_value()) {
            std::string updateParentSql = "UPDATE objects SET parent_object_id = ? WHERE id = ?;";
            sqlite3_stmt* pStmt = m_db.prepare(updateParentSql);
            if (pStmt) {
                sqlite3_bind_int64(pStmt, 1, mappedParent.value());
                sqlite3_bind_int64(pStmt, 2, objId);
                sqlite3_step(pStmt);
                m_db.finalize(pStmt);
            }
        }

        if (!addSystemObject(systemId, objId, mappedParent, order++)) {
            return false;
        }
    }

    tx.commit();
    return true;
}


bool ObjectRepository::duplicateSystem(int64_t sourceSystemId, const std::string& newName, int64_t* outNewId) {
    auto srcSysOpt = getSystemById(sourceSystemId);
    if (!srcSysOpt.has_value()) return false;

    const auto& srcSys = srcSysOpt.value();
    auto srcBodies = getSystemBodies(srcSys.name);

    SystemRecord newSys;
    newSys.name = newName;
    newSys.type = "Custom";
    newSys.source = "Duplicated from " + srcSys.name;
    newSys.description = "Duplicated copy of " + srcSys.name + " (" + srcSys.description + ")";

    // Give each body a unique slug suffix to prevent slug collisions
    int64_t timeSuffix = (int64_t)time(nullptr);
    for (auto& b : srcBodies) {
        b.id = b.id + "_copy_" + std::to_string(timeSuffix % 100000);
        b.category = newName;
        b.dbId = 0; // force new insert
    }

    return saveCustomSystem(newSys, srcBodies, outNewId);
}

std::vector<SystemValidationWarning> ObjectRepository::validateSystem(const std::vector<CelestialBody>& bodies) {
    std::vector<SystemValidationWarning> warnings;

    if (bodies.empty()) {
        warnings.push_back({
            SystemValidationWarning::Severity::Error,
            "Empty System",
            "At least one celestial body is required to run a simulation.",
            "", ""
        });
        return warnings;
    }

    bool hasAnchor = false;
    for (const auto& b : bodies) {
        if (b.type.find("Star") != std::string::npos || b.type.find("Black Hole") != std::string::npos || b.massKg > 1e28) {
            hasAnchor = true;
            break;
        }
    }

    if (!hasAnchor && bodies.size() > 1) {
        warnings.push_back({
            SystemValidationWarning::Severity::Info,
            "No Massive Anchor Body",
            "System does not contain a primary star or black hole. All bodies will orbit their mutual barycenter.",
            "", ""
        });
    }

    // Physical radius vs orbital distance check
    for (const auto& b : bodies) {
        if (b.semiMajorAxisAU > 0.0) {
            double smaM = b.semiMajorAxisAU * UnitConverter::AU_TO_METERS;
            if (b.radiusM >= smaM * 0.9) {
                warnings.push_back({
                    SystemValidationWarning::Severity::Warning,
                    "Radius Exceeds Orbital Distance",
                    "Physical radius of " + b.name + " (" + UnitConverter::formatDistance(b.radiusM) + ") exceeds or is close to its semi-major axis.",
                    b.name, ""
                });
            }
        }
    }

    // Proximity / Collision detection check
    for (size_t i = 0; i < bodies.size(); ++i) {
        for (size_t j = i + 1; j < bodies.size(); ++j) {
            const auto& b1 = bodies[i];
            const auto& b2 = bodies[j];

            double distM = glm::length(b1.positionM - b2.positionM);
            double minDist = (b1.radiusM + b2.radiusM);

            if (distM < minDist && minDist > 0.0) {
                warnings.push_back({
                    SystemValidationWarning::Severity::Warning,
                    "Immediate Collision Detected",
                    "Bodies '" + b1.name + "' and '" + b2.name + "' overlap in 3D space (Distance: " + 
                    UnitConverter::formatDistance(distM) + " < Sum of Radii: " + UnitConverter::formatDistance(minDist) + ").",
                    b1.name, b2.name
                });
            } else if (distM < 1000000.0 && distM > 0.0) {
                warnings.push_back({
                    SystemValidationWarning::Severity::Info,
                    "Close Proximity",
                    "Bodies '" + b1.name + "' and '" + b2.name + "' are within 1,000 km of each other.",
                    b1.name, b2.name
                });
            }
        }
    }

    return warnings;
}

} // namespace AstroGenesis

