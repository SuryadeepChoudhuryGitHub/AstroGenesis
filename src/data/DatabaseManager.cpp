#include "data/DatabaseManager.hpp"
#include "renderer/ShaderLoader.hpp"
#include <iostream>
#include <filesystem>
#include <sstream>

namespace AstroGenesis {

DatabaseManager& DatabaseManager::getInstance() {
    static DatabaseManager instance;
    return instance;
}

DatabaseManager::DatabaseManager() {}

DatabaseManager::~DatabaseManager() {
    close();
}

std::string DatabaseManager::resolveAuthoritativePath(const std::string& dbPath) {
    std::error_code ec;
    std::filesystem::path p(dbPath);
    if (p.is_absolute() && std::filesystem::exists(p, ec)) {
        return std::filesystem::canonical(p, ec).string();
    }

    std::string exeDir = getExecutableDir();
    std::vector<std::filesystem::path> searchRoots = {
        std::filesystem::current_path(),
        std::filesystem::current_path() / "..",
        std::filesystem::current_path() / "../.."
    };
    if (!exeDir.empty()) {
        searchRoots.push_back(exeDir);
        searchRoots.push_back(std::filesystem::path(exeDir) / "..");
        searchRoots.push_back(std::filesystem::path(exeDir) / "../..");
        searchRoots.push_back(std::filesystem::path(exeDir) / "../../..");
    }

    // Priority 1: Check for Project Root (contains CMakeLists.txt and data directory)
    for (const auto& root : searchRoots) {
        if (std::filesystem::exists(root / "CMakeLists.txt", ec)) {
            auto fullDb = root / dbPath;
            if (std::filesystem::exists(fullDb, ec)) {
                return std::filesystem::canonical(fullDb, ec).string();
            }
            if (std::filesystem::exists(root / "data", ec)) {
                return std::filesystem::absolute(fullDb).lexically_normal().string();
            }
        }
    }

    // Priority 2: Standalone/Packaged mode (data folder alongside executable or current dir)
    for (const auto& root : searchRoots) {
        auto fullDb = root / dbPath;
        if (std::filesystem::exists(fullDb, ec)) {
            return std::filesystem::canonical(fullDb, ec).string();
        }
    }

    // Fallback: Current working directory
    return std::filesystem::absolute(std::filesystem::current_path() / dbPath).lexically_normal().string();
}

bool DatabaseManager::validateSchema(std::string& outError) {
    if (!m_db) {
        outError = "Database is not open";
        return false;
    }

    // 1. SQLite Quick Integrity Check
    sqlite3_stmt* stmt = prepare("PRAGMA quick_check;");
    if (!stmt) {
        outError = "Failed to run PRAGMA quick_check: " + m_lastError;
        return false;
    }
    std::string checkResult = "unknown";
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        checkResult = columnTextSafe(stmt, 0);
    }
    finalize(stmt);
    if (checkResult != "ok") {
        outError = "SQLite integrity check failed: " + checkResult;
        return false;
    }

    // 2. Foreign Key Integrity Check
    stmt = prepare("PRAGMA foreign_key_check;");
    if (stmt) {
        if (sqlite3_step(stmt) == SQLITE_ROW) {
            std::string violatedTable = columnTextSafe(stmt, 0);
            finalize(stmt);
            outError = "Foreign key constraint violation in table: " + violatedTable;
            return false;
        }
        finalize(stmt);
    }

    // 3. Verify all 14 required tables exist
    const std::vector<std::string> requiredTables = {
        "schema_migrations", "data_sources", "objects", "physical_properties",
        "orbital_elements", "state_vectors", "composition", "data_imports",
        "ephemeris_records", "simulation_runs", "simulation_states",
        "validation_results", "systems", "system_objects"
    };

    stmt = prepare("SELECT name FROM sqlite_master WHERE type='table';");
    if (!stmt) {
        outError = "Failed to query sqlite_master for table definitions: " + m_lastError;
        return false;
    }
    std::vector<std::string> existingTables;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        existingTables.push_back(columnTextSafe(stmt, 0));
    }
    finalize(stmt);

    for (const auto& req : requiredTables) {
        bool found = false;
        for (const auto& ex : existingTables) {
            if (ex == req) { found = true; break; }
        }
        if (!found) {
            outError = "Missing required database table: " + req;
            return false;
        }
    }

    return true;
}

