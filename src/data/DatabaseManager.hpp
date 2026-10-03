#pragma once

#include <string>
#include <vector>
#include <memory>
#include <mutex>
#include <functional>
#include <sqlite/sqlite3.h>
#include "data/AstronomicalModels.hpp"

namespace AstroGenesis {

class DatabaseManager {
public:
    static DatabaseManager& getInstance();

    DatabaseManager();
    ~DatabaseManager();

    // Initialize database connection and run schema migrations
    bool initialize(const std::string& dbPath = "data/astrogenesis.db");
    void close();

    bool isOpen() const { return m_db != nullptr; }
    sqlite3* getHandle() { return m_db; }

    // Transaction Management
    bool beginTransaction();
    bool commit();
    bool rollback();

    // RAII Transaction Guard
    class ScopedTransaction {
    public:
        explicit ScopedTransaction(DatabaseManager& db) : m_db(db) {
            m_active = m_db.beginTransaction();
        }
        ~ScopedTransaction() {
            if (m_active) {
                m_db.rollback();
            }
        }
        void commit() {
            if (m_active) {
                m_db.commit();
                m_active = false;
            }
        }
    private:
        DatabaseManager& m_db;
        bool m_active = false;
    };

    // Schema Migrations & Validation
    bool runMigrations();
    bool validateSchema(std::string& outError);

    // Path Resolution
    static std::string resolveAuthoritativePath(const std::string& dbPath = "data/astrogenesis.db");
    const std::string& getDatabasePath() const { return m_dbPath; }

    // Query Execution Helpers
    bool execute(const std::string& sql);
    int64_t getLastInsertId();

    // Prepared Statement Execution
    sqlite3_stmt* prepare(const std::string& sql);
    void finalize(sqlite3_stmt*& stmt);

    std::string getLastError() const;

private:
    bool applyMigration(int version, const std::string& name, const std::string& sql);
    int getCurrentSchemaVersion();

    sqlite3* m_db = nullptr;
    std::string m_dbPath;
    mutable std::recursive_mutex m_mutex;
    mutable std::string m_lastError;
    int m_transactionDepth = 0;
};

// Safe SQLite column text extraction to prevent std::string(nullptr) crash
inline std::string columnTextSafe(sqlite3_stmt* stmt, int col, const char* defaultVal = "") {
    const unsigned char* txt = sqlite3_column_text(stmt, col);
    return txt ? reinterpret_cast<const char*>(txt) : (defaultVal ? defaultVal : "");
}

} // namespace AstroGenesis

