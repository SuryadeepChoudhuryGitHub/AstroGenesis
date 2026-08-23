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

    // Schema Migrations
    bool runMigrations();

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
    mutable std::mutex m_mutex;
    mutable std::string m_lastError;
    int m_transactionDepth = 0;
};

} // namespace AstroGenesis