bool DatabaseManager::initialize(const std::string& dbPath) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);

    std::string resolvedPath = resolveAuthoritativePath(dbPath);

    if (m_db != nullptr) {
        if (m_dbPath == resolvedPath) {
            return true;
        }
        close();
    }

    m_dbPath = resolvedPath;

    // Ensure parent directory exists
    try {
        std::filesystem::path path(resolvedPath);
        if (path.has_parent_path()) {
            std::filesystem::create_directories(path.parent_path());
        }
    } catch (const std::exception& e) {
        m_lastError = "Failed to create database directory: " + std::string(e.what());
        std::cerr << "[DatabaseManager] " << m_lastError << std::endl;
        return false;
    }

    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    int rc = sqlite3_open_v2(resolvedPath.c_str(), &m_db, flags, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = m_db ? sqlite3_errmsg(m_db) : "Failed to open SQLite database";
        std::cerr << "[DatabaseManager] " << m_lastError << " (Path: " << resolvedPath << ")" << std::endl;
        if (m_db) {
            sqlite3_close_v2(m_db);
            m_db = nullptr;
        }
        return false;
    }

    // Configure SQLite for high concurrency, WAL journaling, foreign keys, and 5-sec busy timeout
    execute("PRAGMA foreign_keys = ON;");
    execute("PRAGMA journal_mode = WAL;");
    execute("PRAGMA synchronous = NORMAL;");
    execute("PRAGMA busy_timeout = 5000;");
    execute("PRAGMA wal_checkpoint(PASSIVE);");

    // Run schema migrations
    if (!runMigrations()) {
        std::cerr << "[DatabaseManager] Schema migration failed: " << m_lastError << std::endl;
        return false;
    }

    // Validate schema integrity
    std::string schemaError;
    if (!validateSchema(schemaError)) {
        m_lastError = "Database schema validation failed: " + schemaError;
        std::cerr << "[DatabaseManager] " << m_lastError << std::endl;
        return false;
    }

    std::cout << "[DatabaseManager] Successfully opened database: " << resolvedPath << " (Schema integrity verified: PASS)" << std::endl;
    return true;
}

void DatabaseManager::close() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_db) {
        execute("PRAGMA wal_checkpoint(TRUNCATE);");
        int rc = sqlite3_close(m_db);
        if (rc == SQLITE_BUSY) {
            sqlite3_close_v2(m_db);
        }
        m_db = nullptr;
    }
}

bool DatabaseManager::execute(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_db) {
        if (!initialize()) {
            m_lastError = "Database connection is not open and auto-initialization failed.";
            std::cerr << "[DatabaseManager] " << m_lastError << std::endl;
            return false;
        }
    }
    char* zErrMsg = nullptr;
    int rc = sqlite3_exec(m_db, sql.c_str(), nullptr, nullptr, &zErrMsg);
    if (rc != SQLITE_OK) {
        m_lastError = zErrMsg ? zErrMsg : sqlite3_errmsg(m_db);
        std::cerr << "[DatabaseManager] SQL Execute Error [" << rc << "]: " << m_lastError << " | Query: " << sql << std::endl;
        sqlite3_free(zErrMsg);
        return false;
    }
    return true;
}

int64_t DatabaseManager::getLastInsertId() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_db) return 0;
    return (int64_t)sqlite3_last_insert_rowid(m_db);
}

sqlite3_stmt* DatabaseManager::prepare(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (!m_db) {
        if (!initialize()) {
            m_lastError = "Database connection is not open and auto-initialization failed.";
            std::cerr << "[DatabaseManager] " << m_lastError << std::endl;
            return nullptr;
        }
    }
    sqlite3_stmt* stmt = nullptr;
    int rc = sqlite3_prepare_v2(m_db, sql.c_str(), -1, &stmt, nullptr);
    if (rc != SQLITE_OK) {
        m_lastError = sqlite3_errmsg(m_db);
        std::cerr << "[DatabaseManager] SQL Prepare Error [" << rc << "]: " << m_lastError << " | Query: " << sql << std::endl;
        return nullptr;
    }
    return stmt;
}

void DatabaseManager::finalize(sqlite3_stmt*& stmt) {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (stmt) {
        sqlite3_finalize(stmt);
        stmt = nullptr;
    }
}

std::string DatabaseManager::getLastError() const {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    return m_lastError;
}

bool DatabaseManager::beginTransaction() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_transactionDepth == 0) {
        bool ok = execute("BEGIN IMMEDIATE TRANSACTION;");
        if (ok) m_transactionDepth++;
        return ok;
    } else {
        std::string sp = "SAVEPOINT sp_" + std::to_string(m_transactionDepth) + ";";
        bool ok = execute(sp);
        if (ok) m_transactionDepth++;
        return ok;
    }
}

bool DatabaseManager::commit() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_transactionDepth <= 0) return false;
    if (m_transactionDepth == 1) {
        m_transactionDepth = 0;
        return execute("COMMIT;");
    } else {
        m_transactionDepth--;
        std::string sp = "RELEASE SAVEPOINT sp_" + std::to_string(m_transactionDepth) + ";";
        return execute(sp);
    }
}

bool DatabaseManager::rollback() {
    std::lock_guard<std::recursive_mutex> lock(m_mutex);
    if (m_transactionDepth <= 0) return false;
    if (m_transactionDepth == 1) {
        m_transactionDepth = 0;
        return execute("ROLLBACK;");
    } else {
        m_transactionDepth--;
        std::string sp = "ROLLBACK TO SAVEPOINT sp_" + std::to_string(m_transactionDepth) + ";";
        return execute(sp);
    }
}


int DatabaseManager::getCurrentSchemaVersion() {
    if (!m_db) return 0;
    
    // Create schema_migrations table if not exists
    execute("CREATE TABLE IF NOT EXISTS schema_migrations ("
            "version INTEGER PRIMARY KEY, "
            "name TEXT NOT NULL, "
            "applied_at TEXT NOT NULL DEFAULT (datetime('now')));");

    sqlite3_stmt* stmt = prepare("SELECT COALESCE(MAX(version), 0) FROM schema_migrations;");
    if (!stmt) return 0;

    int version = 0;
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        version = sqlite3_column_int(stmt, 0);
    }
    finalize(stmt);
    return version;
}

bool DatabaseManager::applyMigration(int version, const std::string& name, const std::string& sql) {
    std::cout << "[DatabaseManager] Applying Migration " << version << ": " << name << "..." << std::endl;
    
    ScopedTransaction tx(*this);
    if (!execute(sql)) {
        std::cerr << "[DatabaseManager] Failed to apply migration " << version << ": " << m_lastError << std::endl;
        return false;
    }

    std::string recSql = "INSERT INTO schema_migrations (version, name) VALUES (" + 
                         std::to_string(version) + ", '" + name + "');";
    if (!execute(recSql)) {
        return false;
    }

    tx.commit();
    std::cout << "[DatabaseManager] Migration " << version << " applied successfully." << std::endl;
    return true;
}

bool DatabaseManager::runMigrations() {
    int currentVer = getCurrentSchemaVersion();

    // Migration 1: Core Astronomical Data Models & Data Sources
    if (currentVer < 1) {
        const char* mig1 = R"(
            -- 1. Data Sources
            CREATE TABLE IF NOT EXISTS data_sources (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                base_url TEXT,
                description TEXT,
                is_official INTEGER NOT NULL DEFAULT 1
            );

            -- 2. Objects
            CREATE TABLE IF NOT EXISTS objects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                slug TEXT UNIQUE NOT NULL,
                name TEXT NOT NULL,
                type TEXT NOT NULL,
                parent_object_id INTEGER REFERENCES objects(id) ON DELETE SET NULL,
                category TEXT NOT NULL DEFAULT 'Solar System',
                is_synthetic INTEGER NOT NULL DEFAULT 0,
                color_r REAL NOT NULL DEFAULT 0.0,
                color_g REAL NOT NULL DEFAULT 0.83,
                color_b REAL NOT NULL DEFAULT 1.0,
                texture_path TEXT,
                created_at TEXT NOT NULL DEFAULT (datetime('now')),
                updated_at TEXT NOT NULL DEFAULT (datetime('now'))
            );

            -- 3. Physical Properties
            CREATE TABLE IF NOT EXISTS physical_properties (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                object_id INTEGER NOT NULL UNIQUE REFERENCES objects(id) ON DELETE CASCADE,
                mass_kg REAL,
                radius_m REAL,
                albedo REAL,
                greenhouse_k REAL,
                luminosity_w REAL,
                axial_tilt_deg REAL,
                rotation_period_hours REAL,
                mean_density_kg_m3 REAL,
                surface_gravity_mps2 REAL,
                escape_velocity_mps REAL,
                surface_temp_k REAL,
                surface_pressure_kpa REAL,
                magnetic_field_str TEXT,
                atmosphere_summary TEXT,
                rings_json TEXT,
                source_id INTEGER REFERENCES data_sources(id),
                source_record_id TEXT,
                import_timestamp TEXT NOT NULL DEFAULT (datetime('now'))
            );

            -- 4. Orbital Elements
            CREATE TABLE IF NOT EXISTS orbital_elements (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                object_id INTEGER NOT NULL UNIQUE REFERENCES objects(id) ON DELETE CASCADE,
                epoch_jd REAL NOT NULL DEFAULT 2451545.0,
                semi_major_axis_m REAL,
                semi_major_axis_au REAL,
                eccentricity REAL,
                inclination_deg REAL,
                long_ascending_node_deg REAL,
                arg_periapsis_deg REAL,
                mean_anomaly_deg REAL,
                true_anomaly_deg REAL,
                orbital_period_days REAL,
                reference_frame TEXT NOT NULL DEFAULT 'Ecliptic/J2000',
                source_id INTEGER REFERENCES data_sources(id),
                import_timestamp TEXT NOT NULL DEFAULT (datetime('now'))
            );

            -- 5. State Vectors (Position & Velocity in 3D SI)
            CREATE TABLE IF NOT EXISTS state_vectors (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                object_id INTEGER NOT NULL UNIQUE REFERENCES objects(id) ON DELETE CASCADE,
                epoch_jd REAL NOT NULL DEFAULT 2451545.0,
                pos_x_m REAL NOT NULL DEFAULT 0.0,
                pos_y_m REAL NOT NULL DEFAULT 0.0,
                pos_z_m REAL NOT NULL DEFAULT 0.0,
                vel_x_mps REAL NOT NULL DEFAULT 0.0,
                vel_y_mps REAL NOT NULL DEFAULT 0.0,
                vel_z_mps REAL NOT NULL DEFAULT 0.0,
                reference_frame TEXT NOT NULL DEFAULT 'ICRF/Barycentric',
                source_id INTEGER REFERENCES data_sources(id),
                import_timestamp TEXT NOT NULL DEFAULT (datetime('now'))
            );

            -- 6. Composition
            CREATE TABLE IF NOT EXISTS composition (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE,
                element_or_compound TEXT NOT NULL,
                percentage REAL NOT NULL,
                color_r REAL NOT NULL DEFAULT 0.5,
                color_g REAL NOT NULL DEFAULT 0.5,
                color_b REAL NOT NULL DEFAULT 0.5,
                color_a REAL NOT NULL DEFAULT 1.0
            );

            -- 7. Data Imports History
            CREATE TABLE IF NOT EXISTS data_imports (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                source_id INTEGER REFERENCES data_sources(id),
                target_object TEXT NOT NULL,
                status TEXT NOT NULL DEFAULT 'SUCCESS',
                records_count INTEGER NOT NULL DEFAULT 1,
                details TEXT,
                timestamp TEXT NOT NULL DEFAULT (datetime('now'))
            );
        )";

        if (!applyMigration(1, "Create core astronomical tables", mig1)) {
            return false;
        }
    }

    // Migration 2: Ephemeris Time Series, Simulation Runs, State Snapshots & Validation
    if (currentVer < 2) {
        const char* mig2 = R"(
            -- 8. Ephemeris Records (Observed ground truth trajectories for validation)
            CREATE TABLE IF NOT EXISTS ephemeris_records (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE,
                target_name TEXT NOT NULL,
                epoch_utc TEXT NOT NULL,
                epoch_jd REAL NOT NULL,
                pos_x_m REAL NOT NULL,
                pos_y_m REAL NOT NULL,
                pos_z_m REAL NOT NULL,
                vel_x_mps REAL NOT NULL,
                vel_y_mps REAL NOT NULL,
                vel_z_mps REAL NOT NULL,
                reference_frame TEXT NOT NULL DEFAULT 'ICRF/Barycentric',
                source_id INTEGER REFERENCES data_sources(id)
            );

            -- 9. Simulation Runs
            CREATE TABLE IF NOT EXISTS simulation_runs (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT NOT NULL,
                integrator_type TEXT NOT NULL DEFAULT 'Velocity-Verlet (1PN Einstein GR)',
                start_epoch_jd REAL NOT NULL DEFAULT 2451545.0,
                time_scale REAL NOT NULL DEFAULT 86400.0,
                gr_enabled INTEGER NOT NULL DEFAULT 1,
                start_timestamp TEXT NOT NULL DEFAULT (datetime('now')),
                end_timestamp TEXT,
                total_sim_seconds REAL NOT NULL DEFAULT 0.0
            );

            -- 10. Simulation States (Periodic snapshots during a run)
            CREATE TABLE IF NOT EXISTS simulation_states (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                run_id INTEGER NOT NULL REFERENCES simulation_runs(id) ON DELETE CASCADE,
                object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE,
                sim_time_seconds REAL NOT NULL,
                pos_x_m REAL NOT NULL,
                pos_y_m REAL NOT NULL,
                pos_z_m REAL NOT NULL,
                vel_x_mps REAL NOT NULL,
                vel_y_mps REAL NOT NULL,
                vel_z_mps REAL NOT NULL,
                energy_joules REAL,
                angular_momentum REAL
            );

            -- 11. Validation Results (Real vs Simulation comparisons)
            CREATE TABLE IF NOT EXISTS validation_results (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                run_id INTEGER REFERENCES simulation_runs(id) ON DELETE CASCADE,
                object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE,
                object_name TEXT NOT NULL,
                epoch_jd REAL NOT NULL,
                sim_pos_x REAL NOT NULL,
                sim_pos_y REAL NOT NULL,
                sim_pos_z REAL NOT NULL,
                real_pos_x REAL NOT NULL,
                real_pos_y REAL NOT NULL,
                real_pos_z REAL NOT NULL,
                sim_vel_x REAL NOT NULL,
                sim_vel_y REAL NOT NULL,
                sim_vel_z REAL NOT NULL,
                real_vel_x REAL NOT NULL,
                real_vel_y REAL NOT NULL,
                real_vel_z REAL NOT NULL,
                pos_error_m REAL NOT NULL,
                pos_relative_error REAL NOT NULL,
                vel_error_mps REAL NOT NULL,
                energy_drift_pct REAL NOT NULL DEFAULT 0.0,
                angular_momentum_drift_pct REAL NOT NULL DEFAULT 0.0,
                evaluated_at TEXT NOT NULL DEFAULT (datetime('now')),
                gr_mode INTEGER NOT NULL DEFAULT 1
            );
        )";

        if (!applyMigration(2, "Create ephemeris and validation tables", mig2)) {
            return false;
        }
    }

    // Migration 3: Performance Indexes
    if (currentVer < 3) {
        const char* mig3 = R"(
            CREATE INDEX IF NOT EXISTS idx_objects_slug ON objects(slug);
            CREATE INDEX IF NOT EXISTS idx_objects_category ON objects(category);
            CREATE INDEX IF NOT EXISTS idx_objects_parent ON objects(parent_object_id);
            CREATE INDEX IF NOT EXISTS idx_phys_object ON physical_properties(object_id);
            CREATE INDEX IF NOT EXISTS idx_orbit_object ON orbital_elements(object_id);
            CREATE INDEX IF NOT EXISTS idx_state_object ON state_vectors(object_id);
            CREATE INDEX IF NOT EXISTS idx_comp_object ON composition(object_id);
            CREATE INDEX IF NOT EXISTS idx_ephem_obj_jd ON ephemeris_records(object_id, epoch_jd);
            CREATE INDEX IF NOT EXISTS idx_sim_states ON simulation_states(run_id, object_id);
            CREATE INDEX IF NOT EXISTS idx_valid_obj_run ON validation_results(object_id, run_id);
            CREATE INDEX IF NOT EXISTS idx_imports_timestamp ON data_imports(timestamp);
        )";

        if (!applyMigration(3, "Create performance indexes", mig3)) {
            return false;
        }
    }

    // Migration 4: Complete Star Systems & Custom Object Hierarchies
    if (currentVer < 4) {
        const char* mig4 = R"(
            -- 12. Systems (Custom, Imported, Presets)
            CREATE TABLE IF NOT EXISTS systems (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                name TEXT UNIQUE NOT NULL,
                type TEXT NOT NULL DEFAULT 'Custom',
                source TEXT NOT NULL DEFAULT 'User',
                description TEXT,
                created_at TEXT NOT NULL DEFAULT (datetime('now')),
                updated_at TEXT NOT NULL DEFAULT (datetime('now'))
            );

            -- 13. System Objects (Hierarchical parent-child object links within a system)
            CREATE TABLE IF NOT EXISTS system_objects (
                id INTEGER PRIMARY KEY AUTOINCREMENT,
                system_id INTEGER NOT NULL REFERENCES systems(id) ON DELETE CASCADE,
                object_id INTEGER NOT NULL REFERENCES objects(id) ON DELETE CASCADE,
                parent_object_id INTEGER REFERENCES objects(id) ON DELETE SET NULL,
                orbital_order INTEGER NOT NULL DEFAULT 0,
                UNIQUE(system_id, object_id)
            );

            CREATE INDEX IF NOT EXISTS idx_systems_name ON systems(name);
            CREATE INDEX IF NOT EXISTS idx_sys_obj_sys ON system_objects(system_id);
            CREATE INDEX IF NOT EXISTS idx_sys_obj_obj ON system_objects(object_id);
        )";

        if (!applyMigration(4, "Create systems and system_objects tables", mig4)) {
            return false;
        }
    }

    return true;
}

} // namespace AstroGenesis

